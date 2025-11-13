#ifndef _MB_SMF_SC_PRIV_ASSOCIATED_SESSION_ID_H
#define _MB_SMF_SC_PRIV_ASSOCIATED_SESSION_ID_H
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

#include "associated-session-id.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library internal associated_session_id methods (protected) */
mb_smf_sc_associated_session_id_t *_associated_session_id_new();
void _associated_session_id_free(mb_smf_sc_associated_session_id_t *associated_session_id);
void _associated_session_id_clear(mb_smf_sc_associated_session_id_t *associated_session_id);
void _associated_session_id_copy(mb_smf_sc_associated_session_id_t **dst, const mb_smf_sc_associated_session_id_t *src);
bool _associated_session_id_equal(const mb_smf_sc_associated_session_id_t *a, const mb_smf_sc_associated_session_id_t *b);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_ASSOCIATED_SESSION_ID_H */
