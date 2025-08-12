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

#include "priv_mbs-session.h"
#include "priv_mbs-status-subscription.h"
#include "nmbsmf-mbs-session-build.h"

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

/* fake mbs-session.c functions */
static OpenAPI_ip_addr_t *__new_OpenAPI_ip_addr_from_inaddr(const struct in_addr *addr);
static OpenAPI_ip_addr_t *__new_OpenAPI_ip_addr_from_in6addr(const struct in6_addr *addr);

void _mbs_status_subscription_free(_priv_mbs_status_subscription_t *subsc)
{
    if (subsc) {
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
        UT_STR_IS_NULL(req->h.resource.component[1]);
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
    UT_BOOL_IS_TRUE(create_req_data->mbs_session->is_any_ue_ind);
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
        UT_STR_IS_NULL(req->h.resource.component[1]);
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
    UT_BOOL_IS_TRUE(create_req_data->mbs_session->is_any_ue_ind);
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

static const unit_test_t test_create_sess_with_subsc_desc = {
    .name = "nmbsmf-mbssession: create session with subscription builder",
    .fn = test_create_sess_with_subsc
};

static const unit_test_t test_create_sess_desc = {
    .name = "nmbsmf-mbssession: create session without subscription builder",
    .fn = test_create_sess
};

__attribute__ ((constructor))
static void _init_fn()
{
    register_unit_test(&test_create_sess_desc);
    register_unit_test(&test_create_sess_with_subsc_desc);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
