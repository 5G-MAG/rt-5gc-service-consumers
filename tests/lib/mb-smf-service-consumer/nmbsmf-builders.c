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

#include "priv_associated-session-id.h"
#include "priv_civic-address.h"
#include "priv_flow-description.h"
#include "priv_ncgi-tai.h"
#include "priv_ncgi.h"
#include "priv_tai.h"
#include "priv_mbs-fsa-id.h"
#include "priv_mbs-session.h"
#include "priv_mbs-status-subscription.h"
#include "priv_ssm-addr.h"
#include "priv_ext-mbs-service-area.h"
#include "priv_mbs-service-area.h"
#include "priv_mbs-service-info.h"
#include "priv_mbs-media-comp.h"
#include "priv_mbs-media-info.h"
#include "nmbsmf-mbs-session-build.h"
#include "nmbsmf-tmgi-build.h"

#include "unit-test.h"

static cJSON *__activity_status_to_cJSON(mb_smf_sc_activity_status_e act_status);
static bool __string_equal(const char *a, const char *b);
static bool __ogs_time_equal(const ogs_time_t *a, const ogs_time_t *b);
static bool __snssai_equal(const ogs_s_nssai_t *a, const ogs_s_nssai_t *b);

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

static OGS_LIST(__tmgis);

bool _context_add_tmgi(_priv_tmgi_t *tmgi)
{
    if (!tmgi) return false;
    ogs_list_add(&__tmgis, _priv_tmgi_to_private_lnode(tmgi));
    return true;
}

bool _context_remove_tmgi(_priv_tmgi_t *tmgi)
{
    if (!tmgi) return false;
    ogs_list_remove(&__tmgis, _priv_tmgi_to_private_lnode(tmgi));
    return true;
}

const ogs_list_t *_context_tmgis()
{
    return &__tmgis;
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
        _mbs_session_public_clear(&session->session);
        ogs_free(session);
    }
}

void _mbs_session_public_clear(mb_smf_sc_mbs_session_t *session)
{
    if (!session) return;

    if (session->ssm) {
        ogs_free(session->ssm);
        session->ssm = NULL;
    }

    /* session->tmgi: TMGI held elsewhere in context, don't free here */
    session->tmgi = NULL;
    session->tmgi_req = true;

    if (session->mb_upf_udp_tunnel) {
        ogs_freeaddrinfo(session->mb_upf_udp_tunnel);
        session->mb_upf_udp_tunnel = NULL;
    }

    if (session->area_session_id) {
        ogs_free(session->area_session_id);
        session->area_session_id = NULL;
    }

    _mbs_service_area_free(session->mbs_service_area);
    session->mbs_service_area = NULL;

    _ext_mbs_service_area_free(session->ext_mbs_service_area);
    session->ext_mbs_service_area = NULL;

    if (session->dnn) {
        ogs_free(session->dnn);
        session->dnn = NULL;
    }

    if (session->snssai) {
        ogs_free(session->snssai);
        session->snssai = NULL;
    }

    if (session->start_time) {
        ogs_free(session->start_time);
        session->start_time = NULL;
    }

    if (session->termination_time) {
        ogs_free(session->termination_time);
        session->termination_time = NULL;
    }

    _mbs_service_info_free(session->mbs_service_info);
    session->mbs_service_info = NULL;

    _mbs_fsa_ids_clear(&session->mbs_fsa_ids);

    _associated_session_id_free(session->associated_session_id);
    session->associated_session_id = NULL;
}

ogs_list_t *_mbs_session_public_patch_list(const mb_smf_sc_mbs_session_t *a, const mb_smf_sc_mbs_session_t *b)
{
    ogs_list_t *patches = NULL;

    if (a != b) {
        /* add patches for changed fields for changing a to b */
        if (!a) {
            /* completely new (shouldn't happen for an update) */
            ogs_error("Attempt to update an MBS Session when create should be used");
            return NULL;
        }

        if (!b) {
            /* completely remove (shouldn't happen for an update) */
            ogs_error("Attempt to update an MBS Session when delete should be used");
            return NULL;
        }
#define FIELD_NOT_UPDATEABLE(field, name) \
        do { \
            if (a->field != b->field) { \
                ogs_warn("Attempt to update MBS Session " name " is ignored"); \
            } \
        } while (0)
#define FIELD_STRUCT_NOT_UPDATEABLE(field, name, equalfn) \
        do { \
            if (!equalfn(a->field, b->field)) { \
                ogs_warn("Attempt to update MBS Session " name " is ignored"); \
            } \
        } while (0)
#define FIELD_INLINE_NOT_UPDATEABLE(field, name, equalfn) \
        do { \
            if (!equalfn(&a->field, &b->field)) { \
                ogs_warn("Attempt to update MBS Session " name " is ignored"); \
            } \
        } while (0)
#define PATCH_FIELD(field, attribute, jsonfn) \
        do { \
            if (a->field != b->field) { \
                _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_replace, "/" attribute, jsonfn(b->field)); \
                if (!patches) patches = (ogs_list_t*)ogs_calloc(1,sizeof(*patches)); \
                ogs_list_add(patches, patch); \
            } \
        } while (0)
#define PATCH_NULLABLE_FIELD(field, attribute, jsonfn, nullval) \
        do { \
            if (a->field != b->field) { \
                _json_patch_t *patch = NULL; \
                if (a->field == nullval) { \
                    patch = _json_patch_new(OpenAPI_patch_operation_add, "/" attribute, jsonfn(b->field)); \
                } else if (b->field == nullval) { \
                    patch = _json_patch_new(OpenAPI_patch_operation__remove, "/" attribute, NULL); \
                } else { \
                    patch = _json_patch_new(OpenAPI_patch_operation_replace, "/" attribute, jsonfn(b->field)); \
                } \
                if (!patches) patches = (ogs_list_t*)ogs_calloc(1,sizeof(*patches)); \
                ogs_list_add(patches, patch); \
            } \
        } while (0)
#define PATCH_INLINE_FIELD(field, attribute, listfn) \
        do { \
            patches = _json_patches_append_list(patches, listfn(&a->field, &b->field), "/" attribute); \
        } while(0)
#define APPEND_PATCH_LIST(field, attribute, listfn) \
        do { \
            patches = _json_patches_append_list(patches, listfn(a->field, b->field), "/" attribute); \
        } while (0)
#define APPEND_INLINE_PATCH_LIST(field, attribute, listfn) \
        do { \
            patches = _json_patches_append_list(patches, listfn(&a->field, &b->field), "/" attribute); \
        } while (0)

        FIELD_NOT_UPDATEABLE(service_type, "service type");
        FIELD_STRUCT_NOT_UPDATEABLE(ssm, "SSM", _ssm_addr_equal);
        FIELD_STRUCT_NOT_UPDATEABLE(tmgi, "TMGI", mb_smf_sc_tmgi_equal);
        /* mb_upf_udp_tunnel is read-only */
        FIELD_NOT_UPDATEABLE(tunnel_req, "request udp tunnel flag");
        FIELD_NOT_UPDATEABLE(tmgi_req, "request TMGI flag");
        FIELD_NOT_UPDATEABLE(location_dependent, "location dependant flag");
        FIELD_NOT_UPDATEABLE(any_ue_ind, "any UE indicator flag");
        PATCH_FIELD(contact_pcf_ind, "contactPcfInd", cJSON_CreateBool);
        /* area_session_id is read-only */
        APPEND_PATCH_LIST(mbs_service_area, "mbsServiceArea", _mbs_service_area_patch_list);
        APPEND_PATCH_LIST(ext_mbs_service_area, "extMbsServiceArea", _ext_mbs_service_area_patch_list);
        FIELD_STRUCT_NOT_UPDATEABLE(dnn, "DNN", __string_equal);
        FIELD_STRUCT_NOT_UPDATEABLE(snssai, "S-NSSAI", __snssai_equal);
        FIELD_STRUCT_NOT_UPDATEABLE(start_time, "start time", __ogs_time_equal);
        FIELD_STRUCT_NOT_UPDATEABLE(termination_time, "termination time", __ogs_time_equal);
        APPEND_PATCH_LIST(mbs_service_info, "mbsServiceInfo", _mbs_service_info_patch_list);
        if (a->service_type == MBS_SERVICE_TYPE_BROADCAST) {
            FIELD_NOT_UPDATEABLE(activity_status, "activity status");
            APPEND_INLINE_PATCH_LIST(mbs_fsa_ids, "mbsFsaIds", _mbs_fsa_ids_patch_list);
        } else {
            PATCH_NULLABLE_FIELD(activity_status, "activityStatus", __activity_status_to_cJSON, MBS_SESSION_ACTIVITY_STATUS_NONE);
            FIELD_INLINE_NOT_UPDATEABLE(mbs_fsa_ids, "MBS FSA Ids", _mbs_fsa_ids_equal);
        }
        /* associated_session_id not present in current Open5GS model */

#undef FIELD_NOT_UPDATEABLE
#undef FIELD_STRUCT_NOT_UPDATEABLE
#undef FIELD_INLINE_NOT_UPDATEABLE
#undef PATCH_FIELD
#undef PATCH_NULLABLE_FIELD
#undef PATCH_INLINE_FIELD
#undef APPEND_PATCH_LIST
#undef APPEND_INLINE_PATCH_LIST
    }

    return patches;
}

ogs_list_t *_mbs_session_patch_list(const _priv_mbs_session_t *session)
{
    return _mbs_session_public_patch_list(session->previous_session, &session->session);
}

void _mbs_session_public_copy(mb_smf_sc_mbs_session_t **dest, const mb_smf_sc_mbs_session_t * const src)
{
    if (!src) {
        if (*dest) {
            _mbs_session_public_clear(*dest);
            ogs_free(*dest);
            *dest = NULL;
        }
        return;
    }

    if (!*dest) {
        *dest = (mb_smf_sc_mbs_session_t*)ogs_calloc(1, sizeof(**dest));
    }

    mb_smf_sc_mbs_session_t *dst = *dest;

    /* copy service type */
    dst->service_type = src->service_type;

    /* copy SSM */
    if (src->ssm) {
        if (!dst->ssm) {
            dst->ssm = (mb_smf_sc_ssm_addr_t*)ogs_calloc(1, sizeof(*dst->ssm));
        }
        memcpy(dst->ssm, src->ssm, sizeof(*dst->ssm));
    } else {
        if (dst->ssm) {
            ogs_free(dst->ssm);
            dst->ssm = NULL;
        }
    }

    /* copy TMGI (we don't own, so copying pointer is fine) */
    dst->tmgi = src->tmgi;

    /* copy UDP tunnel */
    if (dst->mb_upf_udp_tunnel) ogs_freeaddrinfo(dst->mb_upf_udp_tunnel);
    ogs_copyaddrinfo(&dst->mb_upf_udp_tunnel, src->mb_upf_udp_tunnel);

    /* copy tunnel request flag */
    dst->tunnel_req = src->tunnel_req;

    /* copy tmgi request flag */
    dst->tmgi_req = src->tmgi_req;

    /* copy location dependent flag */
    dst->location_dependent = src->location_dependent;

    /* copy any ue indicator flag */
    dst->any_ue_ind = src->any_ue_ind;

    /* copy contact PCF on update flag */
    dst->contact_pcf_ind = src->contact_pcf_ind;

    /* copy area session id */
    dst->area_session_id = src->area_session_id;

    /* copy mbs service area lists */
    _mbs_service_area_copy(&dst->mbs_service_area, src->mbs_service_area);

    /* copy external mbs service area lists */
    _ext_mbs_service_area_copy(&dst->ext_mbs_service_area, src->ext_mbs_service_area);

    /* copy dnn */
    if (dst->dnn) {
        ogs_free(dst->dnn);
        dst->dnn = NULL;
    }
    if (src->dnn) dst->dnn = ogs_strdup(src->dnn);

    /* copy snssai */
    if (src->snssai) {
        if (!dst->snssai) dst->snssai = (ogs_s_nssai_t*)ogs_calloc(1, sizeof(*dst->snssai));
        dst->snssai->sst = src->snssai->sst;
        dst->snssai->sd = src->snssai->sd;
    } else {
        if (dst->snssai) {
            ogs_free(dst->snssai);
            dst->snssai = NULL;
        }
    }

    /* copy start_time & termination_time */
    dst->start_time = src->start_time;
    dst->termination_time = dst->termination_time;

    /* copy mbs service info */
    _mbs_service_info_copy(&dst->mbs_service_info, src->mbs_service_info);

    /* copy activity status */
    dst->activity_status = src->activity_status;

    /* copy MBS FSA IDs */
    _mbs_fsa_ids_copy(&dst->mbs_fsa_ids, &src->mbs_fsa_ids);

    /* copy associated session id */
    _associated_session_id_copy(&dst->associated_session_id, src->associated_session_id);
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
        fprintf(stderr, "expected req->http.content to be valid JSON, could not parse string as JSON\n");
        return false;
    }
    OpenAPI_create_req_data_t *create_req_data = OpenAPI_create_req_data_parseFromJSON(json);
    cJSON_Delete(json);
    if (!create_req_data) {
        fprintf(stderr, "expected req->http.content to be valid 3GPP CreateReqData JSON, could not parse JSON as a CreateReqData object\n");
        return false;
    }
    UT_PTR_NOT_NULL(create_req_data->mbs_session);
    UT_BOOL_FALSE(create_req_data->mbs_session->is_any_ue_ind);
    UT_ENUM_EQUAL(create_req_data->mbs_session->service_type, OpenAPI_mbs_service_type_BROADCAST);
    UT_ENUM_EQUAL(create_req_data->mbs_session->activity_status, OpenAPI_mbs_session_activity_status_NULL);

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
        fprintf(stderr, "expected req->http.content to be valid JSON, could not parse string as JSON\n");
        return false;
    }
    OpenAPI_create_req_data_t *create_req_data = OpenAPI_create_req_data_parseFromJSON(json);
    cJSON_Delete(json);
    if (!create_req_data) {
        fprintf(stderr, "expected req->http.content to be valid 3GPP CreateReqData JSON, could not parse JSON as a CreateReqData object\n");
        return false;
    }
    UT_PTR_NOT_NULL(create_req_data->mbs_session);
    UT_BOOL_FALSE(create_req_data->mbs_session->is_any_ue_ind);
    UT_ENUM_EQUAL(create_req_data->mbs_session->service_type, OpenAPI_mbs_service_type_BROADCAST);
    UT_ENUM_EQUAL(create_req_data->mbs_session->activity_status, OpenAPI_mbs_session_activity_status_NULL);

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

static bool test_create_sess_all_fields(unit_test_ctx *ctx)
{
    mb_smf_sc_ssm_addr_t ssm = {
            .family = AF_INET,
            .source.ipv4.s_addr = htonl(0xC0A80102) /* 192.168.1.2 */,
            .dest_mc.ipv4.s_addr = htonl(0xE8000001) /* 232.0.0.1 */
    };

    char mbs_service_id[] = "test-mbs-service-id";
    _priv_tmgi_t tmgi = {
        .tmgi.mbs_service_id = mbs_service_id,
        .tmgi.plmn = { .mcc1 = 1, .mcc2 = 2, .mcc3 = 3, .mnc1 = 4, .mnc2 = 5, .mnc3 = 6 },
        .tmgi.expiry_time = 0 /* read-only */
    };

    mb_smf_sc_mbs_service_area_t mbs_service_area = {};
    uint64_t ncgi_tai_nid1 = 0x123456789AB;
    uint64_t ncgi_tai_nid2 = 0xBA987654321;
    uint64_t ncgi_tai_nid3 = 0x456789ABCDE;
    mb_smf_sc_ncgi_tai_t ncgi_tai1 = {
        .tai.plmn_id = { .mcc1 = 2, .mcc2 = 3, .mcc3 = 4, .mnc1 = 5, .mnc2 = 6, .mnc3 = 7 },
        .tai.tac = 0x123456,
        .tai.nid = &ncgi_tai_nid1
    };
    mb_smf_sc_ncgi_t ncgi1 = {
        .plmn_id = { .mcc1 = 3, .mcc2 = 4, .mcc3 = 5, .mnc1 = 6, .mnc2 = 7, .mnc3 = 8 },
        .nr_cell_id = 0x123456789,
        .nid = &ncgi_tai_nid2
    };
    ogs_list_add(&ncgi_tai1.ncgis, &ncgi1);
    ogs_list_add(&mbs_service_area.ncgi_tais, &ncgi_tai1);
    mb_smf_sc_tai_t tai1 = {
        .plmn_id = { .mcc1 = 4, .mcc2 = 5, .mcc3 = 6, .mnc1 = 7, .mnc2 = 8, .mnc3 = 9 },
        .tac = 0x654321,
        .nid = &ncgi_tai_nid3
    };
    ogs_list_add(&mbs_service_area.tais, &tai1);

    mb_smf_sc_ext_mbs_service_area_t ext_mbs_service_area = {};
    mb_smf_sc_civic_address_t addr1 = {
        .country = (char*)"UK",
        .a = {
            (char*)"State/County",
            (char*)"Province",
            (char*)"City/Town",
            (char*)"Borough",
            (char*)"Neighbourhood",
            (char*)"Group of streets"
        },
        .prd = (char*)"N.",
        .pod = (char*)"SW",
        .sts = (char*)"Ave",
        .hno = (char*)"123",
        .hns = (char*)"A",
        .lmk = (char*)"Madeup University",
        .loc = (char*)"West Wing",
        .nam = (char*)"Occupant",
        .pc = (char*)"Postcode",
        .bld = (char*)"Building",
        .unit = (char*)"Appartment 3",
        .flr = (char*)"5",
        .room = (char*)"Room 505",
        .plc = (char*)"Place type",
        .pcn = (char*)"Postal community name",
        .pobox = (char*)"12345",
        .addcode = (char*)"additional-code",
        .seat = (char*)"desk location",
        .rd = (char*)"Road name",
        .rdsec = (char*)"Road section",
        .rdbr = (char*)"Branch road name",
        .rdsubbr = (char*)"Sub-branch road name",
        .prm = (char*)"Road pre-modifier (e.g. Old)",
        .pom = (char*)"Road post-modifier (e.g. Service)",
        .usage_rules = (char*)"<retransmission-allowed xmlns=\"urn:ietf:params:xml:ns:pidf:geopriv10\">no</retransmission-allowed><retention-expiry xmlns=\"urn:ietf:params:xml:ns:pidf:geopriv10\">2003-06-23T04:57:29Z</retention-expiry>",
        .method = (char*)"GPS",
        .provided_by = (char*)"Provider"
    };
    ogs_list_add(&ext_mbs_service_area.civic_addresses, &addr1);

    char dnn[] = "network-id";

    ogs_s_nssai_t snssai = {
        .sst = 123,
        .sd.v = 0x123456
    };

    ogs_time_t start_time = ogs_time_from_sec(1763394645); /* 2025-11-17T15:50:45Z */
    ogs_time_t term_time = ogs_time_from_sec(1763394686); /* 2025-11-17T15:51:26Z */

    char af_app_id[] = "application-id";
    uint64_t mbs_session_ambr = 1000000;
    mb_smf_sc_mbs_service_info_t mbs_service_info = {
        .mbs_media_comps = ogs_hash_make(),
        .af_app_id = af_app_id,
        .mbs_session_ambr = &mbs_session_ambr,
        .mbs_sdf_reserve_priority = 1
    };
    OGS_LIST(flow_descs);
    mb_smf_sc_flow_description_t flow1 = {
        .string = "flow-description-1"
    };
    ogs_list_add(&flow_descs, &flow1);
    uint64_t max_requested_mbs_bandwidth_downlink = 1000000;
    uint64_t min_requested_mbs_bandwidth_downlink = 1000;
    mb_smf_sc_mbs_media_info_t media_info = {
        .mbs_media_type = MEDIA_TYPE_VIDEO,
        .max_requested_mbs_bandwidth_downlink = &max_requested_mbs_bandwidth_downlink,
        .min_requested_mbs_bandwidth_downlink = &min_requested_mbs_bandwidth_downlink,
        .codecs = { (char*)"Codec1-string", (char*)"Codec2-string" }
    };
    mb_smf_sc_mbs_media_comp_t media_comp1 = {
        .id = 42,
        .flow_descriptions = &flow_descs,
        .mbs_sdf_reserve_priority = 2,
        .media_info = &media_info,
        .qos_ref = NULL,    /* only one of media_info, qos_ref and mbs_qos_req may be present */
        .mbs_qos_req = NULL /* only one of media_info, qos_ref and mbs_qos_req may be present */
    };
    ogs_hash_set(mbs_service_info.mbs_media_comps, &media_comp1.id, sizeof(media_comp1.id), &media_comp1);

    _priv_mbs_session_t session = {
        .session.service_type = MBS_SERVICE_TYPE_MULTICAST,
        .session.ssm = &ssm,
        .session.tmgi = &tmgi.tmgi,
        .session.mb_upf_udp_tunnel = NULL, /* read-only */
        .session.tunnel_req = true,
        .session.tmgi_req = false,
        .session.location_dependent = true,
        .session.any_ue_ind = true,
        .session.contact_pcf_ind = true,
        .session.area_session_id = NULL, /* read-only */
        .session.mbs_service_area = &mbs_service_area,
        .session.ext_mbs_service_area = &ext_mbs_service_area,
        .session.dnn = dnn,
        .session.snssai = &snssai,
        .session.start_time = &start_time,
        .session.termination_time = &term_time,
        .session.mbs_service_info = &mbs_service_info,
        .session.activity_status = MBS_SESSION_ACTIVITY_STATUS_ACTIVE,
        /* .session.mbs_fsa_ids */
        .session.associated_session_id = NULL
        /* mbs_security_context missing */
        /* area_session_policy_id missing */
    };

    ogs_sbi_request_t *req = _nmbsmf_mbs_session_build_create((void*)&session, NULL);

    cJSON *json = cJSON_Parse(req->http.content);
    if (!json) {
        fprintf(stderr, "expected req->http.content to be valid JSON, could not parse string as JSON\n");
        return false;
    }
    OpenAPI_create_req_data_t *create_req_data = OpenAPI_create_req_data_parseFromJSON(json);
    cJSON_Delete(json);
    if (!create_req_data) {
        fprintf(stderr, "expected req->http.content to be valid 3GPP CreateReqData JSON, could not parse JSON as a CreateReqData object\n");
        return false;
    }

    UT_PTR_NOT_NULL(create_req_data->mbs_session);

    UT_ENUM_EQUAL(create_req_data->mbs_session->service_type, OpenAPI_mbs_service_type_MULTICAST);

    UT_PTR_NOT_NULL(create_req_data->mbs_session->mbs_session_id);
    UT_PTR_NOT_NULL(create_req_data->mbs_session->mbs_session_id->ssm);
    UT_PTR_NOT_NULL(create_req_data->mbs_session->mbs_session_id->ssm->source_ip_addr);
    UT_STR_EQUAL(create_req_data->mbs_session->mbs_session_id->ssm->source_ip_addr->ipv4_addr, "192.168.1.2");
    UT_STR_NULL(create_req_data->mbs_session->mbs_session_id->ssm->source_ip_addr->ipv6_addr);
    UT_STR_NULL(create_req_data->mbs_session->mbs_session_id->ssm->source_ip_addr->ipv6_prefix);
    UT_PTR_NOT_NULL(create_req_data->mbs_session->mbs_session_id->ssm->dest_ip_addr);
    UT_STR_EQUAL(create_req_data->mbs_session->mbs_session_id->ssm->dest_ip_addr->ipv4_addr, "232.0.0.1");
    UT_STR_NULL(create_req_data->mbs_session->mbs_session_id->ssm->dest_ip_addr->ipv6_addr);
    UT_STR_NULL(create_req_data->mbs_session->mbs_session_id->ssm->dest_ip_addr->ipv6_prefix);

    UT_PTR_NOT_NULL(create_req_data->mbs_session->mbs_session_id->tmgi);
    UT_STR_EQUAL(create_req_data->mbs_session->mbs_session_id->tmgi->mbs_service_id, "test-mbs-service-id");
    UT_PTR_NOT_NULL(create_req_data->mbs_session->mbs_session_id->tmgi->plmn_id);
    UT_STR_EQUAL(create_req_data->mbs_session->mbs_session_id->tmgi->plmn_id->mcc, "123");
    UT_STR_EQUAL(create_req_data->mbs_session->mbs_session_id->tmgi->plmn_id->mnc, "456");

    UT_PTR_NULL(create_req_data->mbs_session->tmgi); /* read-only */
    UT_STR_NULL(create_req_data->mbs_session->expiration_time); /* read-only */

    UT_PTR_NULL(create_req_data->mbs_session->ingress_tun_addr);

    UT_BOOL_TRUE(create_req_data->mbs_session->is_ingress_tun_addr_req);
    UT_INT_EQUAL(create_req_data->mbs_session->ingress_tun_addr_req, 1);

    UT_BOOL_FALSE(create_req_data->mbs_session->is_tmgi_alloc_req);

    UT_BOOL_TRUE(create_req_data->mbs_session->is_location_dependent);
    UT_INT_EQUAL(create_req_data->mbs_session->location_dependent, 1);

    UT_BOOL_TRUE(create_req_data->mbs_session->is_any_ue_ind);
    UT_INT_EQUAL(create_req_data->mbs_session->any_ue_ind, 1);

    UT_BOOL_FALSE(create_req_data->mbs_session->is_contact_pcf_ind); /* contact_pcf_ind not used in create */

    /* area_session_id not present in OpenAPI model also read-only */

    UT_PTR_NOT_NULL(create_req_data->mbs_session->mbs_service_area);
    UT_PTR_NOT_NULL(create_req_data->mbs_session->mbs_service_area->ncgi_list);
    UT_PTR_NOT_NULL(create_req_data->mbs_session->mbs_service_area->tai_list);
    UT_JSON_LIST_SIZE(create_req_data->mbs_session->mbs_service_area->ncgi_list, 1);
    UT_JSON_LIST_SIZE(create_req_data->mbs_session->mbs_service_area->tai_list, 1);
    UT_PTR_NOT_NULL(create_req_data->mbs_session->mbs_service_area->ncgi_list->first->data);
    OpenAPI_ncgi_tai_t *ncgi_tai_item = create_req_data->mbs_session->mbs_service_area->ncgi_list->first->data;
    UT_PTR_NOT_NULL(ncgi_tai_item->tai);
    UT_PTR_NOT_NULL(ncgi_tai_item->tai->plmn_id);
    UT_STR_EQUAL(ncgi_tai_item->tai->plmn_id->mcc, "234");
    UT_STR_EQUAL(ncgi_tai_item->tai->plmn_id->mnc, "567");
    UT_STR_EQUAL(ncgi_tai_item->tai->tac, "123456");
    UT_STR_EQUAL(ncgi_tai_item->tai->nid, "123456789AB");
    UT_PTR_NOT_NULL(ncgi_tai_item->cell_list);
    UT_JSON_LIST_SIZE(ncgi_tai_item->cell_list, 1);
    UT_PTR_NOT_NULL(ncgi_tai_item->cell_list->first->data);
    OpenAPI_ncgi_t *ncgi_item = ncgi_tai_item->cell_list->first->data;
    UT_PTR_NOT_NULL(ncgi_item->plmn_id);
    UT_STR_EQUAL(ncgi_item->plmn_id->mcc, "345");
    UT_STR_EQUAL(ncgi_item->plmn_id->mnc, "678");
    UT_STR_EQUAL(ncgi_item->nr_cell_id, "123456789");
    UT_STR_EQUAL(ncgi_item->nid, "BA987654321");
    UT_PTR_NOT_NULL(create_req_data->mbs_session->mbs_service_area->tai_list->first->data);
    OpenAPI_tai_t *tai_item = create_req_data->mbs_session->mbs_service_area->tai_list->first->data;
    UT_PTR_NOT_NULL(tai_item->plmn_id);
    UT_STR_EQUAL(tai_item->plmn_id->mcc, "456");
    UT_STR_EQUAL(tai_item->plmn_id->mnc, "789");
    UT_STR_EQUAL(tai_item->tac, "654321");
    UT_STR_EQUAL(tai_item->nid, "456789ABCDE");

    UT_PTR_NOT_NULL(create_req_data->mbs_session->ext_mbs_service_area);
    UT_PTR_NULL(create_req_data->mbs_session->ext_mbs_service_area->geographic_area_list);
    UT_PTR_NOT_NULL(create_req_data->mbs_session->ext_mbs_service_area->civic_address_list);
    UT_JSON_LIST_SIZE(create_req_data->mbs_session->ext_mbs_service_area->civic_address_list, 1);
    UT_PTR_NOT_NULL(create_req_data->mbs_session->ext_mbs_service_area->civic_address_list->first->data);
    OpenAPI_civic_address_t *api_civic_addr =
                    (OpenAPI_civic_address_t*)create_req_data->mbs_session->ext_mbs_service_area->civic_address_list->first->data;
    UT_STR_EQUAL(api_civic_addr->country, "UK");
    UT_STR_EQUAL(api_civic_addr->a1, "State/County");
    UT_STR_EQUAL(api_civic_addr->a2, "Province");
    UT_STR_EQUAL(api_civic_addr->a3, "City/Town");
    UT_STR_EQUAL(api_civic_addr->a4, "Borough");
    UT_STR_EQUAL(api_civic_addr->a5, "Neighbourhood");
    UT_STR_EQUAL(api_civic_addr->a6, "Group of streets");
    UT_STR_EQUAL(api_civic_addr->prd, "N.");
    UT_STR_EQUAL(api_civic_addr->pod, "SW");
    UT_STR_EQUAL(api_civic_addr->sts, "Ave");
    UT_STR_EQUAL(api_civic_addr->hno, "123");
    UT_STR_EQUAL(api_civic_addr->hns, "A");
    UT_STR_EQUAL(api_civic_addr->lmk, "Madeup University");
    UT_STR_EQUAL(api_civic_addr->loc, "West Wing");
    UT_STR_EQUAL(api_civic_addr->nam, "Occupant");
    UT_STR_EQUAL(api_civic_addr->pc, "Postcode");
    UT_STR_EQUAL(api_civic_addr->bld, "Building");
    UT_STR_EQUAL(api_civic_addr->unit, "Appartment 3");
    UT_STR_EQUAL(api_civic_addr->flr, "5");
    UT_STR_EQUAL(api_civic_addr->room, "Room 505");
    UT_STR_EQUAL(api_civic_addr->plc, "Place type");
    UT_STR_EQUAL(api_civic_addr->pcn, "Postal community name");
    UT_STR_EQUAL(api_civic_addr->pobox, "12345");
    UT_STR_EQUAL(api_civic_addr->addcode, "additional-code");
    UT_STR_EQUAL(api_civic_addr->seat, "desk location");
    UT_STR_EQUAL(api_civic_addr->rd, "Road name");
    UT_STR_EQUAL(api_civic_addr->rdsec, "Road section");
    UT_STR_EQUAL(api_civic_addr->rdbr, "Branch road name");
    UT_STR_EQUAL(api_civic_addr->rdsubbr, "Sub-branch road name");
    UT_STR_EQUAL(api_civic_addr->prm, "Road pre-modifier (e.g. Old)");
    UT_STR_EQUAL(api_civic_addr->pom, "Road post-modifier (e.g. Service)");
    UT_STR_EQUAL(api_civic_addr->usage_rules, "<retransmission-allowed xmlns=\"urn:ietf:params:xml:ns:pidf:geopriv10\">no</retransmission-allowed><retention-expiry xmlns=\"urn:ietf:params:xml:ns:pidf:geopriv10\">2003-06-23T04:57:29Z</retention-expiry>");
    UT_STR_EQUAL(api_civic_addr->method, "GPS");
    UT_STR_EQUAL(api_civic_addr->provided_by, "Provider");

    UT_STR_EQUAL(create_req_data->mbs_session->dnn, "network-id");

    UT_PTR_NOT_NULL(create_req_data->mbs_session->snssai);
    UT_INT_EQUAL(create_req_data->mbs_session->snssai->sst, 123);
    UT_STR_EQUAL(create_req_data->mbs_session->snssai->sd, "123456");

    UT_STR_NULL(create_req_data->mbs_session->activation_time); /* deprecated */

    UT_STR_EQUAL(create_req_data->mbs_session->start_time, "2025-11-17T15:50:45.000000Z");

    UT_STR_EQUAL(create_req_data->mbs_session->termination_time, "2025-11-17T15:51:26.000000Z");

    UT_PTR_NOT_NULL(create_req_data->mbs_session->mbs_serv_info);
    UT_PTR_NOT_NULL(create_req_data->mbs_session->mbs_serv_info->mbs_media_comps);
    UT_JSON_LIST_SIZE(create_req_data->mbs_session->mbs_serv_info->mbs_media_comps, 1);
    /* TODO: check media components */
    UT_ENUM_EQUAL(create_req_data->mbs_session->mbs_serv_info->mbs_sdf_res_prio, OpenAPI_reserv_priority_PRIO_1);
    UT_STR_EQUAL(create_req_data->mbs_session->mbs_serv_info->af_app_id, "application-id");
    UT_STR_EQUAL(create_req_data->mbs_session->mbs_serv_info->mbs_session_ambr, "1.000000 Mbps");

    UT_PTR_NULL(create_req_data->mbs_session->mbs_session_subsc);

    UT_ENUM_EQUAL(create_req_data->mbs_session->activity_status, OpenAPI_mbs_session_activity_status_ACTIVE);

    UT_PTR_NULL(create_req_data->mbs_session->mbs_fsa_id_list);

    /* mbs_security_context */
    /* area_session_policy_id not present in OpenAPI model */

    OpenAPI_create_req_data_free(create_req_data);
    ogs_sbi_request_free(req);

    return true;
}

#define FAKE_SESSION_ID "12345678-9abc-4def-8123-456789abcdef"

static bool test_update_sess(unit_test_ctx *ctx)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)ogs_calloc(1, sizeof(*session));
    session->id = ogs_strdup(FAKE_SESSION_ID);
    session->session.service_type = MBS_SERVICE_TYPE_MULTICAST;
    _mbs_session_public_copy(&session->previous_session, &session->session);

    session->session.activity_status = MBS_SESSION_ACTIVITY_STATUS_ACTIVE;

    ogs_sbi_request_t *req = _nmbsmf_mbs_session_build_update((void*)session, NULL);

    UT_STR_EQUAL(req->h.method, OGS_SBI_HTTP_METHOD_PATCH);
    if (!req->h.uri) {
        UT_STR_EQUAL(req->h.service.name, OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION);
        UT_STR_EQUAL(req->h.api.version, OGS_SBI_API_V1);
        UT_STR_EQUAL(req->h.resource.component[0], OGS_SBI_RESOURCE_NAME_MBS_SESSIONS);
        UT_STR_EQUAL(req->h.resource.component[1], FAKE_SESSION_ID);
        UT_STR_NULL(req->h.resource.component[2]);
    } else {
        UT_STR_MATCHES(req->h.uri, "^https?://(?:[a-zA-Z0-9.]*|\\[[0-9A-Fa-f:]*\\])(?::[1-9][0-9]*)?/" OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION "/" OGS_SBI_API_V1 "/" OGS_SBI_RESOURCE_NAME_MBS_SESSIONS "/" FAKE_SESSION_ID "$");
    }

    cJSON *json = cJSON_Parse(req->http.content);
    if (!json) {
        fprintf(stderr, "expected req->http.content to be valid JSON, could not parse string as JSON\n");
        return false;
    }

    if (!cJSON_IsArray(json)) {
        fprintf(stderr, "expected req->http.content to be a JSON array\n");
        return false;
    }

    cJSON *item;
    size_t count = 0;
    cJSON_ArrayForEach(item, json) {
        count++;
        if (!cJSON_IsObject(item)) {
            fprintf(stderr, "expected req->http.content to be a JSON array of objects\n");
            return false;
        }

        OpenAPI_patch_item_t *patch_item = OpenAPI_patch_item_parseFromJSON(item);
        if (!patch_item) {
            fprintf(stderr, "expected req->http.content to be a JSON array of patch objects\n");
            return false;
        }

        if (count == 1) {
            UT_ENUM_EQUAL(patch_item->op, OpenAPI_patch_operation_add);
            UT_STR_EQUAL(patch_item->path, "/activityStatus");
            UT_PTR_NOT_NULL(patch_item->value);
            UT_PTR_NOT_NULL(patch_item->value->json);
            if (!cJSON_IsString(patch_item->value->json)) {
                fprintf(stderr, "expected patch_item->value->json to be a string value\n");
                return false;
            }
            const char *value = cJSON_GetStringValue(patch_item->value->json);
            UT_STR_EQUAL(value, "ACTIVE");
        }
    }

    if (count != 1) {
        fprintf(stderr, "expected 1 JSON patch, got %zu\n", count);
        return false;
    }

    return true;
}

static bool test_delete_sess(unit_test_ctx *ctx)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)ogs_calloc(1, sizeof(*session));
    session->id = ogs_strdup(FAKE_SESSION_ID);

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
        fprintf(stderr, "expected req->http.content to be valid JSON, could not parse string as JSON\n");
        return false;
    }
    OpenAPI_status_subscribe_req_data_t *status_subsc_req_data = OpenAPI_status_subscribe_req_data_parseFromJSON(json);
    cJSON_Delete(json);
    if (!status_subsc_req_data) {
        fprintf(stderr, "expected req->http.content to be valid 3GPP StatusSubscribeReqData JSON, could not parse JSON as a StatusSubscribeReqData object\n");
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
    fprintf(stderr, "TODO: implement update builder test for MBS Session Status Subscriptions\n");
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

static cJSON *__activity_status_to_cJSON(mb_smf_sc_activity_status_e act_status)
{
    switch (act_status) {
    case MBS_SESSION_ACTIVITY_STATUS_ACTIVE:
        return cJSON_CreateString("ACTIVE");
    case MBS_SESSION_ACTIVITY_STATUS_INACTIVE:
        return cJSON_CreateString("INACTIVE");
    default:
        break;
    }
    return cJSON_CreateNull();
}

static bool __string_equal(const char *a, const char *b)
{
    if (a == b) return true;
    if (!a || !b) return false;
    return strcmp(a,b) == 0;
}

static bool __ogs_time_equal(const ogs_time_t *a, const ogs_time_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;
    return *a == *b;
}

static bool __snssai_equal(const ogs_s_nssai_t *a, const ogs_s_nssai_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    return a->sst == b->sst && a->sd.v == b->sd.v;
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

static const unit_test_t test_create_sess_desc = {
    .name = "nmbsmf-mbssession: create session without subscription builder",
    .fn = test_create_sess
};

static const unit_test_t test_create_sess_with_subsc_desc = {
    .name = "nmbsmf-mbssession: create session with subscription builder",
    .fn = test_create_sess_with_subsc
};

static const unit_test_t test_create_sess_all_fields_desc = {
    .name = "nmbsmf-mbssession: create session with all fields builder",
    .fn = test_create_sess_all_fields
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
    register_unit_test(&test_create_sess_all_fields_desc);
    register_unit_test(&test_update_sess_desc);
    register_unit_test(&test_delete_sess_desc);
    register_unit_test(&test_create_status_subsc_desc);
    register_unit_test(&test_update_status_subsc_desc);
    register_unit_test(&test_delete_status_subsc_desc);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
