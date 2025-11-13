#ifndef _MB_SMF_SC_PRIV_MBS_SESSION_H_
#define _MB_SMF_SC_PRIV_MBS_SESSION_H_
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <stdbool.h>

#include "ogs-core.h"
#include "ogs-sbi.h"

#include "macros.h"
#include "json_patch.h"
#include "mbs-session.h"
#include "priv_tmgi.h"
#include "ref_count_sbi_object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Data types */
typedef struct _priv_mbs_status_subscription_s _priv_mbs_status_subscription_t;

typedef struct _priv_mbs_session_s {
    /* Enable use in ogs_list_t */
    ogs_lnode_t             node;

    /* Public part */
    mb_smf_sc_mbs_session_t session;

    /* private part */
    char                    *id;
    ogs_list_t               new_subscriptions;    /**< list of _priv_mbs_status_subscription_t that have not been created yet */
    ogs_hash_t              *active_subscriptions; /**< list of _priv_mbs_status_subscription_t indexed by their id */
    ogs_list_t               deleted_subscriptions;/**< list of _priv_mbs_status_subscription_t that are scheduled for deletion */
    mb_smf_sc_mbs_session_t *previous_session;     /**< Cached copy of the MBS Session as last accepted by the MB-SMF */
    mb_smf_sc_mbs_session_create_result_cb create_result_cb;
    void                    *create_result_cb_data;
    bool                     deleted;
    _ref_count_sbi_object_t *sbi_object;
} _priv_mbs_session_t;

static inline mb_smf_sc_mbs_session_t *_priv_mbs_session_to_public(_priv_mbs_session_t *session)
{
    if (session) return &session->session;
    return NULL;
}

static inline const mb_smf_sc_mbs_session_t *_priv_mbs_session_to_public_const(const _priv_mbs_session_t *session)
{
    if (session) return &session->session;
    return NULL;
}

static inline _priv_mbs_session_t *_priv_mbs_session_from_public(mb_smf_sc_mbs_session_t *session)
{
    if (session) return ogs_container_of(session, _priv_mbs_session_t, session);
    return NULL;
}

static inline const _priv_mbs_session_t *_priv_mbs_session_from_public_const(const mb_smf_sc_mbs_session_t *session)
{
    if (session) return ogs_container_of(session, _priv_mbs_session_t, session);
    return NULL;
}

_priv_mbs_session_t *_mbs_session_new();
void _mbs_session_delete(_priv_mbs_session_t *session);
void _mbs_session_public_clear(mb_smf_sc_mbs_session_t *session);
void _mbs_session_public_free(mb_smf_sc_mbs_session_t *session);
void _mbs_session_public_copy(mb_smf_sc_mbs_session_t **dst, const mb_smf_sc_mbs_session_t * const src);
bool _mbs_session_public_equal(const mb_smf_sc_mbs_session_t *a, const mb_smf_sc_mbs_session_t *b);
ogs_list_t *_mbs_session_public_patch_list(const mb_smf_sc_mbs_session_t *a, const mb_smf_sc_mbs_session_t *b);
ogs_list_t *_mbs_session_patch_list(const _priv_mbs_session_t *session);

bool _mbs_session_set_tmgi(_priv_mbs_session_t *session, _priv_tmgi_t *tmgi);
bool _mbs_session_set_callback(_priv_mbs_session_t *session, mb_smf_sc_mbs_session_create_result_cb callback, void *data);

bool _mbs_session_push_changes(_priv_mbs_session_t *session);
void _mbs_session_send_create(_priv_mbs_session_t *session);
void _mbs_session_send_update(_priv_mbs_session_t *session);
void _mbs_session_send_remove(_priv_mbs_session_t *session);

void _mbs_session_subscriptions_update(_priv_mbs_session_t *sess);
_priv_mbs_status_subscription_t *_mbs_session_find_active_subscription(const _priv_mbs_session_t *session, const char *id);
_priv_mbs_status_subscription_t *_mbs_session_find_subscription(const _priv_mbs_session_t *session, const char *correlation_id);
OpenAPI_mbs_session_id_t *_mbs_session_create_mbs_session_id(_priv_mbs_session_t *session);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_SC_PRIV_MBS_SESSION_H_ */
