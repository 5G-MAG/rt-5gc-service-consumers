/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <stdint.h>

#include "ogs-core.h"
#include "ogs-proto.h"

#include "macros.h"

#include "ncgi.h"
#include "priv_ncgi.h"

/* mb_smf_sc_ncgi Type functions */
MB_SMF_CLIENT_API mb_smf_sc_ncgi_t *mb_smf_sc_ncgi_new()
{
    return _ncgi_new();
}

MB_SMF_CLIENT_API mb_smf_sc_ncgi_t *mb_smf_sc_ncgi_new_values(uint16_t mcc, uint16_t mnc, uint64_t nr_cell_id, const uint64_t *network_id)
{
    mb_smf_sc_ncgi_t *ret = _ncgi_new();
    ogs_plmn_id_build(&ret->plmn_id, mcc, mnc, mnc<100?2:3);
    ret->nr_cell_id = nr_cell_id;
    if (network_id) {
        ret->nid = (uint64_t*)ogs_malloc(sizeof(*ret->nid));
        *ret->nid = *network_id;
    }

    return ret;
}

MB_SMF_CLIENT_API mb_smf_sc_ncgi_t *mb_smf_sc_ncgi_new_copy(const mb_smf_sc_ncgi_t *other)
{
    mb_smf_sc_ncgi_t *ret = NULL;
    _ncgi_copy(&ret, other);
    return ret;
}

MB_SMF_CLIENT_API void mb_smf_sc_ncgi_delete(mb_smf_sc_ncgi_t *ncgi)
{
    _ncgi_free(ncgi);
}

MB_SMF_CLIENT_API mb_smf_sc_ncgi_t *mb_smf_sc_ncgi_set_plmn_id(mb_smf_sc_ncgi_t *ncgi, uint16_t mcc, uint16_t mnc)
{
    if (ncgi) {
        ogs_plmn_id_build(&ncgi->plmn_id, mcc, mnc, mnc<100?2:3);
    }
    return ncgi;
}

/* Library internal ncgi methods (protected) */
mb_smf_sc_ncgi_t *_ncgi_new()
{
    return (mb_smf_sc_ncgi_t*)ogs_calloc(1, sizeof(mb_smf_sc_ncgi_t));
}

void _ncgi_free(mb_smf_sc_ncgi_t *ncgi)
{
    if (!ncgi) return;
    _ncgi_clear(ncgi);
    ogs_free(ncgi);
}

void _ncgi_clear(mb_smf_sc_ncgi_t *ncgi)
{
    if (!ncgi) return;
    memset(&ncgi->plmn_id, 0, sizeof(*ncgi) - sizeof(ogs_lnode_t));
}

void _ncgi_copy(mb_smf_sc_ncgi_t **dst, const mb_smf_sc_ncgi_t *src)
{
    if (!src) {
        if (*dst) {
            _ncgi_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = _ncgi_new();
    } else {
        _ncgi_clear(*dst);
    }

    memcpy(&(*dst)->plmn_id, &src->plmn_id, sizeof(*src) - sizeof(ogs_lnode_t));
}

bool _ncgi_equal(const mb_smf_sc_ncgi_t *a, const mb_smf_sc_ncgi_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    if (a->nr_cell_id != b->nr_cell_id) return false;
    if (memcmp(&a->plmn_id, &b->plmn_id, sizeof(b->plmn_id))) return false;
    if (!a->nid != !b->nid) return false;
    if (a->nid && *a->nid != *b->nid) return false;

    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
