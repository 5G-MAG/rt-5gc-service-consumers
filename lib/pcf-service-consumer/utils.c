/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2022 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-core.h"
#include "ogs-sbi.h"

#include "pcf-service-consumer.h"

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library Internals */
char *_sockaddr_to_string(const ogs_sockaddr_t *addr)
{
    return ogs_ipstrdup((ogs_sockaddr_t*)addr); /* safe to remove const as OGS routines just use it read-only */
}

int events_notification_to_events_mask(const OpenAPI_events_notification_t *events_notif)
{
    int events_mask = 0;

    if (events_notif) {
        OpenAPI_lnode_t *node;
        OpenAPI_list_for_each(events_notif->ev_notifs, node) {
            OpenAPI_af_event_notification_t *af_event_notif = (OpenAPI_af_event_notification_t*)node->data;
            events_mask |= _npcf_af_event_to_event_mask(af_event_notif->event);
        }
    }

    return events_mask;
}

int events_subsc_req_data_to_events_mask(const OpenAPI_events_subsc_req_data_t *evt_subsc_req)
{
    int events_mask = 0;
    OpenAPI_lnode_t *node;

    if (evt_subsc_req) {
        OpenAPI_list_for_each(evt_subsc_req->events, node) {
            OpenAPI_af_event_subscription_t *af_event_subsc = (OpenAPI_af_event_subscription_t*)node->data;
            events_mask |= _npcf_af_event_to_event_mask(af_event_subsc->event);
        }
    }

    return events_mask;
}

int _npcf_af_event_to_event_mask(const OpenAPI_npcf_af_event_e event_type)
{
    switch (event_type) {
    case OpenAPI_npcf_af_event_ACCESS_TYPE_CHANGE:
        return PCF_APP_SESSION_EVENT_TYPE_ACCESS_TYPE_CHANGE;
    case OpenAPI_npcf_af_event_ANI_REPORT:
        return PCF_APP_SESSION_EVENT_TYPE_ANI_REPORT;
    case OpenAPI_npcf_af_event_APP_DETECTION:
        return PCF_APP_SESSION_EVENT_TYPE_APP_DETECTION;
    case OpenAPI_npcf_af_event_CHARGING_CORRELATION:
        return PCF_APP_SESSION_EVENT_TYPE_CHARGING_CORRELATION;
    case OpenAPI_npcf_af_event_EPS_FALLBACK:
        return PCF_APP_SESSION_EVENT_TYPE_EPS_FALLBACK;
    case OpenAPI_npcf_af_event_FAILED_QOS_UPDATE:
        return PCF_APP_SESSION_EVENT_TYPE_FAILED_QOS_UPDATE;
    case OpenAPI_npcf_af_event_FAILED_RESOURCES_ALLOCATION:
        return PCF_APP_SESSION_EVENT_TYPE_FAILED_RESOURCES_ALLOCATION;
    case OpenAPI_npcf_af_event_OUT_OF_CREDIT:
        return PCF_APP_SESSION_EVENT_TYPE_OUT_OF_CREDIT;
    case OpenAPI_npcf_af_event_PDU_SESSION_STATUS:
        return PCF_APP_SESSION_EVENT_TYPE_PDU_SESSION_STATUS;
    case OpenAPI_npcf_af_event_PLMN_CHG:
        return PCF_APP_SESSION_EVENT_TYPE_PLMN_CHG;
    case OpenAPI_npcf_af_event_QOS_MONITORING:
        return PCF_APP_SESSION_EVENT_TYPE_QOS_MONITORING;
    case OpenAPI_npcf_af_event_QOS_NOTIF:
        return PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF;
    case OpenAPI_npcf_af_event_RAN_NAS_CAUSE:
        return PCF_APP_SESSION_EVENT_TYPE_RAN_NAS_CAUSE;
    case OpenAPI_npcf_af_event_REALLOCATION_OF_CREDIT:
        return PCF_APP_SESSION_EVENT_TYPE_REALLOCATION_OF_CREDIT;
    case OpenAPI_npcf_af_event_SAT_CATEGORY_CHG:
        return PCF_APP_SESSION_EVENT_TYPE_SAT_CATEGORY_CHG;
    case OpenAPI_npcf_af_event_SUCCESSFUL_QOS_UPDATE:
        return PCF_APP_SESSION_EVENT_TYPE_SUCCESSFUL_QOS_UPDATE;
    case OpenAPI_npcf_af_event_SUCCESSFUL_RESOURCES_ALLOCATION:
        return PCF_APP_SESSION_EVENT_TYPE_SUCCESSFUL_RESOURCES_ALLOCATION;
    case OpenAPI_npcf_af_event_TSN_BRIDGE_INFO:
        return PCF_APP_SESSION_EVENT_TYPE_TSN_BRIDGE_INFO;
    case OpenAPI_npcf_af_event_UP_PATH_CHG_FAILURE:
        return PCF_APP_SESSION_EVENT_TYPE_UP_PATH_CHG_FAILURE;
    case OpenAPI_npcf_af_event_USAGE_REPORT:
        return PCF_APP_SESSION_EVENT_TYPE_USAGE_REPORT;
    default:
        break;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
