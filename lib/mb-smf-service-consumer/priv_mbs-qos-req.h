#ifndef _MB_SMF_SC_PRIV_MBS_QOS_REQ_H
#define _MB_SMF_SC_PRIV_MBS_QOS_REQ_H
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include "macros.h"

#include "mbs-qos-req.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library internal mbs_qos_req methods (protected) */
mb_smf_sc_mbs_qos_req_t *_mbs_qos_req_new();
void _mbs_qos_req_free(mb_smf_sc_mbs_qos_req_t *mbs_qos_req);
void _mbs_qos_req_clear(mb_smf_sc_mbs_qos_req_t *mbs_qos_req);
void _mbs_qos_req_copy(mb_smf_sc_mbs_qos_req_t **dst, const mb_smf_sc_mbs_qos_req_t *src);
bool _mbs_qos_req_equal(const mb_smf_sc_mbs_qos_req_t *a, const mb_smf_sc_mbs_qos_req_t *b);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_MBS_QOS_REQ_H */
