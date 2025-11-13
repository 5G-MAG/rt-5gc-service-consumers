/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include "ogs-core.h"

#include "macros.h"
#include "ssm-addr.h"
#include "priv_ssm-addr.h"

#include "associated-session-id.h"
#include "priv_associated-session-id.h"

/* mb_smf_sc_associated_session_id Type functions */

MB_SMF_CLIENT_API mb_smf_sc_associated_session_id_t *mb_smf_sc_associated_session_id_new()
{
    return _associated_session_id_new();
}

MB_SMF_CLIENT_API void mb_smf_sc_associated_session_id_delete(mb_smf_sc_associated_session_id_t *session_id)
{
    _associated_session_id_free(session_id);
}

/* Library internal associated_session_id methods (protected) */
mb_smf_sc_associated_session_id_t *_associated_session_id_new()
{
    return (mb_smf_sc_associated_session_id_t*)ogs_calloc(1, sizeof(mb_smf_sc_associated_session_id_t));
}

void _associated_session_id_free(mb_smf_sc_associated_session_id_t *associated_session_id)
{
    if (!associated_session_id) return;
    _associated_session_id_clear(associated_session_id);
    ogs_free(associated_session_id);
}

void _associated_session_id_clear(mb_smf_sc_associated_session_id_t *associated_session_id)
{
    if (!associated_session_id) return;

    _ssm_addr_clear(&associated_session_id->ssm);
    if (associated_session_id->string) {
        ogs_free(associated_session_id->string);
        associated_session_id->string = NULL;
    }
}

void _associated_session_id_copy(mb_smf_sc_associated_session_id_t **dst, const mb_smf_sc_associated_session_id_t *src)
{
    if (!src) {
        if (*dst) {
            _associated_session_id_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = _associated_session_id_new();
    } else {
        _associated_session_id_clear(*dst);
    }

    mb_smf_sc_ssm_addr_t *dst_ssm = &(*dst)->ssm;
    _ssm_addr_copy(&dst_ssm, &src->ssm);
    if (src->string) (*dst)->string = ogs_strdup(src->string);
}

bool _associated_session_id_equal(const mb_smf_sc_associated_session_id_t *a, const mb_smf_sc_associated_session_id_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    /* check pointers set for fields */
    if (!a->string != !b->string) return false;

    if (a->string && strcmp(a->string, b->string)) return false;
    if (!_ssm_addr_equal(&a->ssm, &b->ssm)) return false;

    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
