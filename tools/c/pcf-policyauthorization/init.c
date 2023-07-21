/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ogs-app.h"
#include "ogs-core.h"
#include "ogs-sbi.h"

#include "bsf-service-consumer.h"
#include "pcf-service-consumer.h"

#include "app-log.h"
#include "app-sm.h"
#include "options.h"

#include "init.h"

static struct {
    app_options_t *options;
    int argc;
    char **argv;
    char *app_config_filename;
    struct {
        ogs_thread_mutex_t mutex;
        ogs_thread_cond_t cond;
        char *address;
        int port;
        bool running;
    } pcf_discovery;
    pcf_session_t *pcf_session;
    pcf_app_session_t *app_session;
} *g_app_context = NULL;

static void _context_init(int argc, char *argv[]);
static void _context_final(void);
static void _write_app_yaml_config();
static void _start_bsf_discovery(const char *ue_address);
static bool _bsf_retrieve_callback(OpenAPI_pcf_binding_t *pcf_binding, void *user_data);
static bool _app_session_notification_callback(pcf_app_session_t *app_session, const OpenAPI_events_notification_t *notifications,
        void *user_data);
static bool _app_session_change_callback(pcf_app_session_t *app_session, void *user_data);
static void _dump_notifications(const OpenAPI_events_notification_t *notifications);

/* Event type dump to ogs_info() */
static void _info_event_access_type_change(const OpenAPI_events_notification_t *notifications, int indent);
static void _info_event_ani_report(const OpenAPI_events_notification_t *notifications, int indent);

/* OpenAPI structures dump to ogs_info() */
static void _info_user_location(const OpenAPI_user_location_t *userlocn, int indent);
static void _info_eutra_location(const OpenAPI_eutra_location_t *eutra_locn, int indent);
static void _info_nr_location(const OpenAPI_nr_location_t *nr_locn, int indent);
static void _info_n3ga_location(const OpenAPI_n3ga_location_t *n3ga_locn, int indent);
static void _info_utra_location(const OpenAPI_utra_location_t *utra_locn, int indent);
static void _info_gera_location(const OpenAPI_gera_location_t *gera_locn, int indent);
static void _info_global_ran_node_id(const OpenAPI_global_ran_node_id_t *grni, int indent, const char *node_type);
static void _info_gnb_id(const OpenAPI_gnb_id_t *gnbid, int indent);
static void _info_tai(const OpenAPI_tai_t *tai, int indent);
static void _info_ecgi(const OpenAPI_ecgi_t *ecgi, int indent);
static void _info_additional_access_info(const OpenAPI_additional_access_info_t *aai, int indent, const char *info_type);
static void _info_an_gw_address(const OpenAPI_an_gw_address_t *aga, int indent);
static void _info_plmn_id(const OpenAPI_plmn_id_t *plmn_id, int indent);
static void _info_plmn_id_nid(const OpenAPI_plmn_id_nid_t *plmn_id, int indent);

static char *_bw_string(const double bitrate);
static const char *const _protocol_to_string(const int protocol);
static char *_port_clause(const unsigned short int port);

/**************************/
/**** Public functions ****/
/**************************/

bool pcf_policyauth_init(int argc, char *argv[])
{
    ogs_sbi_nf_instance_t *nf_instance;

    _context_init(argc, argv);

    /* Write out an Open5GS app config based on the command line options */
    _write_app_yaml_config();

    /* Add the new config file as an option on the command line parameters */
    g_app_context->argv[g_app_context->argc++] = "-c";
    g_app_context->argv[g_app_context->argc++] = g_app_context->app_config_filename;
    g_app_context->argv[g_app_context->argc] = NULL;

    /* Start-up the Open5GS application */
    ogs_app_initialize(PCF_POLICYAUTH_VERSION, g_app_context->app_config_filename, (const char* const*)g_app_context->argv);

    /* initialise logging */
    app_log_init();

    /* Setup Open5GS SBI library */
    ogs_sbi_context_init(OpenAPI_nf_type_AF);
    ogs_sbi_context_parse_config(NULL, g_app_context->options->nrf_address?"nrf":NULL, NULL);
    nf_instance = ogs_sbi_self()->nf_instance;
    ogs_assert(nf_instance);
    ogs_sbi_nf_fsm_init(nf_instance);

    /* Setup Service Consumers */
    if (g_app_context->options->nrf_address) {
        bsf_parse_config("bsf", "bsf-service-consumer");
    }

    return true;
}

void pcf_policyauth_event_termination(void)
{
    if (g_app_context->pcf_session) pcf_session_free(g_app_context->pcf_session);
    bsf_terminate();
    pcf_service_consumer_final();
    ogs_sbi_server_stop_all();
    if (ogs_sbi_self()->nf_instance) ogs_sbi_nf_fsm_fini(ogs_sbi_self()->nf_instance);
    ogs_sbi_context_final();

    /* finally, terminate the event queue */
    ogs_queue_term(ogs_app()->queue);
    ogs_pollset_notify(ogs_app()->pollset);
}

void pcf_policyauth_final(void)
{
    app_log_final();
    _context_final();
    ogs_app_terminate();
}

const char *pcf_policyauth_get_pcf_address(void)
{
    if (g_app_context->options->pcf_address) return g_app_context->options->pcf_address;
    if (g_app_context->pcf_discovery.address) return g_app_context->pcf_discovery.address;
    _start_bsf_discovery(g_app_context->options->ue_address);
    return NULL;
}

const char *pcf_policyauth_wait_for_pcf_address(void)
{
    int rv;
    const char *pcf;

    pcf = pcf_policyauth_get_pcf_address();
    if (pcf) return pcf;

    /* wait for PCF discovery */
    do {
        ogs_thread_mutex_lock(&g_app_context->pcf_discovery.mutex);
        rv = ogs_thread_cond_wait(&g_app_context->pcf_discovery.cond, &g_app_context->pcf_discovery.mutex);
        ogs_thread_mutex_unlock(&g_app_context->pcf_discovery.mutex);
        if (rv < 0) return NULL;
    } while (rv != 0);

    return g_app_context->pcf_discovery.address;
}

short unsigned int pcf_policyauth_get_pcf_port(void)
{
    if (g_app_context->options->pcf_address) return g_app_context->options->pcf_port;
    if (g_app_context->pcf_discovery.address) return g_app_context->pcf_discovery.port;
    _start_bsf_discovery(g_app_context->options->ue_address);
    return 0;
}

void pcf_policyauth_get_app_session_context(void)
{
    const char *pcf_address = pcf_policyauth_get_pcf_address();
    ue_network_identifier_t ue_connection = {NULL, NULL, NULL, NULL, NULL};
    OpenAPI_list_t *media_comp_list;
    OpenAPI_media_component_t *media_comp;
    OpenAPI_map_t *media_comp_map;
    OpenAPI_list_t *media_sub_comp_list = NULL;

    if (ogs_unlikely(!pcf_address)) return;

    if (!g_app_context->pcf_session) {
        ogs_sockaddr_t *pcf_sa = NULL;

        ogs_getaddrinfo(&pcf_sa, AF_UNSPEC, pcf_address, pcf_policyauth_get_pcf_port(), 0);
        ogs_assert(pcf_sa);

        g_app_context->pcf_session = pcf_session_new(pcf_sa);
        ogs_assert(g_app_context->pcf_session);

        ogs_freeaddrinfo(pcf_sa);
    }

    ogs_getaddrinfo(&ue_connection.address, AF_UNSPEC, g_app_context->options->ue_address, 0U, 0);
    ogs_assert(ue_connection.address);

    media_comp_list = OpenAPI_list_create();
    ogs_assert(media_comp_list);

    if (g_app_context->options->ue_port !=0 || g_app_context->options->protocol != IPPROTO_IP ||
            g_app_context->options->remote_address || g_app_context->options->remote_port != 0) {
        OpenAPI_media_sub_component_t *media_sub_comp;
        OpenAPI_list_t *flow_descs;
        OpenAPI_map_t *media_sub_comp_map;
        char *flow_desc;
        char *ue_port_clause;
        const char *remote_addr_clause = g_app_context->options->remote_address?g_app_context->options->remote_address:"any";
        char *remote_port_clause;

        ue_port_clause = _port_clause(g_app_context->options->ue_port);
        remote_port_clause = _port_clause(g_app_context->options->remote_port);

        flow_descs = OpenAPI_list_create();
        ogs_assert(flow_descs);

        /* NOTE: downlink only rules for now (might need another command line option to indicate uplink - switch in/out) */
        flow_desc = ogs_msprintf("permit in %s from %s%s to %s%s", _protocol_to_string(g_app_context->options->protocol),
                g_app_context->options->ue_address, ue_port_clause, remote_addr_clause, remote_port_clause);
        ogs_assert(flow_desc);

        OpenAPI_list_add(flow_descs, flow_desc);

        flow_desc = ogs_msprintf("permit out %s from %s%s to %s%s", _protocol_to_string(g_app_context->options->protocol),
                remote_addr_clause, remote_port_clause, g_app_context->options->ue_address, ue_port_clause);
        ogs_assert(flow_desc);

        OpenAPI_list_add(flow_descs, flow_desc);

        ogs_free(ue_port_clause);
        ogs_free(remote_port_clause);

        media_sub_comp = OpenAPI_media_sub_component_create(OpenAPI_af_sig_protocol_NULL /* afSigProtocol */,
                NULL /* ethfDescs */, 0 /* fNum (map key) */, flow_descs /* fDescs */, OpenAPI_flow_status_ENABLED /* fStatus */,
                _bw_string(g_app_context->options->max_dl_bit_rate) /* marBwDl */,
                _bw_string(g_app_context->options->max_ul_bit_rate) /* marBwUl */,
                NULL /* tosTrCl */, OpenAPI_flow_usage_NULL /* flowUsage */
                );
        ogs_assert(media_sub_comp);

        media_sub_comp_map = OpenAPI_map_create(ogs_msprintf("%d", media_sub_comp->f_num), media_sub_comp);
        ogs_assert(media_sub_comp_map);

        media_sub_comp_list = OpenAPI_list_create();
        OpenAPI_list_add(media_sub_comp_list, media_sub_comp_map);
    }

    media_comp = OpenAPI_media_component_create(NULL /* afAppId */, NULL /* afRoutReq */, NULL /* qosReference */,
            false /* have disUeNotif */, 0 /* disUeNotif */, NULL /* altSerReqs */, NULL /* altSerReqsData */,
            false /* have contVer */, 0 /* contVer */, NULL /* codecs */, false /* have desMaxLatency */, 0.0 /* desMaxLatency */,
            false /* have desMaxLoss */, 0.0 /* desMaxLoss */, NULL /* flusId */, OpenAPI_flow_status_NULL /* fStatus */,
            _bw_string(g_app_context->options->max_dl_bit_rate) /* marBwDl */,
            _bw_string(g_app_context->options->max_ul_bit_rate) /* marBwUl */,
            false /* have maxPacketLossRateDl */, 0 /* maxPacketLossRateDl */,
            false /* have maxPacketLossRateUl */, 0 /* maxPacketLossRateUl */,
            NULL /* maxSuppBwDl */, NULL /* maxSuppBwUl */,
            0 /* medCompN (map key) */, media_sub_comp_list /* medSubComps */, g_app_context->options->media_type /* medType */,
            NULL /* minDesBwDl */, NULL /* minDesBwUl */,
            _bw_string(g_app_context->options->min_dl_bit_rate) /* mirBwDl */,
            _bw_string(g_app_context->options->min_ul_bit_rate) /* mirBwUl */,
            OpenAPI_preemption_capability_NULL /* preemptCap */, OpenAPI_preemption_vulnerability_NULL /* preemtVuln */,
            OpenAPI_priority_sharing_indicator_NULL /* prioSharingInd */, OpenAPI_reserv_priority_NULL /* resPrio */,
            NULL /* rrBw */, NULL /* rsBw */, false /* have sharingKeyDl */, 0 /* sharingKeyDl */,
            false /* have sharingKeyUl */, 0 /* sharingKeyUl */, NULL /* tsnQos */, NULL /* tscaiInputDl */,
            NULL /* tscaiInputUl */, false /* have tscaiTimeDom */, 0 /* tscaiTimeDom */);

    media_comp_map = OpenAPI_map_create(ogs_msprintf("%i", media_comp->med_comp_n), media_comp);
    ogs_assert(media_comp_map);

    OpenAPI_list_add(media_comp_list, media_comp_map);

    pcf_session_create_app_session(g_app_context->pcf_session, &ue_connection, PCF_APP_SESSION_EVENT_TYPE_ALL, media_comp_list,
            _app_session_notification_callback, NULL, _app_session_change_callback, NULL);

    ogs_freeaddrinfo(ue_connection.address);
}

/***************************/
/**** Private Functions ****/
/***************************/

static void _context_init(int argc, char *argv[])
{
    g_app_context = ogs_calloc(1,sizeof(*g_app_context));
    ogs_assert(g_app_context);

    /* Initialise the context with the command line arguments */
    g_app_context->argc = argc;
    g_app_context->argv = argv;
    g_app_context->options = app_options_parse(&g_app_context->argc, &g_app_context->argv);

    /* Initialise the thread signalling for PCF discovery */
    ogs_thread_mutex_init(&g_app_context->pcf_discovery.mutex);
    ogs_thread_cond_init(&g_app_context->pcf_discovery.cond);
}

static void _context_final(void)
{
    if (!g_app_context) return;

    ogs_thread_cond_destroy(&g_app_context->pcf_discovery.cond);
    ogs_thread_mutex_destroy(&g_app_context->pcf_discovery.mutex);

    if (g_app_context->pcf_discovery.address) ogs_free(g_app_context->pcf_discovery.address);
    if (g_app_context->app_config_filename) {
        unlink(g_app_context->app_config_filename);
        ogs_free(g_app_context->app_config_filename);
    }
    app_options_final(g_app_context->options, g_app_context->argc, g_app_context->argv);

    ogs_free(g_app_context);
    g_app_context = NULL;
}

static void _write_app_yaml_config()
{
    int fd;
    static const char config_hdr[] =
        "pcf-policyauth:\n"
        "  discovery:\n"
        "    delegated: auto\n"
        "\n"
        "sbi:\n"
        "  server:\n"
        "    no_tls: true\n"
        "  client:\n"
        "    no_tls: true\n"
        "\n";
    const char *tmpdir;

    tmpdir = ogs_env_get("TMPDIR");
#ifdef P_tmpdir
    if (!tmpdir) tmpdir = P_tmpdir;
#endif
    if (!tmpdir) tmpdir = OGS_DIR_SEPARATOR_S "tmp";
    g_app_context->app_config_filename = ogs_msprintf("%s%cpcf-policyauth-yaml.XXXXXX", tmpdir, OGS_DIR_SEPARATOR);
    fd = mkostemp(g_app_context->app_config_filename, O_CREAT|O_WRONLY|O_TRUNC|O_CLOEXEC);

    if (fd < 0) {
        ogs_free(g_app_context->app_config_filename);
        g_app_context->app_config_filename = NULL;
        return;
    }

    write(fd, config_hdr, sizeof(config_hdr)-1);

    if (g_app_context->options->nrf_address) {
        dprintf(fd,
                "nrf:\n"
                "  sbi:\n"
                "    - addr:\n"
                "      - %s\n"
                "      port: %i\n"
                "\n", g_app_context->options->nrf_address, g_app_context->options->nrf_port);
    }

    close(fd);
}

static void _start_bsf_discovery(const char *ue_address)
{
    ogs_thread_mutex_lock(&g_app_context->pcf_discovery.mutex);
    if (!g_app_context->pcf_discovery.running) {
        bool rv;
        ogs_sockaddr_t *ue_sockaddr = NULL;

        g_app_context->pcf_discovery.running = true;

        ogs_getaddrinfo(&ue_sockaddr, AF_UNSPEC, ue_address, 0, 0);

        rv = bsf_retrieve_pcf_binding_for_pdu_session(ue_sockaddr, _bsf_retrieve_callback, NULL);
        if (!rv) {
            g_app_context->pcf_discovery.running = false;
        }

        ogs_freeaddrinfo(ue_sockaddr);
    }
    ogs_thread_mutex_unlock(&g_app_context->pcf_discovery.mutex);
}

static bool _bsf_retrieve_callback(OpenAPI_pcf_binding_t *pcf_binding, void *user_data)
{
    ogs_thread_mutex_lock(&g_app_context->pcf_discovery.mutex);
    g_app_context->pcf_discovery.running = false;
    if (g_app_context->pcf_discovery.address) {
        ogs_free(g_app_context->pcf_discovery.address);
    }
    if (pcf_binding) {
        OpenAPI_lnode_t *node;
        node = OpenAPI_list_find(pcf_binding->pcf_ip_end_points, 0);
        if (node) {
            const OpenAPI_ip_end_point_t *end_point = (const OpenAPI_ip_end_point_t*)node->data;
            g_app_context->pcf_discovery.address = ogs_strdup(
                    end_point->ipv4_address?end_point->ipv4_address:end_point->ipv6_address);
            g_app_context->pcf_discovery.port = end_point->is_port?end_point->port:0;
            app_local_send_discovered_pcf();
        } else {
            g_app_context->pcf_discovery.address = NULL;
            g_app_context->pcf_discovery.port = 0;
            app_local_send_pcf_discovery_failed();
        }
    } else {
        g_app_context->pcf_discovery.address = NULL;
        g_app_context->pcf_discovery.port = 0;
        app_local_send_pcf_discovery_failed();
    }
    ogs_thread_cond_broadcast(&g_app_context->pcf_discovery.cond);
    ogs_thread_mutex_unlock(&g_app_context->pcf_discovery.mutex);

    return true;
}

static bool _app_session_notification_callback(pcf_app_session_t *app_session, const OpenAPI_events_notification_t *notifications, void *user_data)
{
    if (notifications) _dump_notifications(notifications);
    return true;
}

static bool _app_session_change_callback(pcf_app_session_t *app_session, void *user_data)
{
    g_app_context->app_session = app_session;

    if (!app_session) {
        app_local_send_app_session_destroyed();
    } else {
        app_local_send_app_session_created();
    }
    return true;
}

static void _dump_notifications(const OpenAPI_events_notification_t *notifications)
{
    OpenAPI_lnode_t *node;

    ogs_info("Notifications from [%s]", notifications->ev_subs_uri);
    OpenAPI_list_for_each(notifications->ev_notifs, node) {
        OpenAPI_af_event_notification_t *af_event = (OpenAPI_af_event_notification_t*)node->data;
        ogs_info("  Event: %s [%i]", OpenAPI_af_event_ToString(af_event->event), af_event->event);
        switch (af_event->event) {
            case OpenAPI_npcf_af_event_ACCESS_TYPE_CHANGE:
                _info_event_access_type_change(notifications, 4);
                break;
            case OpenAPI_npcf_af_event_ANI_REPORT:
                _info_event_ani_report(notifications, 4);
                break;
            case OpenAPI_npcf_af_event_APP_DETECTION:
                ogs_info("    Detected Application Reports:");
                break;
            case OpenAPI_npcf_af_event_CHARGING_CORRELATION:
                ogs_info("    Access Network Charging Correlation:");
                break;
            case OpenAPI_npcf_af_event_EPS_FALLBACK:
                ogs_info("    QoS flow failed - Fallback to EPS:");
                break;
            case OpenAPI_npcf_af_event_FAILED_QOS_UPDATE:
                ogs_info("    QoS update Failed:");
                break;
            case OpenAPI_npcf_af_event_FAILED_RESOURCES_ALLOCATION:
                ogs_info("    Resource Allocation Failed:");
                break;
            case OpenAPI_npcf_af_event_OUT_OF_CREDIT:
                ogs_info("    Out of Credit:");
                break;
            case OpenAPI_npcf_af_event_PDU_SESSION_STATUS:
                ogs_info("    PDU Session Status:");
                break;
            case OpenAPI_npcf_af_event_PLMN_CHG:
                ogs_info("    PLMN Change:");
                break;
            case OpenAPI_npcf_af_event_QOS_MONITORING:
                ogs_info("    QoS Monitoring:");
                break;
            case OpenAPI_npcf_af_event_QOS_NOTIF:
                ogs_info("    QoS Notification:");
                break;
            case OpenAPI_npcf_af_event_RAN_NAS_CAUSE:
                ogs_info("    RAN-NAS Release Cause:");
                break;
            case OpenAPI_npcf_af_event_REALLOCATION_OF_CREDIT:
                ogs_info("    Reallocation of Credit:");
                break;
            case OpenAPI_npcf_af_event_SAT_CATEGORY_CHG:
                ogs_info("    Satellite Backhaul Change:");
                break;
            case OpenAPI_npcf_af_event_SUCCESSFUL_QOS_UPDATE:
                ogs_info("    QoS Update Successful:");
                break;
            case OpenAPI_npcf_af_event_SUCCESSFUL_RESOURCES_ALLOCATION:
                ogs_info("    Resource Allocation Successful:");
                break;
            case OpenAPI_npcf_af_event_TSN_BRIDGE_INFO:
                ogs_info("    5GS Bridge Information:");
                break;
            case OpenAPI_npcf_af_event_UP_PATH_CHG_FAILURE:
                ogs_info("    AF Required Routing Failed:");
                break;
            case OpenAPI_npcf_af_event_USAGE_REPORT:
                ogs_info("    Usage Report:");
                break;
            default:
                ogs_error("Unknown notification type");
                break;
        }

        if (af_event->flows) {
            OpenAPI_lnode_t *flow_node;
            ogs_info("  Affected flows:");
            OpenAPI_list_for_each(af_event->flows, flow_node) {
                OpenAPI_flows_t *flows = (OpenAPI_flows_t*)flow_node->data;
                ogs_info("    Media component %i", flows->med_comp_n);
            }
        }
    }
}

static void _info_event_access_type_change(const OpenAPI_events_notification_t *notifications, int indent)
{
    ogs_info("%*sAccess Type Change Event:", indent, "");
    ogs_info("%*s  Access type = %s", indent, "", OpenAPI_access_type_ToString(notifications->access_type));
    if (notifications->rat_type != OpenAPI_rat_type_NULL) {
        ogs_info("%*s  Rat type = %s", indent, "", OpenAPI_rat_type_ToString(notifications->rat_type));
    }
    _info_additional_access_info(notifications->add_access_info, indent+2, "Additional");
    _info_additional_access_info(notifications->rel_access_info, indent+2, "Released");
    _info_an_gw_address(notifications->an_gw_addr, indent+2);
}

static void _info_event_ani_report(const OpenAPI_events_notification_t *notifications, int indent)
{
    ogs_info("%*sAccess Network Information Report:", indent, "");
    _info_plmn_id_nid(notifications->plmn_id, indent+2);
    _info_user_location(notifications->ue_loc, indent+2);
}

static void _info_user_location(const OpenAPI_user_location_t *userlocn, int indent)
{
    if (userlocn) {
        ogs_info("%*sUser Location:", indent, "");
        _info_eutra_location(userlocn->eutra_location, indent+2);
        _info_nr_location(userlocn->nr_location, indent+2);
        _info_n3ga_location(userlocn->n3ga_location, indent+2);
        _info_utra_location(userlocn->utra_location, indent+2);
        _info_gera_location(userlocn->gera_location, indent+2);
    }
}

static void _info_eutra_location(const OpenAPI_eutra_location_t *eutra_locn, int indent)
{
    if (eutra_locn) {
        ogs_info("%*sE-UTRA Location:", indent, "");
        _info_tai(eutra_locn->tai, indent+2);
        if (eutra_locn->is_ignore_tai) {
            ogs_info("%*s  Ignore Tracking Area Code: %s", indent, "", eutra_locn->ignore_tai?"True":"False");
        }
        _info_ecgi(eutra_locn->ecgi, indent+2);
        if (eutra_locn->is_ignore_ecgi) {
            ogs_info("%*s  Ignore E-UTRA Cell Id: %s", indent, "", eutra_locn->ignore_ecgi?"True":"False");
        }
        if (eutra_locn->is_age_of_location_information) {
            ogs_info("%*s  Age of location information: %i mins", indent, "", eutra_locn->age_of_location_information);
        }
        if (eutra_locn->ue_location_timestamp) {
            ogs_info("%*s  UE Location Acquired: %s", indent, "", eutra_locn->ue_location_timestamp);
        }
        if (eutra_locn->geographical_information) {
            ogs_info("%*s  Geographical Information: %s", indent, "", eutra_locn->geographical_information);
        }
        if (eutra_locn->geodetic_information) {
            ogs_info("%*s  Geodetic Information: %s", indent, "", eutra_locn->geodetic_information);
        }
        _info_global_ran_node_id(eutra_locn->global_ngenb_id, indent+2, "ng-eNodeB");
        _info_global_ran_node_id(eutra_locn->global_enb_id, indent+2, "eNodeB");
    }
}

static void _info_nr_location(const OpenAPI_nr_location_t *nr_locn, int indent)
{
    if (nr_locn) {
        ogs_info("%*sNR Location:", indent, "");
    }
}

static void _info_n3ga_location(const OpenAPI_n3ga_location_t *n3ga_locn, int indent)
{
    if (n3ga_locn) {
        ogs_info("%*sNon-3GPP Access Location:", indent, "");
    }
}

static void _info_utra_location(const OpenAPI_utra_location_t *utra_locn, int indent)
{
    if (utra_locn) {
        ogs_info("%*sUTRAN Location:", indent, "");
    }
}

static void _info_gera_location(const OpenAPI_gera_location_t *gera_locn, int indent)
{   
    if (gera_locn) {
        ogs_info("%*sGERAN Location:", indent, "");
    }
}

static void _info_global_ran_node_id(const OpenAPI_global_ran_node_id_t *grni, int indent, const char *node_type)
{
    if (grni) {
        ogs_info("%*sGlobal Identity of the %s:", indent, "", node_type);
        _info_plmn_id(grni->plmn_id, indent+2);
        if (grni->n3_iwf_id) {
            ogs_info("%*s  N3IWF Id: %s", indent, "", grni->n3_iwf_id);
        }
        _info_gnb_id(grni->g_nb_id, indent+2);
        if (grni->nge_nb_id) {
            ogs_info("%*s  NG-eNodeB Id: %s", indent, "", grni->nge_nb_id);
        }
        if (grni->wagf_id) {
            ogs_info("%*s  W-AGF Id: %s", indent, "", grni->wagf_id);
        }
        if (grni->tngf_id) {
            ogs_info("%*s  TNGF Id: %s", indent, "", grni->tngf_id);
        }
        if (grni->nid) {
            ogs_info("%*s  Network Id: %s", indent, "", grni->nid);
        }
        if (grni->e_nb_id) {
            ogs_info("%*s  eNodeB Id: %s", indent, "", grni->e_nb_id);
        }
    }
}

static void _info_gnb_id(const OpenAPI_gnb_id_t *gnbid, int indent)
{
    if (gnbid) {
        ogs_info("%*sgNodeB Id: %s [%i bits]", indent, "", gnbid->g_nb_value, gnbid->bit_length);
    }
}

static void _info_tai(const OpenAPI_tai_t *tai, int indent)
{   
    if (tai) {
        ogs_info("%*sTracking Area Id:", indent, "");
        _info_plmn_id(tai->plmn_id, indent+2);
        ogs_info("%*s  Tracking Area Code: %s", indent, "", tai->tac);
        if (tai->nid) {
            ogs_info("%*s  Network Id: %s", indent, "", tai->nid);
        }
    }
}

static void _info_ecgi(const OpenAPI_ecgi_t *ecgi, int indent)
{
    if (ecgi) {
        ogs_info("%*sE-UTRA Cell Identifier:", indent, "");
        _info_plmn_id(ecgi->plmn_id, indent+2);
        ogs_info("%*s  E-UTRA Cell Id: %s", indent, "", ecgi->eutra_cell_id);
        if (ecgi->nid) {
            ogs_info("%*s  Network Id: %s", indent, "", ecgi->nid);
        }
    }
}

static void _info_additional_access_info(const OpenAPI_additional_access_info_t *aai, int indent, const char *info_type)
{
    if (aai) {
        ogs_info("%*s%s Access Information:", indent, "", info_type);
        ogs_info("%*s  Access type = %s", indent, "", OpenAPI_access_type_ToString(aai->access_type));
        if (aai->rat_type != OpenAPI_rat_type_NULL) {
            ogs_info("%*s  Rat type = %s", indent, "", OpenAPI_rat_type_ToString(aai->rat_type));
        }
    }
}

static void _info_an_gw_address(const OpenAPI_an_gw_address_t *aga, int indent)
{
    if (aga) {
        ogs_info("%*sAccess Network Gateway Address:", indent, "");
        if (aga->an_gw_ipv4_addr) {
            ogs_info("%*s  IPv4 = %s", indent, "", aga->an_gw_ipv4_addr);
        }
        if (aga->an_gw_ipv6_addr) {
            ogs_info("%*s  IPv6 = %s", indent, "", aga->an_gw_ipv6_addr);
        }
    }
}

static void _info_plmn_id(const OpenAPI_plmn_id_t *plmn_id, int indent)
{
    if (!plmn_id) return;
    ogs_info("%*sPLMN: MCC=%s, MNC=%s", indent, "", plmn_id->mcc, plmn_id->mnc);
}

static void _info_plmn_id_nid(const OpenAPI_plmn_id_nid_t *plmn_id, int indent)
{
    if (!plmn_id) return;
    if (plmn_id->nid) {
        ogs_info("%*sPLMN: MCC=%s, MNC=%s, NID=%s", indent, "", plmn_id->mcc, plmn_id->mnc, plmn_id->nid);
    } else {
        ogs_info("%*sPLMN: MCC=%s, MNC=%s", indent, "", plmn_id->mcc, plmn_id->mnc);
    }
}

static char *_bw_string(double bitrate)
{
    double factor = 1.0;
    const char *units = "";

    if (bitrate <= 0.0) return NULL;
    if (bitrate >= 1000000000.0) {
        factor = 1000000000.0;
        units = "G";
    } else if (bitrate >= 1000000.0) {
        factor = 1000000.0;
        units = "M";
    } else if (bitrate >= 1000.0) {
        factor = 1000.0;
        units = "K";
    }
    return ogs_msprintf("%0.3f %sbps", bitrate/factor, units);
}

static const char *const _protocol_to_string(const int protocol)
{
    switch (protocol) {
    case IPPROTO_IP:
        return "ip";
    case IPPROTO_TCP:
        return "tcp";
    case IPPROTO_UDP:
        return "udp";
    case IPPROTO_ICMP:
        return "icmp";
    case IPPROTO_SCTP:
        return "sctp";
    default:
        break;
    }
    return "ip";
}

static char *_port_clause(const unsigned short int port)
{
    if (port == 0) return ogs_strdup("");
    return ogs_msprintf(" %u", port);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
