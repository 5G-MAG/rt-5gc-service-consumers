/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2026 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

/* Open5GS includes */

#include "ogs-sbi.h"
#include "test-common.h"

/* Local library includes (the internals are linked into this executable, see meson.build) */
#include "pcf-service-consumer.h"
#include "pcf-client.h"
#include "pcf-client-sess.h"
#include "pcf-build.h"

/* Unit test includes */
#include "pcf-test-unit.h"

/*
 * The requests that the library builds and the answers that it reads, checked against TS 29.514 V19.8.0 without a core network: a request
 * is built and its content examined, an answer is made up and handed to pcf_session_process_event().
 */

static pcf_session_t *session = NULL;

static void create_ipv4_func(abts_case *tc, void *data);
static void create_ipv6_func(abts_case *tc, void *data);
static void create_answer_func(abts_case *tc, void *data);
static void no_query_func(abts_case *tc, void *data);
static void update_sponsor_func(abts_case *tc, void *data);
static void update_answer_func(abts_case *tc, void *data);
static void update_request_func(abts_case *tc, void *data);

int pcf_unit_initialise(void)
{
    int rv;
    ogs_sockaddr_t *pcf_address = NULL;

    rv = ogs_app_parse_local_conf("af");
    if (rv != OGS_OK) return rv;

    ogs_sbi_context_init(OpenAPI_nf_type_AF);

    rv = ogs_getaddrinfo(&pcf_address, AF_UNSPEC, "127.0.0.13", 7777, 0);
    if (rv != OGS_OK) return rv;
    session = pcf_session_new(pcf_address);
    ogs_freeaddrinfo(pcf_address);

    return session ? OGS_OK : OGS_ERROR;
}

void pcf_unit_terminate(void)
{
    if (session) pcf_session_free(session);
    session = NULL;
    pcf_service_consumer_final();
    ogs_sbi_context_final();
}

abts_suite *test_pcf_unit(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    abts_run_test(suite, create_ipv4_func, NULL);
    abts_run_test(suite, create_ipv6_func, NULL);
    abts_run_test(suite, create_answer_func, NULL);
    abts_run_test(suite, update_request_func, NULL);
    abts_run_test(suite, no_query_func, NULL);
    abts_run_test(suite, update_sponsor_func, NULL);
    abts_run_test(suite, update_answer_func, NULL);

    return suite;
}

static bool notification_callback(pcf_app_session_t *app_session, const OpenAPI_events_notification_t *notifications, void *user_data)
{
    return true;
}

/* An application session for the UE address, as pcf_session_create_app_session() makes it, without the notification server */
static pcf_app_session_t *app_session_new(const char *address, int events)
{
    ue_network_identifier_t ue;
    ogs_sockaddr_t *ue_address = NULL;
    pcf_app_session_t *sess;

    ogs_assert(OGS_OK == ogs_getaddrinfo(&ue_address, AF_UNSPEC, address, 0, 0));
    memset(&ue, 0, sizeof(ue));
    ue.address = ue_address;
    sess = _pcf_app_session_new(session, &ue, events, notification_callback, NULL, NULL, NULL);
    ogs_freeaddrinfo(ue_address);
    ogs_assert(sess);
    sess->notif_url = ogs_strdup("http://127.0.0.1:9999/npcf-policyauthorization/v1/app-session-instance/1");
    return sess;
}

/* The JSON of the content of a request; the caller releases it */
static cJSON *content_of(abts_case *tc, ogs_sbi_request_t *request)
{
    cJSON *json = NULL;

    ABTS_PTR_NOTNULL(tc, request);
    if (!request) return NULL;
    ABTS_PTR_NOTNULL(tc, request->http.content);
    if (request->http.content) json = cJSON_Parse(request->http.content);
    ABTS_PTR_NOTNULL(tc, json);
    return json;
}

static const char *string_of(cJSON *object, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);

    return cJSON_IsString(item) ? item->valuestring : NULL;
}

static void create_ipv4_func(abts_case *tc, void *data)
{
    pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF);
    pcf_app_session_options_t options;
    ogs_sbi_request_t *request;
    cJSON *json, *req_data;

    memset(&options, 0, sizeof(options));
    options.events = PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF;
    request = pcf_policyauthorization_request_create(sess, NULL, &options);
    json = content_of(tc, request);
    req_data = cJSON_GetObjectItemCaseSensitive(json, "ascReqData");
    ABTS_PTR_NOTNULL(tc, req_data);
    if (req_data) {
        /* TS 29.514 V19.8.0 clause 4.2.2.2: the address of the UE, and the URI for the termination requests of the PCF */
        ABTS_STR_EQUAL(tc, "10.45.0.2", string_of(req_data, "ueIpv4") ? string_of(req_data, "ueIpv4") : "");
        ABTS_PTR_NOTNULL(tc, string_of(req_data, "notifUri"));
        ABTS_PTR_NOTNULL(tc, string_of(req_data, "suppFeat"));
    }
    if (json) cJSON_Delete(json);
    if (request) ogs_sbi_request_free(request);
    _pcf_app_session_free(sess);
}

static void create_ipv6_func(abts_case *tc, void *data)
{
    pcf_app_session_t *sess = app_session_new("2001:db8:cafe::2", PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF);
    pcf_app_session_options_t options;
    ogs_sbi_request_t *request;
    cJSON *json, *req_data;

    memset(&options, 0, sizeof(options));
    options.events = PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF;
    request = pcf_policyauthorization_request_create(sess, NULL, &options);
    json = content_of(tc, request);
    req_data = cJSON_GetObjectItemCaseSensitive(json, "ascReqData");
    ABTS_PTR_NOTNULL(tc, req_data);
    if (req_data) {
        /* TS 29.514 V19.8.0 clause 4.2.2.2: for an IP type PDU session the IPv6 address of the UE is in ueIpv6, and only one address is given */
        ABTS_STR_EQUAL(tc, "2001:db8:cafe::2", string_of(req_data, "ueIpv6") ? string_of(req_data, "ueIpv6") : "");
        ABTS_TRUE(tc, cJSON_GetObjectItemCaseSensitive(req_data, "ueIpv4") == NULL);
    }
    if (json) cJSON_Delete(json);
    if (request) ogs_sbi_request_free(request);
    _pcf_app_session_free(sess);
}

/* The answer of the PCF to a request, as the client of the library hands it over */
static void answer(abts_case *tc, pcf_app_session_t *sess, const char *method, const char *path, int status, const char *location,
                   const char *content)
{
    ogs_event_t *e = ogs_event_new(OGS_EVENT_SBI_CLIENT);
    ogs_sbi_response_t *response = ogs_sbi_response_new();

    ogs_assert(e && response);
    response->status = status;
    response->h.method = ogs_strdup(method);
    response->h.uri = ogs_msprintf("http://127.0.0.13:7777/npcf-policyauthorization/v1/%s", path);
    if (location) ogs_sbi_header_set(response->http.headers, OGS_SBI_LOCATION, location);
    if (content) {
        response->http.content = ogs_strdup(content);
        response->http.content_length = strlen(content);
        ogs_sbi_header_set(response->http.headers, OGS_SBI_CONTENT_TYPE, OGS_SBI_CONTENT_JSON_TYPE);
    }
    e->sbi.response = response;
    e->sbi.data = sess;
    e->sbi.state = OGS_OK;
    ABTS_TRUE(tc, pcf_session_process_event(e));
    ogs_event_free(e);
}

typedef struct outcome_s {
    int calls;
    pcf_app_session_operation_t operation;
    bool success;
    int http_status;
    uint64_t features;
} outcome_t;

static void result_callback(pcf_app_session_t *app_session, const pcf_app_session_result_t *result, void *user_data)
{
    outcome_t *outcome = user_data;

    outcome->calls++;
    outcome->operation = result->operation;
    outcome->success = result->success;
    outcome->http_status = result->http_status;
    outcome->features = pcf_app_session_get_features(app_session);
}

#define CREATED_URI "http://127.0.0.13:7777/npcf-policyauthorization/v1/app-sessions/42"

static void create_answer_func(abts_case *tc, void *data)
{
    outcome_t outcome;
    pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF);

    /* the PCF repeats the AppSessionContextReqData with the features that it supports (what the Open5GS PCF does) */
    memset(&outcome, 0, sizeof(outcome));
    sess->result.callback = result_callback;
    sess->result.user_data = &outcome;
    sess->operation = PCF_APP_SESSION_OP_CREATE;
    sess->policyauthorization_features = PCF_LIB_FEATURES;
    answer(tc, sess, "POST", "app-sessions", 201, CREATED_URI,
           "{\"ascReqData\":{\"notifUri\":\"http://127.0.0.1:9999/n\",\"suppFeat\":\"2\",\"ueIpv4\":\"10.45.0.2\"}}");
    ABTS_INT_EQUAL(tc, 1, outcome.calls);
    ABTS_INT_EQUAL(tc, PCF_APP_SESSION_OP_CREATE, outcome.operation);
    ABTS_TRUE(tc, outcome.success);
    ABTS_INT_EQUAL(tc, 201, outcome.http_status);
    /* feature 2 only: the features that both support */
    ABTS_TRUE(tc, outcome.features == 2);
    _pcf_app_session_free(sess);
}

/* The MediaComponentRm entries of an update: media component 0 with a bit rate */
static OpenAPI_list_t *media_component_rm_new(void)
{
    OpenAPI_list_t *list = OpenAPI_list_create();
    OpenAPI_media_component_rm_t *component = ogs_calloc(1, sizeof(*component));

    ogs_assert(list && component);
    component->med_comp_n = 0;
    component->f_status = OpenAPI_flow_status_ENABLED;
    component->mar_bw_dl = ogs_sbi_bitrate_to_string(128000, OGS_SBI_BITRATE_BPS);
    component->mar_bw_ul = ogs_sbi_bitrate_to_string(128000, OGS_SBI_BITRATE_BPS);
    OpenAPI_list_add(list, OpenAPI_map_create(ogs_msprintf("%d", component->med_comp_n), component));
    return list;
}

static void update_request_func(abts_case *tc, void *data)
{
    pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_NONE);
    OpenAPI_list_t *media_component_rm = media_component_rm_new();
    ogs_sbi_request_t *request;
    const char *content_type;

    sess->pcf_app_session_id = ogs_strdup("42");
    request = pcf_policyauthorization_request_update(sess, media_component_rm);
    ABTS_PTR_NOTNULL(tc, request);
    if (request) {
        /* TS 29.514 V19.8.0 clause 5.2.2.2: the body of the PATCH is a JSON Merge Patch */
        ABTS_STR_EQUAL(tc, OGS_SBI_HTTP_METHOD_PATCH, request->h.method);
        content_type = ogs_sbi_header_get(request->http.headers, OGS_SBI_CONTENT_TYPE);
        ABTS_STR_EQUAL(tc, OGS_SBI_CONTENT_MERGE_PATCH_TYPE, content_type ? content_type : "");
        ogs_sbi_request_free(request);
    }
    pcf_media_components_rm_free(media_component_rm);
    _pcf_app_session_free(sess);
}

/* TS 29.514 V19.8.0 tables 5.3.2.3.1-1 and 5.3.3.3.2-1: the methods that create, modify and delete take no URI query parameter */
static void no_query_func(abts_case *tc, void *data)
{
    pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF);
    pcf_app_session_options_t options;
    OpenAPI_list_t *media_component_rm = media_component_rm_new();
    ogs_sbi_request_t *request;

    memset(&options, 0, sizeof(options));
    options.events = PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF;
    sess->pcf_app_session_id = ogs_strdup("42");

    request = pcf_policyauthorization_request_create(sess, NULL, &options);
    ABTS_PTR_NOTNULL(tc, request);
    if (request) {
        ABTS_INT_EQUAL(tc, 0, (int)ogs_hash_count(request->http.params));
        ogs_sbi_request_free(request);
    }
    request = pcf_policyauthorization_request_update(sess, media_component_rm);
    ABTS_PTR_NOTNULL(tc, request);
    if (request) {
        ABTS_INT_EQUAL(tc, 0, (int)ogs_hash_count(request->http.params));
        ogs_sbi_request_free(request);
    }
    request = pcf_policyauthorization_request_delete(sess);
    ABTS_PTR_NOTNULL(tc, request);
    if (request) {
        ABTS_INT_EQUAL(tc, 0, (int)ogs_hash_count(request->http.params));
        ogs_sbi_request_free(request);
    }
    pcf_media_components_rm_free(media_component_rm);
    _pcf_app_session_free(sess);
}

/*
 * TS 29.514 V19.8.0 clause 4.2.3.5 and RFC 7396: an update that does not speak of sponsored connectivity leaves it as it is, so it carries no
 * sponStatus (SPONSOR_DISABLED would end the sponsoring of a session that was created with SPONSOR_ENABLED).
 */
static void update_sponsor_func(abts_case *tc, void *data)
{
    pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_NONE);
    OpenAPI_list_t *media_component_rm = media_component_rm_new();
    ogs_sbi_request_t *request;
    cJSON *json, *req_data;

    sess->pcf_app_session_id = ogs_strdup("42");
    request = pcf_policyauthorization_request_update(sess, media_component_rm);
    json = content_of(tc, request);
    req_data = cJSON_GetObjectItemCaseSensitive(json, "ascReqData");
    ABTS_PTR_NOTNULL(tc, req_data);
    if (req_data) {
        ABTS_PTR_NOTNULL(tc, cJSON_GetObjectItemCaseSensitive(req_data, "medComponents"));
        ABTS_TRUE(tc, cJSON_GetObjectItemCaseSensitive(req_data, "sponStatus") == NULL);
    }
    if (json) cJSON_Delete(json);
    if (request) ogs_sbi_request_free(request);
    pcf_media_components_rm_free(media_component_rm);
    _pcf_app_session_free(sess);
}

/* TS 29.514 V19.8.0 table 5.3.3.3.2-3: the PATCH is answered with 200 and the representation, or with 204 and none; both are success */
static void update_answer_func(abts_case *tc, void *data)
{
    static const int statuses[] = {200, 204};
    int i;

    for (i = 0; i < 2; i++) {
        outcome_t outcome;
        pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_NONE);

        memset(&outcome, 0, sizeof(outcome));
        sess->pcf_app_session_id = ogs_strdup("42");
        sess->result.callback = result_callback;
        sess->result.user_data = &outcome;
        sess->operation = PCF_APP_SESSION_OP_UPDATE;
        answer(tc, sess, "PATCH", "app-sessions/42", statuses[i], NULL,
               statuses[i] == 200 ? "{\"ascReqData\":{\"notifUri\":\"http://127.0.0.1:9999/n\",\"suppFeat\":\"2\"}}" : NULL);
        ABTS_INT_EQUAL(tc, 1, outcome.calls);
        ABTS_INT_EQUAL(tc, PCF_APP_SESSION_OP_UPDATE, outcome.operation);
        ABTS_TRUE(tc, outcome.success);
        ABTS_INT_EQUAL(tc, statuses[i], outcome.http_status);
        _pcf_app_session_free(sess);
    }
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
