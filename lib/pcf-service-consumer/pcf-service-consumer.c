/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2022 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#include "pcf-build.h"
#include "pcf-service-consumer.h"
#include "pcf-client-sess.h"
#include "npcf-process.h"
#include "utils.h"


static int client_notify_cb(int status, ogs_sbi_response_t *response, void *data);

pcf_session_t *pcf_session_new(const ogs_sockaddr_t *pcf_address)
{
    pcf_session_t *pcf_session;
    OpenAPI_uri_scheme_e scheme = OpenAPI_uri_scheme_http;

    pcf_session = ogs_calloc(1, sizeof(pcf_session_t));
    if(!pcf_session){
        return NULL;
    }
    ogs_copyaddrinfo(&pcf_session->pcf_addr, pcf_address);
    
    pcf_session->client = ogs_sbi_client_add(scheme, NULL /*pcf_session->pcf_addr.fqdn*/, 0 /*port_num*/,
                                             (pcf_session->pcf_addr->ogs_sa_family == AF_INET)?pcf_session->pcf_addr:NULL,
                                             (pcf_session->pcf_addr->ogs_sa_family == AF_INET6)?pcf_session->pcf_addr:NULL);
    if(!pcf_session->client) {
	ogs_freeaddrinfo(pcf_session->pcf_addr);
	ogs_free(pcf_session);
        return NULL;
    }
    ogs_list_init(&pcf_session->pcf_app_sessions);
    ogs_list_add(&pcf_self()->pcf_sessions, pcf_session);
    return pcf_session;
}

void pcf_service_consumer_final(void)
{
    pcf_context_final();
}

void pcf_session_free(pcf_session_t *session)
{
    ogs_assert(session);
    pcf_sess_remove_all(session);  //Free pcf_app_sessions
    ogs_list_remove(&pcf_self()->pcf_sessions, session);
    if(session->pcf_addr)
    	ogs_freeaddrinfo(session->pcf_addr);
    if(session->client)
        ogs_sbi_client_remove(session->client);
    ogs_free(session);
}

bool pcf_session_create_app_session(pcf_session_t *session,
                const ue_network_identifier_t *ue_connection, int events,
                OpenAPI_list_t *media_component,
                pcf_app_session_notification_callback notify_callback, void *notify_user_data,
                pcf_app_session_change_callback change_callback, void *change_user_data)
{
    pcf_app_session_t *sess; 
    ogs_sbi_request_t *request;
    bool rv;

    sess = _pcf_app_session_new(session, ue_connection, events, notify_callback, notify_user_data, change_callback, change_user_data);
    if(!sess)
        return false;

    _pcf_client_sess_ipv4addr_set_from_sockaddr(sess, (const ogs_sockaddr_t *)sess->ue_network_identifier->address);

    _pcf_client_sess_ipv6prefix_set_from_sockaddr(sess, (const ogs_sockaddr_t *)sess->ue_network_identifier->address);

    request = pcf_policyauthorization_request_create(sess, media_component, events);
    rv =  ogs_sbi_client_send_request(session->client, client_notify_cb, request, sess);
    if (ogs_unlikely(rv == false)) {
        ogs_error("Error sending request");
    }      
    ogs_sbi_request_free(request);
    return rv;
}

bool pcf_session_update_app_session(pcf_app_session_t *app_sess, OpenAPI_list_t *media_component)
{

    ogs_sbi_request_t *request;
    bool rv;
    pcf_app_session_t *sess;

    if(!app_sess)
        return false;

    sess = _pcf_client_context_active_sessions_exists(app_sess);

    if(sess){

        request = pcf_policyauthorization_request_update(sess, media_component);

        rv =  ogs_sbi_client_send_request(sess->pcf_session->client, client_notify_cb, request, sess);
        ogs_expect(rv == true);
        if (rv == false){
            ogs_error("Error sending request");
            ogs_sbi_request_free(request);
            return false;
        }

        ogs_sbi_request_free(request);
        return true;
    }
    return false;
}


void pcf_app_session_free(pcf_app_session_t *sess)
{   
    ogs_sbi_request_t *request;
    bool rv;

    if(!sess) return;

    request = pcf_policyauthorization_request_delete(sess);
    ogs_assert(sess->pcf_session->client);
    rv =  ogs_sbi_client_send_request(sess->pcf_session->client, client_notify_cb, request, sess);
    if (rv == false){
        ogs_error("Error sending request to delete");
       
    }
    ogs_sbi_request_free(request);

    /*_pcf_app_session_free(sess);*/
}

bool pcf_app_session_subscribe_event(pcf_app_session_t *app_session,
                OpenAPI_events_subsc_req_data_t *evt_subsc_req, pcf_app_session_notification_callback callback,
                void *user_data)
{
    ogs_sbi_request_t *request;
    bool rv;
    int merged = 0;
    int events_mask;

    if(!app_session || !callback)
        return false;

    events_mask = events_subsc_req_data_to_events_mask(evt_subsc_req);
    _pcf_app_session_add_event_notification(app_session, events_mask, callback, user_data);

    merged = merge_event_subsc_to_app_session_context(app_session, evt_subsc_req);

    if (merged) {
        request = pcf_policyauthorization_req_subscribe_event(app_session);

        rv =  ogs_sbi_client_send_request(app_session->pcf_session->client, client_notify_cb, request, app_session);

        if (rv == false){
            ogs_error("Error sending request to delete");
	}

        ogs_sbi_request_free(request);

    }   

    return true;
}

bool pcf_app_session_unsubscribe_event(pcf_app_session_t *app_session,
                OpenAPI_events_subsc_req_data_t *evt_subsc_req)
{
    ogs_sbi_request_t *request;
    bool rv;

    if(!app_session)
        return 0;

    request = pcf_policyauthorization_req_unsubscribe_event(app_session);

    rv =  ogs_sbi_client_send_request(app_session->pcf_session->client, client_notify_cb, request, app_session);

    if (rv == false){
       ogs_error("Error sending request to delete");
    }
    ogs_sbi_request_free(request);

    return 1;

}

bool pcf_session_process_event(ogs_event_t *e)
{
    return _pcf_process_event(e);
}

/******************* Private functions ********************/

static int client_notify_cb(int status, ogs_sbi_response_t *response, void *data)
{
    int rv;
    ogs_event_t *event;

    ogs_assert(data);

    if (status != OGS_OK) {
        ogs_log_message(
                status == OGS_DONE ? OGS_LOG_DEBUG : OGS_LOG_WARN, 0,
                "client_notify_cb() failed [%d]", status);
    }

    event = ogs_event_new(OGS_EVENT_SBI_CLIENT);
    event->sbi.response = response;
    event->sbi.data = data;
    event->sbi.state = status;

    rv = ogs_queue_push(ogs_app()->queue, event);
    if (rv !=OGS_OK) {
        ogs_error("OGS Queue Push failed %d", rv);
        ogs_sbi_response_free(response);
        ogs_event_free(event);
        return OGS_ERROR;
    }

    ogs_pollset_notify(ogs_app()->pollset);
	    
    return (status == OGS_OK)?OGS_OK:OGS_ERROR;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
