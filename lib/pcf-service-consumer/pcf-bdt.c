/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2022 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "pcf-bdt.h"
#include "context.h"

/*
 * Npcf_BDTPolicyControl (TS 29.554 V19.2.1): the bodies are JSON that is read and written here, as the generated models of its data types
 * (BdtPolicyData, TransferPolicy) are those of the Nudr_DataRepository and have other attributes.
 */

/* Feature 3, PatchCorrection (table 5.8-1): a transfer policy is selected with a PatchBdtPolicy */
#define BDT_FEATURE_PATCH_CORRECTION 3
#define BDT_OFFERED_FEATURES ((uint64_t)1 << (BDT_FEATURE_PATCH_CORRECTION - 1))

static char *base_uri(const pcf_session_t *session)
{
    char addr[OGS_ADDRSTRLEN];

    if (!session || !session->pcf_addr)
        return NULL;
    OGS_ADDR(session->pcf_addr, addr);
    return ogs_msprintf(session->pcf_addr->ogs_sa_family == AF_INET6 ?
            "http://[%s]:%d/npcf-bdtpolicycontrol/v1/bdtpolicies" : "http://%s:%d/npcf-bdtpolicycontrol/v1/bdtpolicies",
            addr, OGS_PORT(session->pcf_addr));
}

static OGS_LIST(all_policies);

static void policy_release(pcf_bdt_policy_t *policy)
{
    ogs_assert(policy);
    ogs_list_remove(&all_policies, policy);
    if (policy->xact) ogs_sbi_xact_remove(policy->xact);
    if (policy->uri)
        ogs_free(policy->uri);
    if (policy->bdt_ref_id)
        ogs_free(policy->bdt_ref_id);
    if (policy->transf_policies)
        cJSON_Delete(policy->transf_policies);
    if (policy->pending_body)
        cJSON_Delete(policy->pending_body);
    if (policy->pending_method)
        ogs_free(policy->pending_method);
    if (policy->pending_id)
        ogs_free(policy->pending_id);
    if (policy->pending_text)
        ogs_free(policy->pending_text);
    ogs_free(policy);
}

void _pcf_bdt_policies_free_all(void)
{
    pcf_bdt_policy_t *policy = NULL, *next = NULL;

    ogs_list_for_each_safe(&all_policies, next, policy)
        policy_release(policy);
}

void _pcf_bdt_policies_remove_all(pcf_session_t *session)
{
    pcf_bdt_policy_t *policy = NULL, *next = NULL;

    ogs_assert(session);
    ogs_list_for_each_safe(&all_policies, next, policy) {
        if (policy->pcf_session == session)
            policy_release(policy);
    }
}

static bool policy_exists(const pcf_bdt_policy_t *policy)
{
    pcf_bdt_policy_t *p = NULL;

    if (!policy)
        return false;
    ogs_list_for_each(&all_policies, p)
        if (p == policy)
            return true;
    return false;
}

static pcf_bdt_policy_t *policy_new(pcf_session_t *session, pcf_bdt_operation_t operation,
        pcf_bdt_result_callback callback, void *user_data)
{
    pcf_bdt_policy_t *policy = ogs_calloc(1, sizeof(*policy));

    if (!policy)
        return NULL;
    policy->pcf_session = session;
    policy->operation = operation;
    policy->callback = callback;
    policy->user_data = user_data;
    ogs_list_add(&all_policies, policy);
    return policy;
}

static ogs_sbi_request_t *build_request(const char *method, const char *uri, const char *content)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;
    bool patch = !strcmp(method, OGS_SBI_HTTP_METHOD_PATCH);

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)method;
    message.h.uri = (char *)uri;
    request = ogs_sbi_build_request(&message);
    if (request && content) {
        ogs_sbi_header_set(request->http.headers, OGS_SBI_CONTENT_TYPE,
                patch ? OGS_SBI_CONTENT_MERGE_PATCH_TYPE : OGS_SBI_CONTENT_JSON_TYPE);
        request->http.content = ogs_strdup(content);
        request->http.content_length = strlen(content);
    }
    return request;
}

static bool send_discovered(pcf_bdt_policy_t *policy, const char *method, const char *id, cJSON *body);

static bool send_request(pcf_bdt_policy_t *policy, const char *method, const char *uri, cJSON *body)
{
    ogs_sbi_request_t *request = NULL;
    char *text = NULL;
    bool rv;

    /* a policy that was found through the NRF has no session: its requests go the same way, to the resource named by its identifier */
    if (!policy->pcf_session) {
        const char *slash = strrchr(uri, '/');

        return send_discovered(policy, method, (slash && slash[1]) ? slash + 1 : NULL, body);
    }
    text = body ? cJSON_PrintUnformatted(body) : NULL;

    request = build_request(method, uri, text);
    if (text)
        cJSON_free(text);
    if (!request) {
        ogs_error("The request to the BDT policy control service cannot be built");
        return false;
    }
    rv = ogs_sbi_client_send_request(policy->pcf_session->client, client_notify_cb, request, policy);
    if (!rv)
        ogs_error("Error sending request to the BDT policy control service");
    ogs_sbi_request_free(request);
    return rv;
}

/* The first request of a policy whose PCF is found through the NRF: built when the NRF has answered and the instance is chosen */
static ogs_sbi_request_t *discovered_request(void *context, void *data)
{
    pcf_bdt_policy_t *policy = context;
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;

    if (!policy_exists(policy) || !policy->pending_method)
        return NULL;
    memset(&message, 0, sizeof(message));
    message.h.method = policy->pending_method;
    message.h.service.name = (char *)OpenAPI_service_name_ToString(OpenAPI_service_name_npcf_bdtpolicycontrol);
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] = (char *)OGS_SBI_RESOURCE_NAME_BDT_POLICIES;
    message.h.resource.component[1] = policy->pending_id;
    request = ogs_sbi_build_request(&message);
    if (request && policy->pending_text) {
        ogs_sbi_header_set(request->http.headers, OGS_SBI_CONTENT_TYPE,
                !strcmp(policy->pending_method, OGS_SBI_HTTP_METHOD_PATCH) ? OGS_SBI_CONTENT_MERGE_PATCH_TYPE : OGS_SBI_CONTENT_JSON_TYPE);
        request->http.content = ogs_strdup(policy->pending_text);
        request->http.content_length = strlen(policy->pending_text);
    }
    return request;
}

static bool send_discovered(pcf_bdt_policy_t *policy, const char *method, const char *id, cJSON *body)
{
    char *text = body ? cJSON_PrintUnformatted(body) : NULL;

    if (policy->pending_method) ogs_free(policy->pending_method);
    if (policy->pending_id) ogs_free(policy->pending_id);
    if (policy->pending_text) ogs_free(policy->pending_text);
    policy->pending_method = ogs_strdup(method);
    policy->pending_id = id ? ogs_strdup(id) : NULL;
    policy->pending_text = text ? ogs_strdup(text) : NULL;
    if (text)
        cJSON_free(text);
    policy->xact = ogs_sbi_xact_add(OGS_INVALID_POOL_ID, &policy->sbi, OpenAPI_service_name_npcf_bdtpolicycontrol, NULL,
            (ogs_sbi_build_f)discovered_request, policy, NULL);
    if (!policy->xact) {
        ogs_error("The transaction to find the PCF cannot be made");
        return false;
    }
    if (ogs_sbi_discover_and_send(policy->xact) != OGS_OK) {
        ogs_error("The PCF cannot be looked up in the NRF");
        ogs_sbi_xact_remove(policy->xact);
        policy->xact = NULL;
        return false;
    }
    return true;
}

static cJSON *request_json(const pcf_bdt_request_t *r)
{
    cJSON *body = cJSON_CreateObject(), *window = NULL, *volume = NULL, *snssai = NULL;
    char *feat = NULL, sd[8];

    ogs_assert(body);
    cJSON_AddStringToObject(body, "aspId", r->asp_id);
    window = cJSON_AddObjectToObject(body, "desTimeInt");
    ogs_assert(window);
    cJSON_AddStringToObject(window, "startTime", r->start_time);
    cJSON_AddStringToObject(window, "stopTime", r->stop_time);
    cJSON_AddNumberToObject(body, "numOfUes", r->num_of_ues);
    volume = cJSON_AddObjectToObject(body, "volPerUe");
    ogs_assert(volume);
    if (r->total_volume >= 0)
        cJSON_AddNumberToObject(volume, "totalVolume", (double)r->total_volume);
    if (r->downlink_volume >= 0)
        cJSON_AddNumberToObject(volume, "downlinkVolume", (double)r->downlink_volume);
    if (r->uplink_volume >= 0)
        cJSON_AddNumberToObject(volume, "uplinkVolume", (double)r->uplink_volume);
    if (r->duration >= 0)
        cJSON_AddNumberToObject(volume, "duration", (double)r->duration);
    if (r->dnn)
        cJSON_AddStringToObject(body, "dnn", r->dnn);
    if (r->s_nssai) {
        snssai = cJSON_AddObjectToObject(body, "snssai");
        ogs_assert(snssai);
        cJSON_AddNumberToObject(snssai, "sst", r->s_nssai->sst);
        if (r->s_nssai->sd.v != OGS_S_NSSAI_NO_SD_VALUE) {
            snprintf(sd, sizeof(sd), "%06X", (unsigned)r->s_nssai->sd.v);
            cJSON_AddStringToObject(snssai, "sd", sd);
        }
    }
    if (r->inter_group_id)
        cJSON_AddStringToObject(body, "interGroupId", r->inter_group_id);
    feat = ogs_uint64_to_string(BDT_OFFERED_FEATURES);
    cJSON_AddStringToObject(body, "suppFeat", feat);
    ogs_free(feat);
    return body;
}

static bool request_valid(const pcf_bdt_request_t *r)
{
    return r && r->asp_id && r->start_time && r->stop_time && r->num_of_ues >= 1 &&
        (r->total_volume >= 0 || r->downlink_volume >= 0 || r->uplink_volume >= 0 || r->duration >= 0);
}

bool pcf_session_create_bdt_policy(pcf_session_t *session, const pcf_bdt_request_t *request,
        pcf_bdt_result_callback callback, void *user_data)
{
    pcf_bdt_policy_t *policy = NULL;
    char *uri = NULL;
    cJSON *body = NULL;
    bool rv;

    if (!session || !callback || !request_valid(request)) {
        ogs_error("The BDT policy request is not complete");
        return false;
    }
    uri = base_uri(session);
    if (!uri)
        return false;
    policy = policy_new(session, PCF_BDT_OP_CREATE, callback, user_data);
    if (!policy) {
        ogs_free(uri);
        return false;
    }
    body = request_json(request);
    rv = send_request(policy, OGS_SBI_HTTP_METHOD_POST, uri, body);
    cJSON_Delete(body);
    ogs_free(uri);
    if (!rv)
        policy_release(policy);
    return rv;
}

bool pcf_session_get_bdt_policy(pcf_session_t *session, const char *bdt_policy_id,
        pcf_bdt_result_callback callback, void *user_data)
{
    pcf_bdt_policy_t *policy = NULL;
    char *base = NULL, *uri = NULL;
    bool rv;

    if (!session || !callback || !bdt_policy_id || !bdt_policy_id[0] || strchr(bdt_policy_id, '/'))
        return false;
    base = base_uri(session);
    if (!base)
        return false;
    uri = ogs_msprintf("%s/%s", base, bdt_policy_id);
    ogs_free(base);
    policy = policy_new(session, PCF_BDT_OP_GET, callback, user_data);
    if (!policy) {
        ogs_free(uri);
        return false;
    }
    policy->uri = ogs_strdup(uri);
    rv = send_request(policy, OGS_SBI_HTTP_METHOD_GET, uri, NULL);
    ogs_free(uri);
    if (!rv)
        policy_release(policy);
    return rv;
}

bool pcf_discover_create_bdt_policy(const pcf_bdt_request_t *request, pcf_bdt_result_callback callback, void *user_data)
{
    pcf_bdt_policy_t *policy = NULL;
    cJSON *body = NULL;
    bool rv;

    if (!callback || !request_valid(request)) {
        ogs_error("The BDT policy request is not complete");
        return false;
    }
    policy = policy_new(NULL, PCF_BDT_OP_CREATE, callback, user_data);
    if (!policy)
        return false;
    body = request_json(request);
    rv = send_discovered(policy, OGS_SBI_HTTP_METHOD_POST, NULL, body);
    cJSON_Delete(body);
    if (!rv)
        policy_release(policy);
    return rv;
}

bool pcf_discover_get_bdt_policy(const char *bdt_policy_id, pcf_bdt_result_callback callback, void *user_data)
{
    pcf_bdt_policy_t *policy = NULL;
    bool rv;

    if (!callback || !bdt_policy_id || !bdt_policy_id[0] || strchr(bdt_policy_id, '/'))
        return false;
    policy = policy_new(NULL, PCF_BDT_OP_GET, callback, user_data);
    if (!policy)
        return false;
    rv = send_discovered(policy, OGS_SBI_HTTP_METHOD_GET, bdt_policy_id, NULL);
    if (!rv)
        policy_release(policy);
    return rv;
}

const char *pcf_bdt_policy_get_ref_id(const pcf_bdt_policy_t *policy)
{
    return policy ? policy->bdt_ref_id : NULL;
}

const char *pcf_bdt_policy_get_uri(const pcf_bdt_policy_t *policy)
{
    return policy ? policy->uri : NULL;
}

int pcf_bdt_policy_get_num_transfer_policies(const pcf_bdt_policy_t *policy)
{
    return (policy && policy->transf_policies) ? cJSON_GetArraySize(policy->transf_policies) : 0;
}

bool pcf_bdt_policy_get_transfer_policy(const pcf_bdt_policy_t *policy, int index, pcf_bdt_transfer_policy_t *out)
{
    cJSON *t = NULL, *window = NULL, *item = NULL;

    if (!policy || !out || index < 0 || !policy->transf_policies)
        return false;
    t = cJSON_GetArrayItem(policy->transf_policies, index);
    if (!cJSON_IsObject(t))
        return false;
    memset(out, 0, sizeof(*out));
    item = cJSON_GetObjectItem(t, "transPolicyId");
    out->trans_policy_id = cJSON_IsNumber(item) ? item->valueint : 0;
    item = cJSON_GetObjectItem(t, "ratingGroup");
    out->rating_group = cJSON_IsNumber(item) ? item->valueint : 0;
    window = cJSON_GetObjectItem(t, "recTimeInt");
    item = cJSON_GetObjectItem(window, "startTime");
    out->start_time = cJSON_IsString(item) ? item->valuestring : NULL;
    item = cJSON_GetObjectItem(window, "stopTime");
    out->stop_time = cJSON_IsString(item) ? item->valuestring : NULL;
    item = cJSON_GetObjectItem(t, "maxBitRateDl");
    out->max_bit_rate_dl = cJSON_IsString(item) ? item->valuestring : NULL;
    item = cJSON_GetObjectItem(t, "maxBitRateUl");
    out->max_bit_rate_ul = cJSON_IsString(item) ? item->valuestring : NULL;
    return true;
}

int pcf_bdt_policy_get_selected_transfer_policy_id(const pcf_bdt_policy_t *policy)
{
    return policy ? policy->selected : 0;
}

bool pcf_bdt_policy_select(pcf_bdt_policy_t *policy, int trans_policy_id)
{
    cJSON *body = NULL, *data = NULL;
    pcf_bdt_transfer_policy_t t;
    bool offered = false;
    int i;

    if (!policy_exists(policy) || !policy->uri || policy->operation != PCF_BDT_OP_NONE)
        return false;
    /* the value 0 means that none is selected, which needs BdtNotification_5G; the PCF has to have offered the policy */
    for (i = 0; i < pcf_bdt_policy_get_num_transfer_policies(policy); i++)
        if (pcf_bdt_policy_get_transfer_policy(policy, i, &t) && t.trans_policy_id == trans_policy_id)
            offered = true;
    if (!offered) {
        ogs_error("Transfer policy [%d] was not offered by the PCF", trans_policy_id);
        return false;
    }

    body = cJSON_CreateObject();
    data = cJSON_AddObjectToObject(body, "bdtPolData");
    ogs_assert(body && data);
    cJSON_AddNumberToObject(data, "selTransPolicyId", trans_policy_id);
    policy->operation = PCF_BDT_OP_SELECT;
    policy->pending_selected = trans_policy_id;
    if (!send_request(policy, OGS_SBI_HTTP_METHOD_PATCH, policy->uri, body)) {
        policy->operation = PCF_BDT_OP_NONE;
        cJSON_Delete(body);
        return false;
    }
    cJSON_Delete(body);
    return true;
}

void pcf_bdt_policy_free(pcf_bdt_policy_t *policy)
{
    if (!policy_exists(policy))
        return;
    if (!policy->uri || policy->operation != PCF_BDT_OP_NONE) {
        /* the PCF has not created the policy, or another operation waits for it: there is nothing to delete yet */
        ogs_error("The BDT policy cannot be deleted now");
        return;
    }
    policy->operation = PCF_BDT_OP_DELETE;
    if (!send_request(policy, OGS_SBI_HTTP_METHOD_DELETE, policy->uri, NULL))
        policy_release(policy);
}

/* The BdtPolicy of an answer into the object */
static bool policy_read(pcf_bdt_policy_t *policy, const char *content)
{
    cJSON *doc = content ? cJSON_Parse(content) : NULL, *data = NULL, *item = NULL;

    if (!cJSON_IsObject(doc)) {
        if (doc)
            cJSON_Delete(doc);
        return false;
    }
    data = cJSON_GetObjectItem(doc, "bdtPolData");
    item = cJSON_GetObjectItem(data, "bdtRefId");
    if (!cJSON_IsString(item) || !cJSON_IsArray(cJSON_GetObjectItem(data, "transfPolicies"))) {
        cJSON_Delete(doc);
        return false;
    }
    if (policy->bdt_ref_id)
        ogs_free(policy->bdt_ref_id);
    policy->bdt_ref_id = ogs_strdup(item->valuestring);
    if (policy->transf_policies)
        cJSON_Delete(policy->transf_policies);
    policy->transf_policies = cJSON_DetachItemFromObject(data, "transfPolicies");
    item = cJSON_GetObjectItem(data, "selTransPolicyId");
    policy->selected = cJSON_IsNumber(item) ? item->valueint : 0;
    item = cJSON_GetObjectItem(data, "suppFeat");
    policy->features = cJSON_IsString(item) ? (ogs_uint64_from_string_hexadecimal(item->valuestring) & BDT_OFFERED_FEATURES) : 0;
    cJSON_Delete(doc);
    return true;
}

static const char *last_segment(const char *uri)
{
    const char *p = uri ? strrchr(uri, '/') : NULL;

    return (p && p[1]) ? p + 1 : NULL;
}

/* The NRF has answered the discovery of the PCF: the first request goes to the instance that it named */
static void discovered_nf_instance(pcf_bdt_policy_t *policy, ogs_sbi_xact_t *xact, ogs_sbi_response_t *response)
{
    ogs_sbi_message_t message;
    ogs_sbi_nf_instance_t *nf_instance = NULL;
    char *method;
    pcf_bdt_result_t result;
    pcf_bdt_result_callback callback = policy->callback;
    void *user_data = policy->user_data;

    method = response->h.method;
    response->h.method = NULL;
    ogs_sbi_header_free(&response->h);
    response->h.method = method;
    if (ogs_sbi_parse_response(&message, response) == OGS_OK) {
        ogs_nnrf_disc_handle_nf_discover_search_result(message.SearchResult);
        nf_instance = ogs_sbi_nf_instance_find_by_service(xact->service_name, xact->requester_nf_type);
    }
    ogs_sbi_message_free(&message);
    if (nf_instance) {
        ogs_sbi_send_request_to_nf_instance(nf_instance, xact);
        return;
    }
    /* the NRF knows no PCF with the service: the requester is told */
    ogs_error("The NRF found no PCF with the BDT policy control service");
    memset(&result, 0, sizeof(result));
    result.operation = policy->operation;
    policy->operation = PCF_BDT_OP_NONE;
    policy_release(policy);
    if (callback)
        callback(NULL, &result, user_data);
}

bool _pcf_bdt_process_timer_event(ogs_event_t *e)
{
    ogs_sbi_xact_t *xact = NULL;
    pcf_bdt_policy_t *policy = NULL;
    pcf_bdt_result_t result;
    pcf_bdt_result_callback callback;
    void *user_data;

    if (e->timer_id != OGS_TIMER_SBI_CLIENT_WAIT)
        return false;
    xact = ogs_sbi_xact_find_by_id(OGS_POINTER_TO_UINT(e->sbi.data));
    if (!xact || !xact->sbi_object)
        return false;
    policy = ogs_container_of(xact->sbi_object, pcf_bdt_policy_t, sbi);
    if (!policy_exists(policy) || policy->xact != xact)
        return false;

    ogs_error("Timed out trying to reach a PCF with the BDT policy control service");
    memset(&result, 0, sizeof(result));
    result.operation = policy->operation;
    callback = policy->callback;
    user_data = policy->user_data;
    policy->operation = PCF_BDT_OP_NONE;
    policy_release(policy);
    if (callback)
        callback(NULL, &result, user_data);
    return true;
}

bool _pcf_bdt_process_client_event(ogs_event_t *e)
{
    pcf_bdt_policy_t *policy = e->sbi.data;
    ogs_sbi_response_t *response = e->sbi.response;
    ogs_sbi_xact_t *xact = NULL;
    pcf_bdt_result_t result;
    cJSON *problem = NULL, *item = NULL;
    const char *content = NULL, *location = NULL;
    bool release = false, done = false;

    if (!policy_exists(policy)) {
        /* the answer to a request that went through the NRF carries the id of its transaction */
        xact = ogs_sbi_xact_find_by_id(OGS_POINTER_TO_UINT(e->sbi.data));
        if (!xact || !xact->sbi_object)
            return false;
        policy = ogs_container_of(xact->sbi_object, pcf_bdt_policy_t, sbi);
        if (!policy_exists(policy) || policy->xact != xact)
            return false;
        if (e->sbi.state == OGS_OK && response && response->status == OGS_SBI_HTTP_STATUS_OK && response->h.uri &&
                strstr(response->h.uri, "nf-instances")) {
            /* the NRF's answer to the discovery */
            discovered_nf_instance(policy, xact, response);
            ogs_sbi_response_free(response);
            return true;
        }
        /* the answer of the PCF: the transaction is over */
        ogs_sbi_xact_remove(xact);
        policy->xact = NULL;
    }

    memset(&result, 0, sizeof(result));
    result.operation = policy->operation;
    if (e->sbi.state == OGS_DONE) {
        /* the application shuts down: the request was cut off, nobody is told */
        policy_release(policy);
        if (response)
            ogs_sbi_response_free(response);
        return true;
    }

    if (e->sbi.state == OGS_OK && response) {
        result.http_status = response->status;
        content = response->http.content;
        location = ogs_sbi_header_get(response->http.headers, OGS_SBI_LOCATION);
        if (response->status >= 400 && content) {
            problem = cJSON_Parse(content);
            if (cJSON_IsObject(problem)) {
                item = cJSON_GetObjectItem(problem, "title");
                result.problem_title = cJSON_IsString(item) ? item->valuestring : NULL;
                item = cJSON_GetObjectItem(problem, "detail");
                result.problem_detail = cJSON_IsString(item) ? item->valuestring : NULL;
                item = cJSON_GetObjectItem(problem, "cause");
                result.problem_cause = cJSON_IsString(item) ? item->valuestring : NULL;
            }
        }
    }

    switch (policy->operation) {
    case PCF_BDT_OP_CREATE:
        if (result.http_status == OGS_SBI_HTTP_STATUS_CREATED && location && policy_read(policy, content)) {
            policy->uri = ogs_strdup(location);
            result.success = true;
        } else {
            if (result.http_status == OGS_SBI_HTTP_STATUS_SEE_OTHER)
                result.existing_policy_id = last_segment(location);
            release = true;
        }
        break;
    case PCF_BDT_OP_GET:
        if (result.http_status == OGS_SBI_HTTP_STATUS_OK && policy_read(policy, content)) {
            result.success = true;
            /* a policy that was asked for through the NRF knows its resource from the request that was sent */
            if (!policy->uri && response->h.uri)
                policy->uri = ogs_strdup(response->h.uri);
        } else {
            release = true;
        }
        break;
    case PCF_BDT_OP_SELECT:
        if (result.http_status == OGS_SBI_HTTP_STATUS_OK && policy_read(policy, content)) {
            result.success = true;
        } else if (result.http_status == OGS_SBI_HTTP_STATUS_NO_CONTENT) {
            policy->selected = policy->pending_selected;
            result.success = true;
        }
        break;
    case PCF_BDT_OP_DELETE:
        result.success = result.http_status == OGS_SBI_HTTP_STATUS_NO_CONTENT;
        release = true;
        break;
    default:
        done = true;
        break;
    }

    if (!done) {
        pcf_bdt_result_callback callback = policy->callback;
        void *user_data = policy->user_data;

        policy->operation = PCF_BDT_OP_NONE;
        if (callback)
            callback(policy, &result, user_data);
        if (release && policy_exists(policy))
            policy_release(policy);
    }
    if (problem)
        cJSON_Delete(problem);
    if (response)
        ogs_sbi_response_free(response);
    return true;
}
