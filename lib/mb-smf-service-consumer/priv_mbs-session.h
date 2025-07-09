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
#include "mbs-session.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Data types */
typedef struct _priv_mbs_session_s {
    /* Enable use in ogs_list_t */
    ogs_lnode_t             node;

    /* Public part */
    mb_smf_sc_mbs_session_t session;

    /* private part */
    char                   *id;
    ogs_list_t              new_subscriptions;
    ogs_list_t              deleted_subscriptions;
    mb_smf_sc_ssm_addr_t    previous_ssm;
    mb_smf_sc_tmgi_t        previous_tmgi;
    bool                    changed;
    bool                    deleted;
    ogs_sbi_object_t        sbi_object;
} _priv_mbs_session_t;

mb_smf_sc_mbs_session_t *_priv_mbs_session_to_public(_priv_mbs_session_t *session);
const mb_smf_sc_mbs_session_t *_priv_mbs_session_to_public_const(const _priv_mbs_session_t *session);
_priv_mbs_session_t *_priv_mbs_session_from_public(mb_smf_sc_mbs_session_t *session);
const _priv_mbs_session_t *_priv_mbs_session_from_public_const(const mb_smf_sc_mbs_session_t *session);

void _mbs_session_delete(_priv_mbs_session_t *session);
bool _mbs_session_push_changes(_priv_mbs_session_t *session);
void _mbs_session_send_create(_priv_mbs_session_t *session);
void _mbs_session_send_update(_priv_mbs_session_t *session);
void _mbs_session_send_remove(_priv_mbs_session_t *session);
void _mbs_session_subscriptions_update(_priv_mbs_session_t *sess);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_SC_PRIV_MBS_SESSION_H_ */
