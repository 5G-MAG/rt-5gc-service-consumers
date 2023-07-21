/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2022 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#include "pcf-test-event-subsc.h"

OpenAPI_list_t *tests_events_subsc_create(int events, OpenAPI_af_notif_method_e notif_method)
{
    static struct {
        int event_bit;
        OpenAPI_npcf_af_event_e openapi_event;
    } events_map[] = {
        {PCF_APP_SESSION_EVENT_TYPE_CHARGING_CORRELATION, OpenAPI_npcf_af_event_CHARGING_CORRELATION},
        {PCF_APP_SESSION_EVENT_TYPE_ACCESS_TYPE_CHANGE, OpenAPI_npcf_af_event_ACCESS_TYPE_CHANGE},
        {PCF_APP_SESSION_EVENT_TYPE_ANI_REPORT, OpenAPI_npcf_af_event_ANI_REPORT},
        {PCF_APP_SESSION_EVENT_TYPE_APP_DETECTION, OpenAPI_npcf_af_event_APP_DETECTION},
        {PCF_APP_SESSION_EVENT_TYPE_UP_PATH_CHG_FAILURE, OpenAPI_npcf_af_event_UP_PATH_CHG_FAILURE},
        {PCF_APP_SESSION_EVENT_TYPE_EPS_FALLBACK, OpenAPI_npcf_af_event_EPS_FALLBACK},
        {PCF_APP_SESSION_EVENT_TYPE_FAILED_QOS_UPDATE, OpenAPI_npcf_af_event_FAILED_QOS_UPDATE},
        {PCF_APP_SESSION_EVENT_TYPE_FAILED_RESOURCES_ALLOCATION, OpenAPI_npcf_af_event_FAILED_RESOURCES_ALLOCATION},
        {PCF_APP_SESSION_EVENT_TYPE_OUT_OF_CREDIT, OpenAPI_npcf_af_event_OUT_OF_CREDIT},
        {PCF_APP_SESSION_EVENT_TYPE_PDU_SESSION_STATUS, OpenAPI_npcf_af_event_PDU_SESSION_STATUS},
        {PCF_APP_SESSION_EVENT_TYPE_PLMN_CHG, OpenAPI_npcf_af_event_PLMN_CHG},
        {PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF, OpenAPI_npcf_af_event_QOS_NOTIF},
        {PCF_APP_SESSION_EVENT_TYPE_QOS_MONITORING, OpenAPI_npcf_af_event_QOS_MONITORING},
        {PCF_APP_SESSION_EVENT_TYPE_RAN_NAS_CAUSE, OpenAPI_npcf_af_event_RAN_NAS_CAUSE},
        {PCF_APP_SESSION_EVENT_TYPE_REALLOCATION_OF_CREDIT, OpenAPI_npcf_af_event_REALLOCATION_OF_CREDIT},
        {PCF_APP_SESSION_EVENT_TYPE_SAT_CATEGORY_CHG, OpenAPI_npcf_af_event_SAT_CATEGORY_CHG},
        {PCF_APP_SESSION_EVENT_TYPE_SUCCESSFUL_QOS_UPDATE, OpenAPI_npcf_af_event_SUCCESSFUL_QOS_UPDATE},
        {PCF_APP_SESSION_EVENT_TYPE_SUCCESSFUL_RESOURCES_ALLOCATION, OpenAPI_npcf_af_event_SUCCESSFUL_RESOURCES_ALLOCATION},
        {PCF_APP_SESSION_EVENT_TYPE_TSN_BRIDGE_INFO, OpenAPI_npcf_af_event_TSN_BRIDGE_INFO},
        {PCF_APP_SESSION_EVENT_TYPE_USAGE_REPORT, OpenAPI_npcf_af_event_USAGE_REPORT},
    };

    OpenAPI_list_t *EventList = NULL;
    OpenAPI_af_event_subscription_t *Event = NULL;
    OpenAPI_lnode_t *node = NULL, *node2 = NULL, *node3 = NULL;
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
            Event->notif_method = notif_method;
            OpenAPI_list_add(EventList, Event);
	    }
    }

    return EventList;

}
