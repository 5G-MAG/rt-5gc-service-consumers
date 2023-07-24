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

#include "ogs-core.h"
#include "ogs-sbi.h"

#include "dump-openapi.h"

static void _dump_af_event_notification(ogs_log_level_e lvl, const OpenAPI_af_event_notification_t *af_event,
                                        const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_access_type_change(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_ani_report(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_app_detection(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);

#define log_msg(fmt,...) ogs_log_message(lvl, 0, "%*s" fmt, indent, "", ##__VA_ARGS__)

/**************************/
/**** Public functions ****/
/**************************/

void dump_events_notification(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    OpenAPI_lnode_t *node;

    log_msg("Notifications from [%s]", notifications->ev_subs_uri);
    OpenAPI_list_for_each(notifications->ev_notifs, node) {
        OpenAPI_af_event_notification_t *af_event = (OpenAPI_af_event_notification_t*)node->data;
        _dump_af_event_notification(lvl, af_event, notifications, indent+2);
    }
}

void dump_user_location(ogs_log_level_e lvl, const OpenAPI_user_location_t *userlocn, int indent)
{
    if (userlocn) {
        log_msg("User Location:");
        dump_eutra_location(lvl, userlocn->eutra_location, indent+2);
        dump_nr_location(lvl, userlocn->nr_location, indent+2);
        dump_n3ga_location(lvl, userlocn->n3ga_location, indent+2);
        dump_utra_location(lvl, userlocn->utra_location, indent+2);
        dump_gera_location(lvl, userlocn->gera_location, indent+2);
    }
}

void dump_eutra_location(ogs_log_level_e lvl, const OpenAPI_eutra_location_t *eutra_locn, int indent)
{
    if (eutra_locn) {
        log_msg("E-UTRA Location:");
        dump_tai(lvl, eutra_locn->tai, indent+2);
        if (eutra_locn->is_ignore_tai) {
            log_msg("  Ignore Tracking Area Code: %s", eutra_locn->ignore_tai?"True":"False");
        }
        dump_ecgi(lvl, eutra_locn->ecgi, indent+2);
        if (eutra_locn->is_ignore_ecgi) {
            log_msg("  Ignore E-UTRA Cell Id: %s", eutra_locn->ignore_ecgi?"True":"False");
        }
        if (eutra_locn->is_age_of_location_information) {
            log_msg("  Age of location information: %i mins", eutra_locn->age_of_location_information);
        }
        if (eutra_locn->ue_location_timestamp) {
            log_msg("  UE Location Acquired: %s", eutra_locn->ue_location_timestamp);
        }
        if (eutra_locn->geographical_information) {
            log_msg("  Geographical Information: %s", eutra_locn->geographical_information);
        }
        if (eutra_locn->geodetic_information) {
            log_msg("  Geodetic Information: %s", eutra_locn->geodetic_information);
        }
        dump_global_ran_node_id(lvl, eutra_locn->global_ngenb_id, indent+2, "ng-eNodeB");
        dump_global_ran_node_id(lvl, eutra_locn->global_enb_id, indent+2, "eNodeB");
    }
}

void dump_nr_location(ogs_log_level_e lvl, const OpenAPI_nr_location_t *nr_locn, int indent)
{
    if (nr_locn) {
        log_msg("NR Location:");
    }
}

void dump_n3ga_location(ogs_log_level_e lvl, const OpenAPI_n3ga_location_t *n3ga_locn, int indent)
{
    if (n3ga_locn) {
        log_msg("Non-3GPP Access Location:");
    }
}

void dump_utra_location(ogs_log_level_e lvl, const OpenAPI_utra_location_t *utra_locn, int indent)
{
    if (utra_locn) {
        log_msg("UTRAN Location:");
    }
}

void dump_gera_location(ogs_log_level_e lvl, const OpenAPI_gera_location_t *gera_locn, int indent)
{   
    if (gera_locn) {
        log_msg("GERAN Location:");
    }
}

void dump_global_ran_node_id(ogs_log_level_e lvl, const OpenAPI_global_ran_node_id_t *grni, int indent, const char *node_type)
{
    if (grni) {
        log_msg("Global Identity of the %s:", node_type);
        dump_plmn_id(lvl, grni->plmn_id, indent+2);
        if (grni->n3_iwf_id) {
            log_msg("  N3IWF Id: %s", grni->n3_iwf_id);
        }
        dump_gnb_id(lvl, grni->g_nb_id, indent+2);
        if (grni->nge_nb_id) {
            log_msg("  NG-eNodeB Id: %s", grni->nge_nb_id);
        }
        if (grni->wagf_id) {
            log_msg("  W-AGF Id: %s", grni->wagf_id);
        }
        if (grni->tngf_id) {
            log_msg("  TNGF Id: %s", grni->tngf_id);
        }
        if (grni->nid) {
            log_msg("  Network Id: %s", grni->nid);
        }
        if (grni->e_nb_id) {
            log_msg("  eNodeB Id: %s", grni->e_nb_id);
        }
    }
}

void dump_gnb_id(ogs_log_level_e lvl, const OpenAPI_gnb_id_t *gnbid, int indent)
{
    if (gnbid) {
        log_msg("gNodeB Id: %s [%i bits]", gnbid->g_nb_value, gnbid->bit_length);
    }
}

void dump_tai(ogs_log_level_e lvl, const OpenAPI_tai_t *tai, int indent)
{   
    if (tai) {
        log_msg("Tracking Area Id:");
        dump_plmn_id(lvl, tai->plmn_id, indent+2);
        log_msg("  Tracking Area Code: %s", tai->tac);
        if (tai->nid) {
            log_msg("  Network Id: %s", tai->nid);
        }
    }
}

void dump_ecgi(ogs_log_level_e lvl, const OpenAPI_ecgi_t *ecgi, int indent)
{
    if (ecgi) {
        log_msg("E-UTRA Cell Identifier:");
        dump_plmn_id(lvl, ecgi->plmn_id, indent+2);
        log_msg("  E-UTRA Cell Id: %s", ecgi->eutra_cell_id);
        if (ecgi->nid) {
            log_msg("  Network Id: %s", ecgi->nid);
        }
    }
}

void dump_additional_access_info(ogs_log_level_e lvl, const OpenAPI_additional_access_info_t *aai, int indent, const char *info_type)
{
    if (aai) {
        log_msg("%s Access Information:", info_type);
        log_msg("  Access type = %s", OpenAPI_access_type_ToString(aai->access_type));
        if (aai->rat_type != OpenAPI_rat_type_NULL) {
            log_msg("  Rat type = %s", OpenAPI_rat_type_ToString(aai->rat_type));
        }
    }
}

void dump_an_gw_address(ogs_log_level_e lvl, const OpenAPI_an_gw_address_t *aga, int indent)
{
    if (aga) {
        log_msg("Access Network Gateway Address:");
        if (aga->an_gw_ipv4_addr) {
            log_msg("  IPv4 = %s", aga->an_gw_ipv4_addr);
        }
        if (aga->an_gw_ipv6_addr) {
            log_msg("  IPv6 = %s", aga->an_gw_ipv6_addr);
        }
    }
}

void dump_plmn_id(ogs_log_level_e lvl, const OpenAPI_plmn_id_t *plmn_id, int indent)
{
    if (!plmn_id) return;
    log_msg("PLMN: MCC=%s, MNC=%s", plmn_id->mcc, plmn_id->mnc);
}

void dump_plmn_id_nid(ogs_log_level_e lvl, const OpenAPI_plmn_id_nid_t *plmn_id, int indent)
{
    if (!plmn_id) return;
    if (plmn_id->nid) {
        log_msg("PLMN: MCC=%s, MNC=%s, NID=%s", plmn_id->mcc, plmn_id->mnc, plmn_id->nid);
    } else {
        log_msg("PLMN: MCC=%s, MNC=%s", plmn_id->mcc, plmn_id->mnc);
    }
}

/*************************************/
/********* Private Functions *********/
/*************************************/

static void _dump_af_event_notification(ogs_log_level_e lvl, const OpenAPI_af_event_notification_t *af_event,
                                        const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("Event: %s [%i]", OpenAPI_af_event_ToString(af_event->event), af_event->event);
    switch (af_event->event) {
        case OpenAPI_npcf_af_event_ACCESS_TYPE_CHANGE:
            _dump_event_access_type_change(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_ANI_REPORT:
            _dump_event_ani_report(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_APP_DETECTION:
            _dump_event_app_detection(lvl, notifications, indent+2);
            log_msg("    Detected Application Reports:");
            break;
        case OpenAPI_npcf_af_event_CHARGING_CORRELATION:
            log_msg("    Access Network Charging Correlation:");
            break;
        case OpenAPI_npcf_af_event_EPS_FALLBACK:
            log_msg("    QoS flow failed - Fallback to EPS:");
            break;
        case OpenAPI_npcf_af_event_FAILED_QOS_UPDATE:
            log_msg("    QoS update Failed:");
            break;
        case OpenAPI_npcf_af_event_FAILED_RESOURCES_ALLOCATION:
            log_msg("    Resource Allocation Failed:");
            break;
        case OpenAPI_npcf_af_event_OUT_OF_CREDIT:
            log_msg("    Out of Credit:");
            break;
        case OpenAPI_npcf_af_event_PDU_SESSION_STATUS:
            log_msg("    PDU Session Status:");
            break;
        case OpenAPI_npcf_af_event_PLMN_CHG:
            log_msg("    PLMN Change:");
            break;
        case OpenAPI_npcf_af_event_QOS_MONITORING:
            log_msg("    QoS Monitoring:");
            break;
        case OpenAPI_npcf_af_event_QOS_NOTIF:
            log_msg("    QoS Notification:");
            break;
        case OpenAPI_npcf_af_event_RAN_NAS_CAUSE:
            log_msg("    RAN-NAS Release Cause:");
            break;
        case OpenAPI_npcf_af_event_REALLOCATION_OF_CREDIT:
            log_msg("    Reallocation of Credit:");
            break;
        case OpenAPI_npcf_af_event_SAT_CATEGORY_CHG:
            log_msg("    Satellite Backhaul Change:");
            break;
        case OpenAPI_npcf_af_event_SUCCESSFUL_QOS_UPDATE:
            log_msg("    QoS Update Successful:");
            break;
        case OpenAPI_npcf_af_event_SUCCESSFUL_RESOURCES_ALLOCATION:
            log_msg("    Resource Allocation Successful:");
            break;
        case OpenAPI_npcf_af_event_TSN_BRIDGE_INFO:
            log_msg("    5GS Bridge Information:");
            break;
        case OpenAPI_npcf_af_event_UP_PATH_CHG_FAILURE:
            log_msg("    AF Required Routing Failed:");
            break;
        case OpenAPI_npcf_af_event_USAGE_REPORT:
            log_msg("    Usage Report:");
            break;
        default:
            ogs_error("Unknown notification type");
            break;
    }
    if (af_event->flows) {
        OpenAPI_lnode_t *flow_node;
        log_msg("Affected flows:");
        OpenAPI_list_for_each(af_event->flows, flow_node) {
            OpenAPI_flows_t *flows = (OpenAPI_flows_t*)flow_node->data;
            log_msg("  Media component %i", flows->med_comp_n);
        }
    }
}

static void _dump_event_access_type_change(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("Access Type Change Event:");
    log_msg("  Access type = %s", OpenAPI_access_type_ToString(notifications->access_type));
    if (notifications->rat_type != OpenAPI_rat_type_NULL) {
        log_msg("  Rat type = %s", OpenAPI_rat_type_ToString(notifications->rat_type));
    }
    dump_additional_access_info(lvl, notifications->add_access_info, indent+2, "Additional");
    dump_additional_access_info(lvl, notifications->rel_access_info, indent+2, "Released");
    dump_an_gw_address(lvl, notifications->an_gw_addr, indent+2);
}

static void _dump_event_ani_report(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("Access Network Information Report:");
    dump_plmn_id_nid(lvl, notifications->plmn_id, indent+2);
    dump_user_location(lvl, notifications->ue_loc, indent+2);
}

static void _dump_event_app_detection(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("Detected Application Reports:");
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
