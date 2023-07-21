/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2022 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef PCF_SESSION_H
#define PCF_SESSION_H

#include "pcf-client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ue_network_identifier_s ue_network_identifier_t;

typedef struct pcf_app_session_s pcf_app_session_t;

typedef struct pcf_session_s {
    ogs_lnode_t   node;
    ogs_sockaddr_t *pcf_addr;
    ogs_sbi_client_t *client;
    ogs_list_t pcf_app_sessions; // Nodes of this list are of type pcf_app_session_t *
    ogs_sbi_server_t *notif_server;
} pcf_session_t;

extern void pcf_app_sess_remove(pcf_app_session_t *sess);
extern void pcf_sess_remove(pcf_session_t *session, pcf_app_session_t *sess);
void pcf_sess_remove_all(pcf_session_t *sess);
void pcf_session_remove_all(void);

bool pcf_sess_set_pcf_app_session_id(pcf_app_session_t *sess, char *pcf_app_session_id);

pcf_app_session_t *pcf_session_find_by_pcf_app_session_id(char *pcf_app_session_id);

pcf_app_session_t *pcf_sess_find_by_pcf_app_session_id(char *pcf_app_session_id);

bool _pcf_client_sess_ue_address_set(pcf_app_session_t *sess, const ogs_sockaddr_t *ue_address);
bool _pcf_client_sess_ipv4addr_set_from_sockaddr(pcf_app_session_t *sess, const ogs_sockaddr_t *addr);

void _pcf_client_sess_notification_callback_init(pcf_app_session_t *sess);
bool _pcf_client_sess_notification_callback_set(pcf_app_session_t *sess, pcf_app_session_notification_callback cb, int events, OpenAPI_events_subsc_req_data_t *evt_subsc_req, void *user_data);
void _pcf_app_sess_event_notifications_remove(pcf_app_session_t *sess);

bool _pcf_client_app_sess_context_updates_set(pcf_app_session_t *sess, OpenAPI_app_session_context_update_data_patch_t *AppSessionContext);
bool _pcf_client_app_sess_context_received_updates_set(pcf_app_session_t *sess, OpenAPI_app_session_context_update_data_patch_t *AppSessionContext);


bool _pcf_client_sess_change_callback_set(pcf_app_session_t *sess,  pcf_app_session_change_callback cb, void *user_data);

bool _pcf_client_sess_ipv6prefix_set_from_sockaddr(pcf_app_session_t *sess, const ogs_sockaddr_t *addr);

bool _pcf_client_requested_app_sess_context_set(pcf_app_session_t *sess, OpenAPI_app_session_context_t *AppSessionContext);

bool _pcf_client_app_sess_context_free(OpenAPI_app_session_context_t *AppSessionContext);

bool _pcf_client_received_app_sess_context_set(pcf_app_session_t *sess, OpenAPI_app_session_context_t *AppSessionContext);

int merge_event_subsc_to_app_session_context(pcf_app_session_t *app_session, OpenAPI_events_subsc_req_data_t *evt_subsc_req);

void merge_evt_subsc_to_app_session_context(pcf_app_session_t *app_session, OpenAPI_events_subsc_req_data_t *evt_subsc_req);

extern ogs_sbi_server_t *_pcf_session_get_notifications_server(pcf_session_t *pcf_session);
extern bool _pcf_session_has_notification_server(pcf_session_t *pcf_session, ogs_sbi_server_t *server);

#ifdef __cplusplus
}
#endif

#endif /* PCF_SESSION_H */
