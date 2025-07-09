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
#include "dump-openapi.h"
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

    media_comp = OpenAPI_media_component_create(
            NULL /* afAppId */,
            NULL /* afRoutReq */,
            NULL /* qosReference */,
            false /* have disUeNotif */, 0 /* disUeNotif */,
            NULL /* altSerReqs */,
            NULL /* altSerReqsData */,
            false /* have contVer */, 0 /* contVer */,
            NULL /* codecs */,
            false /* have desMaxLatency */, 0.0 /* desMaxLatency */,
            false /* have desMaxLoss */, 0.0 /* desMaxLoss */,
            NULL /* flusId */,
            OpenAPI_flow_status_NULL /* fStatus */,
            _bw_string(g_app_context->options->max_dl_bit_rate) /* marBwDl */,
            _bw_string(g_app_context->options->max_ul_bit_rate) /* marBwUl */,
            false /* maxPacketLossRateDl is null */, false /* have maxPacketLossRateDl */, 0 /* maxPacketLossRateDl */,
            false /* maxPacketLossRateUl is null */, false /* have maxPacketLossRateUl */, 0 /* maxPacketLossRateUl */,
            NULL /* maxSuppBwDl */,
            NULL /* maxSuppBwUl */,
            0 /* medCompN (map key) */,
            media_sub_comp_list /* medSubComps */,
            g_app_context->options->media_type /* medType */,
            NULL /* minDesBwDl */,
            NULL /* minDesBwUl */,
            _bw_string(g_app_context->options->min_dl_bit_rate) /* mirBwDl */,
            _bw_string(g_app_context->options->min_ul_bit_rate) /* mirBwUl */,
            OpenAPI_preemption_capability_NULL /* preemptCap */,
            OpenAPI_preemption_vulnerability_NULL /* preemtVuln */,
            OpenAPI_priority_sharing_indicator_NULL /* prioSharingInd */,
            OpenAPI_reserv_priority_NULL /* resPrio */,
            NULL /* rrBw */,
            NULL /* rsBw */,
            false /* have sharingKeyDl */, 0 /* sharingKeyDl */,
            false /* have sharingKeyUl */, 0 /* sharingKeyUl */,
            NULL /* tsnQos */,
            false /* tscaiInputDl is null */, NULL /* tscaiInputDl */,
            false /* tscaiInputUl is null */, NULL /* tscaiInputUl */,
            false /* have tscaiTimeDom */, 0 /* tscaiTimeDom */);

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
        "global:\n"
        "  max:\n"
        "    ue: 64\n"
        "\n"
        "sbi:\n"
        "  server:\n"
        "    no_tls: true\n"
        "  client:\n"
        "    no_tls: true\n"
        "\n"
        "pcf-policyauth:\n"
        "  discovery:\n"
        "    delegated: auto\n"
        "  sbi:\n"
        "    server:\n"
        "      - address: 127.0.0.1\n"
        "        port: 0\n"
        ;
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
                "    client:\n"
                "      nrf:\n"
                "        - uri: http://%s:%i\n"
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
    if (notifications) info_events_notification(notifications);
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
