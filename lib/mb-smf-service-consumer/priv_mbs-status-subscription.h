#ifndef _MB_SMF_SC_PRIV_MBS_STATUS_SUBSCRIPTION_H_
#define _MB_SMF_SC_PRIV_MBS_STATUS_SUBSCRIPTION_H_
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
#include "mbs-status-subscription.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _priv_mbs_session_s _priv_mbs_session_t;

/* Data types */
typedef struct _priv_mbs_status_subscription_s {
    /* Enable use in ogs_list_t */
    ogs_lnode_t             node;

    /* Public part */
    uint16_t                              area_session_id;
    int                                   flags;
    char                                 *correlation_id;
    time_t                                expiry_time;
    mb_smf_sc_mbs_status_notification_cb  callback;
    void                                 *callback_data;

    /* private part */
    char                       *id;
    _priv_mbs_session_t        *session;
    bool                        changed;
    struct {
        char             *repr_string;
        ogs_sbi_server_t *notif_server;
        char             *notif_url;
    }                          *cache;
} _priv_mbs_status_subscription_t;

mb_smf_sc_mbs_status_subscription_t *_priv_mbs_status_subscription_to_public(_priv_mbs_status_subscription_t *subscription);
_priv_mbs_status_subscription_t *_priv_mbs_status_subscription_from_public(mb_smf_sc_mbs_status_subscription_t *subscription);
const _priv_mbs_status_subscription_t *_priv_mbs_status_subscription_from_public_const(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription);

void _mbs_status_subscription_delete(_priv_mbs_status_subscription_t *subscription);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_SC_PRIV_MBS_STATUS_SUBSCRIPTION_H_ */
