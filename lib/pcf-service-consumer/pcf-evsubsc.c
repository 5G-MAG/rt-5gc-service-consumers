/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2022 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/


#include <stdio.h>

#include "ogs-sbi.h"
#include "pcf-service-consumer.h"
#include "pcf-evsubsc.h"

OpenAPI_list_t *events_subsc_create(int events)
{
    static struct {
        int event_bit;
        OpenAPI_npcf_af_event_e openapi_event;
    } events_map[] = {
	
	{PCF_APP_SESSION_EVENT_TYPE_ACCESS_TYPE_CHANGE, OpenAPI_npcf_af_event_ACCESS_TYPE_CHANGE},
        {PCF_APP_SESSION_EVENT_TYPE_ANI_REPORT, OpenAPI_npcf_af_event_ANI_REPORT},
        {PCF_APP_SESSION_EVENT_TYPE_APP_DETECTION, OpenAPI_npcf_af_event_APP_DETECTION},
        {PCF_APP_SESSION_EVENT_TYPE_CHARGING_CORRELATION, OpenAPI_npcf_af_event_CHARGING_CORRELATION},
        {PCF_APP_SESSION_EVENT_TYPE_EPS_FALLBACK, OpenAPI_npcf_af_event_EPS_FALLBACK},
        {PCF_APP_SESSION_EVENT_TYPE_FAILED_QOS_UPDATE, OpenAPI_npcf_af_event_FAILED_QOS_UPDATE},
        {PCF_APP_SESSION_EVENT_TYPE_FAILED_RESOURCES_ALLOCATION, OpenAPI_npcf_af_event_FAILED_RESOURCES_ALLOCATION},
        {PCF_APP_SESSION_EVENT_TYPE_OUT_OF_CREDIT, OpenAPI_npcf_af_event_OUT_OF_CREDIT},
        {PCF_APP_SESSION_EVENT_TYPE_PDU_SESSION_STATUS, OpenAPI_npcf_af_event_PDU_SESSION_STATUS},
        {PCF_APP_SESSION_EVENT_TYPE_PLMN_CHG, OpenAPI_npcf_af_event_PLMN_CHG},
        {PCF_APP_SESSION_EVENT_TYPE_QOS_MONITORING, OpenAPI_npcf_af_event_QOS_MONITORING},
        {PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF, OpenAPI_npcf_af_event_QOS_NOTIF},
        {PCF_APP_SESSION_EVENT_TYPE_RAN_NAS_CAUSE, OpenAPI_npcf_af_event_RAN_NAS_CAUSE},
        {PCF_APP_SESSION_EVENT_TYPE_REALLOCATION_OF_CREDIT, OpenAPI_npcf_af_event_REALLOCATION_OF_CREDIT},
        {PCF_APP_SESSION_EVENT_TYPE_SAT_CATEGORY_CHG, OpenAPI_npcf_af_event_SAT_CATEGORY_CHG},
        {PCF_APP_SESSION_EVENT_TYPE_SUCCESSFUL_QOS_UPDATE, OpenAPI_npcf_af_event_SUCCESSFUL_QOS_UPDATE},
        {PCF_APP_SESSION_EVENT_TYPE_SUCCESSFUL_RESOURCES_ALLOCATION, OpenAPI_npcf_af_event_SUCCESSFUL_RESOURCES_ALLOCATION},
        {PCF_APP_SESSION_EVENT_TYPE_TSN_BRIDGE_INFO, OpenAPI_npcf_af_event_TSN_BRIDGE_INFO},
        {PCF_APP_SESSION_EVENT_TYPE_UP_PATH_CHG_FAILURE, OpenAPI_npcf_af_event_UP_PATH_CHG_FAILURE},
        {PCF_APP_SESSION_EVENT_TYPE_USAGE_REPORT, OpenAPI_npcf_af_event_USAGE_REPORT},
        {PCF_APP_SESSION_EVENT_TYPE_L4S_SUPP, OpenAPI_npcf_af_event_L4S_SUPP},	    
	    
    };

    OpenAPI_list_t *EventList = NULL;
    OpenAPI_af_event_subscription_t *Event = NULL;
    int i;

    EventList = OpenAPI_list_create();
    ogs_assert(EventList);

    for (i=0; i < (sizeof(events_map)/sizeof(events_map[0])); i++) {
	ogs_debug("events[%d]: %d", i, events);    
        if (events & events_map[i].event_bit) {
	    ogs_debug("events[%d]: %d", i, events);    
            Event = ogs_calloc(1, sizeof(*Event));
            ogs_assert(Event);
            Event->event = events_map[i].openapi_event;
            Event->notif_method = OpenAPI_af_notif_method_EVENT_DETECTION;
            OpenAPI_list_add(EventList, Event);
	}
    }

    return EventList;

}

/*
 * The feature (its number in table 5.8-1 of TS 29.514 V19.8.0) that an AfEvent relates to (column Applicability of table 5.6.3.7-1), 0 for an event
 * of no feature. The text that no feature of the library offers: NetLoc, ApplicationDetectionEvents, IMS_SBI, RoutingReqOutcome, EPSFallbackReport,
 * ExtraUEaddrReport, MPSforDTS, QoSMonCapRepo, RAN-NAS-Cause, ReallocationOfCredit, SatelliteBackhaul, TimeSensitiveNetworking, UEUnreachable, EnTSCAC,
 * URSPEnforcement, EnQoSMon, RateLimitReport, UeSatUeComm and CnHealthMonitor.
 */
static int event_feature(OpenAPI_npcf_af_event_e event)
{
    switch (event) {
    case OpenAPI_npcf_af_event_ANI_REPORT: return 6;
    case OpenAPI_npcf_af_event_APP_DETECTION: return 30;
    case OpenAPI_npcf_af_event_CHARGING_CORRELATION: return 5;
    case OpenAPI_npcf_af_event_UP_PATH_CHG_FAILURE: return 34;
    case OpenAPI_npcf_af_event_L4S_SUPP: return 63;
    case OpenAPI_npcf_af_event_EPS_FALLBACK: return 22;
    case OpenAPI_npcf_af_event_EXTRA_UE_ADDR: return 53;
    case OpenAPI_npcf_af_event_FAILED_QOS_UPDATE: return 29;
    case OpenAPI_npcf_af_event_OUT_OF_CREDIT: return 5;
    case OpenAPI_npcf_af_event_QOS_MONITORING: return 16;
    case OpenAPI_npcf_af_event_QOS_MON_CAP_REPO: return 64;
    case OpenAPI_npcf_af_event_RAN_NAS_CAUSE: return 14;
    case OpenAPI_npcf_af_event_REALLOCATION_OF_CREDIT: return 25; /* ReallocationOfCredit requires IMS_SBI (5) */
    case OpenAPI_npcf_af_event_SAT_CATEGORY_CHG: return 33;
    case OpenAPI_npcf_af_event_SUCCESSFUL_QOS_UPDATE: return 29;
    case OpenAPI_npcf_af_event_TSN_BRIDGE_INFO: return 18;
    case OpenAPI_npcf_af_event_USAGE_REPORT: return 2;
    case OpenAPI_npcf_af_event_UE_REACH_STATUS_CH: return 42;
    case OpenAPI_npcf_af_event_BAT_OFFSET_INFO: return 45;
    case OpenAPI_npcf_af_event_URSP_ENF_INFO: return 56;
    case OpenAPI_npcf_af_event_PACK_DEL_VAR: return 61;
    case OpenAPI_npcf_af_event_RT_DELAY_TWO_QOS_FLOWS: return 61;
    case OpenAPI_npcf_af_event_RATE_LIMIT_INFO_REPO: return 75;
    case OpenAPI_npcf_af_event_UP_PATH_CHANGE: return 72;
    case OpenAPI_npcf_af_event_CN_HEALTH_MONITOR: return 82;
    case OpenAPI_npcf_af_event_UE_TEMPORARILY_UNAVAILABLE: return 42;
    default: return 0;
    }
}

static bool feature_in(int feature, uint64_t lo, uint64_t hi)
{
    if (feature <= 0) return true;
    if (feature <= 64) return (lo >> (feature - 1)) & 1;
    return (hi >> (feature - 65)) & 1;
}

#define NEEDS(test, feature, name) \
    if ((test) && !feature_in(feature, features, features_hi)) { snprintf(what, what_size, "%s (feature %d)", name, feature); return false; }

bool pcf_events_subscription_allowed(int events, const OpenAPI_events_subsc_req_data_t *subsc, uint64_t features, uint64_t features_hi,
                char *what, size_t what_size)
{
    OpenAPI_list_t *list = events_subsc_create(events);
    OpenAPI_lnode_t *node = NULL;
    bool allowed = true;

    OpenAPI_list_for_each(list, node) {
        OpenAPI_af_event_subscription_t *event = node->data;
        int feature = event ? event_feature(event->event) : 0;

        if (allowed && !feature_in(feature, features, features_hi)) {
            snprintf(what, what_size, "event %s (feature %d)", OpenAPI_npcf_af_event_ToString(event->event), feature);
            allowed = false;
        }
        ogs_free(event);
    }
    OpenAPI_list_free(list);
    if (!allowed || !subsc) return allowed;

    OpenAPI_list_for_each(subsc->events, node) {
        OpenAPI_af_event_subscription_t *event = node->data;
        int feature;

        if (!event) continue;
        feature = event_feature(event->event);
        if (!feature_in(feature, features, features_hi)) {
            snprintf(what, what_size, "event %s (feature %d)", OpenAPI_npcf_af_event_ToString(event->event), feature);
            return false;
        }
        /* table 5.6.3.8-1 and table 5.6.2.10-1 */
        NEEDS(event->notif_method == OpenAPI_af_notif_method_PERIODIC, 15, "notifMethod PERIODIC")
        NEEDS(event->is_rep_period && !feature_in(44, features, features_hi), 15, "repPeriod")
        NEEDS(event->is_wait_time, 15, "waitTime")
        NEEDS(event->qos_mon_param_type != OpenAPI_qos_monitoring_param_type_NULL, 61, "qosMonParamType")
        NEEDS(event->cap_types != NULL, 64, "capTypes")
    }
    /* table 5.6.2.6-1 */
    NEEDS(subsc->req_qos_mon_params != NULL, 16, "reqQosMonParams")
    NEEDS(subsc->qos_mon != NULL, 16, "qosMon")
    NEEDS(subsc->qos_mon_dat_rate != NULL, 61, "qosMonDatRate")
    NEEDS(subsc->pdv_req_mon_params != NULL, 61, "pdvReqMonParams")
    NEEDS(subsc->pdv_mon != NULL, 61, "pdvMon")
    NEEDS(subsc->congest_mon != NULL, 61, "congestMon")
    NEEDS(subsc->rtt_mon != NULL, 61, "rttMon")
    NEEDS(subsc->rtt_flow_ref != NULL, 61, "rttFlowRef")
    NEEDS(subsc->avl_bit_rate_mon != NULL, 80, "avlBitRateMon")
    NEEDS(subsc->req_anis != NULL, 6, "reqAnis")
    NEEDS(subsc->usg_thres != NULL, 2, "usgThres")
    NEEDS(subsc->notif_corre_id != NULL, 15, "notifCorreId")
    NEEDS(subsc->af_app_ids != NULL, 30, "afAppIds")
    /* ExposureToEAS (32) or EnQoSMon (61) */
    NEEDS(subsc->is_direct_notif_ind && !feature_in(32, features, features_hi), 61, "directNotifInd")
    NEEDS(subsc->is_avrg_wndw, 61, "avrgWndw")
    return true;
}
#undef NEEDS

int pcf_app_session_evts(OpenAPI_list_t *evt_subsc_req_events)  {

    static struct {
        pcf_app_session_event_type_t pcf_app_session_event;
        int event_bit;
    } events_map[] = {

        {PCF_APP_SESSION_EVENT_TYPE_ACCESS_TYPE_CHANGE, OpenAPI_npcf_af_event_ACCESS_TYPE_CHANGE},
        {PCF_APP_SESSION_EVENT_TYPE_ANI_REPORT, OpenAPI_npcf_af_event_ANI_REPORT},
        {PCF_APP_SESSION_EVENT_TYPE_APP_DETECTION, OpenAPI_npcf_af_event_APP_DETECTION},
        {PCF_APP_SESSION_EVENT_TYPE_CHARGING_CORRELATION, OpenAPI_npcf_af_event_CHARGING_CORRELATION},
        {PCF_APP_SESSION_EVENT_TYPE_EPS_FALLBACK, OpenAPI_npcf_af_event_EPS_FALLBACK},
        {PCF_APP_SESSION_EVENT_TYPE_FAILED_QOS_UPDATE, OpenAPI_npcf_af_event_FAILED_QOS_UPDATE},
        {PCF_APP_SESSION_EVENT_TYPE_FAILED_RESOURCES_ALLOCATION, OpenAPI_npcf_af_event_FAILED_RESOURCES_ALLOCATION},
        {PCF_APP_SESSION_EVENT_TYPE_OUT_OF_CREDIT, OpenAPI_npcf_af_event_OUT_OF_CREDIT},
        {PCF_APP_SESSION_EVENT_TYPE_PDU_SESSION_STATUS, OpenAPI_npcf_af_event_PDU_SESSION_STATUS},
        {PCF_APP_SESSION_EVENT_TYPE_PLMN_CHG, OpenAPI_npcf_af_event_PLMN_CHG},
        {PCF_APP_SESSION_EVENT_TYPE_QOS_MONITORING, OpenAPI_npcf_af_event_QOS_MONITORING},
        {PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF, OpenAPI_npcf_af_event_QOS_NOTIF},
        {PCF_APP_SESSION_EVENT_TYPE_RAN_NAS_CAUSE, OpenAPI_npcf_af_event_RAN_NAS_CAUSE},
        {PCF_APP_SESSION_EVENT_TYPE_REALLOCATION_OF_CREDIT, OpenAPI_npcf_af_event_REALLOCATION_OF_CREDIT},
        {PCF_APP_SESSION_EVENT_TYPE_SAT_CATEGORY_CHG, OpenAPI_npcf_af_event_SAT_CATEGORY_CHG},
        {PCF_APP_SESSION_EVENT_TYPE_SUCCESSFUL_QOS_UPDATE, OpenAPI_npcf_af_event_SUCCESSFUL_QOS_UPDATE},
        {PCF_APP_SESSION_EVENT_TYPE_SUCCESSFUL_RESOURCES_ALLOCATION, OpenAPI_npcf_af_event_SUCCESSFUL_RESOURCES_ALLOCATION},
        {PCF_APP_SESSION_EVENT_TYPE_TSN_BRIDGE_INFO, OpenAPI_npcf_af_event_TSN_BRIDGE_INFO},
        {PCF_APP_SESSION_EVENT_TYPE_UP_PATH_CHG_FAILURE, OpenAPI_npcf_af_event_UP_PATH_CHG_FAILURE},
        {PCF_APP_SESSION_EVENT_TYPE_USAGE_REPORT, OpenAPI_npcf_af_event_USAGE_REPORT},
    };

    OpenAPI_lnode_t *node = NULL;
    OpenAPI_af_event_subscription_t *Event = NULL;
    int events = 0;
    int i;
    for (i=0; i < (sizeof(events_map)/sizeof(events_map[0])); i++) {
        OpenAPI_list_for_each(evt_subsc_req_events, node) {
	    Event = node->data;
            if (Event->event == events_map[i].event_bit) {
                events = events | events_map[i].pcf_app_session_event;
            }
        }
    }
    return events;
}

