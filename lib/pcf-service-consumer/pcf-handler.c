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
    OpenAPI_app_session_context_t *AppSessionContext = NULL;
    cJSON *app_sess_context;
    char *app_sess_context_text;

    if (!recvmsg->h.resource.component[1]) {
        ogs_error("[%s:%s] No AppSessionId[%s]",
                sess->ipv4addr ? sess->ipv4addr : "Unknown",
                sess->ipv6addr ? sess->ipv6addr : "Unknown",
                recvmsg->http.location);
        _pcf_app_session_change_callback_call(sess, true);
        return;
    }

    /* TS 29.514 V19.8.0 table 5.3.3.3.2-3: the 200 of a PATCH carries the AppSessionContext after the modification */
    AppSessionContext = recvmsg->AppSessionContext;
    if (!AppSessionContext) {
        ogs_error("[%s:%s] No AppSessionContext",
                sess->ipv4addr ? sess->ipv4addr : "Unknown",
                sess->ipv6addr ? sess->ipv6addr : "Unknown");
        _pcf_app_session_change_callback_call(sess, true);
        return;
    }

    if(!_pcf_client_received_app_sess_context_set(sess, AppSessionContext))
        ogs_error("Failed to set the updated app session context");

    ogs_debug("\n PCF policy authorization update (Respone header): \n Method: [%s] \n URI: [%s] \n  Resource Component: [%s]\n", recvmsg->h.method, recvmsg->h.uri, recvmsg->h.resource.component[0]);

    app_sess_context = OpenAPI_app_session_context_convertToJSON(AppSessionContext);
    app_sess_context_text = cJSON_Print(app_sess_context);
    ogs_debug("Update app_sess_context_text: %s", app_sess_context_text);
    cJSON_Delete(app_sess_context);
    cJSON_free(app_sess_context_text);

    if(!_pcf_app_session_change_callback_call(sess, false))
        ogs_error("AppSessionContext change callback failed");

    /* the notification of events that the PCF added to its answer (clause 4.2.3.2) */
    if (AppSessionContext->evs_notif)
        _pcf_app_session_answer_notifications(sess, AppSessionContext->evs_notif);
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
        if (!evt_notif) {
            ogs_error("The PCF sent an EventsNotification that is not JSON");
            return;
        }
        {
            char *txt = cJSON_Print(evt_notif);
            ogs_debug("Parsed JSON: %s", txt);
            cJSON_free(txt);
        }

        event_notif = OpenAPI_events_notification_parseFromJSON(evt_notif);
        cJSON_Delete(evt_notif);
        if (!event_notif) {
            ogs_error("The PCF sent an EventsNotification that cannot be parsed");
            return;
        }

        /* Do something with event_notif */

        /* Tidy up EventsNotification */
        OpenAPI_events_notification_free(event_notif);
    }
}

void pcf_policyauthorization_subscribe_event(pcf_app_session_t *sess, ogs_sbi_message_t *recvmsg, ogs_sbi_response_t *response)
{
    OpenAPI_events_subsc_put_data_t *events_subsc_put_data;

    if (recvmsg->h.method && !strcmp(recvmsg->h.method, OGS_SBI_HTTP_METHOD_DELETE)) {
        /*
         * TS 29.514 V19.8.0 clause 4.2.7.2: the Events Subscription sub-resource is deleted with all its events (204). Implementation choice, text
         * silent about the consumer's own state, see CONSUMERS audit O10 (no CR): the events that the library holds for the application session
         * and the callbacks registered for them go as well, so that a later subscription sends only the events it names.
         */
        if (response->status == OGS_SBI_HTTP_STATUS_NO_CONTENT) {
            _pcf_app_session_clear_events(sess);
            if (!_pcf_app_session_change_callback_call(sess, false))
                ogs_error("AppSessionContext change callback failed");
        } else {
            if (!_pcf_app_session_change_callback_call(sess, true))
                ogs_error("AppSessionContext change callback failed");
        }
        return;
    }

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

        /* EventsSubscPutData "may contain the notification of the already met events" (table 5.6.2.7-1): it is an EventsNotification as well */
        {
            cJSON *json = cJSON_Parse(response->http.content);
            OpenAPI_events_notification_t *notifications = json ? OpenAPI_events_notification_parseFromJSON(json) : NULL;

            if (notifications) {
                _pcf_app_session_answer_notifications(sess, notifications);
                OpenAPI_events_notification_free(notifications);
            }
            if (json) cJSON_Delete(json);
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
    OpenAPI_app_session_context_t *AppSessionContext = NULL;
    OpenAPI_app_session_context_req_data_t *AscReqData = NULL;
    OpenAPI_app_session_context_resp_data_t *AscRespData = NULL;
    const char *supp_feat = NULL;
    char *path = NULL, *id = NULL;

    cJSON *app_sess_context;
    char *app_sess_context_text;

    uint64_t supported_features = 0, supported_features_hi = 0;

    if (!recvmsg->http.location) {
        ogs_error("[%s:%s] No http.location",
                sess->ipv4addr ? sess->ipv4addr : "Unknown",
                sess->ipv6addr ? sess->ipv6addr : "Unknown");
        /* the PCF said "created" without saying where: the application session context cannot be used */
        pcf_app_sess_remove(sess);
        return;
    }

    /*
     * TS 29.501 V19.5.0 clause 4.6.1.1.1.2: the Location is the URI of the created resource, absolute or relative to the target URI of the POST;
     * TS 29.514 V19.8.0 clause 4.2.2.2 builds the URIs of the sub-resources from it. Its last segment is the identifier of the context.
     */
    path = _location_path(recvmsg->http.location, "/npcf-policyauthorization/v1/app-sessions");
    id = path ? strrchr(path, '/') : NULL;
    if (!id || !id[1]) {
        ogs_error("[%s:%s] No AppSessionId[%s]",
                sess->ipv4addr ? sess->ipv4addr : "Unknown",
                sess->ipv6addr ? sess->ipv6addr : "Unknown",
                recvmsg->http.location);
        if (path) ogs_free(path);
        pcf_app_sess_remove(sess);
        return;
    }

    /*
     * Clause 5.8 and table 5.6.2.2-1: the PCF gives the supported features in the AppSessionContextRespData. The AppSessionContextReqData
     * that some PCFs repeat in the answer (the Open5GS PCF) carries them as well. TS 29.501 clause 4.6.1.1.1.2: the body of the 201 "should"
     * represent the created resource, so an answer may have none.
     */
    AppSessionContext = recvmsg->AppSessionContext;
    if (AppSessionContext) {
        _pcf_client_received_app_sess_context_set(sess, AppSessionContext);
        AscRespData = AppSessionContext->asc_resp_data;
        AscReqData = AppSessionContext->asc_req_data;
    }
    if (AscRespData && AscRespData->supp_feat) {
        supp_feat = AscRespData->supp_feat;
    } else if (AscReqData && AscReqData->supp_feat) {
        supp_feat = AscReqData->supp_feat;
    } else {
        /*
         * Implementation choice, text conflicting, see CR-055 (CONSUMERS audit O8): clause 5.8 has the PCF indicate the supported features in
         * the answer, but table 5.6.2.3-1 describes suppFeat as provided "only when feature negotiation needs to take place" (and TS 29.500
         * clause 6.6.2 takes the features that both sides support from what the server indicates). An answer without them has no optional
         * feature negotiated: the context that the PCF created is kept and the session goes on with none, instead of leaving a context
         * behind in the PCF that nobody holds.
         */
        ogs_warn("[%s:%s] No suppFeat in the answer: no optional feature is negotiated",
                sess->ipv4addr ? sess->ipv4addr : "Unknown",
                sess->ipv6addr ? sess->ipv6addr : "Unknown");
    }

    if (supp_feat)
        ogs_sbi_features_from_string(supp_feat, &supported_features, &supported_features_hi);
    sess->policyauthorization_features &= supported_features;
    sess->policyauthorization_features_hi &= supported_features_hi;
    sess->pcf_app_session_id = ogs_strdup(id + 1);
    sess->resource_path = path;

    if (AppSessionContext) {
        app_sess_context = OpenAPI_app_session_context_convertToJSON(AppSessionContext);
        app_sess_context_text = cJSON_Print(app_sess_context);
        ogs_debug("App Session Context: %s", app_sess_context_text);
        cJSON_Delete(app_sess_context);
        cJSON_free(app_sess_context_text);
    }

    if(!_pcf_app_session_change_callback_call(sess, false))
    {
        ogs_error("AppSessionContext change callback failed");
    }

    /* the notification of events that the PCF added to its answer (clause 4.2.2.2) */
    if (AppSessionContext && AppSessionContext->evs_notif)
        _pcf_app_session_answer_notifications(sess, AppSessionContext->evs_notif);
}

/*
 * The 200 answer to the deletion carries the AppSessionContext with the usage report of clause 4.2.4.3 (evsNotif); libsbi does not read the body of
 * this answer, so it is read here. The report goes to the callbacks, as a notification would.
 */
void pcf_policyauthorization_delete_answer(pcf_app_session_t *sess, ogs_sbi_response_t *response)
{
    cJSON *json, *evs_notif;
    OpenAPI_events_notification_t *notifications;

    if (!response->http.content) return;
    json = cJSON_Parse(response->http.content);
    if (!json) return;
    evs_notif = cJSON_GetObjectItemCaseSensitive(json, "evsNotif");
    notifications = evs_notif ? OpenAPI_events_notification_parseFromJSON(evs_notif) : NULL;
    if (notifications) {
        _pcf_app_session_answer_notifications(sess, notifications);
        OpenAPI_events_notification_free(notifications);
    }
    cJSON_Delete(json);
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
        /* the object itself: an application session of the same PCF session that the PCF has not answered yet has no identifier to compare */
        if (sess == app_sess) {
	    break;
	}

    }

   if(sess)
   {
       /* pcf_app_sess_remove() tells the requester and releases the object, which takes it out of the session's list */
       pcf_app_sess_remove(sess);
   }


}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
