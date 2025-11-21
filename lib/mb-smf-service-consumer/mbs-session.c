/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <string.h>
#include <netinet/in.h>

#include "ogs-core.h"
#include "ogs-sbi.h"

#include "macros.h"
#include "context.h"
#include "log.h"
#include "ssm-addr.h"
#include "tai.h"
#include "tmgi.h"
#include "mbs-status-subscription.h"
#include "nmbsmf-mbs-session-build.h"
#include "priv_associated-session-id.h"
#include "priv_ext-mbs-service-area.h"
#include "priv_mbs-fsa-id.h"
#include "priv_mbs-session.h"
#include "priv_mbs-service-area.h"
#include "priv_mbs-service-info.h"
#include "priv_mbs-status-subscription.h"
#include "priv_ssm-addr.h"
#include "priv_tai.h"
#include "priv_tmgi.h"

#include "mbs-session.h"

/* Private types, variables and function prototypes */

typedef struct __mbs_session_find_subsc_filter_s {
    const char *correlation_id;
    _priv_mbs_status_subscription_t *found;
} __mbs_session_find_subsc_filter_t;

static OpenAPI_ip_addr_t *__new_OpenAPI_ip_addr_from_inaddr(const struct in_addr *addr);
static OpenAPI_ip_addr_t *__new_OpenAPI_ip_addr_from_in6addr(const struct in6_addr *addr);
static int __mbs_session_find_subsc_hash_do(void *rec, const void *key, int klen, const void *value);
static int __update_status_subscription(void *rec, const void *key, int klen, const void *value);
static int __free_status_subscription(void *rec, const void *key, int klen, const void *value);

static cJSON *__activity_status_to_cJSON(mb_smf_sc_activity_status_e act_status);
static bool __string_equal(const char *a, const char *b);
static bool __ogs_time_equal(const ogs_time_t *a, const ogs_time_t *b);
static bool __snssai_equal(const ogs_s_nssai_t *a, const ogs_s_nssai_t *b);

/*================ Public mbs_session functions =================*/
MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new()
{
    return mb_smf_sc_mbs_session_new_ipv4(NULL, NULL);
}

MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new_ipv4(const struct in_addr *source, const struct in_addr *dest)
{
    _priv_mbs_session_t *session = _mbs_session_new();

    if (source) {
        session->session.ssm = ogs_calloc(1, sizeof(*session->session.ssm));
        session->session.ssm->family = AF_INET;
        memcpy(&session->session.ssm->source.ipv4, source, sizeof(session->session.ssm->source.ipv4));
    }
    if (dest) {
        if (!session->session.ssm) {
            session->session.ssm = ogs_calloc(1, sizeof(*session->session.ssm));
            session->session.ssm->family = AF_INET;
        }
        memcpy(&session->session.ssm->dest_mc.ipv4, dest, sizeof(session->session.ssm->dest_mc.ipv4));
    }

    _context_add_mbs_session(session);

    return _priv_mbs_session_to_public(session);
}

MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new_ipv6(const struct in6_addr *source, const struct in6_addr *dest)
{
    _priv_mbs_session_t *session = _mbs_session_new();

    if (source) {
        session->session.ssm = ogs_calloc(1, sizeof(*session->session.ssm));
        session->session.ssm->family = AF_INET6;
        memcpy(&session->session.ssm->source.ipv6, source, sizeof(session->session.ssm->source.ipv6));
    }
    if (dest) {
        if (!session->session.ssm) {
            session->session.ssm = ogs_calloc(1, sizeof(*session->session.ssm));
            session->session.ssm->family = AF_INET6;
        }
        memcpy(&session->session.ssm->dest_mc.ipv6, dest, sizeof(session->session.ssm->dest_mc.ipv6));
    }

    _context_add_mbs_session(session);

    return _priv_mbs_session_to_public(session);
}

MB_SMF_CLIENT_API void mb_smf_sc_mbs_session_delete(mb_smf_sc_mbs_session_t *mbs_session)
{
    _priv_mbs_session_t *session = _priv_mbs_session_from_public(mbs_session);

    if (!session) return;

    session->deleted = true;
}

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_tmgi(mb_smf_sc_mbs_session_t *session, mb_smf_sc_tmgi_t *tmgi)
{
    _priv_mbs_session_t *sess = _priv_mbs_session_from_public(session);
    _priv_tmgi_t *t = _priv_tmgi_from_public(tmgi);
    return _mbs_session_set_tmgi(sess, t);
}

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_add_subscription(mb_smf_sc_mbs_session_t *session, mb_smf_sc_mbs_status_subscription_t *subscription)
{
    _priv_mbs_session_t *sess = _priv_mbs_session_from_public(session);
    _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public(subscription);

    if (!sess || !subscription || sess->deleted) return false;

    if (!sess->active_subscriptions) {
        sess->active_subscriptions = ogs_hash_make();
    }

    if (subsc->id) {
        // Remove from active list of current mbs session
        ogs_hash_set(subsc->session->active_subscriptions, subsc->id, OGS_HASH_KEY_STRING, NULL);

        _mbs_status_subscription_send_delete(subsc);

        // forget the assigned id
        ogs_free(subsc->id);
        subsc->id = NULL;
    }
    // Add to new subscription list
    ogs_list_add(&sess->new_subscriptions, subsc);
    subsc->session = sess;
    subsc->changed = true;

    return true;
}

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_remove_subscription(mb_smf_sc_mbs_session_t *session, mb_smf_sc_mbs_status_subscription_t *subscription)
{
    _priv_mbs_session_t *sess = _priv_mbs_session_from_public(session);
    _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public(subscription);

    if (!sess || !subsc || sess->deleted) return false;
    if (subsc->session != sess) return false; // wrong session
    if (subsc->id) {
        // active subscription - move to deleted list
        if (sess->active_subscriptions) {
            ogs_hash_set(sess->active_subscriptions, subsc->id, OGS_HASH_KEY_STRING, NULL);
        }
        ogs_list_add(&sess->deleted_subscriptions, subsc);
        subsc->changed = true;
    } else {
        // if subscription is still in new list, just remove
        _priv_mbs_status_subscription_t *next, *node;
        ogs_list_for_each_safe(&sess->new_subscriptions, next, node) {
            if (node == subsc) {
                ogs_list_remove(&sess->new_subscriptions, node);
                _mbs_status_subscription_delete(node);
                subsc->changed = true;
                break;
            }
        }
    }

    return true;
}

MB_SMF_CLIENT_API mb_smf_sc_mbs_status_subscription_t *mb_smf_sc_mbs_session_find_subscription(const mb_smf_sc_mbs_session_t *session, const char *subscription_id)
{
    const _priv_mbs_session_t *sess = _priv_mbs_session_from_public_const(session);
    return _priv_mbs_status_subscription_to_public(_mbs_session_find_active_subscription(sess, subscription_id));
}

MB_SMF_CLIENT_API mb_smf_sc_mbs_status_subscription_t *mb_smf_sc_mbs_session_find_subscription_by_correlation(const mb_smf_sc_mbs_session_t *session, const char *correlation_id)
{
    const _priv_mbs_session_t *sess = _priv_mbs_session_from_public_const(session);
    return _priv_mbs_status_subscription_to_public(_mbs_session_find_subscription(sess, correlation_id));
}

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_push_all_changes()
{
    ogs_list_t *sessions = _context_mbs_sessions();
    _priv_mbs_session_t *sess, *next;
    bool ret = true;
    ogs_list_for_each_safe(sessions, next, sess) {
        ret &= _mbs_session_push_changes(sess);
    }
    return ret;
}

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_callback(mb_smf_sc_mbs_session_t *session, mb_smf_sc_mbs_session_create_result_cb callback, void *data)
{
    return _mbs_session_set_callback(_priv_mbs_session_from_public(session), callback, data);
}

MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_session_get_resource_id(const mb_smf_sc_mbs_session_t *session)
{
    if (!session) return NULL;
    return _priv_mbs_session_from_public_const(session)->id;
}

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_push_changes(mb_smf_sc_mbs_session_t *session)
{
    if (!session) return false;
    _priv_mbs_session_t *sess = _priv_mbs_session_from_public(session);
    return _mbs_session_push_changes(sess);
}

MB_SMF_CLIENT_API OpenAPI_mbs_session_id_t *mb_smf_sc_mbs_session_create_mbs_session_id(mb_smf_sc_mbs_session_t *session)
{
    if (!session) return NULL;
    _priv_mbs_session_t *sess = _priv_mbs_session_from_public(session);
    return _mbs_session_create_mbs_session_id(sess);
}

/*==================== Protected mbs_session functions ====================*/

_priv_mbs_session_t *_mbs_session_new()
{
    /* create new mbs_session object */
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)ogs_calloc(1, sizeof(*session));

    /* set defaults */
    session->session.tmgi_req = true;

    return session;
}

void _mbs_session_delete(_priv_mbs_session_t *session)
{
    if (!session) return;

    // mb_smf_sc_mbs_session_t session->session
    _mbs_session_public_clear(&session->session);
    _mbs_session_public_free(session->previous_session);
    session->previous_session = NULL;

    _priv_mbs_status_subscription_t *next, *node;

    ogs_list_for_each_safe(&session->new_subscriptions, next, node) {
        ogs_list_remove(&session->new_subscriptions, node);
        _mbs_status_subscription_delete(node);
    }

    if (session->active_subscriptions) {
        ogs_hash_do(__free_status_subscription, session, session->active_subscriptions);
        ogs_hash_destroy(session->active_subscriptions);
        session->active_subscriptions = NULL;
    }

    ogs_list_for_each_safe(&session->deleted_subscriptions, next, node) {
        ogs_list_remove(&session->new_subscriptions, node);
        _mbs_status_subscription_delete(node);
    }

    if (session->id) {
        ogs_free(session->id);
        session->id = NULL;
    }

    if (session->sbi_object) {
        _ref_count_sbi_object_unref(session->sbi_object);
        session->sbi_object = NULL;
    }

    // session
    ogs_free(session);
}

void _mbs_session_public_free(mb_smf_sc_mbs_session_t *session)
{
    if (!session) return;
    _mbs_session_public_clear(session);
    ogs_free(session);
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

bool _mbs_session_public_equal(const mb_smf_sc_mbs_session_t *a, const mb_smf_sc_mbs_session_t *b)
{
    if (a == b) return true; /* same object or both NULL */

    /* compare two MBS sessions */
    if (!a || !b) return false; /* one NULL and the other not NULL is not equal */

    /* both set, compare the fields, simple types first then more complex later */
    if (a->service_type != b->service_type) return false;
    if (a->tunnel_req != b->tunnel_req) return false;
    if (a->tmgi_req != b->tmgi_req) return false;
    if (a->location_dependent != b->location_dependent) return false;
    if (a->any_ue_ind != b->any_ue_ind) return false;
    if (a->contact_pcf_ind != b->contact_pcf_ind) return false;
    if (a->activity_status != b->activity_status) return false;

    /* check pointers for optional fields, not equal if one set and one NULL */
    if (!a->ssm != !b->ssm) return false;
    if (!a->tmgi != !b->tmgi) return false;
    if (!a->mb_upf_udp_tunnel != !b->mb_upf_udp_tunnel) return false;
    if (!a->area_session_id != !b->area_session_id) return false;
    if (!a->mbs_service_area != !b->mbs_service_area) return false;
    if (!a->ext_mbs_service_area != !b->ext_mbs_service_area) return false;
    if (!a->dnn != !b->dnn) return false;
    if (!a->snssai != !b->snssai) return false;
    if (!a->start_time != !b->start_time) return false;
    if (!a->termination_time != !b->termination_time) return false;
    if (!a->mbs_service_info != !b->mbs_service_info) return false;
    if (!a->associated_session_id != !b->associated_session_id) return false;

    /* check array sizes */
    if (ogs_list_count(&a->mbs_fsa_ids) != ogs_list_count(&b->mbs_fsa_ids)) return false;

    /* check pointed to types, simple types first */
    if (a->area_session_id && *a->area_session_id != *b->area_session_id) return false;
    if (a->start_time && *a->start_time != *b->start_time) return false;
    if (a->termination_time && *a->termination_time != *b->termination_time) return false;
    if (a->dnn && strcmp(a->dnn, b->dnn)) return false;
    if (a->ssm && !mb_smf_sc_ssm_equal(a->ssm, b->ssm)) return false;
    if (a->tmgi && !mb_smf_sc_tmgi_equal(a->tmgi, b->tmgi)) return false;
    /* a->mb_upf_udp_tunnel is read-only, so ignore for comparison */
    if (a->mbs_service_area && !_mbs_service_area_equal(a->mbs_service_area, b->mbs_service_area)) return false;
    if (a->ext_mbs_service_area && !_ext_mbs_service_area_equal(a->ext_mbs_service_area, b->ext_mbs_service_area))
        return false;
    if (a->snssai && memcmp(a->snssai, b->snssai, sizeof(*a->snssai))) return false;
    if (a->mbs_service_info && !_mbs_service_info_equal(a->mbs_service_info, b->mbs_service_info)) return false;
    if (a->associated_session_id && !_associated_session_id_equal(a->associated_session_id, b->associated_session_id))
        return false;

    /* check array contents */
    if (!_mbs_fsa_ids_equal(&a->mbs_fsa_ids, &b->mbs_fsa_ids)) return false;

    return true;
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

bool _mbs_session_set_tmgi(_priv_mbs_session_t *session, _priv_tmgi_t *tmgi)
{
    if (!session || session->deleted) return false;
    if (session->session.tmgi != _priv_tmgi_to_public(tmgi)) {
        if (session->session.tmgi && tmgi && _tmgi_equal(tmgi, _priv_tmgi_from_public(session->session.tmgi))) return true;
        session->session.tmgi = _priv_tmgi_to_public(tmgi);
        if (tmgi) {
            _ref_count_sbi_object_t *old_sbi_obj = session->sbi_object;
            session->sbi_object = _ref_count_sbi_object_ref(tmgi->sbi_object);
            _ref_count_sbi_object_unref(old_sbi_obj);
            session->session.tmgi_req = false;
        }
    }
    return true;
}

bool _mbs_session_set_callback(_priv_mbs_session_t *session, mb_smf_sc_mbs_session_create_result_cb callback, void *data)
{
    if (!session || session->deleted) return false;
    session->create_result_cb = callback;
    session->create_result_cb_data = data;
    return true;
}

bool _mbs_session_push_changes(_priv_mbs_session_t *sess)
{
    if (!sess) return false;

    ogs_debug("Pushing changes for MbsSession [%p (%p)]", sess, _priv_mbs_session_to_public(sess));

    if (sess->deleted) {
        ogs_debug("Session has been deleted, removing...");
        _mbs_session_send_remove(sess);
        _context_remove_mbs_session(sess); // TODO: This drops the MBS session object before the removal request has been responded
        _mbs_session_delete(sess);         //       to. Move these two lines to client response for MBS Session removal?
        return true;
    }

    if (!sess->previous_session) {
        /* No cached copy of accepted MB-SMF version so this must be new, push it */
        ogs_debug("New MbsSession");
        _mbs_session_send_create(sess);
        return true;
    }

    if (!_mbs_session_public_equal(sess->previous_session, &sess->session)) {
        ogs_debug("MbsSession changed, updating");
        if (!mb_smf_sc_ssm_equal(sess->previous_session->ssm, sess->session.ssm)) {
            ogs_debug("SSM changed, recreating MbsSession");
            /* SSM address change */
            if (sess->previous_session && sess->previous_session->ssm) {
                /* remove session at old SSM */
                _mbs_session_send_remove(sess);
            }

            /* create new session (includes subscriptions) */
            _mbs_session_send_create(sess);

            return true;
        }

        if (!_tmgi_equal(_priv_tmgi_from_public_const(sess->previous_session->tmgi),
                         _priv_tmgi_from_public_const(sess->session.tmgi))) {
            ogs_debug("TMGI changed, recreating MbsSession");
            /* TMGI change */
            if (sess->previous_session && sess->previous_session->tmgi) {
                /* remove session for previous TMGI */
                _mbs_session_send_remove(sess);
            }

            /* create new session (includes subscriptions) */
            _mbs_session_send_create(sess);

            return true;
        }

        _mbs_session_send_update(sess);
    } else {
        ogs_debug("MbsSession [%p (%p)] not changed", sess, _priv_mbs_session_to_public(sess));
    }

    /* main session not changed, let's update the subscriptions */
    _mbs_session_subscriptions_update(sess);

    return true;
}

void _mbs_session_send_create(_priv_mbs_session_t *session)
{
    if (!session) return;

    ogs_debug("Send create for MbsSession [%p (%p)]", session, _priv_mbs_session_to_public(session));

    ogs_sbi_discovery_option_t *discovery_option = ogs_sbi_discovery_option_new();
    ogs_sbi_service_type_e service_type = OGS_SBI_SERVICE_TYPE_NMBSMF_MBS_SESSION;

    OGS_SBI_FEATURES_SET(discovery_option->requester_features, OGS_SBI_NNRF_DISC_SERVICE_MAP);
    ogs_sbi_discovery_option_add_service_names(discovery_option, (char *)ogs_sbi_service_type_to_name(service_type));

    if (session->session.tmgi) {
        ogs_sbi_discovery_option_add_target_plmn_list(discovery_option, &session->session.tmgi->plmn);
    }

    if (!session->sbi_object) session->sbi_object = _ref_count_sbi_object_new();

    ogs_sbi_object_t *sbi_object = _ref_count_sbi_object_ptr(session->sbi_object);

    ogs_sbi_xact_t *xact = ogs_sbi_xact_add(0, sbi_object, service_type, discovery_option,
                                            _nmbsmf_mbs_session_build_create, session, NULL);

    ogs_sbi_discover_and_send(xact);
}

void _mbs_session_send_update(_priv_mbs_session_t *session)
{
    if (!session) return;

    ogs_debug("Send update for MbsSession [%p (%p)]", session, _priv_mbs_session_to_public(session));

    ogs_sbi_service_type_e service_type = OGS_SBI_SERVICE_TYPE_NMBSMF_MBS_SESSION;

    ogs_sbi_object_t *sbi_object = _ref_count_sbi_object_ptr(session->sbi_object);
    ogs_sbi_xact_t *xact = ogs_sbi_xact_add(0, sbi_object, service_type, NULL,
                                            _nmbsmf_mbs_session_build_update, session, NULL);
    ogs_sbi_discover_and_send(xact);
}

void _mbs_session_send_remove(_priv_mbs_session_t *session)
{
    if (!session) return;

    ogs_debug("Send removal of MbsSession [%p (%p)]", session, _priv_mbs_session_to_public(session));

    ogs_sbi_service_type_e service_type = OGS_SBI_SERVICE_TYPE_NMBSMF_MBS_SESSION;

    /* Shouldn't need discovery options as we are talking to previously discovered MB-SMF (details in session->sbi_object) */

    ogs_sbi_object_t *sbi_object = _ref_count_sbi_object_ptr(session->sbi_object);
    ogs_sbi_xact_t *xact = ogs_sbi_xact_add(0, sbi_object, service_type, NULL,
                                            _nmbsmf_mbs_session_build_remove, session, NULL);

    ogs_sbi_discover_and_send(xact);
}

void _mbs_session_subscriptions_update(_priv_mbs_session_t *sess)
{
    /* Process subscription updates for the MBS Session */
    _priv_mbs_status_subscription_t *next, *node;

    /* Add new ones that haven't been requested yet */
    ogs_list_for_each_safe(&sess->new_subscriptions, next, node) {
        if (node->changed) _mbs_status_subscription_send_create(node);
        node->changed = false;
    }

    /* Do any updates to existing subscriptions */
    if (sess->active_subscriptions) {
        ogs_hash_do(__update_status_subscription, sess, sess->active_subscriptions);
    }

    /* Remove any subscriptions pending deletion */
    ogs_list_for_each_safe(&sess->deleted_subscriptions, next, node) {
        _mbs_status_subscription_send_delete(node);
        node->changed = false;
        // TODO: move this to response processing?
        ogs_list_remove(&sess->deleted_subscriptions, node);
        _mbs_status_subscription_delete(node);
    }
}

_priv_mbs_status_subscription_t *_mbs_session_find_active_subscription(const _priv_mbs_session_t *session, const char *id)
{
    if (!session || !id || session->deleted || !session->active_subscriptions) return NULL;
    return (_priv_mbs_status_subscription_t*)ogs_hash_get(session->active_subscriptions, id, OGS_HASH_KEY_STRING);
}

_priv_mbs_status_subscription_t *_mbs_session_find_subscription(const _priv_mbs_session_t *session, const char *correlation_id)
{
    if (!session || session->deleted) return NULL;

    _priv_mbs_status_subscription_t *subsc;
    ogs_list_for_each(&session->new_subscriptions, subsc) {
        if (!correlation_id && !subsc->correlation_id) return subsc;
        if (correlation_id && subsc->correlation_id && !strcmp(correlation_id, subsc->correlation_id)) return subsc;
    }

    __mbs_session_find_subsc_filter_t filter = {
        .correlation_id = correlation_id,
        .found = NULL
    };

    if (session->active_subscriptions) {
        if (!ogs_hash_do(__mbs_session_find_subsc_hash_do, &filter, session->active_subscriptions)) {
            return filter.found;
        }
    }

    return NULL;
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

/*========================== Local private functions ==========================*/

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

static int __mbs_session_find_subsc_hash_do(void *rec, const void *key, int klen, const void *value)
{
    _priv_mbs_status_subscription_t *subsc = (_priv_mbs_status_subscription_t*)value;
    __mbs_session_find_subsc_filter_t *filter = (__mbs_session_find_subsc_filter_t*)rec;
    if (!filter->correlation_id && !subsc->correlation_id) {
        filter->found = subsc;
        return 0;
    }
    if (filter->correlation_id && subsc->correlation_id && !strcmp(filter->correlation_id, subsc->correlation_id)) {
        filter->found = subsc;
        return 0;
    }
    return 1;
}

static int __update_status_subscription(void *rec, const void *key, int klen, const void *value)
{
    //_priv_mbs_session_t *sess = (_priv_mbs_session_t*)rec;
    _priv_mbs_status_subscription_t *subsc = (_priv_mbs_status_subscription_t*)value;
    if (subsc->changed) {
        _mbs_status_subscription_send_update(subsc);
        subsc->changed = false;
    }
    return 1;
}

static int __free_status_subscription(void *rec, const void *key, int klen, const void *value)
{
    ogs_hash_t *hash = (ogs_hash_t*)rec;
    _priv_mbs_status_subscription_t *subsc = (_priv_mbs_status_subscription_t*)value;
    ogs_hash_set(hash, key, klen, NULL);
    _mbs_status_subscription_delete(subsc);
    return 1;
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

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
