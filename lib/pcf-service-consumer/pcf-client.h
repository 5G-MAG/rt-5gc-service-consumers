/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2022 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef PCF_APP_SESSION_H
#define PCF_APP_SESSION_H

#include "ogs-sbi.h"
#include "ogs-app.h"
#include "ogs-proto.h"
#include "context.h"

#include "pcf-service-consumer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The optional features of the Npcf_PolicyAuthorization API that the library implements (TS 29.514 V19.8.0 table 5.8-1): 2 SponsoredConnectivity
 * (the sponsor and the usage threshold of an application session context), 15 EnhancedSubscriptionToNotification (the notification method,
 * the period and the waiting time of an event, and the notification correlation identifier of a subscription), 16 QoSMonitoring (which requires
 * feature 15), 28 PatchCorrection (the update is a JSON Merge Patch), 63 L4S (l4sInd of a media component and the L4S_SUPP event), 17
 * AuthorizationWithRequiredQoS (altSerReqs of a media component), 59 PDUSetHandling (pduSetQosDl, pduSetQosUl, protoDescDl and protoDescUl). The
 * features above number 64 are in the second word: 68 MpxMedia (mpxMediaUlInfos and mpxMediaDlInfos) and 69 TrafficCharChange (datBurstSizeInd,
 * timetoNextBurstInd and expTranInd).
 */
#define PCF_LIB_FEATURES (((uint64_t)1 << (2 - 1)) | ((uint64_t)1 << (15 - 1)) | ((uint64_t)1 << (16 - 1)) | ((uint64_t)1 << (17 - 1)) | \
                          ((uint64_t)1 << (28 - 1)) | ((uint64_t)1 << (59 - 1)) | ((uint64_t)1 << (63 - 1)))
#define PCF_LIB_FEATURES_HI (((uint64_t)1 << (68 - 65)) | ((uint64_t)1 << (69 - 65)))

typedef struct pcf_event_notification_s {
    ogs_lnode_t   node;	
    pcf_app_session_notification_callback callback;
    int events;
    void *user_data;
} pcf_event_notification_t;
	
typedef struct pcf_app_session_s {
    
    ogs_lnode_t   node;	

    uint64_t policyauthorization_features;
    uint64_t policyauthorization_features_hi;   /* the features above number 64 */

    char *pcf_app_session_id;
    /* the path of the Individual Application Session Context resource, from the Location of the answer to its creation */
    char *resource_path;

    /* names this object in the notification URL; unrelated to its address */
    uint64_t instance_id;

    char *ipv4addr;
    char *ipv6addr;

    char *supi;
    char *gpsi;

    ogs_s_nssai_t s_nssai;
    char *dnn;

    pcf_session_t *pcf_session;
    ue_network_identifier_t *ue_network_identifier;

    OpenAPI_app_session_context_t *pcf_app_session_context_requested;
    OpenAPI_app_session_context_t *pcf_app_session_context_received;
    OpenAPI_app_session_context_update_data_patch_t *pcf_app_session_context_updates;


    struct {
        pcf_app_session_change_callback callback;
        void *user_data;
    } change;

    /* the callback that reports each operation with its outcome (pcf_session_create_app_session_with_result()) */
    struct {
        pcf_app_session_result_callback callback;
        void *user_data;
    } result;

    /* the operation that waits for the PCF, and what the last answer was */
    pcf_app_session_operation_t operation;
    int last_status;
    char *problem_title;
    char *problem_detail;
    char *problem_cause;
    int retry_after;            /* seconds, -1 if the answer had none */
    char *redirect_location;
    char *termination_cause;

    ogs_list_t pcf_event_notifications; // Nodes of this list are of type pcf_event_notification_t *

    char *notif_url;

} pcf_app_session_t;

typedef struct pcf_npcf_policyauthorization_param_s {
    OpenAPI_media_type_e med_type;
    int flow_type;
    int qos_type;
} pcf_npcf_policyauthorization_param_t;

extern pcf_app_session_t *_pcf_app_session_new(pcf_session_t *pcf_session, const ue_network_identifier_t *ue_connection, int events,
                                               pcf_app_session_notification_callback notify_callback, void *notify_user_data,
                                               pcf_app_session_change_callback change_callback, void *change_user_data);
extern void _pcf_app_session_free(pcf_app_session_t *app_session);
extern const char *_pcf_app_session_get_notif_url(pcf_app_session_t *app_session);
extern bool _pcf_app_session_add_event_notification(pcf_app_session_t *app_session, int events_mask,
                                                    pcf_app_session_notification_callback notify_callback, void *notify_user_data);
extern bool _pcf_app_session_remove_event_notification(pcf_app_session_t *app_session, int events_mask,
                                                       pcf_app_session_notification_callback notify_callback, void *notify_user_data);
extern bool _pcf_app_session_change_callback_call(pcf_app_session_t *app_session, bool delete_or_error);
extern bool _pcf_app_session_notifications_callback_call(pcf_app_session_t *app_session,
	       					         OpenAPI_events_notification_t *notifications);
extern bool _pcf_app_session_exists(pcf_app_session_t *app_session);
extern void _pcf_app_session_set_answer(pcf_app_session_t *app_session, int http_status, const OpenAPI_problem_details_t *problem);
extern void _pcf_app_session_set_answer_headers(pcf_app_session_t *app_session, const ogs_sbi_response_t *response);
extern void _pcf_app_session_answer_notifications(pcf_app_session_t *app_session, const OpenAPI_events_notification_t *notifications);
extern void _pcf_app_session_set_termination_cause(pcf_app_session_t *app_session, const char *termination_info_json);
extern pcf_app_session_t *_pcf_app_session_find_by_instance_id(uint64_t instance_id);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* PCF_APP_SESSION_H */
