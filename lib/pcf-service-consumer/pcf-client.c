/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2022-2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-sbi.h"
#include "ogs-app.h"
#include "ogs-proto.h"

#include "context.h"
#include "pcf-client-sess.h"
#include "pcf-service-consumer.h"
#include "utils.h"

#include "pcf-client.h"

static ue_network_identifier_t *__ue_network_identifier_clone(const ue_network_identifier_t *to_clone);
static void __ue_network_identifier_free(ue_network_identifier_t *ue_net);

/******************* Library functions *********************/

pcf_app_session_t *_pcf_app_session_new(pcf_session_t *pcf_session, const ue_network_identifier_t *ue_connection, int events,
                                        pcf_app_session_notification_callback notify_callback, void *notify_user_data,
                                        pcf_app_session_change_callback change_callback, void *change_user_data)
{
    pcf_app_session_t *sess;

    sess = ogs_calloc(1, sizeof(*sess));
    if (!sess) return NULL;

    sess->pcf_session = pcf_session;
    sess->ue_network_identifier = __ue_network_identifier_clone(ue_connection);

    ogs_list_init(&sess->pcf_event_notifications);

    _pcf_app_session_add_event_notification(sess, events, notify_callback, notify_user_data);

    sess->change.callback = change_callback;
    sess->change.user_data = change_user_data;

    ogs_list_add(&sess->pcf_session->pcf_app_sessions, sess);

    return sess;
}

void _pcf_app_session_free(pcf_app_session_t *app_session)
{
    pcf_event_notification_t *node, *next;

    if (!app_session) return;

    /* remove app session from pcf session */
    ogs_list_remove(&app_session->pcf_session->pcf_app_sessions, app_session);

    /* remove notification callbacks */
    ogs_list_for_each_safe(&app_session->pcf_event_notifications, next, node) {
        ogs_list_remove(&app_session->pcf_event_notifications, node);
        ogs_free(node);
    }

    if (app_session->notif_url) ogs_free(app_session->notif_url);
    if (app_session->ue_network_identifier) __ue_network_identifier_free(app_session->ue_network_identifier);
    if (app_session->pcf_app_session_id) ogs_free(app_session->pcf_app_session_id);
    if (app_session->ipv4addr) ogs_free(app_session->ipv4addr);
    if (app_session->ipv6addr) ogs_free(app_session->ipv6addr);
    if (app_session->ipv6prefix) ogs_free(app_session->ipv6prefix);
    if (app_session->supi) ogs_free(app_session->supi);
    if (app_session->gpsi) ogs_free(app_session->gpsi);
    if (app_session->dnn) ogs_free(app_session->dnn);

    if (app_session->pcf_app_session_context_requested)
        OpenAPI_app_session_context_free(app_session->pcf_app_session_context_requested);
    if (app_session->pcf_app_session_context_received)
        OpenAPI_app_session_context_free(app_session->pcf_app_session_context_received);
    if (app_session->pcf_app_session_context_updates)
        OpenAPI_app_session_context_update_data_patch_free(app_session->pcf_app_session_context_updates);
    if (app_session->pcf_app_session_context_updates_received)
        OpenAPI_app_session_context_update_data_patch_free(app_session->pcf_app_session_context_updates_received);

    ogs_free(app_session);
}

const char *_pcf_app_session_get_notif_url(pcf_app_session_t *app_session)
{
    if (!app_session) return NULL;

    if (!app_session->notif_url) {
        ogs_sbi_server_t *server;
        ogs_sbi_header_t *header;

        server = _pcf_session_get_notifications_server(app_session->pcf_session);
        if (!server) return NULL;

        header = ogs_calloc(1, sizeof(*header));
        header->service.name = (char *)OGS_SBI_SERVICE_NAME_NPCF_POLICYAUTHORIZATION;
        header->api.version = (char *)OGS_SBI_API_V1;
        header->resource.component[0] = (char*)"app-session-instance";
        header->resource.component[1] = ogs_msprintf("%p", app_session);

        app_session->notif_url = ogs_sbi_server_uri(server, header);

        ogs_free(header->resource.component[1]);
        ogs_free(header);
    }

    return app_session->notif_url;
}

bool _pcf_app_session_add_event_notification(pcf_app_session_t *app_session, int events_mask,
                                             pcf_app_session_notification_callback notify_callback, void *notify_user_data)
{
    pcf_event_notification_t *node;

    if (!app_session) return false;

    if (!notify_callback) return false;

    node = ogs_calloc(1, sizeof(*node));
    if (!node) return false;

    node->callback = notify_callback;
    node->user_data = notify_user_data;
    node->events = events_mask;

    ogs_list_add(&app_session->pcf_event_notifications, node);

    return true;
}

bool _pcf_app_session_remove_event_notification(pcf_app_session_t *app_session, int events_mask,
                                                pcf_app_session_notification_callback notify_callback, void *notify_user_data)
{
    pcf_event_notification_t *node, *next;

    if (!app_session) return false;

    ogs_list_for_each_safe(&app_session->pcf_event_notifications, next, node) {
        if (notify_callback == NULL || (node->callback == notify_callback && (notify_user_data == NULL || node->user_data == notify_user_data))) {
            int events_left = node->events & ~events_mask;
            if (events_left == 0) {
                ogs_list_remove(&app_session->pcf_event_notifications, node);
                ogs_free(node);
            } else {
                node->events = events_left;
            }
        }
    }

    return true;
}

bool _pcf_app_session_change_callback_call(pcf_app_session_t *app_session, bool delete_or_error)
{
    if (!app_session || !app_session->change.callback) return false;

    return app_session->change.callback(delete_or_error?NULL:app_session, app_session->change.user_data);
}

bool _pcf_app_session_notifications_callback_call(pcf_app_session_t *app_session, OpenAPI_events_notification_t *notifications)
{
    bool result = true;
    pcf_event_notification_t *pcf_event_notification;
    int events_mask;

    if (!app_session || !notifications) return false;

    events_mask = events_notification_to_events_mask(notifications);

    ogs_list_for_each(&app_session->pcf_event_notifications, pcf_event_notification) {
        if (pcf_event_notification->events & events_mask) {
            bool res = pcf_event_notification->callback(app_session, notifications, pcf_event_notification->user_data);
            result &= res;
            if (!res) {
                ogs_error("PCF Event notification callback %p(%p) reported failure", pcf_event_notification->callback, pcf_event_notification->user_data);
            }
        }
    }

    return result;
}

bool _pcf_app_session_exists(pcf_app_session_t *app_session)
{
    pcf_session_t *pcf_session;

    ogs_list_for_each(&pcf_self()->pcf_sessions, pcf_session) {
        pcf_app_session_t *app_node;
        ogs_list_for_each(&pcf_session->pcf_app_sessions, app_node) {
            if (app_node == app_session) return true;
        }
    }

    return false;
}

/****************** Private functions *********************/

static ue_network_identifier_t *__ue_network_identifier_clone(const ue_network_identifier_t *to_clone)
{
    ue_network_identifier_t *ret;

    ret = ogs_calloc(1, sizeof(*ret));
    if (ret) {
        if (to_clone->address) ogs_copyaddrinfo(&ret->address, to_clone->address);
        if (to_clone->supi) ret->supi = ogs_strdup(to_clone->supi);
        if (to_clone->gpsi) ret->gpsi = ogs_strdup(to_clone->gpsi);
        if (to_clone->dnn) ret->dnn = ogs_strdup(to_clone->dnn);
        if (to_clone->ip_domain) ret->ip_domain = ogs_strdup(to_clone->ip_domain);
    }
    return ret;
}

static void __ue_network_identifier_free(ue_network_identifier_t *ue_net)
{
    if (!ue_net) return;
    if (ue_net->address) ogs_freeaddrinfo(ue_net->address);
    if (ue_net->supi) ogs_free(ue_net->supi);
    if (ue_net->gpsi) ogs_free(ue_net->gpsi);
    if (ue_net->dnn) ogs_free(ue_net->dnn);
    if (ue_net->ip_domain) ogs_free(ue_net->ip_domain);
    ogs_free(ue_net);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
