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

#include "macros.h"
#include "priv_mbs-media-comp.h"

#include "mbs-service-info.h"
#include "priv_mbs-service-info.h"

/* mb_smf_sc_mbs_service_info Type functions */
MB_SMF_CLIENT_API mb_smf_sc_mbs_service_info_t *mb_smf_sc_mbs_service_info_new()
{
    return _mbs_service_info_new();
}

MB_SMF_CLIENT_API void mb_smf_sc_mbs_service_info_delete(mb_smf_sc_mbs_service_info_t *service_info)
{
    _mbs_service_info_free(service_info);
}

/* Library internal mbs_service_info methods (protected) */
mb_smf_sc_mbs_service_info_t *_mbs_service_info_new()
{
    return (mb_smf_sc_mbs_service_info_t*)ogs_calloc(1, sizeof(mb_smf_sc_mbs_service_info_t));
}

void _mbs_service_info_free(mb_smf_sc_mbs_service_info_t *service_info)
{
    if (!service_info) return;
    _mbs_service_info_clear(service_info);
    ogs_free(service_info);
}

void _mbs_service_info_clear(mb_smf_sc_mbs_service_info_t *mbs_svc_info)
{
    if (!mbs_svc_info) return;
    if (mbs_svc_info->mbs_media_comps) {
        ogs_hash_index_t *idx = ogs_hash_index_make(mbs_svc_info->mbs_media_comps);
        ogs_hash_index_t *it = idx;
        for (it = ogs_hash_next(it); it; it = ogs_hash_next(it)) {
            mb_smf_sc_mbs_media_comp_t *media_comp = (mb_smf_sc_mbs_media_comp_t*)ogs_hash_this_val(it);
            ogs_hash_set(mbs_svc_info->mbs_media_comps, &media_comp->id, sizeof(media_comp->id), NULL);
            _mbs_media_comp_free(media_comp);
        }
        ogs_free(idx);
        ogs_hash_destroy(mbs_svc_info->mbs_media_comps);
        mbs_svc_info->mbs_media_comps = NULL;
    }
    if (mbs_svc_info->af_app_id) {
        ogs_free(mbs_svc_info->af_app_id);
        mbs_svc_info->af_app_id = NULL;
    }
    if (mbs_svc_info->mbs_session_ambr) {
        ogs_free(mbs_svc_info->mbs_session_ambr);
        mbs_svc_info->mbs_session_ambr = NULL;
    }
    mbs_svc_info->mbs_sdf_reserve_priority = 0;
}

void _mbs_service_info_copy(mb_smf_sc_mbs_service_info_t **dst, const mb_smf_sc_mbs_service_info_t *src)
{
    if (!src) {
        _mbs_service_info_free(*dst);
        *dst = NULL;
        return;
    }

    if (!*dst) {
        *dst = _mbs_service_info_new();
    } else {
        /* clear old service info */
        _mbs_service_info_clear(*dst);
    }

    mb_smf_sc_mbs_service_info_t *dest = *dst;

    if (src->mbs_media_comps) {
        ogs_hash_index_t *idx = ogs_hash_index_make(src->mbs_media_comps);
        ogs_hash_index_t *it = idx;
        dest->mbs_media_comps = ogs_hash_make();
        for (it = ogs_hash_next(it); it; it = ogs_hash_next(it)) {
            mb_smf_sc_mbs_media_comp_t *media_comp, *new_media_comp = NULL;
            media_comp = (mb_smf_sc_mbs_media_comp_t*)ogs_hash_this_val(it);
            _mbs_media_comp_copy(&new_media_comp, media_comp);
            ogs_hash_set(dest->mbs_media_comps, &new_media_comp->id, sizeof(new_media_comp->id), new_media_comp);
        }
        ogs_free(idx);
    }
    if (src->af_app_id) {
        dest->af_app_id = ogs_strdup(src->af_app_id);
    }
    if (src->mbs_session_ambr) {
        dest->mbs_session_ambr = (uint64_t*)ogs_malloc(sizeof(*dest->mbs_session_ambr));
        *dest->mbs_session_ambr = *src->mbs_session_ambr;
    }
    dest->mbs_sdf_reserve_priority = src->mbs_sdf_reserve_priority;
}

bool _mbs_service_info_equal(const mb_smf_sc_mbs_service_info_t *a, const mb_smf_sc_mbs_service_info_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    /* simple types */
    if (a->mbs_sdf_reserve_priority != b->mbs_sdf_reserve_priority) return false;

    /* pointers for optional types: not equal if one NULL and the other not */
    if (!a->mbs_media_comps != !b->mbs_media_comps) return false;
    if (!a->af_app_id != !b->af_app_id) return false;
    if (!a->mbs_session_ambr != !b->mbs_session_ambr) return false;

    /* check optional values if set */
    if (a->mbs_session_ambr && *a->mbs_session_ambr != *b->mbs_session_ambr) return false;
    if (a->af_app_id && strcmp(a->af_app_id, b->af_app_id)) return false;
    if (a->mbs_media_comps) {
        if (ogs_hash_count(a->mbs_media_comps) != ogs_hash_count(b->mbs_media_comps)) return false;
        ogs_hash_index_t *idx = ogs_hash_index_make(a->mbs_media_comps);
        ogs_hash_index_t *it = idx;
        for (it = ogs_hash_next(it); it; it = ogs_hash_next(it)) {
            const void *key;
            int key_len;
            mb_smf_sc_mbs_media_comp_t *value;
            ogs_hash_this(it, &key, &key_len, (void**)&value);
            if (!_mbs_media_comp_equal(value, (mb_smf_sc_mbs_media_comp_t*)ogs_hash_get(b->mbs_media_comps, key, key_len))) break;
        }
        ogs_free(idx);
        if (it) return false;
    }

    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
