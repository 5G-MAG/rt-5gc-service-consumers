/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2022 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#include <sys/socket.h>

#include "ogs-core.h"
#include "ogs-sbi.h"

#include "context.h"
#include "pcf-service-consumer.h"
#include "pcf-client-sess.h"
#include "pcf-evsubsc.h"
#include "utils.h"

extern ogs_sbi_server_actions_t ogs_sbi_server_actions;

bool _pcf_client_sess_ue_address_set(pcf_app_session_t *sess, const ogs_sockaddr_t *ue_address)
{
    if (!sess) return false;
    if (!ue_address) {
        if (sess->ue_network_identifier->address) {
            ogs_freeaddrinfo(sess->ue_network_identifier->address);
            sess->ue_network_identifier->address = NULL;
        }
    } else {
        if (sess->ue_network_identifier->address) ogs_freeaddrinfo(sess->ue_network_identifier->address);
        ogs_copyaddrinfo(&sess->ue_network_identifier->address, (ogs_sockaddr_t*)ue_address);
    }
    return true;
}

bool _pcf_client_sess_ipv4addr_set_from_sockaddr(pcf_app_session_t *sess, const ogs_sockaddr_t *addr)
{
    if (!sess) return false;
    if (!addr) return false;
    if (addr->ogs_sa_family != AF_INET) return false;

    if (sess->ipv4addr) ogs_free(sess->ipv4addr);
    sess->ipv4addr = _sockaddr_to_string(addr);

    return true;
}



bool _pcf_client_sess_ipv6prefix_set_from_sockaddr(pcf_app_session_t *sess, const ogs_sockaddr_t *addr)
{
    char *ipv6addr;

    if (!sess) return false;
    if (!addr) return false;
    if (addr->ogs_sa_family != AF_INET6) return false;

    if (sess->ipv6prefix) ogs_free(sess->ipv6prefix);
    ipv6addr = _sockaddr_to_string(addr);
    sess->ipv6prefix = ogs_msprintf("%s/128", ipv6addr);
    ogs_free(ipv6addr);

    return true;
}

bool _pcf_client_sess_notification_callback_set(pcf_app_session_t *sess,  pcf_app_session_notification_callback cb, int events, OpenAPI_events_subsc_req_data_t *evt_subsc_req, void *user_data)
{
    pcf_event_notification_t *pcf_event_notification;
    
    if (!sess) return false;

    pcf_event_notification = ogs_calloc(1, sizeof(pcf_event_notification_t));
    
    pcf_event_notification->callback = cb;

    if(events)
    	pcf_event_notification->events = events;

    if(evt_subsc_req){
	if(!pcf_event_notification->events)
	    pcf_event_notification->events = pcf_app_session_evts(evt_subsc_req->events);
    }
    
    if(user_data)
        pcf_event_notification->user_data = user_data;

    ogs_list_add(&sess->pcf_event_notifications, pcf_event_notification);

    return true;

}

int merge_event_subsc_to_app_session_context(pcf_app_session_t *app_session, OpenAPI_events_subsc_req_data_t *evt_subsc_req) {
    OpenAPI_lnode_t *node = NULL;
    OpenAPI_lnode_t *node1 = NULL;
    OpenAPI_af_event_subscription_t *Event = NULL;
    OpenAPI_af_event_subscription_t *Event_from_subsc = NULL;
    OpenAPI_af_event_subscription_t *Event_to_add = NULL;
    int merged_events = 0;

    OpenAPI_list_for_each(app_session->pcf_app_session_context_requested->asc_req_data->ev_subsc->events, node){
        Event = node->data;
	OpenAPI_list_for_each(evt_subsc_req->events, node1){ 
	    Event_from_subsc = node1->data;
	    if(strcmp(OpenAPI_npcf_af_event_ToString(Event->event), OpenAPI_npcf_af_event_ToString(Event_from_subsc->event)) || strcmp(OpenAPI_af_notif_method_ToString(Event->notif_method), OpenAPI_af_notif_method_ToString(Event_from_subsc->notif_method))) {
                Event_to_add = OpenAPI_af_event_subscription_copy(NULL, Event_from_subsc);
                ogs_assert(Event_to_add);
                OpenAPI_list_add(app_session->pcf_app_session_context_requested->asc_req_data->ev_subsc->events, Event_to_add);
		merged_events++;
	    }
	}
    }
    return merged_events;	
}

void merge_evt_subsc_to_app_session_context(pcf_app_session_t *app_session,
                OpenAPI_events_subsc_req_data_t *evt_subsc_req)
{
    OpenAPI_events_subsc_req_data_t *evSubsc;
    OpenAPI_af_event_subscription_t *Event = NULL;
    OpenAPI_af_event_subscription_t *Event_from_subsc = NULL;
    OpenAPI_lnode_t *node = NULL;
    
    evSubsc = app_session->pcf_app_session_context_requested->asc_req_data->ev_subsc;

    OpenAPI_list_for_each(evt_subsc_req->events, node) {
        Event = node->data;
        if (Event) {
	    Event_from_subsc = OpenAPI_af_event_subscription_copy(NULL, Event);
            ogs_assert(Event_from_subsc);
	    OpenAPI_list_add(evSubsc->events, Event_from_subsc);
	}
    }

}

void _pcf_client_sess_notification_callback_init(pcf_app_session_t *sess) {

   	ogs_list_init(&sess->pcf_event_notifications);
	
}

bool _pcf_client_sess_change_callback_set(pcf_app_session_t *sess,  pcf_app_session_change_callback cb, void *user_data)
{
    if (!sess) return false;
    
    sess->change.callback = cb;
    sess->change.user_data = user_data;

    return true;
}

bool _pcf_client_requested_app_sess_context_set(pcf_app_session_t *sess, OpenAPI_app_session_context_t *AppSessionContext)
{
    if (!sess) return false;

    if(!AppSessionContext) {

        if(sess->pcf_app_session_context_requested){
            OpenAPI_app_session_context_free(sess->pcf_app_session_context_requested);
            sess->pcf_app_session_context_requested = NULL;
	    return true;
        } 
    }

    sess->pcf_app_session_context_requested = OpenAPI_app_session_context_copy(sess->pcf_app_session_context_requested, AppSessionContext);
    
    if(!sess->pcf_app_session_context_requested) return false;
       
    return true;
}

bool _pcf_client_app_sess_context_free(OpenAPI_app_session_context_t *AppSessionContext)
{
    OpenAPI_app_session_context_free(AppSessionContext);
    return true;
}


bool _pcf_client_received_app_sess_context_set(pcf_app_session_t *sess, OpenAPI_app_session_context_t *AppSessionContext)
{
    if (!sess) return false;
    if(!AppSessionContext) {
        if(sess->pcf_app_session_context_received){
            OpenAPI_app_session_context_free(sess->pcf_app_session_context_received);
            sess->pcf_app_session_context_received = NULL;
	    return true;
        } 
        return false;
    }
    sess->pcf_app_session_context_received = OpenAPI_app_session_context_copy(sess->pcf_app_session_context_received, AppSessionContext);
    if(!sess->pcf_app_session_context_received) return false;

    return true;
}

bool _pcf_client_app_sess_context_updates_set(pcf_app_session_t *sess, OpenAPI_app_session_context_update_data_patch_t *AppSessionContext)
{
    if (!sess) return false;
    if(!AppSessionContext) {
        if(sess->pcf_app_session_context_updates){
            OpenAPI_app_session_context_update_data_patch_free(sess->pcf_app_session_context_updates);
            sess->pcf_app_session_context_updates = NULL;
            return true;
        }
        return false;
    }
    sess->pcf_app_session_context_updates = OpenAPI_app_session_context_update_data_patch_copy(sess->pcf_app_session_context_updates, AppSessionContext);
    if(!sess->pcf_app_session_context_updates) return false;
    return true;
}

bool _pcf_client_app_sess_context_received_updates_set(pcf_app_session_t *sess, OpenAPI_app_session_context_update_data_patch_t *AppSessionContext)
{
    if (!sess) return false;
    if(!AppSessionContext) {
        if(sess->pcf_app_session_context_updates_received){
            OpenAPI_app_session_context_update_data_patch_free(sess->pcf_app_session_context_updates_received);
            sess->pcf_app_session_context_updates_received = NULL;
            return true;
        }
        return false;
    }
    sess->pcf_app_session_context_updates_received = OpenAPI_app_session_context_update_data_patch_copy(sess->pcf_app_session_context_updates_received, AppSessionContext);
    if(!sess->pcf_app_session_context_updates_received) return false;

    return true;
}


pcf_app_session_t *pcf_session_find_by_pcf_app_session_id(char *pcf_app_session_id)
{
    pcf_session_t *session;
    pcf_app_session_t *app_session;
    ogs_list_for_each(&pcf_self()->pcf_sessions, session)
        ogs_list_for_each(&session->pcf_app_sessions, app_session)
            if(!strcmp(app_session->pcf_app_session_id, pcf_app_session_id))
                return app_session;
    return NULL;
}

void pcf_session_remove_all(void)
{
    pcf_session_t *session = NULL, *next = NULL;
    ogs_list_for_each_safe(&pcf_self()->pcf_sessions, next, session)
       pcf_session_free(session);

}

void _pcf_app_sess_event_notifications_remove(pcf_app_session_t *sess)
{
    pcf_event_notification_t *pcf_event_notif, *next = NULL;
    ogs_list_for_each_safe(&sess->pcf_event_notifications, next, pcf_event_notif){
	ogs_list_remove(&sess->pcf_event_notifications, pcf_event_notif);
        if(pcf_event_notif->user_data) ogs_free(pcf_event_notif->user_data);
	ogs_free(pcf_event_notif);
    }


	
}

void pcf_app_sess_remove(pcf_app_session_t *sess)
{
    ogs_assert(sess);

    if (!_pcf_app_session_change_callback_call(sess, true)) {
        ogs_error("AppSession Change callback failed");
    }

    _pcf_app_session_free(sess);
}

void pcf_sess_remove_all(pcf_session_t *sess)
{
    pcf_app_session_t *session = NULL, *next_sess = NULL;
    ogs_list_for_each_safe(&sess->pcf_app_sessions, next_sess, session) {
        pcf_app_sess_remove(session);
    }
}

ogs_sbi_server_t *_pcf_session_get_notifications_server(pcf_session_t *pcf_session)
{
    if (!pcf_session) return NULL;

    if (!pcf_session->notif_server) {
        ogs_sock_t *sock;
        socklen_t sockaddr_size = sizeof(sock->local_addr.ss);

        /* discover IP address suitable for contacting the PCF */
        sock = ogs_sock_socket(pcf_session->pcf_addr->ogs_sa_family, SOCK_DGRAM, 0);
        ogs_sock_connect(sock, pcf_session->pcf_addr);
        getsockname(sock->fd, &sock->local_addr.sa, &sockaddr_size);

        /* create server with ephemeral port on discovered address */
        if (sock->local_addr.ogs_sa_family == AF_INET) {
            sock->local_addr.ogs_sin_port = 0;
        } else if (sock->local_addr.ogs_sa_family == AF_INET6) {
            sock->local_addr.sin6.sin6_port = 0;
        }
        pcf_session->notif_server = ogs_sbi_server_add(NULL /*interface*/, OpenAPI_uri_scheme_http, &sock->local_addr, NULL);

        ogs_sbi_server_actions.start(pcf_session->notif_server, ogs_sbi_server_handler);

        /* discover ephemeral port for server */
        sockaddr_size = sizeof(sock->local_addr.ss);
        getsockname(pcf_session->notif_server->node.sock->fd, &pcf_session->notif_server->node.addr->sa, &sockaddr_size);

        ogs_sock_destroy(sock);
    }

    return pcf_session->notif_server;
}

bool _pcf_session_has_notification_server(pcf_session_t *pcf_session, ogs_sbi_server_t *server)
{
    if (!pcf_session) return false;
    if (!pcf_session->notif_server) return false;
    return pcf_session->notif_server == server;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
