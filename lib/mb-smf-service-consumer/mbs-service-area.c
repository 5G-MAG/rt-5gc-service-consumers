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
#include "priv_ncgi-tai.h"
#include "priv_tai.h"

#include "mbs-service-area.h"
#include "priv_mbs-service-area.h"

/* mb_smf_sc_mbs_service_area Type functions */
MB_SMF_CLIENT_API mb_smf_sc_mbs_service_area_t *mb_smf_sc_mbs_service_area_new()
{
    return _mbs_service_area_new();
}

MB_SMF_CLIENT_API mb_smf_sc_mbs_service_area_t *mb_smf_sc_mbs_service_area_new_ncgi_tai(const mb_smf_sc_ncgi_tai_t *ncgi_tai)
{
    mb_smf_sc_mbs_service_area_t *ret = _mbs_service_area_new();
    mb_smf_sc_ncgi_tai_t *new_ncgi_tai = NULL;
    _ncgi_tai_copy(&new_ncgi_tai, ncgi_tai);
    if (new_ncgi_tai) ogs_list_add(&ret->ncgi_tais, new_ncgi_tai);
    return ret;
}

MB_SMF_CLIENT_API mb_smf_sc_mbs_service_area_t *mb_smf_sc_mbs_service_area_new_tai(const mb_smf_sc_tai_t *tai)
{
    mb_smf_sc_mbs_service_area_t *ret = _mbs_service_area_new();
    mb_smf_sc_tai_t *new_tai = NULL;
    _tai_copy(&new_tai, tai);
    if (new_tai) ogs_list_add(&ret->tais, new_tai);
    return ret;
}

MB_SMF_CLIENT_API void mb_smf_sc_mbs_service_area_delete(mb_smf_sc_mbs_service_area_t *mbs_service_area)
{
    _mbs_service_area_free(mbs_service_area);
}

/* Library internal mbs_service_area methods (protected) */
mb_smf_sc_mbs_service_area_t *_mbs_service_area_new()
{
    return (mb_smf_sc_mbs_service_area_t*)ogs_calloc(1,sizeof(mb_smf_sc_mbs_service_area_t));
}

void _mbs_service_area_free(mb_smf_sc_mbs_service_area_t *svc_areas)
{
    if (!svc_areas) return;
    _mbs_service_area_clear(svc_areas);
    ogs_free(svc_areas);
}

void _mbs_service_area_clear(mb_smf_sc_mbs_service_area_t *svc_areas)
{
    if (!svc_areas) return;

    mb_smf_sc_ncgi_tai_t *ncgi_tai, *next_ncgi_tai;
    mb_smf_sc_tai_t *tai, *next_tai;

    ogs_list_for_each_safe(&svc_areas->ncgi_tais, next_ncgi_tai, ncgi_tai) {
        ogs_list_remove(&svc_areas->ncgi_tais, ncgi_tai);
        _ncgi_tai_free(ncgi_tai);
    }
    ogs_list_for_each_safe(&svc_areas->tais, next_tai, tai) {
        ogs_list_remove(&svc_areas->tais, tai);
        _tai_free(tai);
    }
}

void _mbs_service_area_copy(mb_smf_sc_mbs_service_area_t **dst, const mb_smf_sc_mbs_service_area_t *src)
{
    mb_smf_sc_ncgi_tai_t *ncgi_tai;
    mb_smf_sc_tai_t *tai;

    if (!src) {
        _mbs_service_area_free(*dst);
        *dst = NULL;
        return;
    }

    if (!*dst) {
        *dst = _mbs_service_area_new();
    } else {
        /* clear old service areas lists */
        _mbs_service_area_clear(*dst);
    }

    /* copy src to dst */
    ogs_list_for_each(&src->ncgi_tais, ncgi_tai) {
        mb_smf_sc_ncgi_tai_t *new_ncgi_tai = NULL;
        _ncgi_tai_copy(&new_ncgi_tai, ncgi_tai);
        ogs_list_add(&((*dst)->ncgi_tais), new_ncgi_tai);
    }
    ogs_list_for_each(&src->tais, tai) {
        mb_smf_sc_tai_t *new_tai = NULL;
        _tai_copy(&new_tai, tai);
        ogs_list_add(&((*dst)->tais), new_tai);
    }
}

bool _mbs_service_area_equal(const mb_smf_sc_mbs_service_area_t *a, const mb_smf_sc_mbs_service_area_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    /* check array sizes */
    if (ogs_list_count(&a->ncgi_tais) != ogs_list_count(&b->ncgi_tais)) return false;
    if (ogs_list_count(&a->tais) != ogs_list_count(&b->tais)) return false;

    /* check array contents (any order matching) */
    mb_smf_sc_mbs_service_area_t *b_copy = NULL;
    _mbs_service_area_copy(&b_copy, b);
    mb_smf_sc_ncgi_tai_t *a_ncgi_tai;
    ogs_list_for_each(&a->ncgi_tais, a_ncgi_tai) {
        mb_smf_sc_ncgi_tai_t *next, *b_ncgi_tai;
        bool found = false;
        ogs_list_for_each_safe(&b_copy->ncgi_tais, next, b_ncgi_tai) {
            if (_ncgi_tai_equal(a_ncgi_tai, b_ncgi_tai)) {
                ogs_list_remove(&b_copy->ncgi_tais, b_ncgi_tai);
                _ncgi_tai_free(b_ncgi_tai);
                found = true;
                break;
            }
        }
        if (!found) {
            _mbs_service_area_free(b_copy);
            return false;
        }
    }
    mb_smf_sc_tai_t *a_tai;
    ogs_list_for_each(&a->tais, a_tai) {
        mb_smf_sc_tai_t *next, *b_tai;
        bool found = false;
        ogs_list_for_each_safe(&b_copy->tais, next, b_tai) {
            if (_tai_equal(a_tai, b_tai)) {
                ogs_list_remove(&b_copy->tais, b_tai);
                _tai_free(b_tai);
                found = true;
                break;
            }
        }
        if (!found) {
            _mbs_service_area_free(b_copy);
            return false;
        }
    }
    _mbs_service_area_free(b_copy);
    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
