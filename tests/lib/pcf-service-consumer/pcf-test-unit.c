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
#include "utils.h"
#include "pcf-evsubsc.h"

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
static void create_resp_data_func(abts_case *tc, void *data);
static void subscribe_merge_func(abts_case *tc, void *data);
static void create_events_func(abts_case *tc, void *data);
static void create_features_func(abts_case *tc, void *data);
static void no_query_func(abts_case *tc, void *data);
static void update_sponsor_func(abts_case *tc, void *data);
static void update_answer_func(abts_case *tc, void *data);
static void update_request_func(abts_case *tc, void *data);
static void create_options_func(abts_case *tc, void *data);
static void ipv6_pcf_func(abts_case *tc, void *data);
static void location_func(abts_case *tc, void *data);
static void create_no_features_func(abts_case *tc, void *data);
static void answer_result_func(abts_case *tc, void *data);
static void retry_after_func(abts_case *tc, void *data);
static void answer_notifications_func(abts_case *tc, void *data);
static void unsubscribe_func(abts_case *tc, void *data);
static void subscribe_details_func(abts_case *tc, void *data);
static void delete_sibling_func(abts_case *tc, void *data);
static void termination_cause_func(abts_case *tc, void *data);
static void features_gate_func(abts_case *tc, void *data);
static void companion_events_func(abts_case *tc, void *data);
static void update_without_patch_correction_func(abts_case *tc, void *data);
static void ue_identifier_func(abts_case *tc, void *data);

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
    abts_run_test(suite, create_resp_data_func, NULL);
    abts_run_test(suite, subscribe_merge_func, NULL);
    abts_run_test(suite, create_events_func, NULL);
    abts_run_test(suite, create_features_func, NULL);
    abts_run_test(suite, update_request_func, NULL);
    abts_run_test(suite, no_query_func, NULL);
    abts_run_test(suite, update_sponsor_func, NULL);
    abts_run_test(suite, update_answer_func, NULL);
    abts_run_test(suite, create_options_func, NULL);
    abts_run_test(suite, ipv6_pcf_func, NULL);
    abts_run_test(suite, location_func, NULL);
    abts_run_test(suite, create_no_features_func, NULL);
    abts_run_test(suite, answer_result_func, NULL);
    abts_run_test(suite, retry_after_func, NULL);
    abts_run_test(suite, answer_notifications_func, NULL);
    abts_run_test(suite, unsubscribe_func, NULL);
    abts_run_test(suite, subscribe_details_func, NULL);
    abts_run_test(suite, delete_sibling_func, NULL);
    abts_run_test(suite, termination_cause_func, NULL);
    abts_run_test(suite, features_gate_func, NULL);
    abts_run_test(suite, companion_events_func, NULL);
    abts_run_test(suite, update_without_patch_correction_func, NULL);
    abts_run_test(suite, ue_identifier_func, NULL);

    return suite;
}

static bool notification_callback(pcf_app_session_t *app_session, const OpenAPI_events_notification_t *notifications, void *user_data)
{
    return true;
}

/* An application session for the UE address, as pcf_session_create_app_session() makes it, without the notification server */
static pcf_app_session_t *app_session_new_on(pcf_session_t *pcf_session, const char *address, int events)
{
    ue_network_identifier_t ue;
    ogs_sockaddr_t *ue_address = NULL;
    pcf_app_session_t *sess;

    ogs_assert(OGS_OK == ogs_getaddrinfo(&ue_address, AF_UNSPEC, address, 0, 0));
    memset(&ue, 0, sizeof(ue));
    ue.address = ue_address;
    sess = _pcf_app_session_new(pcf_session, &ue, events, notification_callback, NULL, NULL, NULL);
    ogs_freeaddrinfo(ue_address);
    ogs_assert(sess);
    sess->notif_url = ogs_strdup("http://127.0.0.1:9999/npcf-policyauthorization/v1/app-session-instance/1");
    return sess;
}

static pcf_app_session_t *app_session_new(const char *address, int events)
{
    return app_session_new_on(session, address, events);
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

/* The answer of the PCF to a request, as the client of the library hands it over; retry_after is the value of a Retry-After header field, or NULL */
static void answer_with(abts_case *tc, pcf_app_session_t *sess, const char *method, const char *path, int status, const char *location,
                        const char *content, const char *retry_after)
{
    ogs_event_t *e = ogs_event_new(OGS_EVENT_SBI_CLIENT);
    ogs_sbi_response_t *response = ogs_sbi_response_new();

    ogs_assert(e && response);
    response->status = status;
    response->h.method = ogs_strdup(method);
    response->h.uri = ogs_msprintf("http://127.0.0.13:7777/npcf-policyauthorization/v1/%s", path);
    if (location) ogs_sbi_header_set(response->http.headers, OGS_SBI_LOCATION, location);
    if (retry_after) ogs_sbi_header_set(response->http.headers, "Retry-After", retry_after);
    if (content) {
        response->http.content = ogs_strdup(content);
        response->http.content_length = strlen(content);
        ogs_sbi_header_set(response->http.headers, OGS_SBI_CONTENT_TYPE, status >= 300 ? OGS_SBI_CONTENT_PROBLEM_TYPE : OGS_SBI_CONTENT_JSON_TYPE);
    }
    e->sbi.response = response;
    e->sbi.data = sess;
    e->sbi.state = OGS_OK;
    ABTS_TRUE(tc, pcf_session_process_event(e));
    ogs_event_free(e);
}

static void answer(abts_case *tc, pcf_app_session_t *sess, const char *method, const char *path, int status, const char *location,
                   const char *content)
{
    answer_with(tc, sess, method, path, status, location, content, NULL);
}

typedef struct outcome_s {
    int calls;
    pcf_app_session_operation_t operation;
    bool success;
    int http_status;
    uint64_t features;
    char problem_cause[64];
    int retry_after;
    char redirect_location[256];
    char termination_cause[64];
    int notifications;      /* the events of the notifications that arrived, as event mask bits */
} outcome_t;

static void result_callback(pcf_app_session_t *app_session, const pcf_app_session_result_t *result, void *user_data)
{
    outcome_t *outcome = user_data;

    outcome->calls++;
    outcome->operation = result->operation;
    outcome->success = result->success;
    outcome->http_status = result->http_status;
    outcome->features = pcf_app_session_get_features(app_session);
    snprintf(outcome->problem_cause, sizeof(outcome->problem_cause), "%s", result->problem_cause ? result->problem_cause : "");
    outcome->retry_after = result->retry_after;
    snprintf(outcome->redirect_location, sizeof(outcome->redirect_location), "%s", result->redirect_location ? result->redirect_location : "");
    snprintf(outcome->termination_cause, sizeof(outcome->termination_cause), "%s", result->termination_cause ? result->termination_cause : "");
}

static bool counting_callback(pcf_app_session_t *app_session, const OpenAPI_events_notification_t *notifications, void *user_data)
{
    outcome_t *outcome = user_data;
    OpenAPI_lnode_t *node = NULL;

    OpenAPI_list_for_each(notifications->ev_notifs, node)
        outcome->notifications |= 1 << ((OpenAPI_af_event_notification_t *)node->data)->event;
    return true;
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

/*
 * TS 29.514 V19.8.0 clause 5.8 and table 5.6.2.2-1: the PCF gives the features in the AppSessionContextRespData (ascRespData), and the
 * AppSessionContextReqData (ascReqData) may be left out of the answer.
 */
static void create_resp_data_func(abts_case *tc, void *data)
{
    static const char *answers[] = {
        "{\"ascRespData\":{\"suppFeat\":\"2\"}}",
        /* what the answer says in ascRespData decides, not what ascReqData repeats */
        "{\"ascReqData\":{\"notifUri\":\"http://127.0.0.1:9999/n\",\"suppFeat\":\"3\"},\"ascRespData\":{\"suppFeat\":\"2\"}}"
    };
    int i;

    for (i = 0; i < 2; i++) {
        outcome_t outcome;
        pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF);

        memset(&outcome, 0, sizeof(outcome));
        sess->result.callback = result_callback;
        sess->result.user_data = &outcome;
        sess->operation = PCF_APP_SESSION_OP_CREATE;
        sess->policyauthorization_features = PCF_LIB_FEATURES | 1;
        answer(tc, sess, "POST", "app-sessions", 201, CREATED_URI, answers[i]);
        ABTS_INT_EQUAL(tc, 1, outcome.calls);
        ABTS_TRUE(tc, outcome.success);
        ABTS_TRUE(tc, outcome.features == 2);
        if (outcome.success) _pcf_app_session_free(sess);
    }
}

/* The events of a subscription (EventsSubscReqData) with the default notification method; the caller releases it with subsc_free() */
static OpenAPI_events_subsc_req_data_t *subsc_new(int num_events, const OpenAPI_npcf_af_event_e *events)
{
    OpenAPI_events_subsc_req_data_t *subsc = ogs_calloc(1, sizeof(*subsc));
    int i;

    ogs_assert(subsc);
    subsc->events = OpenAPI_list_create();
    for (i = 0; i < num_events; i++) {
        OpenAPI_af_event_subscription_t *event = ogs_calloc(1, sizeof(*event));

        ogs_assert(event);
        event->event = events[i];
        event->notif_method = OpenAPI_af_notif_method_EVENT_DETECTION;
        OpenAPI_list_add(subsc->events, event);
    }
    return subsc;
}

static void subsc_free(OpenAPI_events_subsc_req_data_t *subsc)
{
    OpenAPI_lnode_t *node = NULL;

    OpenAPI_list_for_each(subsc->events, node)
        ogs_free(node->data);
    OpenAPI_list_free(subsc->events);
    ogs_free(subsc);
}

/* The number of events that the PUT request of the subscription carries, or -1 if it carries none */
static int put_events(abts_case *tc, pcf_app_session_t *sess)
{
    ogs_sbi_request_t *request = pcf_policyauthorization_req_subscribe_event(sess);
    cJSON *json = content_of(tc, request), *events;
    int count = -1;

    events = cJSON_GetObjectItemCaseSensitive(json, "events");
    if (cJSON_IsArray(events)) count = cJSON_GetArraySize(events);
    if (json) cJSON_Delete(json);
    if (request) ogs_sbi_request_free(request);
    return count;
}

/*
 * TS 29.514 V19.8.0 clause 4.2.6.2: the PUT request of the events subscription carries the events to be subscribed, each of them once,
 * also when the application session was created without any event.
 */
static void subscribe_merge_func(abts_case *tc, void *data)
{
    static const OpenAPI_npcf_af_event_e created[] = {OpenAPI_npcf_af_event_QOS_NOTIF, OpenAPI_npcf_af_event_FAILED_RESOURCES_ALLOCATION};
    static const OpenAPI_npcf_af_event_e again[] = {OpenAPI_npcf_af_event_QOS_NOTIF};
    static const OpenAPI_npcf_af_event_e more[] = {OpenAPI_npcf_af_event_USAGE_REPORT};
    pcf_app_session_options_t options;
    pcf_app_session_t *sess;
    OpenAPI_events_subsc_req_data_t *subsc;
    ogs_sbi_request_t *request;
    int i;

    /* two events at creation: an event that is subscribed again is not entered twice, a new one is added */
    sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_NONE);
    memset(&options, 0, sizeof(options));
    options.events = PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF | PCF_APP_SESSION_EVENT_TYPE_FAILED_RESOURCES_ALLOCATION;
    request = pcf_policyauthorization_request_create(sess, NULL, &options);
    ABTS_PTR_NOTNULL(tc, request);
    if (request) ogs_sbi_request_free(request);
    sess->pcf_app_session_id = ogs_strdup("42");
    subsc = subsc_new(1, again);
    ABTS_INT_EQUAL(tc, 0, merge_event_subsc_to_app_session_context(sess, subsc));
    subsc_free(subsc);
    ABTS_INT_EQUAL(tc, 2, put_events(tc, sess));
    subsc = subsc_new(1, more);
    ABTS_INT_EQUAL(tc, 1, merge_event_subsc_to_app_session_context(sess, subsc));
    subsc_free(subsc);
    ABTS_INT_EQUAL(tc, 3, put_events(tc, sess));
    _pcf_app_session_free(sess);

    /* no event at creation: the first subscription has an event to send */
    sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_NONE);
    memset(&options, 0, sizeof(options));
    request = pcf_policyauthorization_request_create(sess, NULL, &options);
    ABTS_PTR_NOTNULL(tc, request);
    if (request) ogs_sbi_request_free(request);
    sess->pcf_app_session_id = ogs_strdup("42");
    for (i = 0; i < 2; i++) {
        subsc = subsc_new(1, more);
        ABTS_INT_EQUAL(tc, i == 0 ? 1 : 0, merge_event_subsc_to_app_session_context(sess, subsc));
        subsc_free(subsc);
    }
    ABTS_INT_EQUAL(tc, 1, put_events(tc, sess));
    _pcf_app_session_free(sess);
}

/* TS 29.514 V19.8.0 table 5.6.2.6-1: events has 1..N entries, so a request that subscribes to no event carries no evSubsc */
static void create_events_func(abts_case *tc, void *data)
{
    static const int masks[] = {PCF_APP_SESSION_EVENT_TYPE_NONE, PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF};
    int i;

    for (i = 0; i < 2; i++) {
        pcf_app_session_t *sess = app_session_new("10.45.0.2", masks[i]);
        pcf_app_session_options_t options;
        ogs_sbi_request_t *request;
        cJSON *json, *req_data, *ev_subsc;

        memset(&options, 0, sizeof(options));
        options.events = masks[i];
        request = pcf_policyauthorization_request_create(sess, NULL, &options);
        json = content_of(tc, request);
        req_data = cJSON_GetObjectItemCaseSensitive(json, "ascReqData");
        ABTS_PTR_NOTNULL(tc, req_data);
        ev_subsc = cJSON_GetObjectItemCaseSensitive(req_data, "evSubsc");
        if (masks[i] == PCF_APP_SESSION_EVENT_TYPE_NONE) {
            ABTS_TRUE(tc, ev_subsc == NULL);
        } else {
            ABTS_PTR_NOTNULL(tc, ev_subsc);
            ABTS_INT_EQUAL(tc, 1, cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(ev_subsc, "events")));
            ABTS_PTR_NOTNULL(tc, string_of(ev_subsc, "notifUri"));
        }
        if (json) cJSON_Delete(json);
        if (request) ogs_sbi_request_free(request);
        _pcf_app_session_free(sess);
    }
}

/*
 * TS 29.514 V19.8.0 table 5.8-1: QoSMonitoring (16) requires EnhancedSubscriptionToNotification (15), which includes the notification
 * correlation identifier of a subscription to an event (notifCorreId of table 5.6.2.6-1).
 */
static void create_features_func(abts_case *tc, void *data)
{
    pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_NONE);
    pcf_app_session_options_t options;
    static const OpenAPI_npcf_af_event_e event[] = {OpenAPI_npcf_af_event_QOS_MONITORING};
    OpenAPI_events_subsc_req_data_t *subsc = subsc_new(1, event);
    ogs_sbi_request_t *request;
    cJSON *json, *req_data, *ev_subsc;
    uint64_t lo = 0, hi = 0;

    subsc->notif_corre_id = (char *)"correlation-1";
    memset(&options, 0, sizeof(options));
    options.events_subscription = subsc;
    request = pcf_policyauthorization_request_create(sess, NULL, &options);
    json = content_of(tc, request);
    req_data = cJSON_GetObjectItemCaseSensitive(json, "ascReqData");
    ABTS_PTR_NOTNULL(tc, req_data);
    if (req_data) {
        ABTS_PTR_NOTNULL(tc, string_of(req_data, "suppFeat"));
        /* TS 29.571 V19.8.0 clause 5.2.2: hexadecimal characters, feature n is the n-th bit from the right end of the string: 2, 15, 16, 17, 28, 59, 63, 68, 69 */
        ABTS_STR_EQUAL(tc, "18440000000801c002", string_of(req_data, "suppFeat") ? string_of(req_data, "suppFeat") : "");
        if (string_of(req_data, "suppFeat"))
            ogs_sbi_features_from_string(string_of(req_data, "suppFeat"), &lo, &hi);
        ABTS_TRUE(tc, lo & ((uint64_t)1 << (16 - 1)));
        ABTS_TRUE(tc, lo & ((uint64_t)1 << (15 - 1)));
        ev_subsc = cJSON_GetObjectItemCaseSensitive(req_data, "evSubsc");
        ABTS_PTR_NOTNULL(tc, ev_subsc);
        ABTS_STR_EQUAL(tc, "correlation-1", string_of(ev_subsc, "notifCorreId") ? string_of(ev_subsc, "notifCorreId") : "");
    }
    if (json) cJSON_Delete(json);
    if (request) ogs_sbi_request_free(request);
    subsc->notif_corre_id = NULL;
    subsc_free(subsc);
    _pcf_app_session_free(sess);
}

/*
 * TS 29.514 V19.8.0 table 5.6.2.3-1: resPrio is optional and the PCF may take it into account, so none is sent unless the application asks (CONSUMERS
 * audit O1); sponStatus is a matter of the SponsoredConnectivity feature and the absence of it means enabled, so it is sent only with the identities of
 * a sponsor (O7).
 */
static void create_options_func(abts_case *tc, void *data)
{
    static const struct {
        const char *asp_id, *spon_id;
        bool enabled;
        OpenAPI_reserv_priority_e priority;
        const char *spon_status, *res_prio;
    } cases[] = {
        {NULL, NULL, false, OpenAPI_reserv_priority_NULL, NULL, NULL},
        {NULL, NULL, true, OpenAPI_reserv_priority_NULL, NULL, NULL},
        {"asp-1", "sponsor-1", true, OpenAPI_reserv_priority_PRIO_5, "SPONSOR_ENABLED", "PRIO_5"},
        {"asp-1", "sponsor-1", false, OpenAPI_reserv_priority_NULL, "SPONSOR_DISABLED", NULL},
        {"asp-1", NULL, true, OpenAPI_reserv_priority_PRIO_16, NULL, "PRIO_16"},
    };
    int i;

    for (i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
        pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_NONE);
        pcf_app_session_options_t options;
        ogs_sbi_request_t *request;
        cJSON *json, *req_data;

        memset(&options, 0, sizeof(options));
        options.asp_id = cases[i].asp_id;
        options.spon_id = cases[i].spon_id;
        options.sponsoring_enabled = cases[i].enabled;
        options.res_prio = cases[i].priority;
        request = pcf_policyauthorization_request_create(sess, NULL, &options);
        json = content_of(tc, request);
        req_data = cJSON_GetObjectItemCaseSensitive(json, "ascReqData");
        ABTS_PTR_NOTNULL(tc, req_data);
        if (req_data) {
            ABTS_STR_EQUAL(tc, cases[i].spon_status ? cases[i].spon_status : "(none)", string_of(req_data, "sponStatus") ? string_of(req_data, "sponStatus") : "(none)");
            ABTS_STR_EQUAL(tc, cases[i].res_prio ? cases[i].res_prio : "(none)", string_of(req_data, "resPrio") ? string_of(req_data, "resPrio") : "(none)");
        }
        if (json) cJSON_Delete(json);
        if (request) ogs_sbi_request_free(request);
        _pcf_app_session_free(sess);
    }
    /* an update does not speak of the priority either */
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
        if (req_data) ABTS_TRUE(tc, cJSON_GetObjectItemCaseSensitive(req_data, "resPrio") == NULL);
        if (json) cJSON_Delete(json);
        if (request) ogs_sbi_request_free(request);
        pcf_media_components_rm_free(media_component_rm);
        _pcf_app_session_free(sess);
    }
}

/*
 * TS 29.501 V19.5.0 clause 4.4.1 and RFC 3986 clause 3.2.2: the authority of a URI holds an IPv6 address in square brackets; the URIs of a PCF with an
 * IPv6 address are those of the operations of clause 4.2.
 */
static void ipv6_pcf_func(abts_case *tc, void *data)
{
    ogs_sockaddr_t *address = NULL;
    pcf_session_t *session6;
    pcf_app_session_t *sess;
    pcf_app_session_options_t options;
    ogs_sbi_request_t *request;
    OpenAPI_list_t *media_component_rm = media_component_rm_new();
    char *authority;

    ogs_assert(OGS_OK == ogs_getaddrinfo(&address, AF_UNSPEC, "::1", 7777, 0));
    authority = _pcf_authority(address);
    ABTS_STR_EQUAL(tc, "[::1]:7777", authority);
    ogs_free(authority);
    session6 = pcf_session_new(address);
    ogs_freeaddrinfo(address);
    ABTS_PTR_NOTNULL(tc, session6);
    if (!session6) { pcf_media_components_rm_free(media_component_rm); return; }

    sess = app_session_new_on(session6, "10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF);
    memset(&options, 0, sizeof(options));
    options.events = PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF;
    request = pcf_policyauthorization_request_create(sess, NULL, &options);
    ABTS_PTR_NOTNULL(tc, request);
    if (request) {
        ABTS_STR_EQUAL(tc, "http://[::1]:7777/npcf-policyauthorization/v1/app-sessions", request->h.uri ? request->h.uri : "");
        ogs_sbi_request_free(request);
    }
    sess->pcf_app_session_id = ogs_strdup("42");
    request = pcf_policyauthorization_request_update(sess, media_component_rm);
    ABTS_PTR_NOTNULL(tc, request);
    if (request) {
        ABTS_STR_EQUAL(tc, "http://[::1]:7777/npcf-policyauthorization/v1/app-sessions/42", request->h.uri ? request->h.uri : "");
        ogs_sbi_request_free(request);
    }
    request = pcf_policyauthorization_request_delete(sess);
    ABTS_PTR_NOTNULL(tc, request);
    if (request) {
        ABTS_STR_EQUAL(tc, "http://[::1]:7777/npcf-policyauthorization/v1/app-sessions/42/delete", request->h.uri ? request->h.uri : "");
        ogs_sbi_request_free(request);
    }
    request = pcf_policyauthorization_req_unsubscribe_event(sess);
    ABTS_PTR_NOTNULL(tc, request);
    if (request) {
        ABTS_STR_EQUAL(tc, "http://[::1]:7777/npcf-policyauthorization/v1/app-sessions/42/events-subscription", request->h.uri ? request->h.uri : "");
        ogs_sbi_request_free(request);
    }
    pcf_media_components_rm_free(media_component_rm);
    pcf_session_free(session6);
}

/*
 * TS 29.501 V19.5.0 clause 4.6.1.1.1.2 and TS 29.514 V19.8.0 clause 4.2.2.2: the Location of the 201 is the URI of the created context, absolute
 * or relative to the target URI of the POST, and the URIs of the later operations are built from it (CONSUMERS audit O4).
 */
static void location_func(abts_case *tc, void *data)
{
    static const struct { const char *location, *path; } cases[] = {
        {"http://127.0.0.13:7777/npcf-policyauthorization/v1/app-sessions/42", "/npcf-policyauthorization/v1/app-sessions/42"},
        {"http://[::1]:7777/prefix/npcf-policyauthorization/v1/app-sessions/42/", "/prefix/npcf-policyauthorization/v1/app-sessions/42"},
        {"/npcf-policyauthorization/v1/app-sessions/42?x=1#f", "/npcf-policyauthorization/v1/app-sessions/42"},
        {"//pcf.example/a/b/../c", "/a/c"},
        {"app-sessions/42", "/npcf-policyauthorization/v1/app-sessions/42"},
        {"../v1/app-sessions/42", "/npcf-policyauthorization/v1/app-sessions/42"},
        {"http://pcf.example", NULL},
        {"", NULL},
    };
    static const char *locations[] = {
        "http://127.0.0.13:7777/prefix/npcf-policyauthorization/v1/app-sessions/42",
        "/prefix/npcf-policyauthorization/v1/app-sessions/42",
        "app-sessions/42"
    };
    static const char *expected_uri[] = {
        "http://127.0.0.13:7777/prefix/npcf-policyauthorization/v1/app-sessions/42",
        "http://127.0.0.13:7777/prefix/npcf-policyauthorization/v1/app-sessions/42",
        "http://127.0.0.13:7777/npcf-policyauthorization/v1/app-sessions/42"
    };
    int i;

    for (i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
        char *path = _location_path(cases[i].location, "/npcf-policyauthorization/v1/app-sessions");

        ABTS_STR_EQUAL(tc, cases[i].path ? cases[i].path : "(none)", path ? path : "(none)");
        if (path) ogs_free(path);
    }
    for (i = 0; i < 3; i++) {
        outcome_t outcome;
        pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF);
        OpenAPI_list_t *media_component_rm = media_component_rm_new();
        ogs_sbi_request_t *request;

        memset(&outcome, 0, sizeof(outcome));
        sess->result.callback = result_callback;
        sess->result.user_data = &outcome;
        sess->operation = PCF_APP_SESSION_OP_CREATE;
        answer(tc, sess, "POST", "app-sessions", 201, locations[i], "{\"ascRespData\":{\"suppFeat\":\"2\"}}");
        ABTS_TRUE(tc, outcome.success);
        ABTS_STR_EQUAL(tc, "42", sess->pcf_app_session_id ? sess->pcf_app_session_id : "(none)");
        /* the URIs of the operations that follow are those of the context that the PCF named */
        request = pcf_policyauthorization_request_update(sess, media_component_rm);
        ABTS_PTR_NOTNULL(tc, request);
        if (request) {
            ABTS_STR_EQUAL(tc, expected_uri[i], request->h.uri ? request->h.uri : "");
            ogs_sbi_request_free(request);
        }
        pcf_media_components_rm_free(media_component_rm);
        _pcf_app_session_free(sess);
    }
}

/*
 * TS 29.501 V19.5.0 clause 4.6.1.1.1.2: the body of the 201 "should" hold the created resource, and TS 29.514 V19.8.0 table 5.6.2.3-1 has the features
 * "only when feature negotiation needs to take place" while clause 5.8 has the PCF indicate them: the context that the PCF created is kept, with no
 * optional feature (CR-055, CONSUMERS audit O8).
 */
static void create_no_features_func(abts_case *tc, void *data)
{
    static const char *answers[] = {NULL, "{\"ascRespData\":{}}"};
    int i;

    for (i = 0; i < 2; i++) {
        outcome_t outcome;
        pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF);

        memset(&outcome, 0, sizeof(outcome));
        sess->result.callback = result_callback;
        sess->result.user_data = &outcome;
        sess->operation = PCF_APP_SESSION_OP_CREATE;
        sess->policyauthorization_features = PCF_LIB_FEATURES;
        answer(tc, sess, "POST", "app-sessions", 201, CREATED_URI, answers[i]);
        ABTS_INT_EQUAL(tc, 1, outcome.calls);
        ABTS_TRUE(tc, outcome.success);
        ABTS_TRUE(tc, outcome.features == 0);
        if (outcome.success) {
            ABTS_STR_EQUAL(tc, "42", sess->pcf_app_session_id ? sess->pcf_app_session_id : "(none)");
            _pcf_app_session_free(sess);
        }
    }
}

/*
 * TS 29.514 V19.8.0 clause 4.2.2.2 and table 5.3.2.3.1-3: an error answer has the cause of its ProblemDetails and may have a Retry-After; the 303 See
 * Other has the Location of the existing context (CONSUMERS audit O2, O3).
 */
static void answer_result_func(abts_case *tc, void *data)
{
    outcome_t outcome;
    pcf_app_session_t *sess;

    sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF);
    memset(&outcome, 0, sizeof(outcome));
    sess->result.callback = result_callback;
    sess->result.user_data = &outcome;
    sess->operation = PCF_APP_SESSION_OP_CREATE;
    answer_with(tc, sess, "POST", "app-sessions", 403, NULL,
                "{\"title\":\"Forbidden\",\"status\":403,\"cause\":\"REQUESTED_SERVICE_NOT_AUTHORIZED\"}", "120");
    ABTS_INT_EQUAL(tc, 1, outcome.calls);
    ABTS_INT_EQUAL(tc, PCF_APP_SESSION_OP_CREATE, outcome.operation);
    ABTS_TRUE(tc, !outcome.success);
    ABTS_INT_EQUAL(tc, 403, outcome.http_status);
    ABTS_STR_EQUAL(tc, "REQUESTED_SERVICE_NOT_AUTHORIZED", outcome.problem_cause);
    ABTS_INT_EQUAL(tc, 120, outcome.retry_after);
    ABTS_STR_EQUAL(tc, "", outcome.redirect_location);

    sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF);
    memset(&outcome, 0, sizeof(outcome));
    sess->result.callback = result_callback;
    sess->result.user_data = &outcome;
    sess->operation = PCF_APP_SESSION_OP_CREATE;
    answer(tc, sess, "POST", "app-sessions", 303, "http://127.0.0.13:7777/npcf-policyauthorization/v1/app-sessions/41", NULL);
    ABTS_INT_EQUAL(tc, 1, outcome.calls);
    ABTS_TRUE(tc, !outcome.success);
    ABTS_INT_EQUAL(tc, 303, outcome.http_status);
    ABTS_STR_EQUAL(tc, "http://127.0.0.13:7777/npcf-policyauthorization/v1/app-sessions/41", outcome.redirect_location);
    ABTS_INT_EQUAL(tc, -1, outcome.retry_after);

    /* a refused update, with a date as the retry interval far in the future */
    sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_NONE);
    memset(&outcome, 0, sizeof(outcome));
    sess->pcf_app_session_id = ogs_strdup("42");
    sess->result.callback = result_callback;
    sess->result.user_data = &outcome;
    sess->operation = PCF_APP_SESSION_OP_UPDATE;
    answer_with(tc, sess, "PATCH", "app-sessions/42", 403, NULL, "{\"title\":\"Forbidden\",\"status\":403,\"cause\":\"X\"}",
                "Fri, 31 Dec 2100 23:59:59 GMT");
    ABTS_INT_EQUAL(tc, 1, outcome.calls);
    ABTS_TRUE(tc, !outcome.success);
    ABTS_INT_EQUAL(tc, PCF_APP_SESSION_OP_UPDATE, outcome.operation);
    ABTS_TRUE(tc, outcome.retry_after > 1000000);
    _pcf_app_session_free(sess);
}

/* RFC 9110 clause 10.2.3: Retry-After is a number of seconds or a date */
static void retry_after_func(abts_case *tc, void *data)
{
    ABTS_INT_EQUAL(tc, 120, _retry_after_seconds("120", 946684800));
    ABTS_INT_EQUAL(tc, 7, _retry_after_seconds(" 7 ", 946684800));
    ABTS_INT_EQUAL(tc, 0, _retry_after_seconds("0", 946684800));
    ABTS_INT_EQUAL(tc, 60, _retry_after_seconds("Sat, 01 Jan 2000 00:01:00 GMT", 946684800));
    ABTS_INT_EQUAL(tc, 0, _retry_after_seconds("Sat, 01 Jan 2000 00:01:00 GMT", 946684800 + 3600));
    ABTS_INT_EQUAL(tc, -1, _retry_after_seconds("soon", 946684800));
    ABTS_INT_EQUAL(tc, -1, _retry_after_seconds("12x", 946684800));
    ABTS_INT_EQUAL(tc, -1, _retry_after_seconds("", 946684800));
    ABTS_INT_EQUAL(tc, -1, _retry_after_seconds(NULL, 946684800));
    ABTS_INT_EQUAL(tc, 2147483647, _retry_after_seconds("99999999999", 946684800));
}

#define EVENT_BIT(event) (1 << (event))

/*
 * TS 29.514 V19.8.0 clauses 4.2.2.2, 4.2.3.2, 4.2.4.3 and 4.2.6.2: the EventsNotification that the PCF puts into its answer (evsNotif of the
 * AppSessionContext, evNotifs of the EventsSubscPutData) is what a notification would carry, and reaches the callbacks of those events (O5).
 */
static void answer_notifications_func(abts_case *tc, void *data)
{
    outcome_t outcome;
    pcf_app_session_t *sess;

    /* create */
    sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_NONE);
    memset(&outcome, 0, sizeof(outcome));
    outcome.retry_after = -1;
    _pcf_app_session_add_event_notification(sess, PCF_APP_SESSION_EVENT_TYPE_SUCCESSFUL_RESOURCES_ALLOCATION, counting_callback, &outcome);
    sess->operation = PCF_APP_SESSION_OP_CREATE;
    answer(tc, sess, "POST", "app-sessions", 201, CREATED_URI,
           "{\"ascRespData\":{\"suppFeat\":\"2\"},\"evsNotif\":{\"evSubsUri\":\"http://127.0.0.13:7777/npcf-policyauthorization/v1/app-sessions/42/events-subscription\",\"evNotifs\":[{\"event\":\"SUCCESSFUL_RESOURCES_ALLOCATION\"}]}}");
    ABTS_INT_EQUAL(tc, EVENT_BIT(OpenAPI_npcf_af_event_SUCCESSFUL_RESOURCES_ALLOCATION), outcome.notifications);

    /* update */
    memset(&outcome, 0, sizeof(outcome));
    _pcf_app_session_add_event_notification(sess, PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF, counting_callback, &outcome);
    sess->operation = PCF_APP_SESSION_OP_UPDATE;
    answer(tc, sess, "PATCH", "app-sessions/42", 200,  NULL,
           "{\"ascReqData\":{\"notifUri\":\"http://127.0.0.1:9999/n\",\"suppFeat\":\"2\"},\"evsNotif\":{\"evSubsUri\":\"http://127.0.0.13:7777/npcf-policyauthorization/v1/app-sessions/42/events-subscription\",\"evNotifs\":[{\"event\":\"QOS_NOTIF\"}]}}");
    ABTS_INT_EQUAL(tc, EVENT_BIT(OpenAPI_npcf_af_event_QOS_NOTIF), outcome.notifications);

    /* subscription: the EventsSubscPutData "may contain the notification of the already met events" */
    memset(&outcome, 0, sizeof(outcome));
    sess->operation = PCF_APP_SESSION_OP_EVENTS;
    answer(tc, sess, "PUT", "app-sessions/42/events-subscription", 200, NULL,
           "{\"events\":[{\"event\":\"QOS_NOTIF\",\"notifMethod\":\"EVENT_DETECTION\"}],\"evSubsUri\":\"http://127.0.0.13:7777/npcf-policyauthorization/v1/app-sessions/42/events-subscription\",\"evNotifs\":[{\"event\":\"QOS_NOTIF\"}]}");
    ABTS_INT_EQUAL(tc, EVENT_BIT(OpenAPI_npcf_af_event_QOS_NOTIF), outcome.notifications);

    /* delete: the usage report of clause 4.2.4.3 */
    memset(&outcome, 0, sizeof(outcome));
    _pcf_app_session_add_event_notification(sess, PCF_APP_SESSION_EVENT_TYPE_USAGE_REPORT, counting_callback, &outcome);
    sess->operation = PCF_APP_SESSION_OP_DELETE;
    answer(tc, sess, "POST", "app-sessions/42/delete", 200, NULL,
           "{\"ascReqData\":{\"notifUri\":\"http://127.0.0.1:9999/n\",\"suppFeat\":\"2\"},\"evsNotif\":{\"evSubsUri\":\"http://127.0.0.13:7777/npcf-policyauthorization/v1/app-sessions/42/events-subscription\",\"evNotifs\":[{\"event\":\"USAGE_REPORT\"}]}}");
    ABTS_INT_EQUAL(tc, EVENT_BIT(OpenAPI_npcf_af_event_USAGE_REPORT), outcome.notifications);
    /* the answer ended the object: it is not freed again */
}

/*
 * TS 29.514 V19.8.0 clause 4.2.7.2: the DELETE of the Events Subscription removes all its events, and the library forgets them with the callbacks
 * registered for them: a later subscription sends only the events that it names (CONSUMERS audit O10).
 */
static void unsubscribe_func(abts_case *tc, void *data)
{
    static const OpenAPI_npcf_af_event_e more[] = {OpenAPI_npcf_af_event_USAGE_REPORT};
    outcome_t outcome;
    pcf_app_session_options_t options;
    pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF | PCF_APP_SESSION_EVENT_TYPE_FAILED_RESOURCES_ALLOCATION);
    OpenAPI_events_subsc_req_data_t *subsc;
    ogs_sbi_request_t *request;

    memset(&options, 0, sizeof(options));
    options.events = PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF | PCF_APP_SESSION_EVENT_TYPE_FAILED_RESOURCES_ALLOCATION;
    request = pcf_policyauthorization_request_create(sess, NULL, &options);
    ABTS_PTR_NOTNULL(tc, request);
    if (request) ogs_sbi_request_free(request);
    sess->pcf_app_session_id = ogs_strdup("42");
    ABTS_INT_EQUAL(tc, 2, put_events(tc, sess));
    ABTS_INT_EQUAL(tc, 1, (int)ogs_list_count(&sess->pcf_event_notifications));

    memset(&outcome, 0, sizeof(outcome));
    sess->result.callback = result_callback;
    sess->result.user_data = &outcome;
    sess->operation = PCF_APP_SESSION_OP_EVENTS;
    answer(tc, sess, "DELETE", "app-sessions/42/events-subscription", 204, NULL, NULL);
    ABTS_INT_EQUAL(tc, 1, outcome.calls);
    ABTS_INT_EQUAL(tc, PCF_APP_SESSION_OP_EVENTS, outcome.operation);
    ABTS_TRUE(tc, outcome.success);
    ABTS_INT_EQUAL(tc, 0, (int)ogs_list_count(&sess->pcf_event_notifications));

    subsc = subsc_new(1, more);
    ABTS_INT_EQUAL(tc, 1, merge_event_subsc_to_app_session_context(sess, subsc));
    subsc_free(subsc);
    ABTS_INT_EQUAL(tc, 1, put_events(tc, sess));
    _pcf_app_session_free(sess);

    /* an answer that is not 204 leaves the subscription as it was */
    sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF);
    memset(&options, 0, sizeof(options));
    options.events = PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF;
    request = pcf_policyauthorization_request_create(sess, NULL, &options);
    if (request) ogs_sbi_request_free(request);
    sess->pcf_app_session_id = ogs_strdup("42");
    memset(&outcome, 0, sizeof(outcome));
    sess->result.callback = result_callback;
    sess->result.user_data = &outcome;
    sess->operation = PCF_APP_SESSION_OP_EVENTS;
    answer(tc, sess, "DELETE", "app-sessions/42/events-subscription", 404, NULL, "{\"title\":\"Not Found\",\"status\":404}");
    ABTS_TRUE(tc, !outcome.success);
    ABTS_INT_EQUAL(tc, 1, (int)ogs_list_count(&sess->pcf_event_notifications));
    ABTS_INT_EQUAL(tc, 1, put_events(tc, sess));
    _pcf_app_session_free(sess);
}

/*
 * TS 29.514 V19.8.0 table 5.6.2.6-1 and clause 4.2.6.2: the details of an events subscription (the QoS monitoring parameters and thresholds, the
 * notification correlation identifier) given to a later subscription are sent with it (CONSUMERS audit O12).
 */
static void subscribe_details_func(abts_case *tc, void *data)
{
    static const OpenAPI_npcf_af_event_e monitoring[] = {OpenAPI_npcf_af_event_QOS_MONITORING};
    pcf_app_session_options_t options;
    pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_NONE);
    OpenAPI_events_subsc_req_data_t *subsc = subsc_new(1, monitoring);
    ogs_sbi_request_t *request;
    cJSON *json, *params, *qos_mon;

    memset(&options, 0, sizeof(options));
    request = pcf_policyauthorization_request_create(sess, NULL, &options);
    if (request) ogs_sbi_request_free(request);
    sess->pcf_app_session_id = ogs_strdup("42");

    subsc->req_qos_mon_params = OpenAPI_list_create();
    OpenAPI_list_add(subsc->req_qos_mon_params, (void *)(intptr_t)OpenAPI_requested_qos_monitoring_parameter_DOWNLINK);
    subsc->qos_mon = ogs_calloc(1, sizeof(*subsc->qos_mon));
    subsc->qos_mon->is_rep_thresh_dl = true;
    subsc->qos_mon->rep_thresh_dl = 10;
    subsc->notif_corre_id = ogs_strdup("correlation-2");
    ABTS_INT_EQUAL(tc, 1, merge_event_subsc_to_app_session_context(sess, subsc) > 0);
    OpenAPI_list_free(subsc->req_qos_mon_params);
    subsc->req_qos_mon_params = NULL;
    ogs_free(subsc->qos_mon);
    subsc->qos_mon = NULL;
    ogs_free(subsc->notif_corre_id);
    subsc->notif_corre_id = NULL;
    subsc_free(subsc);

    request = pcf_policyauthorization_req_subscribe_event(sess);
    json = content_of(tc, request);
    params = cJSON_GetObjectItemCaseSensitive(json, "reqQosMonParams");
    ABTS_TRUE(tc, cJSON_IsArray(params) && cJSON_GetArraySize(params) == 1);
    if (cJSON_IsArray(params) && cJSON_GetArraySize(params) == 1)
        ABTS_STR_EQUAL(tc, "DOWNLINK", cJSON_GetArrayItem(params, 0)->valuestring);
    qos_mon = cJSON_GetObjectItemCaseSensitive(json, "qosMon");
    ABTS_PTR_NOTNULL(tc, qos_mon);
    if (qos_mon) {
        cJSON *threshold = cJSON_GetObjectItemCaseSensitive(qos_mon, "repThreshDl");
        ABTS_TRUE(tc, cJSON_IsNumber(threshold) && threshold->valueint == 10);
    }
    ABTS_STR_EQUAL(tc, "correlation-2", string_of(json, "notifCorreId") ? string_of(json, "notifCorreId") : "");
    if (json) cJSON_Delete(json);
    if (request) ogs_sbi_request_free(request);
    _pcf_app_session_free(sess);
}

/*
 * The answer to the deletion of one application session context ends that object and no other: another one of the same PCF session that the PCF has
 * not answered yet has no identifier to compare.
 */
static void delete_sibling_func(abts_case *tc, void *data)
{
    outcome_t outcome;
    pcf_app_session_t *pending = app_session_new("10.45.0.3", PCF_APP_SESSION_EVENT_TYPE_NONE);
    pcf_app_session_t *deleted = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_NONE);

    memset(&outcome, 0, sizeof(outcome));
    deleted->pcf_app_session_id = ogs_strdup("42");
    deleted->result.callback = result_callback;
    deleted->result.user_data = &outcome;
    deleted->operation = PCF_APP_SESSION_OP_DELETE;
    answer(tc, deleted, "POST", "app-sessions/42/delete", 204, NULL, NULL);
    ABTS_INT_EQUAL(tc, 1, outcome.calls);
    ABTS_TRUE(tc, outcome.success);
    ABTS_TRUE(tc, _pcf_app_session_exists(pending));
    ABTS_TRUE(tc, !_pcf_app_session_exists(deleted));
    _pcf_app_session_free(pending);
}

/* TS 29.514 V19.8.0 clause 5.5.3 and 4.2.5.3: the termCause of the TerminationInfo of a termination request is given to the application (O14) */
static void termination_cause_func(abts_case *tc, void *data)
{
    static const char *bodies[] = {
        "{\"resUri\":\"http://127.0.0.13:7777/npcf-policyauthorization/v1/app-sessions/42\",\"termCause\":\"PDU_SESSION_TERMINATION\"}",
        "{\"resUri\":\"x\",\"termCause\":\"ALL_SDF_DEACTIVATION\"}",
        "not json",
        NULL
    };
    static const char *causes[] = {"PDU_SESSION_TERMINATION", "ALL_SDF_DEACTIVATION", "", ""};
    int i;

    for (i = 0; i < 4; i++) {
        outcome_t outcome;
        pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_NONE);

        memset(&outcome, 0, sizeof(outcome));
        sess->result.callback = result_callback;
        sess->result.user_data = &outcome;
        sess->operation = PCF_APP_SESSION_OP_TERMINATED;
        _pcf_app_session_set_answer(sess, 0, NULL);
        _pcf_app_session_set_termination_cause(sess, bodies[i]);
        ABTS_TRUE(tc, _pcf_app_session_change_callback_call(sess, true));
        ABTS_INT_EQUAL(tc, PCF_APP_SESSION_OP_TERMINATED, outcome.operation);
        ABTS_STR_EQUAL(tc, causes[i], outcome.termination_cause);
        _pcf_app_session_free(sess);
    }
}

/*
 * TS 29.500 V19.7.1 clause 6.6.2: "attributes or enumerated values" that relate to a feature "shall only be sent" if the feature is supported. The
 * AfEvent values and the attributes of the events subscription carry their feature in the column Applicability of TS 29.514 V19.8.0 tables 5.6.3.7-1,
 * 5.6.2.6-1 and 5.6.2.10-1; the library offers the features 2, 15, 16, 17, 28, 59, 63, 68 and 69 (CONSUMERS audit O11, O13).
 */
static void features_gate_func(abts_case *tc, void *data)
{
    static const OpenAPI_npcf_af_event_e unmapped[] = {
        OpenAPI_npcf_af_event_EXTRA_UE_ADDR, OpenAPI_npcf_af_event_QOS_MON_CAP_REPO, OpenAPI_npcf_af_event_UE_REACH_STATUS_CH,
        OpenAPI_npcf_af_event_BAT_OFFSET_INFO, OpenAPI_npcf_af_event_URSP_ENF_INFO, OpenAPI_npcf_af_event_PACK_DEL_VAR,
        OpenAPI_npcf_af_event_RT_DELAY_TWO_QOS_FLOWS, OpenAPI_npcf_af_event_RATE_LIMIT_INFO_REPO, OpenAPI_npcf_af_event_UP_PATH_CHANGE,
        OpenAPI_npcf_af_event_CN_HEALTH_MONITOR, OpenAPI_npcf_af_event_UE_TEMPORARILY_UNAVAILABLE
    };
    static const OpenAPI_npcf_af_event_e offered[] = {
        OpenAPI_npcf_af_event_ACCESS_TYPE_CHANGE, OpenAPI_npcf_af_event_QOS_NOTIF, OpenAPI_npcf_af_event_QOS_MONITORING,
        OpenAPI_npcf_af_event_USAGE_REPORT, OpenAPI_npcf_af_event_L4S_SUPP, OpenAPI_npcf_af_event_SUCCESSFUL_RESOURCES_ALLOCATION
    };
    char what[96];
    int i;
    OpenAPI_events_subsc_req_data_t *subsc;
    ogs_sockaddr_t *ue_address = NULL;
    ue_network_identifier_t ue;
    pcf_app_session_options_t options;
    int sessions_before = (int)ogs_list_count(&session->pcf_app_sessions);

    /* the 11 events that have no callback bit, and every event of a feature that the library does not offer, are not sent */
    for (i = 0; i < (int)(sizeof(unmapped) / sizeof(unmapped[0])); i++) {
        subsc = subsc_new(1, &unmapped[i]);
        ABTS_TRUE(tc, !pcf_events_subscription_allowed(0, subsc, PCF_LIB_FEATURES, PCF_LIB_FEATURES_HI, what, sizeof(what)));
        subsc_free(subsc);
    }
    ABTS_TRUE(tc, !pcf_events_subscription_allowed(PCF_APP_SESSION_EVENT_TYPE_ANI_REPORT, NULL, PCF_LIB_FEATURES, PCF_LIB_FEATURES_HI, what, sizeof(what)));
    ABTS_TRUE(tc, strstr(what, "ANI_REPORT") != NULL);
    ABTS_TRUE(tc, !pcf_events_subscription_allowed(PCF_APP_SESSION_EVENT_TYPE_OUT_OF_CREDIT, NULL, PCF_LIB_FEATURES, PCF_LIB_FEATURES_HI, what, sizeof(what)));
    /* the events of the features that are offered, and those of no feature, are */
    for (i = 0; i < (int)(sizeof(offered) / sizeof(offered[0])); i++) {
        subsc = subsc_new(1, &offered[i]);
        ABTS_TRUE(tc, pcf_events_subscription_allowed(0, subsc, PCF_LIB_FEATURES, PCF_LIB_FEATURES_HI, what, sizeof(what)));
        subsc_free(subsc);
    }
    ABTS_TRUE(tc, pcf_events_subscription_allowed(PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF | PCF_APP_SESSION_EVENT_TYPE_QOS_MONITORING |
                PCF_APP_SESSION_EVENT_TYPE_USAGE_REPORT, NULL, PCF_LIB_FEATURES, PCF_LIB_FEATURES_HI, what, sizeof(what)));
    /* after the answer of the PCF only the features that both support: QOS_MONITORING needs feature 16 */
    ABTS_TRUE(tc, !pcf_events_subscription_allowed(PCF_APP_SESSION_EVENT_TYPE_QOS_MONITORING, NULL, PCF_LIB_FEATURES & ~((uint64_t)1 << 15), PCF_LIB_FEATURES_HI,
                what, sizeof(what)));
    ABTS_TRUE(tc, pcf_events_subscription_allowed(PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF, NULL, 0, 0, what, sizeof(what)));

    /* the attributes of the subscription */
    subsc = subsc_new(1, &offered[2]);
    subsc->notif_corre_id = (char *)"c";
    subsc->usg_thres = (OpenAPI_usage_threshold_t *)subsc;   /* only its presence is looked at */
    ABTS_TRUE(tc, pcf_events_subscription_allowed(0, subsc, PCF_LIB_FEATURES, PCF_LIB_FEATURES_HI, what, sizeof(what)));
    ABTS_TRUE(tc, !pcf_events_subscription_allowed(0, subsc, PCF_LIB_FEATURES & ~((uint64_t)1 << 1), PCF_LIB_FEATURES_HI, what, sizeof(what)));
    ABTS_TRUE(tc, strstr(what, "usgThres") != NULL);
    subsc->usg_thres = NULL;
    subsc->notif_corre_id = NULL;
    subsc->is_avrg_wndw = true;                 /* EnQoSMon, not offered */
    ABTS_TRUE(tc, !pcf_events_subscription_allowed(0, subsc, PCF_LIB_FEATURES, PCF_LIB_FEATURES_HI, what, sizeof(what)));
    subsc->is_avrg_wndw = false;
    subsc->is_direct_notif_ind = true;          /* ExposureToEAS or EnQoSMon, neither offered */
    ABTS_TRUE(tc, !pcf_events_subscription_allowed(0, subsc, PCF_LIB_FEATURES, PCF_LIB_FEATURES_HI, what, sizeof(what)));
    subsc->is_direct_notif_ind = false;
    ((OpenAPI_af_event_subscription_t *)subsc->events->first->data)->qos_mon_param_type = OpenAPI_qos_monitoring_param_type_CONGESTION;
    ABTS_TRUE(tc, !pcf_events_subscription_allowed(0, subsc, PCF_LIB_FEATURES, PCF_LIB_FEATURES_HI, what, sizeof(what)));
    ((OpenAPI_af_event_subscription_t *)subsc->events->first->data)->qos_mon_param_type = OpenAPI_qos_monitoring_param_type_NULL;
    ((OpenAPI_af_event_subscription_t *)subsc->events->first->data)->notif_method = OpenAPI_af_notif_method_PERIODIC;
    ABTS_TRUE(tc, pcf_events_subscription_allowed(0, subsc, PCF_LIB_FEATURES, PCF_LIB_FEATURES_HI, what, sizeof(what)));
    ABTS_TRUE(tc, !pcf_events_subscription_allowed(0, subsc, PCF_LIB_FEATURES & ~((uint64_t)1 << 14), PCF_LIB_FEATURES_HI, what, sizeof(what)));
    subsc_free(subsc);

    /* the creation of an application session context with such an event is refused, and nothing is sent */
    ogs_assert(OGS_OK == ogs_getaddrinfo(&ue_address, AF_UNSPEC, "10.45.0.2", 0, 0));
    memset(&ue, 0, sizeof(ue));
    ue.address = ue_address;
    memset(&options, 0, sizeof(options));
    options.events = PCF_APP_SESSION_EVENT_TYPE_ANI_REPORT;
    ABTS_TRUE(tc, !pcf_session_create_app_session_with_options(session, &ue, &options, NULL, notification_callback, NULL, result_callback, NULL));
    ABTS_INT_EQUAL(tc, sessions_before, (int)ogs_list_count(&session->pcf_app_sessions));
    ogs_freeaddrinfo(ue_address);

    /* and so is the subscription to it later */
    {
        pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_NONE);

        sess->pcf_app_session_id = ogs_strdup("42");
        sess->policyauthorization_features = PCF_LIB_FEATURES & ~((uint64_t)1 << 15);     /* QoSMonitoring was not negotiated */
        subsc = subsc_new(1, &offered[2]);
        ABTS_TRUE(tc, !pcf_app_session_subscribe_event(sess, subsc, notification_callback, NULL));
        ABTS_INT_EQUAL(tc, 1, (int)ogs_list_count(&sess->pcf_event_notifications));
        subsc_free(subsc);
        _pcf_app_session_free(sess);
    }
}

/*
 * TS 29.514 V19.8.0 clauses 4.2.2.32 and 4.2.2.43: a consumer that provides alternative service requirements "shall also subscribe" to the outcome of the
 * resources allocation and to the QoS notification control, and one that gives the indication of ECN marking for L4S shall subscribe to L4S_SUPP.
 */
static OpenAPI_list_t *media_component_new(bool alternatives, OpenAPI_uplink_downlink_support_e l4s)
{
    OpenAPI_list_t *list = OpenAPI_list_create();
    OpenAPI_media_component_t *component = ogs_calloc(1, sizeof(*component));

    ogs_assert(list && component);
    component->med_comp_n = 1;
    component->f_status = OpenAPI_flow_status_ENABLED;
    if (alternatives) {
        component->alt_ser_reqs = OpenAPI_list_create();
        OpenAPI_list_add(component->alt_ser_reqs, ogs_strdup("reference-1"));
    }
    component->l4s_ind = l4s;
    OpenAPI_list_add(list, OpenAPI_map_create(ogs_msprintf("%d", component->med_comp_n), component));
    return list;
}

static int event_count(cJSON *json, const char *event_name)
{
    cJSON *req_data = cJSON_GetObjectItemCaseSensitive(json, "ascReqData");
    cJSON *ev_subsc = req_data ? cJSON_GetObjectItemCaseSensitive(req_data, "evSubsc") : NULL;
    cJSON *events = ev_subsc ? cJSON_GetObjectItemCaseSensitive(ev_subsc, "events") : NULL;
    cJSON *event;
    int count = 0;

    cJSON_ArrayForEach(event, events)
        if (string_of(event, "event") && !strcmp(string_of(event, "event"), event_name)) count++;
    return count;
}

static void companion_events_func(abts_case *tc, void *data)
{
    static const struct {
        bool alternatives;
        OpenAPI_uplink_downlink_support_e l4s;
        int events;
        int resources, qos_notif, l4s_supp;
    } cases[] = {
        {false, OpenAPI_uplink_downlink_support_NULL, PCF_APP_SESSION_EVENT_TYPE_NONE, 0, 0, 0},
        {true, OpenAPI_uplink_downlink_support_NULL, PCF_APP_SESSION_EVENT_TYPE_NONE, 1, 1, 0},
        /* given by the application: not listed twice */
        {true, OpenAPI_uplink_downlink_support_NULL, PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF | PCF_APP_SESSION_EVENT_TYPE_SUCCESSFUL_RESOURCES_ALLOCATION, 1, 1, 0},
        {false, OpenAPI_uplink_downlink_support_UL_DL, PCF_APP_SESSION_EVENT_TYPE_NONE, 0, 0, 1},
        {false, OpenAPI_uplink_downlink_support_DL, PCF_APP_SESSION_EVENT_TYPE_L4S_SUPP, 0, 0, 1},
    };
    int i;

    for (i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
        pcf_app_session_t *sess = app_session_new("10.45.0.2", cases[i].events);
        pcf_app_session_options_t options;
        OpenAPI_list_t *components = media_component_new(cases[i].alternatives, cases[i].l4s);
        ogs_sbi_request_t *request;
        cJSON *json;

        memset(&options, 0, sizeof(options));
        options.events = cases[i].events;
        request = pcf_policyauthorization_request_create(sess, components, &options);
        json = content_of(tc, request);
        ABTS_INT_EQUAL(tc, cases[i].resources, event_count(json, "SUCCESSFUL_RESOURCES_ALLOCATION"));
        ABTS_INT_EQUAL(tc, cases[i].qos_notif, event_count(json, "QOS_NOTIF"));
        ABTS_INT_EQUAL(tc, cases[i].l4s_supp, event_count(json, "L4S_SUPP"));
        if (json) cJSON_Delete(json);
        if (request) ogs_sbi_request_free(request);
        pcf_media_components_free(components);
        _pcf_app_session_free(sess);
    }
}

/*
 * Implementation choice, text silent, see CONSUMERS audit O9: the update is a PATCH whether or not PatchCorrection (feature 28) was negotiated (TS 29.514
 * V19.8.0 clauses 4.2.3.2 and 5.8 give no other way to update a context).
 */
static void update_without_patch_correction_func(abts_case *tc, void *data)
{
    pcf_app_session_t *sess = app_session_new("10.45.0.2", PCF_APP_SESSION_EVENT_TYPE_NONE);
    OpenAPI_list_t *media_component_rm = media_component_rm_new();
    ogs_sbi_request_t *request;

    sess->pcf_app_session_id = ogs_strdup("42");
    sess->policyauthorization_features = 0;
    sess->policyauthorization_features_hi = 0;
    request = pcf_policyauthorization_request_update(sess, media_component_rm);
    ABTS_PTR_NOTNULL(tc, request);
    if (request) {
        ABTS_STR_EQUAL(tc, OGS_SBI_HTTP_METHOD_PATCH, request->h.method);
        ogs_sbi_request_free(request);
    }
    pcf_media_components_rm_free(media_component_rm);
    _pcf_app_session_free(sess);
}

/*
 * Implementation choice, text silent, see CONSUMERS audit O6: the SUPI, GPSI, DNN and IP domain that the application gives are kept and not sent; the
 * UE address binds the application session (TS 29.514 V19.8.0 table 5.6.2.3-1 notes 2 and 7: the PCF matches all it is given, with the DNN as given).
 */
static void ue_identifier_func(abts_case *tc, void *data)
{
    ue_network_identifier_t ue;
    ogs_sockaddr_t *ue_address = NULL;
    pcf_app_session_t *sess;
    pcf_app_session_options_t options;
    ogs_sbi_request_t *request;
    cJSON *json, *req_data;

    ogs_assert(OGS_OK == ogs_getaddrinfo(&ue_address, AF_UNSPEC, "10.45.0.2", 0, 0));
    memset(&ue, 0, sizeof(ue));
    ue.address = ue_address;
    ue.supi = (char *)"imsi-001010000000001";
    ue.gpsi = (char *)"msisdn-123456";
    ue.dnn = (char *)"internet";
    ue.ip_domain = (char *)"domain-1";
    sess = _pcf_app_session_new(session, &ue, PCF_APP_SESSION_EVENT_TYPE_NONE, notification_callback, NULL, NULL, NULL);
    ogs_freeaddrinfo(ue_address);
    ogs_assert(sess);
    sess->notif_url = ogs_strdup("http://127.0.0.1:9999/npcf-policyauthorization/v1/app-session-instance/1");
    memset(&options, 0, sizeof(options));
    request = pcf_policyauthorization_request_create(sess, NULL, &options);
    json = content_of(tc, request);
    req_data = cJSON_GetObjectItemCaseSensitive(json, "ascReqData");
    ABTS_PTR_NOTNULL(tc, req_data);
    if (req_data) {
        ABTS_STR_EQUAL(tc, "10.45.0.2", string_of(req_data, "ueIpv4") ? string_of(req_data, "ueIpv4") : "");
        ABTS_TRUE(tc, cJSON_GetObjectItemCaseSensitive(req_data, "supi") == NULL);
        ABTS_TRUE(tc, cJSON_GetObjectItemCaseSensitive(req_data, "gpsi") == NULL);
        ABTS_TRUE(tc, cJSON_GetObjectItemCaseSensitive(req_data, "dnn") == NULL);
        ABTS_TRUE(tc, cJSON_GetObjectItemCaseSensitive(req_data, "ipDomain") == NULL);
    }
    if (json) cJSON_Delete(json);
    if (request) ogs_sbi_request_free(request);
    _pcf_app_session_free(sess);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
