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
#include "bsf-service-consumer.h"
#include "bsf-client-sess.h"
#include "context.h"
#include "nbsf-management-build.h"
#include "nbsf-process.h"
#include "utils.h"

/* Unit test includes */
#include "bsf-test-unit.h"

/*
 * The requests that the library builds and the answers that it reads, checked against TS 29.521 V19.7.0 without a core network: a request is built and
 * read, an answer is made up and handed to bsf_process_event().
 */

static void cache_narrowed_func(abts_case *tc, void *data);
static void result_status_func(abts_case *tc, void *data);
static void ipv6_request_func(abts_case *tc, void *data);
static void expiry_func(abts_case *tc, void *data);

int bsf_unit_initialise(void)
{
    int rv;

    rv = ogs_app_parse_local_conf("af");
    if (rv != OGS_OK) return rv;

    ogs_sbi_context_init(OpenAPI_nf_type_AF);

    /* the context of the library is made by reading the configuration, which here has no section for it: the context is what the tests need */
    (void)_bsf_parse_config("bsf-unit-test");
    return _bsf_client_self() ? OGS_OK : OGS_ERROR;
}

void bsf_unit_terminate(void)
{
    bsf_terminate();
    ogs_sbi_context_final();
}

abts_suite *test_bsf_unit(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    abts_run_test(suite, cache_narrowed_func, NULL);
    abts_run_test(suite, result_status_func, NULL);
    abts_run_test(suite, ipv6_request_func, NULL);
    abts_run_test(suite, expiry_func, NULL);

    return suite;
}

typedef struct outcome_s {
    int calls;
    int http_status;
    bool binding;
    char dnn[32];
    char cause[64];
} outcome_t;

static bool result_callback(const bsf_pcf_binding_result_t *result, void *user_data)
{
    outcome_t *outcome = user_data;

    outcome->calls++;
    outcome->http_status = result->http_status;
    outcome->binding = result->pcf_binding != NULL;
    snprintf(outcome->dnn, sizeof(outcome->dnn), "%s", result->pcf_binding && result->pcf_binding->dnn ? result->pcf_binding->dnn : "");
    snprintf(outcome->cause, sizeof(outcome->cause), "%s", result->problem_cause ? result->problem_cause : "");
    return true;
}

/* A request for the PCF binding of the UE address, as bsf_retrieve_pcf_binding_with_result() makes it, waiting for the answer of the BSF */
static ogs_sbi_xact_t *request_new(abts_case *tc, ogs_sockaddr_t *ue_address, const bsf_pcf_binding_query_t *query, outcome_t *outcome)
{
    bsf_client_sess_t *sess = _bsf_client_sess_new();
    ogs_sbi_xact_t *xact;

    ABTS_PTR_NOTNULL(tc, sess);
    _bsf_client_sess_ue_address_set(sess, ue_address);
    if (ue_address->ogs_sa_family == AF_INET)
        _bsf_client_sess_ipv4addr_set_from_sockaddr(sess, ue_address);
    else
        _bsf_client_sess_ipv6prefix_set_from_sockaddr(sess, ue_address);
    if (query) _bsf_client_sess_query_set(sess, query);
    _bsf_client_sess_result_callback_set(sess, result_callback, outcome);
    xact = ogs_sbi_xact_add(OGS_INVALID_POOL_ID, &sess->sbi, OpenAPI_service_name_nbsf_management, NULL,
                            (ogs_sbi_build_f)_nbsf_management_pcf_binding_build, sess, NULL);
    ABTS_PTR_NOTNULL(tc, xact);
    return xact;
}

/* The answer of the BSF to the request, as the client of the library hands it over; the request and its session are released by it */
static void answer_cached(abts_case *tc, ogs_sbi_xact_t *xact, int status, const char *content, const char *cache_control)
{
    ogs_event_t *e = ogs_event_new(OGS_EVENT_SBI_CLIENT);
    ogs_sbi_response_t *response = ogs_sbi_response_new();

    ogs_assert(e && response);
    response->status = status;
    response->h.method = ogs_strdup(OGS_SBI_HTTP_METHOD_GET);
    response->h.uri = ogs_strdup("http://127.0.0.14:7777/nbsf-management/v1/pcfBindings");
    if (content) {
        response->http.content = ogs_strdup(content);
        response->http.content_length = strlen(content);
        ogs_sbi_header_set(response->http.headers, OGS_SBI_CONTENT_TYPE, status >= 300 ? OGS_SBI_CONTENT_PROBLEM_TYPE : OGS_SBI_CONTENT_JSON_TYPE);
    }
    if (cache_control) ogs_sbi_header_set(response->http.headers, "Cache-Control", cache_control);
    e->sbi.response = response;
    e->sbi.data = OGS_UINT_TO_POINTER(xact->id);
    e->sbi.state = OGS_OK;
    ABTS_TRUE(tc, bsf_process_event(e));
    ogs_event_free(e);
}

static void answer(abts_case *tc, ogs_sbi_xact_t *xact, int status, const char *content)
{
    answer_cached(tc, xact, status, content, NULL);
}

#define BINDING "{\"dnn\":\"internet\",\"snssai\":{\"sst\":1},\"ipv4Addr\":\"10.45.0.2\",\"pcfFqdn\":\"pcf.example.net\"}"

/*
 * The cache of the library is keyed by the UE address. An answer to a query with the narrowing keys (DNN, slice, SUPI, GPSI, IP domain) is the one of
 * that combination, so it is not kept under the bare address, where it would answer a plain query for the same address in another slice or domain
 * (CONSUMERS audit O16).
 */
static void cache_narrowed_func(abts_case *tc, void *data)
{
    ogs_sockaddr_t *ue = NULL;
    outcome_t outcome;
    bsf_pcf_binding_query_t query;

    ogs_assert(OGS_OK == ogs_getaddrinfo(&ue, AF_UNSPEC, "10.45.0.2", 0, 0));

    memset(&outcome, 0, sizeof(outcome));
    memset(&query, 0, sizeof(query));
    query.dnn = "internet";
    answer_cached(tc, request_new(tc, ue, &query, &outcome), 200, BINDING, "max-age=60");
    ABTS_INT_EQUAL(tc, 1, outcome.calls);
    ABTS_INT_EQUAL(tc, 200, outcome.http_status);
    ABTS_TRUE(tc, outcome.binding);
    ABTS_TRUE(tc, _bsf_client_pcf_bindings_from_cache(ue) == NULL);

    /* a query with an empty query structure is a plain one */
    memset(&outcome, 0, sizeof(outcome));
    memset(&query, 0, sizeof(query));
    answer_cached(tc, request_new(tc, ue, &query, &outcome), 200, BINDING, "max-age=60");
    ABTS_TRUE(tc, outcome.binding);
    ABTS_TRUE(tc, _bsf_client_pcf_bindings_from_cache(ue) != NULL);

    /* and it is then answered from the cache, at once */
    memset(&outcome, 0, sizeof(outcome));
    ABTS_TRUE(tc, bsf_retrieve_pcf_binding_with_result(ue, NULL, result_callback, &outcome));
    ABTS_INT_EQUAL(tc, 1, outcome.calls);
    ABTS_INT_EQUAL(tc, 200, outcome.http_status);
    ABTS_STR_EQUAL(tc, "internet", outcome.dnn);

    ogs_freeaddrinfo(ue);
}

/*
 * TS 29.521 V19.7.0 table 5.3.2.3.2-3: the answers to the query are 200 with the binding, 204 or 404 without, 400 with the cause
 * (MULTIPLE_BINDING_INFO_FOUND, MANDATORY_QUERY_PARAM_MISSING) and the 307 and 308 redirections (CONSUMERS audit O15): the requester is told which
 */
static void result_status_func(abts_case *tc, void *data)
{
    static const struct {
        int status;
        const char *content;
        const char *cause;
    } cases[] = {
        {204, NULL, ""},
        {404, "{\"title\":\"Not Found\",\"status\":404,\"cause\":\"DATA_NOT_FOUND\"}", "DATA_NOT_FOUND"},
        {400, "{\"title\":\"Bad Request\",\"status\":400,\"cause\":\"MULTIPLE_BINDING_INFO_FOUND\"}", "MULTIPLE_BINDING_INFO_FOUND"},
        {400, "{\"title\":\"Bad Request\",\"status\":400,\"cause\":\"MANDATORY_QUERY_PARAM_MISSING\"}", "MANDATORY_QUERY_PARAM_MISSING"},
        {307, NULL, ""},
        {308, NULL, ""},
        {500, "not a ProblemDetails", ""},
    };
    ogs_sockaddr_t *ue = NULL;
    int i;

    ogs_assert(OGS_OK == ogs_getaddrinfo(&ue, AF_UNSPEC, "10.45.0.3", 0, 0));
    for (i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
        outcome_t outcome;

        memset(&outcome, 0, sizeof(outcome));
        answer(tc, request_new(tc, ue, NULL, &outcome), cases[i].status, cases[i].content);
        ABTS_INT_EQUAL(tc, 1, outcome.calls);
        ABTS_INT_EQUAL(tc, cases[i].status, outcome.http_status);
        ABTS_TRUE(tc, !outcome.binding);
        ABTS_STR_EQUAL(tc, cases[i].cause, outcome.cause);
    }
    ogs_freeaddrinfo(ue);
}

/*
 * TS 29.521 V19.7.0 clause 4.2.4.2: the IPv6 address of the UE "shall be formatted as an IPv6 prefix value including the trailing prefix length /128",
 * and one and only one of ipv4Addr, ipv6Prefix and macAddr48 is present
 */
static void ipv6_request_func(abts_case *tc, void *data)
{
    ogs_sockaddr_t *ue = NULL;
    outcome_t outcome;
    ogs_sbi_xact_t *xact;
    bsf_client_sess_t *sess;
    ogs_sbi_request_t *request;

    ogs_assert(OGS_OK == ogs_getaddrinfo(&ue, AF_UNSPEC, "2001:db8:cafe::2", 0, 0));
    memset(&outcome, 0, sizeof(outcome));
    xact = request_new(tc, ue, NULL, &outcome);
    sess = ogs_container_of(xact->sbi_object, bsf_client_sess_t, sbi);
    request = _nbsf_management_pcf_binding_build(sess, NULL);
    ABTS_PTR_NOTNULL(tc, request);
    if (request) {
        const char *prefix = ogs_hash_get(request->http.params, OGS_SBI_PARAM_IPV6PREFIX, OGS_HASH_KEY_STRING);

        ABTS_STR_EQUAL(tc, "2001:db8:cafe::2/128", prefix ? prefix : "(none)");
        ABTS_TRUE(tc, ogs_hash_get(request->http.params, OGS_SBI_PARAM_IPV4ADDR, OGS_HASH_KEY_STRING) == NULL);
        ogs_sbi_request_free(request);
    }
    answer(tc, xact, 204, NULL);
    ogs_freeaddrinfo(ue);
}

/*
 * TS 29.500 V19.7.1 clause 5.2.2.2: the cache directives no-cache, no-store and max-age "shall be supported"; max-age is in seconds, minus the Age of
 * the answer; an answer without directives is not kept (CONSUMERS audit R7)
 */
static void expiry_func(abts_case *tc, void *data)
{
    static const struct {
        const char *cache_control, *age;
        int seconds;
    } cases[] = {
        {"max-age=60", NULL, 60},
        {"max-age=60", "20", 40},
        {"max-age=60", "90", 0},
        {"no-cache", NULL, 0},
        {"no-store, max-age=60", NULL, 0},
        {"public, max-age=10", NULL, 10},
        {NULL, NULL, 0},
    };
    int i;

    for (i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
        ogs_sbi_response_t *response = ogs_sbi_response_new();
        ogs_time_t before, expiry, after;

        if (cases[i].cache_control) ogs_sbi_header_set(response->http.headers, "Cache-Control", cases[i].cache_control);
        if (cases[i].age) ogs_sbi_header_set(response->http.headers, "Age", cases[i].age);
        before = ogs_time_now();
        expiry = _response_to_expiry_time(response);
        after = ogs_time_now();
        ABTS_TRUE(tc, expiry >= before + ogs_time_from_sec(cases[i].seconds));
        ABTS_TRUE(tc, expiry <= after + ogs_time_from_sec(cases[i].seconds));
        ogs_sbi_response_free(response);
    }
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
