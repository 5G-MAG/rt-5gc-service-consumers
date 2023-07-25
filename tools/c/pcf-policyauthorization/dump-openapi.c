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
static void _dump_event_charging_correlation(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_eps_fallback(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_failed_qos_update(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_failed_resources_allocation(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications,
                                                    int indent);
static void _dump_event_out_of_credit(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_pdu_session_status(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_plmn_chg(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_qos_monitoring(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_qos_notif(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_ran_nas_cause(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_reallocation_of_credit(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_sat_category_chg(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_successful_qos_update(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_successful_resources_allocation(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications,
                                                        int indent);
static void _dump_event_tsn_bridge_info(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_up_path_chg_failure(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
static void _dump_event_usage_report(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);


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
        dump_tai(lvl, nr_locn->tai, indent+2);
        dump_ncgi(lvl, nr_locn->ncgi, indent+2);
        if (nr_locn->is_ignore_ncgi) {
            log_msg("  Ignore NR Cell Identity: %s", nr_locn->ignore_ncgi?"True":"False");
        }
        if (nr_locn->is_age_of_location_information) {
            log_msg("  Age of location information: %i mins", nr_locn->age_of_location_information);
        }
        if (nr_locn->ue_location_timestamp) {
            log_msg("  UE Location Acquired: %s", nr_locn->ue_location_timestamp);
        }
        if (nr_locn->geographical_information) {
            log_msg("  Geographical Information: %s", nr_locn->geographical_information);
        }
        if (nr_locn->geodetic_information) {
            log_msg("  Geodetic Information: %s", nr_locn->geodetic_information);
        }
        dump_global_ran_node_id(lvl, nr_locn->global_gnb_id, indent+2, "gNodeB");
    }
}

void dump_n3ga_location(ogs_log_level_e lvl, const OpenAPI_n3ga_location_t *n3ga_locn, int indent)
{
    if (n3ga_locn) {
        log_msg("Non-3GPP Access Location:");
        dump_tai(lvl, n3ga_locn->n3gpp_tai, indent+2);
        if (n3ga_locn->n3_iwf_id) {
            log_msg("  N3IWF Identifier: %s", n3ga_locn->n3_iwf_id);
        }
        if (n3ga_locn->ue_ipv4_addr) {
            log_msg("  IPv4 Address: %s", n3ga_locn->ue_ipv4_addr);
        }
        if (n3ga_locn->ue_ipv6_addr) {
            log_msg("  IPv6 Address: %s", n3ga_locn->ue_ipv6_addr);
        }
        if (n3ga_locn->is_port_number) {
            log_msg("  Port number: %i", n3ga_locn->port_number);
        }
        if (n3ga_locn->protocol != OpenAPI_transport_protocol_NULL) {
            log_msg("  Protocol: %s", OpenAPI_transport_protocol_ToString(n3ga_locn->protocol));
        }
        dump_tnap_id(lvl, n3ga_locn->tnap_id, indent+2);
        dump_twap_id(lvl, n3ga_locn->twap_id, indent+2);
        dump_hfc_node_id(lvl, n3ga_locn->hfc_node_id, indent+2);
        if (n3ga_locn->gli) {
            log_msg("  Global Line Identifier: %s", n3ga_locn->gli);
        }
        if (n3ga_locn->w5gban_line_type != OpenAPI_line_type_NULL) {
            log_msg("  5G-BRG/FN-BRG wireline access network: %s", OpenAPI_line_type_ToString(n3ga_locn->w5gban_line_type));
        }
        if (n3ga_locn->gci) {
            log_msg("  Global Cable Identifier: %s", n3ga_locn->gci);
        }
    }
}

void dump_utra_location(ogs_log_level_e lvl, const OpenAPI_utra_location_t *utra_locn, int indent)
{
    if (utra_locn) {
        log_msg("UTRAN Location:");
        dump_cell_global_id(lvl, utra_locn->cgi, indent+2);
        dump_service_area_id(lvl, utra_locn->sai, indent+2);
        dump_location_area_id(lvl, utra_locn->lai, indent+2);
        dump_routing_area_id(lvl, utra_locn->rai, indent+2);
        if (utra_locn->is_age_of_location_information) {
            log_msg("  Age of location information: %i mins", utra_locn->age_of_location_information);
        }
        if (utra_locn->ue_location_timestamp) {
            log_msg("  UE Location Acquired: %s", utra_locn->ue_location_timestamp);
        }
        if (utra_locn->geographical_information) {
            log_msg("  Geographical Information: %s", utra_locn->geographical_information);
        }
        if (utra_locn->geodetic_information) {
            log_msg("  Geodetic Information: %s", utra_locn->geodetic_information);
        }
    }
}

void dump_gera_location(ogs_log_level_e lvl, const OpenAPI_gera_location_t *gera_locn, int indent)
{   
    if (gera_locn) {
        log_msg("GERAN Location:");
        dump_cell_global_id(lvl, gera_locn->cgi, indent+2);
        dump_service_area_id(lvl, gera_locn->sai, indent+2);
        dump_location_area_id(lvl, gera_locn->lai, indent+2);
        dump_routing_area_id(lvl, gera_locn->rai, indent+2);
        if (gera_locn->vlr_number) {
            log_msg("  VLR number: %s", gera_locn->vlr_number);
        }
        if (gera_locn->msc_number) {
            log_msg("  MSC number: %s", gera_locn->msc_number);
        }
        if (gera_locn->is_age_of_location_information) {
            log_msg("  Age of location information: %i mins", gera_locn->age_of_location_information);
        }
        if (gera_locn->ue_location_timestamp) {
            log_msg("  UE Location Acquired: %s", gera_locn->ue_location_timestamp);
        }
        if (gera_locn->geographical_information) {
            log_msg("  Geographical Information: %s", gera_locn->geographical_information);
        }
        if (gera_locn->geodetic_information) {
            log_msg("  Geodetic Information: %s", gera_locn->geodetic_information);
        }
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

void dump_ncgi(ogs_log_level_e lvl, const OpenAPI_ncgi_t *ncgi, int indent)
{
    if (ncgi) {
        log_msg("NR Cell Identifier:");
        dump_plmn_id(lvl, ncgi->plmn_id, indent+2);
        if (ncgi->nr_cell_id) {
            log_msg("  NR Cell Id: %s", ncgi->nr_cell_id);
        }
        if (ncgi->nid) {
            log_msg("  Network Id: %s", ncgi->nid);
        }
    }
}

void dump_additional_access_info(ogs_log_level_e lvl, const OpenAPI_additional_access_info_t *aai, int indent, const char *info_type)
{
    if (aai) {
        log_msg("%s Access Information:", info_type);
        log_msg("  Access type: %s", OpenAPI_access_type_ToString(aai->access_type));
        if (aai->rat_type != OpenAPI_rat_type_NULL) {
            log_msg("  Rat type: %s", OpenAPI_rat_type_ToString(aai->rat_type));
        }
    }
}

void dump_an_gw_address(ogs_log_level_e lvl, const OpenAPI_an_gw_address_t *aga, int indent)
{
    if (aga) {
        log_msg("Access Network Gateway Address:");
        if (aga->an_gw_ipv4_addr) {
            log_msg("  IPv4 address: %s", aga->an_gw_ipv4_addr);
        }
        if (aga->an_gw_ipv6_addr) {
            log_msg("  IPv6 address: %s", aga->an_gw_ipv6_addr);
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

void dump_tnap_id(ogs_log_level_e lvl, const OpenAPI_tnap_id_t *tnap_id, int indent)
{
    if (tnap_id) {
        log_msg("TNAP Identifier:");
        if (tnap_id->ss_id) {
            log_msg("  SSID: %s", tnap_id->ss_id);
        }
        if (tnap_id->bss_id) {
            log_msg("  BSSID: %s", tnap_id->bss_id);
        }
        if (tnap_id->civic_address) {
            log_msg("  Civic Address: %s", tnap_id->civic_address);
        }
    }
}

void dump_twap_id(ogs_log_level_e lvl, const OpenAPI_twap_id_t *twap_id, int indent)
{
    if (twap_id) {
        log_msg("TWAP Identifier:");
        log_msg("  SSID: %s", twap_id->ss_id);
        if (twap_id->bss_id) {
            log_msg("  BSSID: %s", twap_id->bss_id);
        }
        if (twap_id->civic_address) {
            log_msg("  Civic Address: %s", twap_id->civic_address);
        }
    }
}

void dump_hfc_node_id(ogs_log_level_e lvl, const OpenAPI_hfc_node_id_t *hfc_node_id, int indent)
{
    if (hfc_node_id) {
        log_msg("HFC Node Identifier: %s", hfc_node_id->hfc_nid);
    }
}

void dump_cell_global_id(ogs_log_level_e lvl, const OpenAPI_cell_global_id_t *cgi, int indent)
{
    if (cgi) {
        log_msg("Cell Global Identifier:");
        dump_plmn_id(lvl, cgi->plmn_id, indent+2);
        log_msg("  Location Area Code: %s", cgi->lac);
        log_msg("  Cell Identity: %s", cgi->cell_id);
    }
}

void dump_service_area_id(ogs_log_level_e lvl, const OpenAPI_service_area_id_t *sai, int indent)
{
    if (sai) {
        log_msg("Service Area Identifier:");
        dump_plmn_id(lvl, sai->plmn_id, indent+2);
        log_msg("  Location Area Code: %s", sai->lac);
        log_msg("  Service Area Code: %s", sai->sac);
    }
}

void dump_location_area_id(ogs_log_level_e lvl, const OpenAPI_location_area_id_t *lai, int indent)
{
    if (lai) {
        log_msg("Location Area Identifier:");
        dump_plmn_id(lvl, lai->plmn_id, indent+2);
        log_msg("  Location Area Code: %s", lai->lac);
    }
}

void dump_routing_area_id(ogs_log_level_e lvl, const OpenAPI_routing_area_id_t *rai, int indent)
{
    if (rai) {
        log_msg("Routing Area Identifier:");
        dump_plmn_id(lvl, rai->plmn_id, indent+2);
        log_msg("  Location Area Code: %s", rai->lac);
        log_msg("  Routing Area Code: %s", rai->rac);
    }
}

void dump_app_detection_report(ogs_log_level_e lvl, const OpenAPI_app_detection_report_t *adr, int indent)
{
    if (adr) {
        log_msg("Application Detect Report:");
        log_msg("  Notification Type: %s", OpenAPI_app_detection_notif_type_ToString(adr->ad_notif_type));
        log_msg("  Application Identifier: %s", adr->af_app_id);
    }
}

void dump_acc_net_charging_address(ogs_log_level_e lvl, const OpenAPI_acc_net_charging_address_t *anca, int indent)
{
    if (anca) {
        log_msg("Access Network Charging Address:");
        if (anca->an_charg_ipv4_addr) {
            log_msg("  IPv4: %s", anca->an_charg_ipv4_addr);
        }
        if (anca->an_charg_ipv6_addr) {
            log_msg("  IPv6: %s", anca->an_charg_ipv6_addr);
        }
    }
}

void dump_access_net_charging_identifier(ogs_log_level_e lvl, const OpenAPI_access_net_charging_identifier_t *anci, int indent)
{
    if (anci) {
        log_msg("Access Network Charging Information:");
        if (anci->is_acc_net_cha_id_value) {
            log_msg("  Access Network Charging Value: %i", anci->acc_net_cha_id_value);
        } else {
            log_msg("  Access Network Charging Id: %s", anci->acc_net_charg_id_string);
        }
        if (anci->flows) {
            OpenAPI_lnode_t *flow_node;
            log_msg("  Relevant QoS Flows:");
            OpenAPI_list_for_each(anci->flows, flow_node) {
                dump_flows(lvl, (const OpenAPI_flows_t*)flow_node->data, indent+4);
            }
        }
    }
}

void dump_resources_allocation_info(ogs_log_level_e lvl, const OpenAPI_resources_allocation_info_t *rai, int indent)
{
    if (rai) {
        log_msg("Resources Allocation Information:");
        if (rai->mc_resourc_status != OpenAPI_media_component_resources_status_NULL) {
            log_msg("  Media Components Resource Status: %s",
                    OpenAPI_media_component_resources_status_ToString(rai->mc_resourc_status));
        }
        if (rai->flows) {
            OpenAPI_lnode_t *flow_node;
            log_msg("  Media Component Flows:");
            OpenAPI_list_for_each(rai->flows, flow_node) {
                dump_flows(lvl, (const OpenAPI_flows_t*)flow_node->data, indent+4);
            }
        }
        if (rai->alt_ser_req) {
            log_msg("  Alternative Service Requirement: %s", rai->alt_ser_req);
        }
    }
}

void dump_out_of_credit_information(ogs_log_level_e lvl, const OpenAPI_out_of_credit_information_t *ooci, int indent)
{
    if (ooci) {
        log_msg("Out Of Credit Information:");
        dump_final_unit_action(lvl, ooci->fin_unit_act, indent+2);
        if (ooci->flows) {
            OpenAPI_lnode_t *flow_node;
            log_msg("  Out Of Credit Flows:");
            OpenAPI_list_for_each(ooci->flows, flow_node) {
                dump_flows(lvl, (const OpenAPI_flows_t*)flow_node->data, indent+4);
            }
        }
    }
}

void dump_qos_monitoring_report(ogs_log_level_e lvl, const OpenAPI_qos_monitoring_report_t *qmr, int indent)
{
}

void dump_qos_notification_control_info(ogs_log_level_e lvl, const OpenAPI_qos_notification_control_info_t *qnci, int indent)
{
}

void dump_ran_nas_rel_cause(ogs_log_level_e lvl, const OpenAPI_ran_nas_rel_cause_t *rnrc, int indent)
{
}

void dump_bridge_management_container(ogs_log_level_e lvl, const OpenAPI_bridge_management_container_t *bmc, int indent)
{
}

void dump_port_management_container(ogs_log_level_e lvl, const OpenAPI_port_management_container_t *pmc, int indent,
                                    const char *port_type)
{
}

void dump_accumulated_usage(ogs_log_level_e lvl, const OpenAPI_accumulated_usage_t *au, int indent)
{
}

void dump_flows(ogs_log_level_e lvl, const OpenAPI_flows_t *flows, int indent)
{
}

void dump_final_unit_action(ogs_log_level_e lvl, const OpenAPI_final_unit_action_t *fua, int indent)
{
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
            break;
        case OpenAPI_npcf_af_event_CHARGING_CORRELATION:
            _dump_event_charging_correlation(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_EPS_FALLBACK:
            _dump_event_eps_fallback(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_FAILED_QOS_UPDATE:
            _dump_event_failed_qos_update(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_FAILED_RESOURCES_ALLOCATION:
            _dump_event_failed_resources_allocation(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_OUT_OF_CREDIT:
            _dump_event_out_of_credit(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_PDU_SESSION_STATUS:
            _dump_event_pdu_session_status(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_PLMN_CHG:
            _dump_event_plmn_chg(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_QOS_MONITORING:
            _dump_event_qos_monitoring(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_QOS_NOTIF:
            _dump_event_qos_notif(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_RAN_NAS_CAUSE:
            _dump_event_ran_nas_cause(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_REALLOCATION_OF_CREDIT:
            _dump_event_reallocation_of_credit(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_SAT_CATEGORY_CHG:
            _dump_event_sat_category_chg(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_SUCCESSFUL_QOS_UPDATE:
            _dump_event_successful_qos_update(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_SUCCESSFUL_RESOURCES_ALLOCATION:
            _dump_event_successful_resources_allocation(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_TSN_BRIDGE_INFO:
            _dump_event_tsn_bridge_info(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_UP_PATH_CHG_FAILURE:
            _dump_event_up_path_chg_failure(lvl, notifications, indent+2);
            break;
        case OpenAPI_npcf_af_event_USAGE_REPORT:
            _dump_event_usage_report(lvl, notifications, indent+2);
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
            log_msg("  Media component: %i", flows->med_comp_n);
        }
    }
}

static void _dump_event_access_type_change(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("Access Type Change Event:");
    log_msg("  Access type: %s", OpenAPI_access_type_ToString(notifications->access_type));
    if (notifications->rat_type != OpenAPI_rat_type_NULL) {
        log_msg("  Rat type: %s", OpenAPI_rat_type_ToString(notifications->rat_type));
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
    if (notifications->ad_reports) {
        OpenAPI_lnode_t *adr_node;
        OpenAPI_list_for_each(notifications->ad_reports, adr_node) {
            dump_app_detection_report(lvl, (const OpenAPI_app_detection_report_t*)adr_node->data, indent+2);
        }
    } else {
        log_msg("  No reports present!");
    }
}

static void _dump_event_charging_correlation(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("Access Network Charging Correlation:");
    dump_acc_net_charging_address(lvl, notifications->an_charg_addr, indent+2);
    if (notifications->an_charg_ids) {
        OpenAPI_lnode_t *anci_node;
        OpenAPI_list_for_each(notifications->an_charg_ids, anci_node) {
            dump_access_net_charging_identifier(lvl, (const OpenAPI_access_net_charging_identifier_t*)anci_node->data, indent+2);
        }
    }
}

static void _dump_event_eps_fallback(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("QoS flow failed - Fallback to EPS");
}

static void _dump_event_failed_qos_update(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("QoS update Failed");
}

static void _dump_event_failed_resources_allocation(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("Resource Allocation Failed:");
    if (notifications->failed_resourc_alloc_reports) {
        OpenAPI_lnode_t *frar_node;
        OpenAPI_list_for_each(notifications->failed_resourc_alloc_reports, frar_node) {
            dump_resources_allocation_info(lvl, (const OpenAPI_resources_allocation_info_t*)frar_node->data, indent+2);
        }
    } else {
        log_msg("  No failed resource allocation reports!");
    }
}

static void _dump_event_out_of_credit(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("Out of Credit:");
    if (notifications->out_of_cred_reports) {
        OpenAPI_lnode_t *ocr_node;
        OpenAPI_list_for_each(notifications->out_of_cred_reports, ocr_node) {
            dump_out_of_credit_information(lvl, (const OpenAPI_out_of_credit_information_t*)ocr_node->data, indent+2);
        }
    }
}

static void _dump_event_pdu_session_status(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("PDU Session Status Event");
}

static void _dump_event_plmn_chg(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("PLMN Change:");
    dump_plmn_id_nid(lvl, notifications->plmn_id, indent+2);
}

static void _dump_event_qos_monitoring(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("QoS Monitoring:");
    if (notifications->qos_mon_reports) {
        OpenAPI_lnode_t *qmr_node;
        OpenAPI_list_for_each(notifications->qos_mon_reports, qmr_node) {
            dump_qos_monitoring_report(lvl, (const OpenAPI_qos_monitoring_report_t*)qmr_node->data, indent+2);
        }
    } else {
        log_msg("  No QoS Monitoring reports!");
    }
}

static void _dump_event_qos_notif(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("QoS Notification:");
    if (notifications->qnc_reports) {
        OpenAPI_lnode_t *qnc_node;
        OpenAPI_list_for_each(notifications->qnc_reports, qnc_node) {
            dump_qos_notification_control_info(lvl, (const OpenAPI_qos_notification_control_info_t*)qnc_node->data, indent+2);
        }
    } else {
        log_msg("  No QoS Notification Control reports!");
    }
}

static void _dump_event_ran_nas_cause(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("RAN-NAS Release Cause:");
    dump_plmn_id_nid(lvl, notifications->plmn_id, indent+2);
    if (notifications->ran_nas_rel_causes) {
        OpenAPI_lnode_t *rnrc_node;
        OpenAPI_list_for_each(notifications->ran_nas_rel_causes, rnrc_node) {
            dump_ran_nas_rel_cause(lvl, (const OpenAPI_ran_nas_rel_cause_t*)rnrc_node->data, indent+2);
        }
    } else {
        log_msg("  No RAN-NAS Release Cause reports!");
    }
    dump_user_location(lvl, notifications->ue_loc, indent+2);
    if (notifications->ue_loc_time) {
        log_msg("  Last location time: %s", notifications->ue_loc_time);
    }
    if (notifications->ue_time_zone) {
        log_msg("  Time Zone of the UE: %s", notifications->ue_time_zone);
    }
}

static void _dump_event_reallocation_of_credit(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("Reallocation of Credit event");
}

static void _dump_event_sat_category_chg(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("Satellite Backhaul Change:");
    if (notifications->sat_backhaul_category != OpenAPI_satellite_backhaul_category_NULL) {
        log_msg("  Satellite Backhaul Category: %s",
                OpenAPI_satellite_backhaul_category_ToString(notifications->sat_backhaul_category));
    }
}

static void _dump_event_successful_qos_update(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("QoS Update Successful event");
}

static void _dump_event_successful_resources_allocation(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("Resource Allocation Successful:");
    if (notifications->succ_resourc_alloc_reports) {
        OpenAPI_lnode_t *rai_node;
        OpenAPI_list_for_each(notifications->succ_resourc_alloc_reports, rai_node) {
            dump_resources_allocation_info(lvl, (const OpenAPI_resources_allocation_info_t*)rai_node->data, indent+2);
        }
    } else {
        log_msg("  No Resource Allocation Information reports!");
    }
}

static void _dump_event_tsn_bridge_info(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("5GS Bridge Information:");
    dump_bridge_management_container(lvl, notifications->tsn_bridge_man_cont, indent+2);
    dump_port_management_container(lvl, notifications->tsn_port_man_cont_dstt, indent+2, "DS-TT");
    if (notifications->tsn_port_man_cont_nwtts) {
        OpenAPI_lnode_t *pmc_node;
        OpenAPI_list_for_each(notifications->tsn_port_man_cont_nwtts, pmc_node) {
            dump_port_management_container(lvl, (const OpenAPI_port_management_container_t*)pmc_node->data, indent+2, "NW-TT");
        }
    }
}

static void _dump_event_up_path_chg_failure(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("AF Required Routing Failed event");
}

static void _dump_event_usage_report(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent)
{
    log_msg("Usage Report:");
    dump_accumulated_usage(lvl, notifications->usg_rep, indent+2);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
