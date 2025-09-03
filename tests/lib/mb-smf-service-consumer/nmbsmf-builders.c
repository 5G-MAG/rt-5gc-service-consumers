/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk> 
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <stdbool.h>
#include <netinet/in.h>
#include <endian.h>

#include "ogs-core.h"
#include "openapi/model/mbs_session_id.h"
#include "openapi/model/create_req_data.h"
#include "openapi/model/status_subscribe_req_data.h"

#include "priv_mbs-session.h"
#include "priv_mbs-status-subscription.h"
#include "nmbsmf-mbs-session-build.h"
#include "nmbsmf-tmgi-build.h"

#include "unit-test.h"

/* fake context.c functions */
const ogs_sockaddr_t *_context_get_notification_address()
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    static const in_addr_t lo = __bswap_constant_32(INADDR_LOOPBACK);
#else
    static const in_addr_t lo = INADDR_LOOPBACK;
#endif
    static const struct ogs_sockaddr_s loopback = {.sin = {.sin_family = AF_INET, .sin_addr = {lo}, .sin_port = 0}};
    return &loopback;
}

/* fake tmgi.c functions */

void _tmgi_free(_priv_tmgi_t *tmgi)
{
    if (tmgi) {
        if (tmgi->tmgi.mbs_service_id) ogs_free(tmgi->tmgi.mbs_service_id);
        if (tmgi->cache) {
            if (tmgi->cache->repr) ogs_free(tmgi->cache->repr);
            ogs_free(tmgi->cache);
        }
        ogs_free(tmgi);
    }
}

OpenAPI_tmgi_t *_tmgi_to_openapi_type(const _priv_tmgi_t *tmgi)
{
    if (!tmgi) return NULL;

    char *mbs_service_id = NULL;
    if (tmgi->tmgi.mbs_service_id) mbs_service_id = ogs_strdup(tmgi->tmgi.mbs_service_id);

    OpenAPI_plmn_id_t *plmn = OpenAPI_plmn_id_create(ogs_plmn_id_mcc_string(&tmgi->tmgi.plmn),
                                                     ogs_plmn_id_mnc_string(&tmgi->tmgi.plmn));

    return OpenAPI_tmgi_create(mbs_service_id, plmn);
}

/* fake mbs-session.c functions */
static OpenAPI_ip_addr_t *__new_OpenAPI_ip_addr_from_inaddr(const struct in_addr *addr);
static OpenAPI_ip_addr_t *__new_OpenAPI_ip_addr_from_in6addr(const struct in6_addr *addr);

void _mbs_status_subscription_free(_priv_mbs_status_subscription_t *subsc)
{
    if (subsc) {
        if (subsc->id) ogs_free(subsc->id);
        if (subsc->cache) {
            if (subsc->cache->repr_string) ogs_free(subsc->cache->repr_string);
            if (subsc->cache->notif_url) ogs_free(subsc->cache->notif_url);
            ogs_free(subsc->cache);
        }
        if (subsc->correlation_id) ogs_free(subsc->correlation_id);
        ogs_free(subsc);
    }
}

void _mbs_session_free(_priv_mbs_session_t *session)
{
    if (session) {
        _priv_mbs_status_subscription_t *next, *node;
        ogs_list_for_each_safe(&session->new_subscriptions, next, node) {
            ogs_list_remove(&session->new_subscriptions, node);
            _mbs_status_subscription_free(node);
        }
        if (session->id) ogs_free(session->id);
        if (session->session.ssm) ogs_free(session->session.ssm);
        if (session->session.tmgi) ogs_free(session->session.tmgi);
        ogs_free(session);
    }
}

OpenAPI_mbs_session_id_t *_mbs_session_create_mbs_session_id(_priv_mbs_session_t *session)
{
    OpenAPI_mbs_session_id_t *mbs_session_id = NULL;
    if (session->session.ssm || session->session.tmgi) {
        mbs_session_id = OpenAPI_mbs_session_id_create(NULL /*tmgi*/, NULL /*ssm*/, NULL /*nid*/);
    }
    if (session->session.ssm) {
        OpenAPI_ip_addr_t *src = NULL, *dest = NULL;
        if (session->session.ssm->family == AF_INET) {
            src = __new_OpenAPI_ip_addr_from_inaddr(&session->session.ssm->source.ipv4);
            dest = __new_OpenAPI_ip_addr_from_inaddr(&session->session.ssm->dest_mc.ipv4);
        } else {
            src = __new_OpenAPI_ip_addr_from_in6addr(&session->session.ssm->source.ipv6);
            dest = __new_OpenAPI_ip_addr_from_in6addr(&session->session.ssm->dest_mc.ipv6);
        }
        mbs_session_id->ssm = OpenAPI_ssm_create(src, dest);
    }
    if (session->session.tmgi) {
        OpenAPI_plmn_id_t *plmn_id = OpenAPI_plmn_id_create(ogs_plmn_id_mcc_string(&session->session.tmgi->plmn),
                                                            ogs_plmn_id_mnc_string(&session->session.tmgi->plmn));
        mbs_session_id->tmgi = OpenAPI_tmgi_create(ogs_strdup(session->session.tmgi->mbs_service_id), plmn_id);
    }
    return mbs_session_id;
}

static OpenAPI_ip_addr_t *__new_OpenAPI_ip_addr_from_inaddr(const struct in_addr *addr)
{
    OpenAPI_ip_addr_t *ret = NULL;
    char addr_str[INET_ADDRSTRLEN];

    if (inet_ntop(AF_INET, addr, addr_str, sizeof(addr_str))) {
        ret = OpenAPI_ip_addr_create(ogs_strdup(addr_str), NULL, NULL);
    }

    return ret;
}

static OpenAPI_ip_addr_t *__new_OpenAPI_ip_addr_from_in6addr(const struct in6_addr *addr)
{
    OpenAPI_ip_addr_t *ret = NULL;
    char addr_str[INET6_ADDRSTRLEN];

    if (inet_ntop(AF_INET6, addr, addr_str, sizeof(addr_str))) {
        ret = OpenAPI_ip_addr_create(NULL, ogs_strdup(addr_str), NULL);
    }

    return ret;
}

/**** Tests ****/

static bool test_create_tmgi(unit_test_ctx *ctx)
{
    _priv_tmgi_t *tmgi = (_priv_tmgi_t*)ogs_calloc(1, sizeof(*tmgi));
    ogs_list_t new_list = {};

    tmgi->cache = ogs_calloc(1, sizeof(*tmgi->cache));

    ogs_list_add(&new_list, tmgi);
    ogs_sbi_request_t *req = _nmbsmf_tmgi_build_create(&new_list, NULL);

    UT_STR_EQUAL(req->h.method, OGS_SBI_HTTP_METHOD_POST);
    if (!req->h.uri) {
        UT_STR_EQUAL(req->h.service.name, OGS_SBI_SERVICE_NAME_NMBSMF_TMGI);
        UT_STR_EQUAL(req->h.api.version, OGS_SBI_API_V1);
        UT_STR_EQUAL(req->h.resource.component[0], OGS_SBI_RESOURCE_NAME_TMGI);
        UT_STR_NULL(req->h.resource.component[1]);
    } else {
        UT_STR_MATCHES(req->h.uri, "^https?://(?:[a-zA-Z0-9.]*|\\[[0-9A-Fa-f:]*\\])(?::[1-9][0-9]*)?/" OGS_SBI_SERVICE_NAME_NMBSMF_TMGI "/" OGS_SBI_API_V1 "/" OGS_SBI_RESOURCE_NAME_TMGI "$");
    }
    cJSON *json = cJSON_Parse(req->http.content);
    if (!json) {
        fprintf(stderr, "expected req->http.content to be valid JSON, could not parse string as JSON: %s\n", req->http.content);
        return false;
    }
    OpenAPI_tmgi_allocate_t *tmgi_alloc = OpenAPI_tmgi_allocate_parseFromJSON(json);
    cJSON_Delete(json);
    if (!tmgi_alloc) {
        fprintf(stderr, "expected req->http.content to be valid 3GPP TmgiAllocate JSON, could not parse JSON as a TmgiAllocate object: %s\n", req->http.content);
        return false;
    }

    UT_INT_EQUAL(tmgi_alloc->tmgi_number, 1);
    UT_PTR_NULL(tmgi_alloc->tmgi_list);

    OpenAPI_tmgi_allocate_free(tmgi_alloc);

    _tmgi_free(tmgi);

    ogs_sbi_request_free(req);

    return true;
}

static bool test_refresh_tmgi(unit_test_ctx *ctx)
{
    _priv_tmgi_t *tmgi = (_priv_tmgi_t*)ogs_calloc(1, sizeof(*tmgi));
    ogs_list_t refresh_list = {};
    OpenAPI_tmgi_allocate_t *tmgi_alloc = NULL;
    cJSON *json = NULL;
    ogs_sbi_request_t *req = NULL;
    bool result = false;
    time_t now = time(NULL);

#define TEST_TMGI_MBS_SERVICE_ID "123456"
#define TEST_TMGI_PLMN_MCC 999
#define TEST_TMGI_PLMN_MNC 01
#define _STR(A) #A
#define STR(A) _STR(A)

    tmgi->cache = ogs_calloc(1, sizeof(*tmgi->cache));
    tmgi->tmgi.mbs_service_id = ogs_strdup(TEST_TMGI_MBS_SERVICE_ID);
    ogs_plmn_id_build(&tmgi->tmgi.plmn, TEST_TMGI_PLMN_MCC, TEST_TMGI_PLMN_MNC, TEST_TMGI_PLMN_MNC<100?2:3);
    tmgi->tmgi.expiry_time = now;

    ogs_list_add(&refresh_list, tmgi);
    req = _nmbsmf_tmgi_build_create(NULL, &refresh_list);

    UT_STR_EQUAL_GOTO(req->h.method, OGS_SBI_HTTP_METHOD_POST, end_test_refresh_tmgi);
    if (!req->h.uri) {
        UT_STR_EQUAL_GOTO(req->h.service.name, OGS_SBI_SERVICE_NAME_NMBSMF_TMGI, end_test_refresh_tmgi);
        UT_STR_EQUAL_GOTO(req->h.api.version, OGS_SBI_API_V1, end_test_refresh_tmgi);
        UT_STR_EQUAL_GOTO(req->h.resource.component[0], OGS_SBI_RESOURCE_NAME_TMGI, end_test_refresh_tmgi);
        UT_STR_NULL_GOTO(req->h.resource.component[1], end_test_refresh_tmgi);
    } else {
        UT_STR_MATCHES_GOTO(req->h.uri, "^https?://(?:[a-zA-Z0-9.]*|\\[[0-9A-Fa-f:]*\\])(?::[1-9][0-9]*)?/" OGS_SBI_SERVICE_NAME_NMBSMF_TMGI "/" OGS_SBI_API_V1 "/" OGS_SBI_RESOURCE_NAME_TMGI "$", end_test_refresh_tmgi);
    }
    json = cJSON_Parse(req->http.content);
    if (!json) {
        fprintf(stderr, "expected req->http.content to be valid JSON, could not parse string as JSON: %s\n", req->http.content);
        goto end_test_refresh_tmgi;
    }
    tmgi_alloc = OpenAPI_tmgi_allocate_parseFromJSON(json);
    if (!tmgi_alloc) {
        fprintf(stderr, "expected req->http.content to be valid 3GPP TmgiAllocate JSON, could not parse JSON as a TmgiAllocate object: %s\n", req->http.content);
        goto end_test_refresh_tmgi;
    }

    UT_INT_EQUAL_GOTO(tmgi_alloc->tmgi_number, 0, end_test_refresh_tmgi);
    UT_PTR_NOT_NULL_GOTO(tmgi_alloc->tmgi_list, end_test_refresh_tmgi);
    UT_JSON_LIST_SIZE_GOTO(tmgi_alloc->tmgi_list, 1, end_test_refresh_tmgi);
    OpenAPI_lnode_t *node;
    OpenAPI_list_for_each(tmgi_alloc->tmgi_list, node) {
        OpenAPI_tmgi_t *api_tmgi = (OpenAPI_tmgi_t*)node->data;
        UT_PTR_NOT_NULL_GOTO(api_tmgi, end_test_refresh_tmgi);
        UT_STR_EQUAL_GOTO(api_tmgi->mbs_service_id, TEST_TMGI_MBS_SERVICE_ID, end_test_refresh_tmgi);
        UT_PTR_NOT_NULL_GOTO(api_tmgi->plmn_id, end_test_refresh_tmgi);
        UT_STR_EQUAL_GOTO(api_tmgi->plmn_id->mcc, STR(TEST_TMGI_PLMN_MCC), end_test_refresh_tmgi);
        UT_STR_EQUAL_GOTO(api_tmgi->plmn_id->mnc, STR(TEST_TMGI_PLMN_MNC), end_test_refresh_tmgi);
    }

    result = true;

end_test_refresh_tmgi:
    if (json) cJSON_Delete(json);
    if (tmgi_alloc) OpenAPI_tmgi_allocate_free(tmgi_alloc);
    _tmgi_free(tmgi);
    if (req) ogs_sbi_request_free(req);

    return result;
}

static bool test_delete_tmgi(unit_test_ctx *ctx)
{
    _priv_tmgi_t *tmgi = (_priv_tmgi_t*)ogs_calloc(1, sizeof(*tmgi));
    ogs_list_t delete_list = {};
    OpenAPI_tmgi_allocate_t *tmgi_alloc = NULL;
    cJSON *json = NULL;
    ogs_sbi_request_t *req = NULL;
    bool result = false;
    time_t now = time(NULL);
    OpenAPI_tmgi_t *api_tmgi = NULL;

    tmgi->cache = ogs_calloc(1, sizeof(*tmgi->cache));
    tmgi->tmgi.mbs_service_id = ogs_strdup(TEST_TMGI_MBS_SERVICE_ID);
    ogs_plmn_id_build(&tmgi->tmgi.plmn, TEST_TMGI_PLMN_MCC, TEST_TMGI_PLMN_MNC, TEST_TMGI_PLMN_MNC<100?2:3);
    tmgi->tmgi.expiry_time = now;

    ogs_list_add(&delete_list, tmgi);
    req = _nmbsmf_tmgi_build_remove(&delete_list, NULL);

    UT_STR_EQUAL_GOTO(req->h.method, OGS_SBI_HTTP_METHOD_DELETE, end_test_delete_tmgi);
    if (!req->h.uri) {
        UT_STR_EQUAL_GOTO(req->h.service.name, OGS_SBI_SERVICE_NAME_NMBSMF_TMGI, end_test_delete_tmgi);
        UT_STR_EQUAL_GOTO(req->h.api.version, OGS_SBI_API_V1, end_test_delete_tmgi);
        UT_STR_EQUAL_GOTO(req->h.resource.component[0], OGS_SBI_RESOURCE_NAME_TMGI, end_test_delete_tmgi);
        UT_STR_NULL_GOTO(req->h.resource.component[1], end_test_delete_tmgi);
    } else {
        UT_STR_MATCHES_GOTO(req->h.uri, "^https?://(?:[a-zA-Z0-9.]*|\\[[0-9A-Fa-f:]*\\])(?::[1-9][0-9]*)?/" OGS_SBI_SERVICE_NAME_NMBSMF_TMGI "/" OGS_SBI_API_V1 "/" OGS_SBI_RESOURCE_NAME_TMGI "?tmgi-list=.*$", end_test_delete_tmgi);
    }

    /* find TMGI list in the query parameters */
    UT_PTR_NOT_NULL_GOTO(req->http.params, end_test_delete_tmgi);
    const char *tmgi_list_param = ogs_hash_get(req->http.params, OGS_SBI_PARAM_TMGI_LIST, OGS_HASH_KEY_STRING);
    UT_STR_NOT_NULL_GOTO(tmgi_list_param, end_test_delete_tmgi);
    json = cJSON_Parse(tmgi_list_param);
    UT_PTR_NOT_NULL_GOTO(json, end_test_delete_tmgi);
    if (!cJSON_IsArray(json)) {
        fprintf(stderr, "expected tmgi-list query parameter to be an array\n");
        goto end_test_delete_tmgi;
    }
    if (cJSON_GetArraySize(json) != 1) {
        fprintf(stderr, "expected tmgi-list query parameter to contain one TMGI\n");
        goto end_test_delete_tmgi;
    }
    api_tmgi = OpenAPI_tmgi_parseFromJSON(cJSON_GetArrayItem(json, 0));
    UT_PTR_NOT_NULL_GOTO(api_tmgi, end_test_delete_tmgi);
    UT_STR_EQUAL_GOTO(api_tmgi->mbs_service_id, TEST_TMGI_MBS_SERVICE_ID, end_test_delete_tmgi);
    UT_PTR_NOT_NULL_GOTO(api_tmgi->plmn_id, end_test_delete_tmgi);
    UT_STR_EQUAL_GOTO(api_tmgi->plmn_id->mcc, STR(TEST_TMGI_PLMN_MCC), end_test_delete_tmgi);
    UT_STR_EQUAL_GOTO(api_tmgi->plmn_id->mnc, STR(TEST_TMGI_PLMN_MNC), end_test_delete_tmgi);

    result = true;

end_test_delete_tmgi:
    if (api_tmgi) OpenAPI_tmgi_free(api_tmgi);
    if (json) cJSON_Delete(json);
    if (tmgi_alloc) OpenAPI_tmgi_allocate_free(tmgi_alloc);
    _tmgi_free(tmgi);
    if (req) ogs_sbi_request_free(req);

    return result;
}

static bool test_create_sess(unit_test_ctx *ctx)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)ogs_calloc(1, sizeof(*session));
    session->changed = true;

    ogs_sbi_request_t *req = _nmbsmf_mbs_session_build_create((void*)session, NULL);

    UT_STR_EQUAL(req->h.method, OGS_SBI_HTTP_METHOD_POST);
    if (!req->h.uri) {
        UT_STR_EQUAL(req->h.service.name, OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION);
        UT_STR_EQUAL(req->h.api.version, OGS_SBI_API_V1);
        UT_STR_EQUAL(req->h.resource.component[0], OGS_SBI_RESOURCE_NAME_MBS_SESSIONS);
        UT_STR_NULL(req->h.resource.component[1]);
    } else {
        UT_STR_MATCHES(req->h.uri, "^https?://(?:[a-zA-Z0-9.]*|\\[[0-9A-Fa-f:]*\\])(?::[1-9][0-9]*)?/" OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION "/" OGS_SBI_API_V1 "/" OGS_SBI_RESOURCE_NAME_MBS_SESSIONS "$");
    }
    cJSON *json = cJSON_Parse(req->http.content);
    if (!json) {
        fprintf(stderr, "expected req->http.content to be valid JSON, could not parse string as JSON");
        return false;
    }
    OpenAPI_create_req_data_t *create_req_data = OpenAPI_create_req_data_parseFromJSON(json);
    cJSON_Delete(json);
    if (!create_req_data) {
        fprintf(stderr, "expected req->http.content to be valid 3GPP CreateReqData JSON, could not parse JSON as a CreateReqData object");
        return false;
    }
    UT_PTR_NOT_NULL(create_req_data->mbs_session);
    UT_BOOL_TRUE(create_req_data->mbs_session->is_any_ue_ind);
    UT_INT_NOT_EQUAL(create_req_data->mbs_session->any_ue_ind, 0);
    UT_ENUM_EQUAL(create_req_data->mbs_session->service_type, OpenAPI_mbs_service_type_BROADCAST);
    UT_ENUM_EQUAL(create_req_data->mbs_session->activity_status, OpenAPI_mbs_session_activity_status_ACTIVE);

    UT_PTR_NULL(create_req_data->mbs_session->mbs_session_subsc);

    OpenAPI_create_req_data_free(create_req_data);

    _mbs_session_free(session);

    ogs_sbi_request_free(req);

    return true;
}

static bool test_create_sess_with_subsc(unit_test_ctx *ctx)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)ogs_calloc(1, sizeof(*session));
    _priv_mbs_status_subscription_t *subsc = (_priv_mbs_status_subscription_t*)ogs_calloc(1, sizeof(*subsc));
    subsc->cache = ogs_calloc(1, sizeof(*subsc->cache));
    subsc->session = session;
    subsc->flags = -1;
    subsc->correlation_id = ogs_strdup("test-correlation-id");
    ogs_list_add(&session->new_subscriptions, subsc);
    subsc->changed = true;
    session->changed = true;

    ogs_sbi_request_t *req = _nmbsmf_mbs_session_build_create((void*)session, NULL);

    UT_STR_EQUAL(req->h.method, OGS_SBI_HTTP_METHOD_POST);
    if (!req->h.uri) {
        UT_STR_EQUAL(req->h.service.name, OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION);
        UT_STR_EQUAL(req->h.api.version, OGS_SBI_API_V1);
        UT_STR_EQUAL(req->h.resource.component[0], OGS_SBI_RESOURCE_NAME_MBS_SESSIONS);
        UT_STR_NULL(req->h.resource.component[1]);
    } else {
        UT_STR_MATCHES(req->h.uri, "^https?://(?:[a-zA-Z0-9.]*|\\[[0-9A-Fa-f:]*\\])(?::[1-9][0-9]*)?/" OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION "/" OGS_SBI_API_V1 "/" OGS_SBI_RESOURCE_NAME_MBS_SESSIONS "$");
    }
    cJSON *json = cJSON_Parse(req->http.content);
    if (!json) {
        fprintf(stderr, "expected req->http.content to be valid JSON, could not parse string as JSON");
        return false;
    }
    OpenAPI_create_req_data_t *create_req_data = OpenAPI_create_req_data_parseFromJSON(json);
    cJSON_Delete(json);
    if (!create_req_data) {
        fprintf(stderr, "expected req->http.content to be valid 3GPP CreateReqData JSON, could not parse JSON as a CreateReqData object");
        return false;
    }
    UT_PTR_NOT_NULL(create_req_data->mbs_session);
    UT_BOOL_TRUE(create_req_data->mbs_session->is_any_ue_ind);
    UT_INT_NOT_EQUAL(create_req_data->mbs_session->any_ue_ind, 0);
    UT_ENUM_EQUAL(create_req_data->mbs_session->service_type, OpenAPI_mbs_service_type_BROADCAST);
    UT_ENUM_EQUAL(create_req_data->mbs_session->activity_status, OpenAPI_mbs_session_activity_status_ACTIVE);
    
    UT_PTR_NOT_NULL(create_req_data->mbs_session->mbs_session_subsc);
    UT_STR_EQUAL(create_req_data->mbs_session->mbs_session_subsc->notify_correlation_id, "test-correlation-id");
    UT_STR_BEGINS_WITH(create_req_data->mbs_session->mbs_session_subsc->notify_uri, "http://");
    UT_STR_ENDS_WITH(create_req_data->mbs_session->mbs_session_subsc->notify_uri, "/mbs-session-notify/v1/notification");
    UT_JSON_LIST_SIZE(create_req_data->mbs_session->mbs_session_subsc->event_list, 3);

    OpenAPI_create_req_data_free(create_req_data);
    _mbs_session_free(session);
    ogs_sbi_request_free(req);

    return true;
}

#define FAKE_SESSION_ID "12345678-9abc-4def-8123-456789abcdef"

static bool test_update_sess(unit_test_ctx *ctx)
{
    // TODO: Implement once update builder implemented in the library
    return true;
}

static bool test_delete_sess(unit_test_ctx *ctx)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)ogs_calloc(1, sizeof(*session));
    session->id = ogs_strdup(FAKE_SESSION_ID);
    session->changed = true;

    ogs_sbi_request_t *req = _nmbsmf_mbs_session_build_remove((void*)session, NULL);

    UT_STR_EQUAL(req->h.method, OGS_SBI_HTTP_METHOD_DELETE);
    if (!req->h.uri) {
        UT_STR_EQUAL(req->h.service.name, OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION);
        UT_STR_EQUAL(req->h.api.version, OGS_SBI_API_V1);
        UT_STR_EQUAL(req->h.resource.component[0], OGS_SBI_RESOURCE_NAME_MBS_SESSIONS);
        UT_STR_EQUAL(req->h.resource.component[1], FAKE_SESSION_ID);
        UT_STR_NULL(req->h.resource.component[2]);
    } else {
        UT_STR_MATCHES(req->h.uri, "^https?://(?:[a-zA-Z0-9.]*|\\[[0-9A-Fa-f:]*\\])(?::[1-9][0-9]*)?/" OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION "/" OGS_SBI_API_V1 "/" OGS_SBI_RESOURCE_NAME_MBS_SESSIONS "/" FAKE_SESSION_ID "$");
    }
    UT_PTR_NULL(req->http.content);
    UT_SIZE_T_EQUAL(req->http.content_length, 0);

    _mbs_session_free(session);
    ogs_sbi_request_free(req);

    return true;
}

static bool test_create_status_subsc(unit_test_ctx *ctx)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)ogs_calloc(1, sizeof(*session));
    session->id = ogs_strdup(FAKE_SESSION_ID);
    session->session.ssm = (mb_smf_sc_ssm_addr_t*)ogs_calloc(1, sizeof(*session->session.ssm));
    session->session.ssm->family = AF_INET;
    session->session.ssm->source.ipv4.s_addr = htonl(0xc0a80001); /* 192.168.0.1 */
    session->session.ssm->dest_mc.ipv4.s_addr = htonl(0xe8000001); /* 232.0.0.1 */

    _priv_mbs_status_subscription_t *subsc = (_priv_mbs_status_subscription_t*)ogs_calloc(1, sizeof(*subsc));
    subsc->cache = ogs_calloc(1, sizeof(*subsc->cache));
    subsc->cache->notif_url = ogs_strdup("http://example.com/my-notifications");
    subsc->session = session;
    subsc->flags = -1;
    subsc->correlation_id = ogs_strdup("test-correlation-id");
    ogs_list_add(&session->new_subscriptions, subsc);

    subsc->changed = true;
    session->changed = true;

    char *nf_instance_id = NF_INSTANCE_ID(ogs_sbi_self()->nf_instance);

    ogs_sbi_request_t *req = _nmbsmf_mbs_session_build_status_subscription_create((void*)session, (void*)subsc);

    UT_STR_EQUAL(req->h.method, OGS_SBI_HTTP_METHOD_POST);
    if (!req->h.uri) {
        UT_STR_EQUAL(req->h.service.name, OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION);
        UT_STR_EQUAL(req->h.api.version, OGS_SBI_API_V1);
        UT_STR_EQUAL(req->h.resource.component[0], OGS_SBI_RESOURCE_NAME_MBS_SESSIONS);
        UT_STR_EQUAL(req->h.resource.component[1], OGS_SBI_RESOURCE_NAME_SUBSCRIPTIONS);
        UT_STR_NULL(req->h.resource.component[2]);
    } else {
        UT_STR_MATCHES(req->h.uri, "^https?://(?:[a-zA-Z0-9.]*|\\[[0-9A-Fa-f:]*\\])(?::[1-9][0-9]*)?/" OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION "/" OGS_SBI_API_V1 "/" OGS_SBI_RESOURCE_NAME_MBS_SESSIONS "/" OGS_SBI_RESOURCE_NAME_SUBSCRIPTIONS "$");
    }
    
    cJSON *json = cJSON_Parse(req->http.content);
    if (!json) {
        fprintf(stderr, "expected req->http.content to be valid JSON, could not parse string as JSON");
        return false;
    }
    OpenAPI_status_subscribe_req_data_t *status_subsc_req_data = OpenAPI_status_subscribe_req_data_parseFromJSON(json);
    cJSON_Delete(json);
    if (!status_subsc_req_data) {
        fprintf(stderr, "expected req->http.content to be valid 3GPP StatusSubscribeReqData JSON, could not parse JSON as a StatusSubscribeReqData object");
        return false;
    }
    /* StatusSubscribeReqData.subscription */
    UT_PTR_NOT_NULL(status_subsc_req_data->subscription);

    /* StatusSubscribeReqData.subscription.mbsSessionId */
    UT_PTR_NOT_NULL(status_subsc_req_data->subscription->mbs_session_id);
    UT_PTR_NULL(status_subsc_req_data->subscription->mbs_session_id->tmgi);
    UT_PTR_NOT_NULL(status_subsc_req_data->subscription->mbs_session_id->ssm);
    UT_PTR_NOT_NULL(status_subsc_req_data->subscription->mbs_session_id->ssm->source_ip_addr);
    UT_STR_EQUAL(status_subsc_req_data->subscription->mbs_session_id->ssm->source_ip_addr->ipv4_addr, "192.168.0.1");
    UT_STR_NULL(status_subsc_req_data->subscription->mbs_session_id->ssm->source_ip_addr->ipv6_addr);
    UT_STR_NULL(status_subsc_req_data->subscription->mbs_session_id->ssm->source_ip_addr->ipv6_prefix);
    UT_PTR_NOT_NULL(status_subsc_req_data->subscription->mbs_session_id->ssm->dest_ip_addr);
    UT_STR_EQUAL(status_subsc_req_data->subscription->mbs_session_id->ssm->dest_ip_addr->ipv4_addr, "232.0.0.1");
    UT_STR_NULL(status_subsc_req_data->subscription->mbs_session_id->ssm->dest_ip_addr->ipv6_addr);
    UT_STR_NULL(status_subsc_req_data->subscription->mbs_session_id->ssm->dest_ip_addr->ipv6_prefix);
    UT_STR_NULL(status_subsc_req_data->subscription->mbs_session_id->nid);

    /* StatusSubscribeReqData.subscription.areaSessionId */
    UT_BOOL_FALSE(status_subsc_req_data->subscription->is_area_session_id);

    /* StatusSubscribeReqData.subscription.eventList */
    UT_JSON_LIST_SIZE(status_subsc_req_data->subscription->event_list, 3);

    /* StatusSubscribeReqData.subscription.notifyUri */
    UT_STR_EQUAL(status_subsc_req_data->subscription->notify_uri, "http://example.com/my-notifications");

    /* StatusSubscribeReqData.subscription.notifyCorrelationId */
    UT_STR_EQUAL(status_subsc_req_data->subscription->notify_correlation_id, "test-correlation-id");

    /* StatusSubscribeReqData.subscription.expiryTime */
    UT_STR_NULL(status_subsc_req_data->subscription->expiry_time);

    /* StatusSubscribeReqData.subscription.nfcInstanceId */
    UT_STR_EQUAL(status_subsc_req_data->subscription->nfc_instance_id, nf_instance_id);

    /* StatusSubscribeReqData.subscription.mbsSessionSubscUri */
    UT_STR_NULL(status_subsc_req_data->subscription->mbs_session_subsc_uri);

    OpenAPI_status_subscribe_req_data_free(status_subsc_req_data);
    _mbs_session_free(session);
    ogs_sbi_request_free(req);

    return true;
}

#define FAKE_SUBSCRIPTION_ID "4b9c4f0f-ed28-4a59-b3bc-9acbefcffc4f"

static bool test_update_status_subsc(unit_test_ctx *ctx)
{
    fprintf(stderr, "TODO: implement update builder test for MBS Session Status Subscriptions");
    return true;
}

static bool test_delete_status_subsc(unit_test_ctx *ctx)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)ogs_calloc(1, sizeof(*session));
    session->id = ogs_strdup(FAKE_SESSION_ID);
    session->session.ssm = (mb_smf_sc_ssm_addr_t*)ogs_calloc(1, sizeof(*session->session.ssm));
    session->session.ssm->family = AF_INET;
    session->session.ssm->source.ipv4.s_addr = htonl(0xc0a80001); /* 192.168.0.1 */
    session->session.ssm->dest_mc.ipv4.s_addr = htonl(0xe8000001); /* 232.0.0.1 */
  
    _priv_mbs_status_subscription_t *subsc = (_priv_mbs_status_subscription_t*)ogs_calloc(1, sizeof(*subsc));
    subsc->id = ogs_strdup(FAKE_SUBSCRIPTION_ID);
    subsc->cache = ogs_calloc(1, sizeof(*subsc->cache));
    subsc->cache->notif_url = ogs_strdup("http://example.com/my-notifications");
    subsc->session = session;
    subsc->flags = -1;
    subsc->correlation_id = ogs_strdup("test-correlation-id");
    ogs_list_add(&session->new_subscriptions, subsc);

    ogs_sbi_request_t *req = _nmbsmf_mbs_session_build_status_subscription_delete((void*)session, (void*)subsc);

    UT_STR_EQUAL(req->h.method, OGS_SBI_HTTP_METHOD_DELETE);
    if (!req->h.uri) {
        UT_STR_EQUAL(req->h.service.name, OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION);
        UT_STR_EQUAL(req->h.api.version, OGS_SBI_API_V1);
        UT_STR_EQUAL(req->h.resource.component[0], OGS_SBI_RESOURCE_NAME_MBS_SESSIONS);
        UT_STR_EQUAL(req->h.resource.component[1], OGS_SBI_RESOURCE_NAME_SUBSCRIPTIONS);
        UT_STR_EQUAL(req->h.resource.component[2], FAKE_SUBSCRIPTION_ID);
        UT_STR_NULL(req->h.resource.component[3]);
    } else {
        UT_STR_MATCHES(req->h.uri, "^https?://(?:[a-zA-Z0-9.]*|\\[[0-9A-Fa-f:]*\\])(?::[1-9][0-9]*)?/" OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION "/" OGS_SBI_API_V1 "/" OGS_SBI_RESOURCE_NAME_MBS_SESSIONS "/" OGS_SBI_RESOURCE_NAME_SUBSCRIPTIONS "/" FAKE_SUBSCRIPTION_ID "$");
    }
    UT_STR_NULL(req->http.content);
    UT_SIZE_T_EQUAL(req->http.content_length, 0);

    _mbs_session_free(session);
    ogs_sbi_request_free(req);

    return true;
}


/** Test descriptors **/

static const unit_test_t test_create_tmgi_desc = {
    .name = "nmbsmf-tmgi: Allocate TMGI (allocate new)",
    .fn = test_create_tmgi
};

static const unit_test_t test_refresh_tmgi_desc = {
    .name = "nmbsmf-tmgi: Allocate TMGI (refresh)",
    .fn = test_refresh_tmgi
};

static const unit_test_t test_delete_tmgi_desc = {
    .name = "nmbsmf-tmgi: Deallocate TMGI",
    .fn = test_delete_tmgi
};

static const unit_test_t test_create_sess_with_subsc_desc = {
    .name = "nmbsmf-mbssession: create session with subscription builder",
    .fn = test_create_sess_with_subsc
};

static const unit_test_t test_create_sess_desc = {
    .name = "nmbsmf-mbssession: create session without subscription builder",
    .fn = test_create_sess
};

static const unit_test_t test_update_sess_desc = {
    .name = "nmbsmf-mbssession: update session",
    .fn = test_update_sess
};

static const unit_test_t test_delete_sess_desc = {
    .name = "nmbsmf-mbssession: delete session",
    .fn = test_delete_sess
};

static const unit_test_t test_create_status_subsc_desc = {
    .name = "nmbsmf-mbssession: create status subscription",
    .fn = test_create_status_subsc
};

static const unit_test_t test_update_status_subsc_desc = {
    .name = "nmbsmf-mbssession: update status subscription",
    .fn = test_update_status_subsc
};

static const unit_test_t test_delete_status_subsc_desc = {
    .name = "nmbsmf-mbssession: delete status subscription",
    .fn = test_delete_status_subsc
};

__attribute__ ((constructor))
static void _init_fn()
{
    register_unit_test(&test_create_tmgi_desc);
    register_unit_test(&test_refresh_tmgi_desc);
    register_unit_test(&test_delete_tmgi_desc);

    register_unit_test(&test_create_sess_desc);
    register_unit_test(&test_create_sess_with_subsc_desc);
    register_unit_test(&test_update_sess_desc);
    register_unit_test(&test_delete_sess_desc);
    register_unit_test(&test_create_status_subsc_desc);
    register_unit_test(&test_update_status_subsc_desc);
    register_unit_test(&test_delete_status_subsc_desc);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
