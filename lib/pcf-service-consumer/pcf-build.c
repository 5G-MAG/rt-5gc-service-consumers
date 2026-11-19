/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-sbi.h"
#include "pcf-client.h"
#include "pcf-client-sess.h"
#include "context.h"
#include "pcf-build.h"
#include "pcf-evsubsc.h"
#include "utils.h"

static void events_free(OpenAPI_list_t *EventList);

/*
 * The URI of the Individual Application Session Context resource, or of its sub-resource: TS 29.514 V19.8.0 clause 4.2.2.2 builds them from the
 * Location of the answer to the creation (the path of it is kept; the PCF is reached at its configured address and port, which is where the
 * Location points for the PCF that answered), and TS 29.501 V19.5.0 clause 4.4.1 puts an IPv6 address into brackets. Implementation choice, text
 * silent about a Location with another authority (it would need a client of its own), see CONSUMERS audit O4 (no CR): the path of the Location is
 * used with the configured address. The caller releases the text.
 */
static char *session_uri(const pcf_app_session_t *sess, const char *suffix)
{
    char *authority = _pcf_authority(sess->pcf_session->pcf_addr), *uri;

    if (!suffix) suffix = "";
    if (sess->resource_path)
        uri = ogs_msprintf("http://%s%s%s", authority, sess->resource_path, suffix);
    else
        uri = ogs_msprintf("http://%s/npcf-policyauthorization/v1/app-sessions/%s%s", authority, sess->pcf_app_session_id, suffix);
    ogs_free(authority);
    return uri;
}

/*
 * The subscriptions that the text ties to what the media components ask for (the consumer "shall also subscribe"): TS 29.514 V19.8.0 clause 4.2.2.32,
 * alternative service requirements need the notifications of the outcome of the resources allocation and of the QoS notification control, and
 * clause 4.2.2.43, the indication of ECN marking for L4S needs the L4S_SUPP event. They are added to the events when the application did not give them.
 */
static void add_event_if_missing(OpenAPI_list_t *events, OpenAPI_npcf_af_event_e event)
{
    OpenAPI_lnode_t *node = NULL;
    OpenAPI_af_event_subscription_t *added;

    OpenAPI_list_for_each(events, node) {
        OpenAPI_af_event_subscription_t *held = node->data;

        if (held && held->event == event) return;
    }
    added = ogs_calloc(1, sizeof(*added));
    ogs_assert(added);
    added->event = event;
    added->notif_method = OpenAPI_af_notif_method_EVENT_DETECTION;
    OpenAPI_list_add(events, added);
}

static void ensure_companion_events(OpenAPI_list_t *events, OpenAPI_list_t *media_components)
{
    OpenAPI_lnode_t *node = NULL;

    OpenAPI_list_for_each(media_components, node) {
        OpenAPI_map_t *entry = node->data;
        OpenAPI_media_component_t *component = entry ? entry->value : NULL;

        if (!component) continue;
        if ((component->alt_ser_reqs && component->alt_ser_reqs->count) || (component->alt_ser_reqs_data && component->alt_ser_reqs_data->count)) {
            add_event_if_missing(events, OpenAPI_npcf_af_event_SUCCESSFUL_RESOURCES_ALLOCATION);
            add_event_if_missing(events, OpenAPI_npcf_af_event_QOS_NOTIF);
        }
        if (component->l4s_ind != OpenAPI_uplink_downlink_support_NULL)
            add_event_if_missing(events, OpenAPI_npcf_af_event_L4S_SUPP);
    }
}

ogs_sbi_request_t *pcf_policyauthorization_request_create(
        pcf_app_session_t *sess, OpenAPI_list_t *media_component, const pcf_app_session_options_t *options)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;
    char *authority;

    OpenAPI_events_subsc_req_data_t evSubsc;

    OpenAPI_snssai_t sNssai;

    OpenAPI_app_session_context_t AppSessionContext;
    OpenAPI_app_session_context_req_data_t AscReqData;
    cJSON *app_sess_context;
    cJSON *asc_req_data;
    char *app_sess_context_text;
    char *asc_req_data_text;

    /* what the request needs has to exist before anything is built: the caller is told, the NF is not stopped */
    if (!sess || !sess->pcf_session || !sess->pcf_session->pcf_addr) {
        ogs_error("The PCF address of the application session is not known");
        return NULL;
    }
    if (!_pcf_app_session_get_notif_url(sess)) {
        ogs_error("No notification server can be created for the application session");
        return NULL;
    }

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_POST;
    message.h.service.name =
        (char *)OpenAPI_service_name_ToString(OpenAPI_service_name_npcf_policyauthorization);
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] =
        (char *)OGS_SBI_RESOURCE_NAME_APP_SESSIONS;

    authority = _pcf_authority(sess->pcf_session->pcf_addr);
    message.h.uri = ogs_msprintf("http://%s/npcf-policyauthorization/v1/app-sessions", authority);
    ogs_free(authority);
    message.AppSessionContext = &AppSessionContext;

    memset(&AppSessionContext, 0, sizeof(AppSessionContext));
    AppSessionContext.asc_req_data = &AscReqData;

    memset(&AscReqData, 0, sizeof(AscReqData));

    AscReqData.notif_uri = ogs_strdup(_pcf_app_session_get_notif_url(sess));

    ogs_debug("\n PCF policy authorization create (Request header): \n Method: [%s] \n URI: [%s] \n Service Name: [%s] \n Resource Component: [%s]\n", message.h.method, message.h.uri, message.h.service.name, message.h.resource.component[0]); 

    AscReqData.supp_feat =
        ogs_sbi_features_to_string(sess->policyauthorization_features, sess->policyauthorization_features_hi);
    ogs_assert(AscReqData.supp_feat);

    AscReqData.ue_ipv4 = sess->ipv4addr;
    AscReqData.ue_ipv6 = sess->ipv6addr;

    /*
     * Implementation choice, text silent, see CONSUMERS audit O6 (no CR): supi, gpsi and dnn of the application session are never set (the fields of
     * ue_network_identifier_t are not copied to it): they are optional ("may"), and the PCF matches every attribute that it is given and takes
     * the DNN in the form that it uses itself (table 5.6.2.3-1 notes 2 and 7), so a value of the application that is not that of the PCF would
     * make the binding fail. The UE address alone binds the application session.
     */
    AscReqData.dnn = sess->dnn;

    memset(&evSubsc, 0, sizeof(evSubsc));
    evSubsc.events = events_subsc_create(options->events);
    if (options->events_subscription) {
        /* the details of the subscription are the caller's (borrowed); its events replace those of the same type in the list */
        const OpenAPI_events_subsc_req_data_t *extra = options->events_subscription;
        OpenAPI_lnode_t *node = NULL, *next = NULL, *node2 = NULL;

        OpenAPI_list_for_each(extra->events, node) {
            OpenAPI_af_event_subscription_t *copy = NULL, *given = node->data;

            if (!given)
                continue;
            OpenAPI_list_for_each(evSubsc.events, node2) {
                OpenAPI_af_event_subscription_t *held = node2->data;

                if (held && held->event == given->event) {
                    ogs_free(held);
                    node2->data = NULL;
                }
            }
            copy = ogs_calloc(1, sizeof(*copy));
            ogs_assert(copy);
            *copy = *given;
            OpenAPI_list_add(evSubsc.events, copy);
        }
        /* the entries that were replaced are taken out of the list */
        for (node = evSubsc.events->first; node; node = next) {
            next = node->next;
            if (!node->data)
                OpenAPI_list_remove(evSubsc.events, node);
        }
        evSubsc.req_qos_mon_params = extra->req_qos_mon_params;
        evSubsc.qos_mon = extra->qos_mon;
        evSubsc.qos_mon_dat_rate = extra->qos_mon_dat_rate;
        evSubsc.pdv_req_mon_params = extra->pdv_req_mon_params;
        evSubsc.pdv_mon = extra->pdv_mon;
        evSubsc.congest_mon = extra->congest_mon;
        evSubsc.rtt_mon = extra->rtt_mon;
        evSubsc.avl_bit_rate_mon = extra->avl_bit_rate_mon;
        evSubsc.rtt_flow_ref = extra->rtt_flow_ref;
        evSubsc.req_anis = extra->req_anis;
        evSubsc.usg_thres = extra->usg_thres;
        evSubsc.notif_corre_id = extra->notif_corre_id;
        evSubsc.af_app_ids = extra->af_app_ids;
        evSubsc.is_direct_notif_ind = extra->is_direct_notif_ind;
        evSubsc.direct_notif_ind = extra->direct_notif_ind;
        evSubsc.is_avrg_wndw = extra->is_avrg_wndw;
        evSubsc.avrg_wndw = extra->avrg_wndw;
    }

    ensure_companion_events(evSubsc.events, media_component);

    evSubsc.notif_uri = ogs_strdup(_pcf_app_session_get_notif_url(sess));
    /* TS 29.514 V19.8.0 table 5.6.2.6-1: events has 1..N entries, so there is no event subscription to send without an event */
    if (evSubsc.events->count) AscReqData.ev_subsc = &evSubsc;

    memset(&sNssai, 0, sizeof(sNssai));
    if (sess->s_nssai.sst) {
        sNssai.sst = sess->s_nssai.sst;
        sNssai.sd = ogs_s_nssai_sd_to_string(sess->s_nssai.sd);
        AscReqData.slice_info = &sNssai;
    }

    /* TS 29.514 V19.8.0 clause 4.2.2.5: the sponsor is given with the identity of the application service provider */
    AscReqData.asp_id = (char *)options->asp_id;
    AscReqData.spon_id = (char *)options->spon_id;
    /*
     * Implementation choice, text silent, see CONSUMERS audit O7 (no CR): table 5.6.2.3-1 gives sponStatus for the SponsoredConnectivity feature and
     * says that its absence means that sponsored connectivity is enabled; clause 4.2.2.5 has the sponsor given with aspId and sponId. Without
     * the identities of a sponsor there is no sponsoring to enable or disable, so no sponStatus is sent.
     */
    if (options->asp_id && options->spon_id)
        AscReqData.spon_status = options->sponsoring_enabled ?
            OpenAPI_sponsoring_status_SPONSOR_ENABLED : OpenAPI_sponsoring_status_SPONSOR_DISABLED;

    /* the IP domain is the one thing of the UE network identifier that goes with the address (see the note at ue_network_identifier_t) */
    AscReqData.ip_domain = sess->ip_domain;
    AscReqData.supi = sess->supi;
    AscReqData.gpsi = sess->gpsi;

    /*
     * Implementation choice, text silent, see CONSUMERS audit O1 (no CR): resPrio is optional ("may") and the PCF "may take the priority into
     * account", so it is sent only when the application gives one (it was always PRIO_16, the highest priority, before)
     */
    AscReqData.res_prio = options->res_prio;

    AscReqData.med_components = media_component;

    if(!_pcf_client_requested_app_sess_context_set(sess, &AppSessionContext))
        ogs_error("Failed to store the requested app session context");
    
    app_sess_context = OpenAPI_app_session_context_convertToJSON(sess->pcf_app_session_context_requested);
    
    app_sess_context_text = cJSON_Print(app_sess_context);
    ogs_debug("app_sess_context_text: %s", app_sess_context_text);
    cJSON_Delete(app_sess_context);
    cJSON_free(app_sess_context_text);
    
    asc_req_data = OpenAPI_app_session_context_req_data_convertToJSON(&AscReqData);
    asc_req_data_text = cJSON_Print(asc_req_data);
    ogs_debug("asc_req_data_text: %s", asc_req_data_text);
    cJSON_Delete(asc_req_data);
    cJSON_free(asc_req_data_text);

    request = ogs_sbi_build_request(&message);
    ogs_expect(request);
    
    ogs_free(message.h.uri);
 
    if (AscReqData.notif_uri)
        ogs_free(AscReqData.notif_uri);

    if (AscReqData.supp_feat)
        ogs_free(AscReqData.supp_feat);
    if (evSubsc.events)
        events_free(evSubsc.events);

    if(evSubsc.notif_uri)
        ogs_free(evSubsc.notif_uri);	    

    if (sNssai.sd)
        ogs_free(sNssai.sd);

    /* the media components belong to the caller */
    return request;
}

ogs_sbi_request_t *pcf_policyauthorization_request_update(pcf_app_session_t *sess, OpenAPI_list_t *media_component)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;
    ogs_sbi_header_t header;

    OpenAPI_app_session_context_update_data_patch_t AppSessionContextUpdateDataPatch;
    OpenAPI_app_session_context_update_data_t AscUpdateData;

    cJSON *app_sess_context;
    cJSON *asc_update_data;
    char *app_sess_context_text;
    char *asc_update_data_text;

    if(!sess) return NULL;
    if(!media_component) return NULL;
    if (!sess->pcf_session || !sess->pcf_session->pcf_addr || !sess->pcf_app_session_id) {
        ogs_error("The application session has not been created in the PCF");
        return NULL;
    }


    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_PATCH;
    message.h.service.name = (char *)OpenAPI_service_name_ToString(OpenAPI_service_name_npcf_policyauthorization);
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] = (char *)OGS_SBI_RESOURCE_NAME_APP_SESSIONS;
    message.h.resource.component[1] = sess->pcf_app_session_id;

    message.h.uri = session_uri(sess, NULL);

    message.AppSessionContextUpdateDataPatch = &AppSessionContextUpdateDataPatch;
    memset(&AppSessionContextUpdateDataPatch, 0, sizeof(AppSessionContextUpdateDataPatch));

    AppSessionContextUpdateDataPatch.asc_req_data = &AscUpdateData;
    memset(&AscUpdateData, 0, sizeof(AscUpdateData));

    memset(&header, 0, sizeof(header));
    header.service.name = (char *)OpenAPI_service_name_ToString(OpenAPI_service_name_npcf_policyauthorization);
    header.api.version = (char *)OGS_SBI_API_V1;
    header.resource.component[0] = (char *)OGS_SBI_RESOURCE_NAME_APP_SESSIONS;

    ogs_debug("\n PCF policy authorization update (Request header): \n Method: [%s] \n URI: [%s] \n Service Name: [%s] \n Resource Component: [%s]\n", message.h.method, message.h.uri, header.service.name, message.h.resource.component[0]);

    /* no resPrio: a PATCH that does not speak of it leaves the priority of the creation as it is (RFC 7396); see the note at the creation (O1) */

    AscUpdateData.med_components = media_component;

    if(!_pcf_client_app_sess_context_updates_set(sess, &AppSessionContextUpdateDataPatch))
        ogs_error("Failed to store the app session updates context");

    app_sess_context = OpenAPI_app_session_context_update_data_patch_convertToJSON(sess->pcf_app_session_context_updates);

    app_sess_context_text = cJSON_Print(app_sess_context);
    ogs_debug("app_sess_context_text: %s", app_sess_context_text);
    cJSON_Delete(app_sess_context);
    cJSON_free(app_sess_context_text);

    asc_update_data = OpenAPI_app_session_context_update_data_convertToJSON(&AscUpdateData);
    asc_update_data_text = cJSON_Print(asc_update_data);
    ogs_debug("asc_update_data_text: %s", asc_update_data_text);
    cJSON_Delete(asc_update_data);
    cJSON_free(asc_update_data_text);

    request = ogs_sbi_build_request(&message);
    ogs_expect(request);

    /* TS 29.514 V19.8.0 clause 4.2.3.2: the body of the PATCH is a JSON Merge Patch and is signalled as such */
    if (request)
        ogs_sbi_header_set(request->http.headers, OGS_SBI_CONTENT_TYPE, OGS_SBI_CONTENT_MERGE_PATCH_TYPE);

    ogs_free(message.h.uri);

    /* the media components belong to the caller */
    return request;
}

ogs_sbi_request_t *pcf_policyauthorization_subscribe_request_event(pcf_app_session_t *sess)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;
    ogs_sbi_header_t header;

    cJSON *app_sess_context;
    char *app_sess_context_text;

    if (!sess || !sess->pcf_session || !sess->pcf_session->pcf_addr || !sess->pcf_app_session_id) {
        ogs_error("The application session has not been created in the PCF");
        return NULL;
    }


    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_PUT;
    message.h.service.name =
        (char *)OpenAPI_service_name_ToString(OpenAPI_service_name_npcf_policyauthorization);
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] = (char *)OGS_SBI_RESOURCE_NAME_APP_SESSIONS;
    message.h.resource.component[1] = (char *)sess->pcf_app_session_id;
    message.h.resource.component[2] = "events-subscription";

    message.h.uri = session_uri(sess, "/events-subscription");

    message.AppSessionContext = sess->pcf_app_session_context_requested;

    memset(&header, 0, sizeof(header));
    header.service.name = (char *)OpenAPI_service_name_ToString(OpenAPI_service_name_npcf_policyauthorization);
    header.api.version = (char *)OGS_SBI_API_V1;
    header.resource.component[0] = (char *)OGS_SBI_RESOURCE_NAME_APP_SESSIONS;
    header.resource.component[1] = (char *)sess->pcf_app_session_id;

    ogs_error("\n PCF policy authorization create (Request header): \n Method: [%s] \n URI: [%s] \n Service Name: [%s] \n Resource Component: [%s]\n", message.h.method, message.h.uri, header.service.name, message.h.resource.component[0]);

    app_sess_context = OpenAPI_app_session_context_convertToJSON(sess->pcf_app_session_context_requested);

    app_sess_context_text = cJSON_Print(app_sess_context);
    ogs_error("app_sess_context_text: %s", app_sess_context_text);
    cJSON_Delete(app_sess_context);
    cJSON_free(app_sess_context_text);

    request = ogs_sbi_build_request(&message);
    ogs_expect(request);

    //ogs_free(message.h.uri);

    return request;
}

ogs_sbi_request_t *pcf_policyauthorization_req_unsubscribe_event(pcf_app_session_t *sess)
{

    ogs_sbi_request_t *request = NULL;


    if (!sess || !sess->pcf_session || !sess->pcf_session->pcf_addr || !sess->pcf_app_session_id) {
        ogs_error("The application session has not been created in the PCF");
        return NULL;
    }

    request = ogs_sbi_request_new();
    request->h.method = ogs_strdup("DELETE");
    request->h.uri = session_uri(sess, "/events-subscription");
    request->h.api.version = ogs_strdup("v1");
    return request;

}

ogs_sbi_request_t *pcf_policyauthorization_req_subscribe_event(pcf_app_session_t *sess)
{

    ogs_sbi_request_t *request = NULL;

    OpenAPI_events_subsc_req_data_t *evt_subsc_req = NULL;

    cJSON *ev_subsc;
    char *data;

    if (!sess || !sess->pcf_session || !sess->pcf_session->pcf_addr || !sess->pcf_app_session_id) {
        ogs_error("The application session has not been created in the PCF");
        return NULL;
    }
    if (!sess->pcf_app_session_context_requested || !sess->pcf_app_session_context_requested->asc_req_data ||
            !sess->pcf_app_session_context_requested->asc_req_data->ev_subsc) {
        ogs_error("The application session has no event subscription to send");
        return NULL;
    }

    evt_subsc_req = OpenAPI_events_subsc_req_data_copy(evt_subsc_req, sess->pcf_app_session_context_requested->asc_req_data->ev_subsc);
    if (!evt_subsc_req) {
        ogs_error("The event subscription cannot be copied");
        return NULL;
    }

    ev_subsc = OpenAPI_events_subsc_req_data_convertToJSON(evt_subsc_req); 
    data = cJSON_Print(ev_subsc);

    ogs_debug("JSON OpenAPI_events_subsc_req_data_convertToJSON: %s\n data_len %ld", data, strlen(data));

    request = ogs_sbi_request_new();
    request->h.method = ogs_strdup("PUT");
    request->h.uri = session_uri(sess, "/events-subscription");
    request->h.api.version = ogs_strdup("v1");

    if (data) {
        request->http.content = ogs_strdup(data);
        request->http.content_length = strlen(data);
        ogs_sbi_header_set(request->http.headers, "Content-Type", "application/json");
    }
    
    ogs_debug("\n Request:\n Method:[%s] \n URI: [%s] \n JSON : %s\n data_len: [%ld] \n", request->h.method, request->h.uri, request->http.content, request->http.content_length);

    cJSON_Delete(ev_subsc);
    cJSON_free(data);

    OpenAPI_events_subsc_req_data_free(evt_subsc_req);

    return request;

}

ogs_sbi_request_t *pcf_policyauthorization_request_subscribe_event(pcf_app_session_t *sess, int events)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;

    OpenAPI_events_subsc_req_data_t *evSubsc;
    OpenAPI_list_t *subsc_events = NULL;
    char *notif_uri = NULL;
    cJSON *ev_subsc;
    char *data;

    if (!sess || !sess->pcf_session || !sess->pcf_session->pcf_addr || !sess->pcf_app_session_id) {
        ogs_error("The application session has not been created in the PCF");
        return NULL;
    }

    evSubsc = OpenAPI_events_subsc_req_data_create(
            subsc_events, notif_uri,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
            false, 0, false, 0);



    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_PUT;
    message.h.service.name =
        (char *)OpenAPI_service_name_ToString(OpenAPI_service_name_npcf_policyauthorization);
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] =
        (char *)OGS_SBI_RESOURCE_NAME_APP_SESSIONS;

    message.h.uri = session_uri(sess, "/events-subscription");	    

    ev_subsc = OpenAPI_events_subsc_req_data_convertToJSON(evSubsc);
    data = cJSON_Print(ev_subsc);

    request = ogs_sbi_build_request(&message);
    ogs_expect(request);

    if (data) {
        request->http.content = ogs_strdup(data);
        request->http.content_length = strlen(data);
	ogs_sbi_header_set(request->http.headers, "Content-Type", "application/json");
    }

    if (evSubsc->events)
        events_free(evSubsc->events);
    
    cJSON_Delete(ev_subsc);
    cJSON_free(data);

    OpenAPI_events_subsc_req_data_free(evSubsc);

    return request;


}

static void events_free(OpenAPI_list_t *EventList){
    OpenAPI_af_event_subscription_t *Event = NULL;
    OpenAPI_lnode_t *node = NULL;
    OpenAPI_list_for_each(EventList, node) {
        Event = node->data;
        if (Event)
            ogs_free(Event);
    }
    OpenAPI_list_free(EventList);

}

/*
 * Release a list of media components (a map from the component number to the component). The components are released
 * with the generated free functions of their own type: the update request carries the "rm" variant, whose layout
 * differs from that of the create request.
 */
void _pcf_media_components_free(OpenAPI_list_t *MediaComponentList, bool is_rm)
{
    OpenAPI_lnode_t *node = NULL;

    if (!MediaComponentList) return;

    OpenAPI_list_for_each(MediaComponentList, node) {
        OpenAPI_map_t *MediaComponentMap = node->data;

        if (!MediaComponentMap) continue;
        if (MediaComponentMap->value) {
            if (is_rm)
                OpenAPI_media_component_rm_free(MediaComponentMap->value);
            else
                OpenAPI_media_component_free(MediaComponentMap->value);
        }
        if (MediaComponentMap->key)
            ogs_free(MediaComponentMap->key);
        ogs_free(MediaComponentMap);
    }
    OpenAPI_list_free(MediaComponentList);
}

ogs_sbi_request_t *pcf_policyauthorization_request_delete(pcf_app_session_t *sess)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;

    if (!sess || !sess->pcf_session || !sess->pcf_session->pcf_addr || !sess->pcf_app_session_id) {
        ogs_error("The application session has not been created in the PCF");
        return NULL;
    }
    

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_POST;
    message.h.service.name =
        (char *)OpenAPI_service_name_ToString(OpenAPI_service_name_npcf_policyauthorization);
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] =
        (char *)OGS_SBI_RESOURCE_NAME_APP_SESSIONS;
    message.h.resource.component[1] = sess->pcf_app_session_id;
    message.h.resource.component[2] =
        (char *)OGS_SBI_RESOURCE_NAME_DELETE;
    
    message.h.uri = session_uri(sess, "/" OGS_SBI_RESOURCE_NAME_DELETE);
    request = ogs_sbi_build_request(&message);
    ogs_free(message.h.uri);

    ogs_expect(request);

    return request;
}

