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
#include "priv_ncgi.h"
#include "priv_tai.h"

#include "ncgi-tai.h"
#include "priv_ncgi-tai.h"

/* mb_smf_sc_ncgi_tai Type functions */
MB_SMF_CLIENT_API mb_smf_sc_ncgi_tai_t *mb_smf_sc_ncgi_tai_new()
{
    return _ncgi_tai_new();
}

MB_SMF_CLIENT_API mb_smf_sc_ncgi_tai_t *mb_smf_sc_ncgi_tai_new_values(const mb_smf_sc_tai_t *tai, const mb_smf_sc_ncgi_t *ncgi)
{
    mb_smf_sc_ncgi_tai_t *ret = _ncgi_tai_new();

    if (tai) {
        mb_smf_sc_tai_t *dst_tai = &ret->tai;
        _tai_copy(&dst_tai, tai);
    }

    if (ncgi) {
        mb_smf_sc_ncgi_t *new_ncgi = NULL;
        _ncgi_copy(&new_ncgi, ncgi);
        ogs_list_add(&ret->ncgis, new_ncgi);
    }

    return ret;
}

MB_SMF_CLIENT_API mb_smf_sc_ncgi_tai_t *mb_smf_sc_ncgi_tai_new_copy(const mb_smf_sc_ncgi_tai_t *other)
{
    mb_smf_sc_ncgi_tai_t *ret = NULL;
    _ncgi_tai_copy(&ret, other);
    return ret;
}

MB_SMF_CLIENT_API void mb_smf_sc_ncgi_tai_delete(mb_smf_sc_ncgi_tai_t *ncgi_tai)
{
    _ncgi_tai_free(ncgi_tai);
}

/* Library internal ncgi_tai methods (protected) */
mb_smf_sc_ncgi_tai_t *_ncgi_tai_new()
{
    return (mb_smf_sc_ncgi_tai_t*)ogs_calloc(1, sizeof(mb_smf_sc_ncgi_tai_t));
}

void _ncgi_tai_free(mb_smf_sc_ncgi_tai_t *ncgi_tai)
{
    if (!ncgi_tai) return;
    _ncgi_tai_clear(ncgi_tai);
    ogs_free(ncgi_tai);
}

void _ncgi_tai_clear(mb_smf_sc_ncgi_tai_t *ncgi_tai)
{
    if (!ncgi_tai) return;

    _tai_clear(&ncgi_tai->tai);

    /* TODO: clear ncgi list */
}

void _ncgi_tai_copy(mb_smf_sc_ncgi_tai_t **dst, const mb_smf_sc_ncgi_tai_t *src)
{
    if (!src) {
        if (*dst) {
            _ncgi_tai_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = _ncgi_tai_new();
    } else {
        _ncgi_tai_clear(*dst);
    }

    mb_smf_sc_tai_t *dst_tai = &(*dst)->tai;
    _tai_copy(&dst_tai, &src->tai);
    ogs_assert(dst_tai == &(*dst)->tai);

    /* TODO: copy ncgi list */
}

bool _ncgi_tai_equal(const mb_smf_sc_ncgi_tai_t *a, const mb_smf_sc_ncgi_tai_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    if (!_tai_equal(&a->tai, &b->tai)) return false;
    /* TODO: compare ncgi arrays */

    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
