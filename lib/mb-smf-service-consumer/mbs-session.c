/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <netinet/in.h>

#include "ogs-core.h"

#include "macros.h"
#include "context.h"
#include "mbs-status-subscription.h"
#include "nmbsmf-mbs-session-build.h"
#include "priv_mbs-session.h"
#include "priv_mbs-status-subscription.h"

#include "mbs-session.h"

/* Type functions */
MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session()
{
    return mb_smf_sc_mbs_session_new_ipv4(NULL, NULL);
}

MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new_ipv4(const struct in_addr *source, const struct in_addr *dest)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)ogs_calloc(1, sizeof(*session));

    session->changed = true;
    if (source) {
        memcpy(&session->session.ssm->source.ipv4, source, sizeof(session->session.ssm->source.ipv4));
    }
    if (dest) {
        memcpy(&session->session.ssm->dest_mc.ipv4, dest, sizeof(session->session.ssm->dest_mc.ipv4));
    }

    return _priv_mbs_session_to_public(session);
}

MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new_ipv6(const struct in6_addr *source, const struct in6_addr *dest)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)ogs_calloc(1, sizeof(*session));

    session->changed = true;
    if (source) {
        memcpy(&session->session.ssm->source.ipv6, source, sizeof(session->session.ssm->source.ipv6));
    }
    if (dest) {
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
        // TODO: send remove subscription
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

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_push_changes(mb_smf_sc_mbs_session_t *session)
{
    _priv_mbs_session_t *sess = _priv_mbs_session_from_public(session);
    return _mbs_session_push_changes(sess);
}

/* protected functions */

mb_smf_sc_mbs_session_t *_priv_mbs_session_to_public(_priv_mbs_session_t *session) {
    return &session->session;
}

const mb_smf_sc_mbs_session_t *_priv_mbs_session_to_public_const(const _priv_mbs_session_t *session) {
    return &session->session;
}

_priv_mbs_session_t *_priv_mbs_session_from_public(mb_smf_sc_mbs_session_t *session) {
    if (!session) return NULL;
    return ogs_container_of(session, _priv_mbs_session_t, session);
}

const _priv_mbs_session_t *_priv_mbs_session_from_public_const(const mb_smf_sc_mbs_session_t *session) {
    if (!session) return NULL;
    return ogs_container_of(session, _priv_mbs_session_t, session);
}

bool _mbs_session_push_changes(_priv_mbs_session_t *sess)
{
    if (!sess) return false;

    if (sess->deleted) {
        _context_remove_mbs_session(sess);
        _mbs_session_send_remove(sess);
        return true;
    }

    if (memcmp(&sess->previous_ssm, sess->session.ssm, sizeof(*sess->session.ssm))!=0) {
        /* SSM address change */
        if (sess->previous_ssm.family != 0) {
            /* remove session at old SSM */
            _mbs_session_send_remove(sess);
        }

        if (sess->session.ssm->family != 0) {
            /* create new session */
            _mbs_session_send_create(sess);
        }

        return true;
    }

    if (!sess->changed) return false;

    sess->changed = false;

    _mbs_session_subscriptions_update(sess);

    return true;
}

static int __update_status_subscription(void *rec, const void *key, int klen, const void *value)
{
    _priv_mbs_session_t *sess = (_priv_mbs_session_t*)rec;
    _priv_mbs_status_subscription_t *subsc = (_priv_mbs_status_subscription_t*)value;
    if (subsc->changed) {
        // TODO: Send update subscription
        subsc->changed = false;
    }
    return 1;
}

void _mbs_session_subscriptions_update(_priv_mbs_session_t *sess)
{
    _priv_mbs_status_subscription_t *next, *node;

    ogs_list_for_each_safe(&sess->new_subscriptions, next, node) {
        // TODO: Send create status subscription
        node->changed = false;
        // Await response then:
        // ogs_list_remove(&sess->new_subscriptions, node);
        // ogs_hash_set(sess->session.subscriptions, node->id, OGS_HASH_KEY_STRING, node);
    }

    ogs_hash_do(__update_status_subscription, sess, sess->session.subscriptions);

    ogs_list_for_each_safe(&sess->deleted_subscriptions, next, node) {
        // TODO: Send delete status subscription
        node->changed = false;
        ogs_list_remove(&sess->deleted_subscriptions, node);
        _mbs_status_subscription_delete(node);
    }
}

static int __free_status_subscription(void *rec, const void *key, int klen, const void *value)
{
    ogs_hash_t *hash = (ogs_hash_t*)rec;
    _priv_mbs_status_subscription_t *subsc = (_priv_mbs_status_subscription_t*)value;
    ogs_hash_set(hash, key, klen, NULL);
    _mbs_status_subscription_delete(subsc);
    return 1;
}

void _mbs_session_delete(_priv_mbs_session_t *session)
{
    // mb_smf_sc_mbs_session_t session->session
    if (session->session.mb_upf_udp_tunnel) {
        ogs_freeaddrinfo(session->session.mb_upf_udp_tunnel);
        session->session.mb_upf_udp_tunnel = NULL;
    }

    if (session->session.subscriptions) {
        ogs_hash_do(__free_status_subscription, session->session.subscriptions, session->session.subscriptions);
        ogs_hash_destroy(session->session.subscriptions);
        session->session.subscriptions = NULL;
    }

    // session
    ogs_free(session);
}

void _mbs_session_send_create(_priv_mbs_session_t *session)
{
    if (!session) return;

    ogs_sbi_discovery_option_t *discovery_option = NULL;
    ogs_sbi_service_type_e service_type = OGS_SBI_SERVICE_TYPE_NMBSMF_MBS_SESSION;

    if (session->session.tmgi) {
        discovery_option = ogs_sbi_discovery_option_new();
        OGS_SBI_FEATURES_SET(discovery_option->requester_features, OGS_SBI_NNRF_DISC_SERVICE_MAP);
        ogs_sbi_discovery_option_add_service_names(discovery_option, (char *)ogs_sbi_service_type_to_name(service_type));
        ogs_sbi_discovery_option_add_target_plmn_list(discovery_option, &session->session.tmgi->plmn);
    }

    ogs_sbi_xact_t *xact = ogs_sbi_xact_add(0, &session->sbi_object, service_type, discovery_option,
                                            _nmbsmf_mbs_session_build_create, session, NULL);

    ogs_sbi_discover_and_send(xact);
}

void _mbs_session_send_update(_priv_mbs_session_t *session)
{
}

void _mbs_session_send_remove(_priv_mbs_session_t *session)
{
    if (!session) return;

    ogs_sbi_service_type_e service_type = OGS_SBI_SERVICE_TYPE_NMBSMF_MBS_SESSION;

    ogs_sbi_xact_t *xact = ogs_sbi_xact_add(0, &session->sbi_object, service_type, NULL,
                                            _nmbsmf_mbs_session_build_remove, session, NULL);

    ogs_sbi_discover_and_send(xact);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
