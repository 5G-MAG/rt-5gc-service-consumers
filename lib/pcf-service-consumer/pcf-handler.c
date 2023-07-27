/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2022 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#include "ogs-sbi.h"
#include "model/events_subsc_put_data.h"

#include "pcf-client-sess.h"
#include "pcf-client.h"
#include "utils.h"
#include "pcf-service-consumer.h"
/*#include "openapi/model/events_subsc_put_data.h"
  #include "openapi/model/event_subscription.h"*/

#include "pcf-handler.h"

void pcf_policyauthorization_update(
        pcf_app_session_t *sess, ogs_sbi_message_t *recvmsg)
{
    OpenAPI_app_session_context_update_data_patch_t *AppSessionContextUpdateDataPatch = NULL;
    OpenAPI_app_session_context_update_data_t *AscUpdateData = NULL;

    cJSON *app_sess_context;
    cJSON *asc_req_data;
    char *app_sess_context_text;
    char *asc_req_data_text;

    if (!recvmsg->h.resource.component[1]) {
        ogs_error("[%s:%s] No AppSessionId[%s]",
                sess->ipv4addr ? sess->ipv4addr : "Unknown",
                sess->ipv6addr ? sess->ipv6addr : "Unknown",
                recvmsg->http.location);
        return;
    }

    AppSessionContextUpdateDataPatch = recvmsg->AppSessionContextUpdateDataPatch;
    if (!AppSessionContextUpdateDataPatch) {
        ogs_error("[%s:%s] No AppSessionContextUpdateDataPatch",
                sess->ipv4addr ? sess->ipv4addr : "Unknown",
                sess->ipv6addr ? sess->ipv6addr : "Unknown");
        return;
    }


    if(!_pcf_client_app_sess_context_received_updates_set(sess, recvmsg->AppSessionContextUpdateDataPatch))
        ogs_error("Failed to set the updated app session context");

    AscUpdateData = AppSessionContextUpdateDataPatch->asc_req_data;
    if (!AscUpdateData) {
        ogs_error("[%s:%s] No AscUpdateData",
                sess->ipv4addr ? sess->ipv4addr : "Unknown",
                sess->ipv6addr ? sess->ipv6addr : "Unknown");
        return;
    }

    ogs_debug("\n PCF policy authorization update (Respone header): \n Method: [%s] \n URI: [%s] \n  Resource Component: [%s]\n", recvmsg->h.method, recvmsg->h.uri, recvmsg->h.resource.component[0]);

    app_sess_context = OpenAPI_app_session_context_update_data_patch_convertToJSON(AppSessionContextUpdateDataPatch);
    app_sess_context_text = cJSON_Print(app_sess_context);
    ogs_debug("Update app_sess_context_text: %s", app_sess_context_text);
    cJSON_Delete(app_sess_context);
    cJSON_free(app_sess_context_text);

    asc_req_data = OpenAPI_app_session_context_update_data_convertToJSON(AppSessionContextUpdateDataPatch->asc_req_data);
    asc_req_data_text = cJSON_Print(asc_req_data);
    ogs_debug("Update: asc_req_data_text: %s", asc_req_data_text);
    cJSON_Delete(asc_req_data);
    cJSON_free(asc_req_data_text);

    if(!_pcf_app_session_change_callback_call(sess, false))
        ogs_error("AppSessionContext change callback failed");
}

void pcf_policyauthorization_event_notification(ogs_sbi_message_t *recvmsg, ogs_sbi_response_t *response) {

    OpenAPI_events_notification_t *event_notif;
    cJSON *evt_notif = NULL;


    if(response->http.content) {
        {
            ogs_hash_index_t *hi;
            for (hi = ogs_hash_first(response->http.headers); hi; hi = ogs_hash_next(hi)) {
                if (!ogs_strcasecmp(ogs_hash_this_key(hi), OGS_SBI_CONTENT_TYPE)) {
                    if (ogs_strcasecmp(ogs_hash_this_val(hi), "application/json")) {
                        const char *type;
                        type = (const char *)ogs_hash_this_val(hi);
                        ogs_error( "Unsupported Media Type: received type: %s, should have been application/json", type);
                        //ogs_sbi_response_free(response);      
                        //ogs_free(recvmsg);
                        return;

                    }
                }
            }
        }
        evt_notif = cJSON_Parse(response->http.content);
        char *txt = cJSON_Print(evt_notif);
        ogs_debug("Parsed JSON: %s", txt);
        cJSON_free(txt);

        event_notif = OpenAPI_events_notification_parseFromJSON(evt_notif);
        ogs_assert(event_notif);
        cJSON_Delete(evt_notif);

        /* Do something with event_notif */

        /* Tidy up EventsNotification */
        OpenAPI_events_notification_free(event_notif);
    }
}

void pcf_policyauthorization_subscribe_event(pcf_app_session_t *sess, ogs_sbi_message_t *recvmsg, ogs_sbi_response_t *response)
{
    OpenAPI_events_subsc_put_data_t *events_subsc_put_data;

    if ((response->status == 201 && response->http.content && recvmsg->http.location) ||
        (response->status == 200 && response->http.content)) {
        cJSON *evt_subsc_put_data;

        evt_subsc_put_data = cJSON_Parse(response->http.content);
	char *txt = cJSON_Print(evt_subsc_put_data);
        ogs_debug("Parsed JSON: %s", txt);
        cJSON_free(txt);
	events_subsc_put_data = OpenAPI_events_subsc_put_data_parseFromJSON(evt_subsc_put_data);
       	cJSON_Delete(evt_subsc_put_data);

        /* Do something with events_subsc_put_data */

        /* Tidy up EventsSubscPutData */
	OpenAPI_events_subsc_put_data_free(events_subsc_put_data);

        /* Notify App about AppSessionContext change */
	if (!_pcf_app_session_change_callback_call(sess, false)) {
            ogs_error("AppSessionContext change callback failed");
	}
    } else if(response->status == 204) {
        /* Notify App about AppSessionContext change */
        if (!_pcf_app_session_change_callback_call(sess, false)) {
            ogs_error("AppSessionContext change callback failed");
        }
    } else {
        /* Notify App about AppSessionContext change failure */
	if (!_pcf_app_session_change_callback_call(sess, true)) {
            ogs_error("AppSessionContext change callback failed");
        }

        /* Delete AppSessionContext?? */
    }
}

void pcf_policyauthorization_create(
        pcf_app_session_t *sess, ogs_sbi_message_t *recvmsg, ogs_sbi_response_t *response)
{
    int rv;

    ogs_sbi_message_t message;
    ogs_sbi_header_t header;

    OpenAPI_app_session_context_t *AppSessionContext = NULL;
    OpenAPI_app_session_context_req_data_t *AscReqData = NULL;

    cJSON *app_sess_context;
    cJSON *asc_req_data;
    char *app_sess_context_text;
    char *asc_req_data_text;

    uint64_t supported_features = 0;

    if (!recvmsg->http.location) {
        ogs_error("[%s:%s] No http.location",
                sess->ipv4addr ? sess->ipv4addr : "Unknown",
                sess->ipv6addr ? sess->ipv6addr : "Unknown");
        return;
    }

    memset(&header, 0, sizeof(header));
    header.uri = recvmsg->http.location;

    rv = ogs_sbi_parse_header(&message, &header);
    if (rv != OGS_OK) {
        ogs_error("[%s:%s] Cannot parse http.location [%s]",
                sess->ipv4addr ? sess->ipv4addr : "Unknown",
                sess->ipv6addr ? sess->ipv6addr : "Unknown",
                recvmsg->http.location);
        return;
    }

    if (!message.h.resource.component[1]) {
        ogs_error("[%s:%s] No AppSessionId[%s]",
                sess->ipv4addr ? sess->ipv4addr : "Unknown",
                sess->ipv6addr ? sess->ipv6addr : "Unknown",
                recvmsg->http.location);
        goto cleanup;
    }

    AppSessionContext = recvmsg->AppSessionContext;
    if (!AppSessionContext) {
        ogs_error("[%s:%s] No AppSessionContext",
                sess->ipv4addr ? sess->ipv4addr : "Unknown",
                sess->ipv6addr ? sess->ipv6addr : "Unknown");
        goto cleanup;
    }

    _pcf_client_received_app_sess_context_set(sess, recvmsg->AppSessionContext);

    AscReqData = AppSessionContext->asc_req_data;
    if (!AscReqData) {
        ogs_error("[%s:%s] No AscReqData",
                sess->ipv4addr ? sess->ipv4addr : "Unknown",
                sess->ipv6addr ? sess->ipv6addr : "Unknown");
        goto cleanup;
    }

    if (!AscReqData->supp_feat) {
        ogs_error("[%s:%s] No AscReqData->suppFeat",
                sess->ipv4addr ? sess->ipv4addr : "Unknown",
                sess->ipv6addr ? sess->ipv6addr : "Unknown");
        goto cleanup;
    }

   
    supported_features = ogs_uint64_from_string(AscReqData->supp_feat);
    sess->policyauthorization_features &= supported_features;
    sess->pcf_app_session_id = ogs_strdup(message.h.resource.component[1]);

    app_sess_context = OpenAPI_app_session_context_convertToJSON(AppSessionContext);
    app_sess_context_text = cJSON_Print(app_sess_context);
    ogs_debug("App Session Context: %s", app_sess_context_text);
    cJSON_Delete(app_sess_context);
    cJSON_free(app_sess_context_text);
    
    if(!_pcf_app_session_change_callback_call(sess, false))
    {
        ogs_error("AppSessionContext change callback failed");
    }

cleanup:
    ogs_sbi_header_free(&header);
}

void pcf_policyauthorization_delete(pcf_app_session_t *app_sess)
{
    pcf_session_t *session;
    pcf_app_session_t *sess, *next = NULL;
    ogs_assert(app_sess);

    if(app_sess->pcf_session) {
        session = app_sess->pcf_session;
        ogs_assert(session);
    }

    ogs_list_for_each_safe(&session->pcf_app_sessions, next, sess){
        if(!strcmp(app_sess->pcf_app_session_id, sess->pcf_app_session_id)){
	    break;
	}

    }

   if(sess)
   {
       ogs_list_remove(&session->pcf_app_sessions, sess);

       pcf_app_sess_remove(sess);

   }


}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
