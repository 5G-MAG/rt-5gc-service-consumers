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

static void media_components_free(OpenAPI_list_t *MediaComponentList);
static void events_free(OpenAPI_list_t *EventList);

ogs_sbi_request_t *pcf_policyauthorization_request_create(
        pcf_app_session_t *sess, OpenAPI_list_t *media_component, int events)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;
    char pcf_ipv4addr[OGS_ADDRSTRLEN];
    int pcf_port;

    OpenAPI_events_subsc_req_data_t evSubsc;

    OpenAPI_snssai_t sNssai;

    OpenAPI_app_session_context_t AppSessionContext;
    OpenAPI_app_session_context_req_data_t AscReqData;
    cJSON *app_sess_context;
    cJSON *asc_req_data;
    char *app_sess_context_text;
    char *asc_req_data_text;

    ogs_assert(sess);

    OGS_ADDR(sess->pcf_session->pcf_addr, pcf_ipv4addr);
    pcf_port = OGS_PORT(sess->pcf_session->pcf_addr);

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_POST;
    message.h.service.name =
        (char *)OGS_SBI_SERVICE_NAME_NPCF_POLICYAUTHORIZATION;
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] =
        (char *)OGS_SBI_RESOURCE_NAME_APP_SESSIONS;

    message.param.ipv4addr = pcf_ipv4addr;
    
    message.h.uri = ogs_msprintf("http://%s:%i/npcf-policyauthorization/v1/app-sessions", pcf_ipv4addr, pcf_port);
    message.AppSessionContext = &AppSessionContext;

    memset(&AppSessionContext, 0, sizeof(AppSessionContext));
    AppSessionContext.asc_req_data = &AscReqData;

    memset(&AscReqData, 0, sizeof(AscReqData));

    AscReqData.notif_uri = ogs_strdup(_pcf_app_session_get_notif_url(sess));
    ogs_assert(AscReqData.notif_uri);

    ogs_debug("\n PCF policy authorization create (Request header): \n Method: [%s] \n URI: [%s] \n Service Name: [%s] \n Resource Component: [%s]\n", message.h.method, message.h.uri, message.h.service.name, message.h.resource.component[0]); 

    AscReqData.supp_feat =
        ogs_uint64_to_string(sess->policyauthorization_features);
    ogs_assert(AscReqData.supp_feat);

    AscReqData.ue_ipv4 = sess->ipv4addr;
    AscReqData.ue_ipv6 = sess->ipv6addr;

    AscReqData.dnn = sess->dnn;

    memset(&evSubsc, 0, sizeof(evSubsc));
    evSubsc.events = events_subsc_create(events);

    evSubsc.notif_uri = ogs_strdup(_pcf_app_session_get_notif_url(sess));
    AscReqData.ev_subsc = &evSubsc;

    memset(&sNssai, 0, sizeof(sNssai));
    if (sess->s_nssai.sst) {
        sNssai.sst = sess->s_nssai.sst;
        sNssai.sd = ogs_s_nssai_sd_to_string(sess->s_nssai.sd);
        AscReqData.slice_info = &sNssai;
    }

    AscReqData.spon_status = OpenAPI_sponsoring_status_SPONSOR_DISABLED;

    AscReqData.supi = sess->supi;
    AscReqData.gpsi = sess->gpsi;

    AscReqData.res_prio = OpenAPI_reserv_priority_PRIO_16;

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

    if(AscReqData.med_components)    
        media_components_free(media_component);    

    return request;
}

ogs_sbi_request_t *pcf_policyauthorization_request_update(pcf_app_session_t *sess, OpenAPI_list_t *media_component)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;
    ogs_sbi_server_t *server = NULL;
    ogs_sbi_header_t header;
    char pcf_addr[OGS_ADDRSTRLEN];
    int pcf_port;

    OpenAPI_app_session_context_update_data_patch_t AppSessionContextUpdateDataPatch;
    OpenAPI_app_session_context_update_data_t AscUpdateData;

    cJSON *app_sess_context;
    cJSON *asc_update_data;
    char *app_sess_context_text;
    char *asc_update_data_text;

    if(!sess) return NULL;
    if(!media_component) return NULL;

    OGS_ADDR(sess->pcf_session->pcf_addr, pcf_addr);
    pcf_port = OGS_PORT(sess->pcf_session->pcf_addr);

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_PATCH;
    message.h.service.name = (char *)OGS_SBI_SERVICE_NAME_NPCF_POLICYAUTHORIZATION;
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] = (char *)OGS_SBI_RESOURCE_NAME_APP_SESSIONS;
    message.h.resource.component[1] = sess->pcf_app_session_id;

    message.param.ipv4addr = pcf_addr;

    message.h.uri = ogs_msprintf("http://%s:%i/npcf-policyauthorization/v1/app-sessions/%s", pcf_addr, pcf_port, sess->pcf_app_session_id);

    message.AppSessionContextUpdateDataPatch = &AppSessionContextUpdateDataPatch;
    memset(&AppSessionContextUpdateDataPatch, 0, sizeof(AppSessionContextUpdateDataPatch));

    AppSessionContextUpdateDataPatch.asc_req_data = &AscUpdateData;
    memset(&AscUpdateData, 0, sizeof(AscUpdateData));

    server = ogs_list_first(&ogs_sbi_self()->server_list);
    ogs_assert(server);

    memset(&header, 0, sizeof(header));
    header.service.name = (char *)OGS_SBI_SERVICE_NAME_NPCF_POLICYAUTHORIZATION;
    header.api.version = (char *)OGS_SBI_API_V1;
    header.resource.component[0] = (char *)OGS_SBI_RESOURCE_NAME_APP_SESSIONS;

    ogs_debug("\n PCF policy authorization update (Request header): \n Method: [%s] \n URI: [%s] \n Service Name: [%s] \n Resource Component: [%s]\n", message.h.method, message.h.uri, header.service.name, message.h.resource.component[0]);

    AscUpdateData.spon_status = OpenAPI_sponsoring_status_SPONSOR_DISABLED;

    AscUpdateData.res_prio = OpenAPI_reserv_priority_PRIO_16;

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

    ogs_free(message.h.uri);

    if(AscUpdateData.med_components)
        media_components_free(media_component);

    return request;
}

ogs_sbi_request_t *pcf_policyauthorization_subscribe_request_event(pcf_app_session_t *sess)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;
    ogs_sbi_server_t *server = NULL;
    ogs_sbi_header_t header;
    char pcf_addr[OGS_ADDRSTRLEN];
    int pcf_port;

    cJSON *app_sess_context;
    char *app_sess_context_text;

    ogs_assert(sess);
    ogs_assert(sess->pcf_app_session_id);

    OGS_ADDR(sess->pcf_session->pcf_addr, pcf_addr);
    pcf_port = OGS_PORT(sess->pcf_session->pcf_addr);

    ogs_error("PCF address in build [%s:%d]",pcf_addr, pcf_port);

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_PUT;
    message.h.service.name =
        (char *)OGS_SBI_SERVICE_NAME_NPCF_POLICYAUTHORIZATION;
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] = (char *)OGS_SBI_RESOURCE_NAME_APP_SESSIONS;
    message.h.resource.component[1] = (char *)sess->pcf_app_session_id;
    message.h.resource.component[2] = "events-subscription";
    message.param.ipv4addr = pcf_addr;

    message.h.uri = ogs_msprintf("http://%s:%i/npcf-policyauthorization/v1/app-sessions/%s/events-subscription", pcf_addr, pcf_port, sess->pcf_app_session_id);

    message.AppSessionContext = sess->pcf_app_session_context_requested;

    server = ogs_list_first(&ogs_sbi_self()->server_list);
    ogs_assert(server);

    memset(&header, 0, sizeof(header));
    header.service.name = (char *)OGS_SBI_SERVICE_NAME_NPCF_POLICYAUTHORIZATION;
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

    char pcf_addr[OGS_ADDRSTRLEN];
    int pcf_port;

    ogs_assert(sess);

    OGS_ADDR(sess->pcf_session->pcf_addr, pcf_addr);
    pcf_port = OGS_PORT(sess->pcf_session->pcf_addr);
    request = ogs_sbi_request_new();
    request->h.method = ogs_strdup("DELETE");
    request->h.uri = ogs_msprintf("http://%s:%i/npcf-policyauthorization/v1/app-sessions/%s/events-subscription", pcf_addr, pcf_port, sess->pcf_app_session_id);
    request->h.api.version = ogs_strdup("v1");
    return request;

}

ogs_sbi_request_t *pcf_policyauthorization_req_subscribe_event(pcf_app_session_t *sess)
{

    ogs_sbi_request_t *request = NULL;
    ogs_sbi_server_t *server = NULL;

    OpenAPI_events_subsc_req_data_t *evt_subsc_req = NULL;

    char pcf_addr[OGS_ADDRSTRLEN];
    int pcf_port;
    cJSON *ev_subsc;
    char *data;

    ogs_assert(sess);

    evt_subsc_req = OpenAPI_events_subsc_req_data_copy(evt_subsc_req, sess->pcf_app_session_context_requested->asc_req_data->ev_subsc);
    ogs_assert(evt_subsc_req);

    server = ogs_list_first(&ogs_sbi_self()->server_list);
    ogs_assert(server);

    OGS_ADDR(sess->pcf_session->pcf_addr, pcf_addr);
    pcf_port = OGS_PORT(sess->pcf_session->pcf_addr);
    ev_subsc = OpenAPI_events_subsc_req_data_convertToJSON(evt_subsc_req); 
    data = cJSON_Print(ev_subsc);

    ogs_debug("JSON OpenAPI_events_subsc_req_data_convertToJSON: %s\n data_len %ld", data, strlen(data));

    request = ogs_sbi_request_new();
    request->h.method = ogs_strdup("PUT");
    request->h.uri = ogs_msprintf("http://%s:%i/npcf-policyauthorization/v1/app-sessions/%s/events-subscription", pcf_addr, pcf_port, sess->pcf_app_session_id);
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
    ogs_sbi_server_t *server = NULL;

    OpenAPI_events_subsc_req_data_t *evSubsc;
    OpenAPI_list_t *subsc_events = NULL;
    char *notif_uri = NULL;
    char pcf_addr[OGS_ADDRSTRLEN];
    int pcf_port;
    cJSON *ev_subsc;
    char *data;

    ogs_assert(sess);

    server = ogs_list_first(&ogs_sbi_self()->server_list);
    ogs_assert(server);

    evSubsc = OpenAPI_events_subsc_req_data_create(subsc_events, notif_uri, NULL, NULL, NULL, NULL, NULL, NULL, false, 0);


    OGS_ADDR(sess->pcf_session->pcf_addr, pcf_addr);
    pcf_port = OGS_PORT(sess->pcf_session->pcf_addr);

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_PUT;
    message.h.service.name =
        (char *)OGS_SBI_SERVICE_NAME_NPCF_POLICYAUTHORIZATION;
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] =
        (char *)OGS_SBI_RESOURCE_NAME_APP_SESSIONS;

    message.param.ipv4addr = pcf_addr;

    message.h.uri = ogs_msprintf("http://%s:%i/npcf-policyauthorization/v1/app-sessions/%s/events-subscription", pcf_addr, pcf_port, sess->pcf_app_session_id);	    

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

static void media_components_free(OpenAPI_list_t *MediaComponentList){

    OpenAPI_map_t *MediaComponentMap = NULL;
    OpenAPI_media_component_t *MediaComponent = NULL;

    OpenAPI_list_t *SubComponentList = NULL;
    OpenAPI_map_t *SubComponentMap = NULL;
    OpenAPI_media_sub_component_t *SubComponent = NULL;

    OpenAPI_list_t *fDescList = NULL;
    OpenAPI_list_t *codecList = NULL;

    OpenAPI_lnode_t *node = NULL, *node2 = NULL, *node3 = NULL;

    OpenAPI_list_for_each(MediaComponentList, node) {
        MediaComponentMap = node->data;
        if (MediaComponentMap) {
            MediaComponent = MediaComponentMap->value;
            if (MediaComponent) {

                if (MediaComponent->mar_bw_dl)
                    ogs_free(MediaComponent->mar_bw_dl);
                if (MediaComponent->mar_bw_ul)
                    ogs_free(MediaComponent->mar_bw_ul);
                if (MediaComponent->mir_bw_dl)
                    ogs_free(MediaComponent->mir_bw_dl);
                if (MediaComponent->mir_bw_ul)
                    ogs_free(MediaComponent->mir_bw_ul);
                if (MediaComponent->rr_bw)
                    ogs_free(MediaComponent->rr_bw);
                if (MediaComponent->rs_bw)
                    ogs_free(MediaComponent->rs_bw);

                codecList = MediaComponent->codecs;
                OpenAPI_list_for_each(codecList, node2) {
                    if (node2->data) ogs_free(node2->data);
                }
                OpenAPI_list_free(codecList);

                SubComponentList = MediaComponent->med_sub_comps;
                OpenAPI_list_for_each(SubComponentList, node2) {
                    SubComponentMap = node2->data;
                    if (SubComponentMap) {
                        SubComponent = SubComponentMap->value;
                        if (SubComponent) {

                            fDescList = SubComponent->f_descs;
                            OpenAPI_list_for_each(fDescList, node3) {
                                if (node3->data) ogs_free(node3->data);
                            }
                            OpenAPI_list_free(fDescList);

                            ogs_free(SubComponent);
                        }
                        if (SubComponentMap->key)
                            ogs_free(SubComponentMap->key);
                        ogs_free(SubComponentMap);
                    }
                }
                OpenAPI_list_free(SubComponentList);

                ogs_free(MediaComponent);
            }
            if (MediaComponentMap->key)
                ogs_free(MediaComponentMap->key);
            ogs_free(MediaComponentMap);
        }
    }
    OpenAPI_list_free(MediaComponentList);

}

ogs_sbi_request_t *pcf_policyauthorization_request_delete(pcf_app_session_t *sess)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;
    char pcf_addr[OGS_ADDRSTRLEN];
    int pcf_port;

    ogs_assert(sess);
    ogs_assert(sess->pcf_app_session_id);

    ogs_assert(sess->pcf_session);

    ogs_assert(sess->pcf_session->pcf_addr);
    
    OGS_ADDR(sess->pcf_session->pcf_addr, pcf_addr);
    pcf_port = OGS_PORT(sess->pcf_session->pcf_addr);

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_POST;
    message.h.service.name =
        (char *)OGS_SBI_SERVICE_NAME_NPCF_POLICYAUTHORIZATION;
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] =
        (char *)OGS_SBI_RESOURCE_NAME_APP_SESSIONS;
    message.h.resource.component[1] = sess->pcf_app_session_id;
    message.h.resource.component[2] =
        (char *)OGS_SBI_RESOURCE_NAME_DELETE;
    
    message.param.ipv4addr = pcf_addr;
    message.h.uri = ogs_msprintf("http://%s:%i/npcf-policyauthorization/v1/app-sessions/%s/%s", pcf_addr, pcf_port, sess->pcf_app_session_id, (char *)OGS_SBI_RESOURCE_NAME_DELETE);
    request = ogs_sbi_build_request(&message);
    ogs_free(message.h.uri);

    ogs_expect(request);

    return request;
}

