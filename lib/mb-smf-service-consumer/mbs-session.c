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
#include "tmgi.h"
#include "mbs-status-subscription.h"
#include "nmbsmf-mbs-session-build.h"
#include "priv_mbs-session.h"
#include "priv_mbs-status-subscription.h"
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

/* SSM Type functions */
MB_SMF_CLIENT_API bool mb_smf_sc_ssm_equal(const mb_smf_sc_ssm_addr_t *a, const mb_smf_sc_ssm_addr_t *b)
{
    if (a == b) return true;

    if (!a || !b) return false;

    return memcmp(a, b, sizeof(*a)) == 0;
}

/* MBS Session Type functions */
MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new()
{
    mb_smf_sc_mbs_session_t *ret = mb_smf_sc_mbs_session_new_ipv4(NULL, NULL);
    ret->tmgi_req = true; /* No SSM or TMGI, so we'll make the default to request a TMGI */
    return ret;
}

MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new_ipv4(const struct in_addr *source, const struct in_addr *dest)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)ogs_calloc(1, sizeof(*session));

    session->changed = true;
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
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)ogs_calloc(1, sizeof(*session));

    session->changed = true;
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
    session->changed = true;
}

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_add_subscription(mb_smf_sc_mbs_session_t *session, mb_smf_sc_mbs_status_subscription_t *subscription)
{
    _priv_mbs_session_t *sess = _priv_mbs_session_from_public(session);
    _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public(subscription);

    if (!sess || !subscription || sess->deleted) return false;

    if (!session->subscriptions) {
        session->subscriptions = ogs_hash_make();
    }

    if (subsc->id) {
        // Remove from active list of current mbs session
        ogs_hash_set(subsc->session->session.subscriptions, subsc->id, OGS_HASH_KEY_STRING, NULL);

        _mbs_status_subscription_send_delete(subsc);

        // forget the assigned id
        ogs_free(subsc->id);
        subsc->id = NULL;
    }
    // Add to new subscription list
    ogs_list_add(&sess->new_subscriptions, subsc);
    subsc->session = sess;
    subsc->changed = true;
    sess->changed = true;

    return true;
}

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_remove_subscription(mb_smf_sc_mbs_session_t *session, mb_smf_sc_mbs_status_subscription_t *subscription)
{
    _priv_mbs_session_t *sess = _priv_mbs_session_from_public(session);
    _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public(subscription);

    if (!sess || !subsc || sess->deleted) return false;
    if (subsc->id) {
        // active subscription - move to deleted list
        ogs_hash_set(session->subscriptions, subsc->id, OGS_HASH_KEY_STRING, NULL);
        ogs_free(subsc->id);
        subsc->id = NULL;
        ogs_list_add(&sess->deleted_subscriptions, subsc);
        subsc->changed = true;
        sess->changed = true;
    } else {
        // if subscription is still in new list, just remove
        _priv_mbs_status_subscription_t *next, *node;
        ogs_list_for_each_safe(&sess->new_subscriptions, next, node) {
            if (node == subsc) {
                ogs_list_remove(&sess->new_subscriptions, node);
                _mbs_status_subscription_delete(node);
                subsc->changed = true;
                sess->changed = true;
                break;
            }
        }
    }

    return true;
}

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_tmgi(mb_smf_sc_mbs_session_t *session, mb_smf_sc_tmgi_t *tmgi)
{
    _priv_mbs_session_t *sess = _priv_mbs_session_from_public(session);
    _priv_tmgi_t *t = _priv_tmgi_from_public(tmgi);
    return _mbs_session_set_tmgi(sess, t);
}

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_tmgi_request(mb_smf_sc_mbs_session_t *session, bool request_tmgi)
{
    _priv_mbs_session_t *sess = _priv_mbs_session_from_public(session);
    return _mbs_session_set_tmgi_request(sess, request_tmgi);
}

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_tunnel_request(mb_smf_sc_mbs_session_t *session, bool request_udp_tunnel)
{
    _priv_mbs_session_t *sess = _priv_mbs_session_from_public(session);
    return _mbs_set_tunnel_request(sess, request_udp_tunnel);
}

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_service_type(mb_smf_sc_mbs_session_t *session,
                                                              mb_smf_sc_mbs_service_type_e service_type)
{
    _priv_mbs_session_t *sess = _priv_mbs_session_from_public(session);
    return _mbs_session_set_service_type(sess, service_type);
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

/* protected functions */

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

    if (!sess->id && sess->changed) {
        /* No ID and not already sent, this must be new, push it */
        ogs_debug("New MbsSession");
        sess->changed = false;
        _mbs_session_send_create(sess);
        return true;
    }

    if (!mb_smf_sc_ssm_equal(sess->previous_ssm, sess->session.ssm)) {
        ogs_debug("SSM changed, recreating MbsSession");
        /* SSM address change */
        if (sess->previous_ssm) {
            /* remove session at old SSM */
            _mbs_session_send_remove(sess);
        }

        /* create new session (includes subscriptions) */
        sess->changed = false;
        _mbs_session_send_create(sess);

        return true;
    }

    if (!_tmgi_equal(sess->previous_tmgi, _priv_tmgi_from_public_const(sess->session.tmgi))) {
        ogs_debug("TMGI changed, recreating MbsSession");
        /* TMGI change */
        if (sess->previous_tmgi) {
            /* remove session for previous TMGI */
            _mbs_session_send_remove(sess);
        }

        /* create new session (includes subscriptions) */
        sess->changed = false;
        _mbs_session_send_create(sess);

        return true;
    }

    if (!sess->changed) {
        ogs_debug("MbsSession [%p (%p)] not changed", sess, _priv_mbs_session_to_public(sess));
        return false;
    }

    sess->changed = false;

    /* main session not changed, let's update the subscriptions */
    _mbs_session_subscriptions_update(sess);

    return true;
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
    ogs_hash_do(__update_status_subscription, sess, sess->session.subscriptions);

    /* Remove any subscriptions pending deletion */
    ogs_list_for_each_safe(&sess->deleted_subscriptions, next, node) {
        _mbs_status_subscription_send_delete(node);
        node->changed = false;
        // TODO: move this to response processing?
        ogs_list_remove(&sess->deleted_subscriptions, node);
        _mbs_status_subscription_delete(node);
    }
}

void _mbs_session_delete(_priv_mbs_session_t *session)
{
    // mb_smf_sc_mbs_session_t session->session
    if (session->session.mb_upf_udp_tunnel) {
        ogs_freeaddrinfo(session->session.mb_upf_udp_tunnel);
        session->session.mb_upf_udp_tunnel = NULL;
    }

    _priv_mbs_status_subscription_t *next, *node;

    ogs_list_for_each_safe(&session->new_subscriptions, next, node) {
        ogs_list_remove(&session->new_subscriptions, node);
        _mbs_status_subscription_delete(node);
    }

    ogs_list_for_each_safe(&session->deleted_subscriptions, next, node) {
        ogs_list_remove(&session->new_subscriptions, node);
        _mbs_status_subscription_delete(node);
    }

    if (session->session.subscriptions) {
        ogs_hash_do(__free_status_subscription, session->session.subscriptions, session->session.subscriptions);
        ogs_hash_destroy(session->session.subscriptions);
        session->session.subscriptions = NULL;
    }

    if (session->session.ssm) {
        ogs_free(session->session.ssm);
        session->session.ssm = NULL;
    }

    /* session->session.tmgi: TMGI held elsewhere in context, don't free here */

    if (session->id) {
        ogs_free(session->id);
        session->id = NULL;
    }

    if (session->previous_ssm) {
        ogs_free(session->previous_ssm);
        session->previous_ssm = NULL;
    }

    if (session->previous_tmgi) {
        _tmgi_free(session->previous_tmgi);
        session->previous_tmgi = NULL;
    }

    if (session->sbi_object) {
        _ref_count_sbi_object_unref(session->sbi_object);
        session->sbi_object = NULL;
    }

    // session
    ogs_free(session);
}

bool _mbs_session_set_tmgi(_priv_mbs_session_t *session, _priv_tmgi_t *tmgi)
{
    if (!session || session->deleted) return false;
    if (session->session.tmgi != _priv_tmgi_to_public(tmgi)) {
        if (session->session.tmgi && tmgi && _tmgi_equal(tmgi, _priv_tmgi_from_public(session->session.tmgi))) return true;
        if (session->session.tmgi) _tmgi_free(_priv_tmgi_from_public(session->session.tmgi));
        session->session.tmgi = _priv_tmgi_to_public(tmgi);
        if (tmgi) {
            _ref_count_sbi_object_unref(session->sbi_object);
            session->sbi_object = _ref_count_sbi_object_ref(tmgi->sbi_object);
            session->session.tmgi_req = false;
        }
    }
    return true;
}

bool _mbs_session_set_tmgi_request(_priv_mbs_session_t *session, bool request_tmgi)
{
    if (!session || session->deleted) return false;

    if (session->session.tmgi_req == request_tmgi) return true;

    session->session.tmgi_req = request_tmgi;
    if (request_tmgi) {
        /* forget the old TMGI if we are requesting a new one */
        session->session.tmgi = NULL;
    }
    session->changed = true;

    return true;
}

bool _mbs_set_tunnel_request(_priv_mbs_session_t *session, bool request_udp_tunnel)
{
    if (!session || session->deleted) return false;

    if (session->session.tunnel_req == request_udp_tunnel) return false;

    session->session.tunnel_req = request_udp_tunnel;
    session->changed = true;

    return true;
}

bool _mbs_session_set_service_type(_priv_mbs_session_t *session, mb_smf_sc_mbs_service_type_e service_type)
{
    if (!session || session->deleted) return false;

    if (session->session.service_type == service_type) return false;

    session->session.service_type = service_type;
    session->changed = true;

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
    ogs_warn("TODO: _mbs_session_send_update(%p (%p))", session, _priv_mbs_session_to_public(session));
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

_priv_mbs_status_subscription_t *_mbs_session_find_subscription(_priv_mbs_session_t *session, const char *correlation_id)
{
    if (!session) return NULL;

    _priv_mbs_status_subscription_t *subsc;
    ogs_list_for_each(&session->new_subscriptions, subsc) {
        if (!correlation_id && !subsc->correlation_id) return subsc;
        if (correlation_id && subsc->correlation_id && !strcmp(correlation_id, subsc->correlation_id)) return subsc;
    }

    __mbs_session_find_subsc_filter_t filter = {
        .correlation_id = correlation_id,
        .found = NULL
    };

    if (session->session.subscriptions) {
        if (!ogs_hash_do(__mbs_session_find_subsc_hash_do, &filter, session->session.subscriptions)) {
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

/* Private functions */

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

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
