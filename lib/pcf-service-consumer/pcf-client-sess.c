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



bool _pcf_client_sess_ipv6addr_set_from_sockaddr(pcf_app_session_t *sess, const ogs_sockaddr_t *addr)
{
    if (!sess) return false;
    if (!addr) return false;
    if (addr->ogs_sa_family != AF_INET6) return false;

    if (sess->ipv6addr) ogs_free(sess->ipv6addr);
    sess->ipv6addr = _sockaddr_to_string(addr);

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

static bool event_subscription_equal(const OpenAPI_af_event_subscription_t *a, const OpenAPI_af_event_subscription_t *b)
{
    return a->event == b->event && a->notif_method == b->notif_method &&
           a->is_rep_period == b->is_rep_period && (!a->is_rep_period || a->rep_period == b->rep_period) &&
           a->is_wait_time == b->is_wait_time && (!a->is_wait_time || a->wait_time == b->wait_time) &&
           a->qos_mon_param_type == b->qos_mon_param_type;
}

/*
 * The details of an events subscription (EventsSubscReqData of table 5.6.2.6-1: the QoS monitoring information and parameters, the usage threshold,
 * the notification correlation id, ...) that the caller gives replace those held; what is not given stays. They are deep copies: the caller keeps its
 * own. Returns 1 if some detail was given (the request is then sent although no event changed), else 0. See CONSUMERS audit O12 (no CR: library
 * behaviour).
 */
#define TAKE(field, release) \
    if (copy->field) { if (held->field) { release(held->field); } held->field = copy->field; copy->field = NULL; given = 1; }
static void list_free_values(OpenAPI_list_t *list) { OpenAPI_list_free(list); }
static void list_free_strings(OpenAPI_list_t *list)
{
    OpenAPI_lnode_t *node = NULL;

    OpenAPI_list_for_each(list, node)
        ogs_free(node->data);
    OpenAPI_list_free(list);
}

static int merge_details(OpenAPI_events_subsc_req_data_t *held, OpenAPI_events_subsc_req_data_t *given_data)
{
    OpenAPI_events_subsc_req_data_t *copy;
    int given = 0;

    if (!given_data->events || !given_data->events->count) return 0;
    copy = OpenAPI_events_subsc_req_data_copy(NULL, given_data);
    if (!copy) return 0;
    TAKE(req_qos_mon_params, list_free_values)
    TAKE(qos_mon, OpenAPI_qos_monitoring_information_free)
    TAKE(qos_mon_dat_rate, OpenAPI_qos_monitoring_information_free)
    TAKE(pdv_req_mon_params, list_free_values)
    TAKE(pdv_mon, OpenAPI_qos_monitoring_information_free)
    TAKE(congest_mon, OpenAPI_qos_monitoring_information_free)
    TAKE(rtt_mon, OpenAPI_qos_monitoring_information_free)
    TAKE(avl_bit_rate_mon, OpenAPI_qos_monitoring_information_free)
    TAKE(rtt_flow_ref, OpenAPI_rtt_flow_reference_free)
    TAKE(req_anis, list_free_values)
    TAKE(usg_thres, OpenAPI_usage_threshold_free)
    TAKE(notif_corre_id, ogs_free)
    TAKE(af_app_ids, list_free_strings)
    if (copy->is_direct_notif_ind) { held->is_direct_notif_ind = true; held->direct_notif_ind = copy->direct_notif_ind; given = 1; }
    if (copy->is_avrg_wndw) { held->is_avrg_wndw = true; held->avrg_wndw = copy->avrg_wndw; given = 1; }
    OpenAPI_events_subsc_req_data_free(copy);
    return given;
}
#undef TAKE

/*
 * TS 29.514 V19.8.0 clause 4.2.6.2: the PUT request carries the complete list of the subscribed events, so an event that is subscribed again
 * replaces its entry and is not listed twice. Returns the number of entries that were added or changed.
 */
int merge_event_subsc_to_app_session_context(pcf_app_session_t *app_session, OpenAPI_events_subsc_req_data_t *evt_subsc_req) {
    OpenAPI_app_session_context_req_data_t *asc_req_data;
    OpenAPI_events_subsc_req_data_t *held;
    OpenAPI_lnode_t *node = NULL;
    OpenAPI_lnode_t *node1 = NULL;
    int merged_events = 0;

    if (!app_session->pcf_app_session_context_requested || !app_session->pcf_app_session_context_requested->asc_req_data) return 0;
    asc_req_data = app_session->pcf_app_session_context_requested->asc_req_data;

    held = asc_req_data->ev_subsc;
    if (!held) {
        /* the application session was created without an event subscription: it is made with the same notification URI */
        held = ogs_calloc(1, sizeof(*held));
        ogs_assert(held);
        held->events = OpenAPI_list_create();
        ogs_assert(held->events);
        held->notif_uri = ogs_strdup(asc_req_data->notif_uri);
        asc_req_data->ev_subsc = held;
    }

    OpenAPI_list_for_each(evt_subsc_req->events, node) {
        OpenAPI_af_event_subscription_t *Event_from_subsc = node->data;
        OpenAPI_af_event_subscription_t *Event_held = NULL;

        if (!Event_from_subsc) continue;
        OpenAPI_list_for_each(held->events, node1) {
            OpenAPI_af_event_subscription_t *Event = node1->data;

            if (Event && Event->event == Event_from_subsc->event) {
                Event_held = Event;
                break;
            }
        }
        if (Event_held) {
            if (event_subscription_equal(Event_held, Event_from_subsc)) continue;
            node1->data = OpenAPI_af_event_subscription_copy(NULL, Event_from_subsc);
            ogs_assert(node1->data);
            OpenAPI_af_event_subscription_free(Event_held);
        } else {
            OpenAPI_list_add(held->events, OpenAPI_af_event_subscription_copy(NULL, Event_from_subsc));
        }
        merged_events++;
    }
    /* the details of the subscription (O12) */
    merged_events += merge_details(held, evt_subsc_req);
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

void _pcf_app_session_clear_events(pcf_app_session_t *sess)
{
    OpenAPI_app_session_context_req_data_t *asc_req_data;

    if (!sess) return;
    if (sess->pcf_app_session_context_requested && (asc_req_data = sess->pcf_app_session_context_requested->asc_req_data) != NULL &&
        asc_req_data->ev_subsc) {
        OpenAPI_events_subsc_req_data_free(asc_req_data->ev_subsc);
        asc_req_data->ev_subsc = NULL;
    }
    /* a callback node is released with the events that it was for (the user data is the application's) */
    _pcf_app_session_remove_event_notification(sess, 0x7fffffff, NULL, NULL);
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
        pcf_session->notif_server = ogs_sbi_server_add(
                NULL, ogs_sbi_self()->tls.server.scheme, &sock->local_addr, NULL);

        pcf_session->notif_server->actions->start(pcf_session->notif_server, ogs_sbi_server_handler);

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
