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

typedef struct pcf_event_notification_s {
    ogs_lnode_t   node;	
    pcf_app_session_notification_callback callback;
    int events;
    void *user_data;
} pcf_event_notification_t;
	
typedef struct pcf_app_session_s {
    
    ogs_lnode_t   node;	

    uint64_t policyauthorization_features;

    char *pcf_app_session_id;

    char *ipv4addr;
    char *ipv6addr;
    char *ipv6prefix;

    char *supi;
    char *gpsi;

    ogs_s_nssai_t s_nssai;
    char *dnn;

    pcf_session_t *pcf_session;
    ue_network_identifier_t *ue_network_identifier;

    OpenAPI_app_session_context_t *pcf_app_session_context_requested;
    OpenAPI_app_session_context_t *pcf_app_session_context_received;
    OpenAPI_app_session_context_update_data_patch_t *pcf_app_session_context_updates;
    OpenAPI_app_session_context_update_data_patch_t *pcf_app_session_context_updates_received;


    struct {
        pcf_app_session_change_callback callback;
        void *user_data;
    } change;

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

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* PCF_APP_SESSION_H */
