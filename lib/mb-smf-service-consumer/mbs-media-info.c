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

#include "mbs-media-info.h"
#include "priv_mbs-media-info.h"

/* mb_smf_sc_mbs_media_info Type functions */
MB_SMF_CLIENT_API mb_smf_sc_mbs_media_info_t *mb_smf_sc_mbs_media_info_new()
{
    return _mbs_media_info_new();
}

MB_SMF_CLIENT_API void mb_smf_sc_mbs_media_info_delete(mb_smf_sc_mbs_media_info_t *media_info)
{
    _mbs_media_info_free(media_info);
}

/* Library internal mbs_media_info methods (protected) */
mb_smf_sc_mbs_media_info_t *_mbs_media_info_new()
{
    return (mb_smf_sc_mbs_media_info_t*)ogs_calloc(1,sizeof(mb_smf_sc_mbs_media_info_t));
}

void _mbs_media_info_free(mb_smf_sc_mbs_media_info_t *media_info)
{
    if (!media_info) return;
    _mbs_media_info_clear(media_info);
    ogs_free(media_info);
}

void _mbs_media_info_clear(mb_smf_sc_mbs_media_info_t *media_info)
{
    if (!media_info) return;
    media_info->mbs_media_type = MEDIA_TYPE_NULL;
    if (media_info->max_requested_mbs_bandwidth_downlink) {
        ogs_free(media_info->max_requested_mbs_bandwidth_downlink);
        media_info->max_requested_mbs_bandwidth_downlink = NULL;
    }
    if (media_info->min_requested_mbs_bandwidth_downlink) {
        ogs_free(media_info->min_requested_mbs_bandwidth_downlink);
        media_info->min_requested_mbs_bandwidth_downlink = NULL;
    }
    if (media_info->codecs[0]) {
        ogs_free(media_info->codecs[0]);
        media_info->codecs[0] = NULL;
    }
    if (media_info->codecs[1]) {
        ogs_free(media_info->codecs[1]);
        media_info->codecs[1] = NULL;
    }
}

void _mbs_media_info_copy(mb_smf_sc_mbs_media_info_t **dst, const mb_smf_sc_mbs_media_info_t *src)
{
    if (!src) {
        if (*dst) {
            _mbs_media_info_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = _mbs_media_info_new();
    } else {
        _mbs_media_info_clear(*dst);
    }

    (*dst)->mbs_media_type = src->mbs_media_type;
    if (src->max_requested_mbs_bandwidth_downlink) {
        (*dst)->max_requested_mbs_bandwidth_downlink = (uint64_t*)ogs_malloc(sizeof(*(*dst)->max_requested_mbs_bandwidth_downlink));
        *(*dst)->max_requested_mbs_bandwidth_downlink = *src->max_requested_mbs_bandwidth_downlink;
    }
    if (src->min_requested_mbs_bandwidth_downlink) {
        (*dst)->min_requested_mbs_bandwidth_downlink = (uint64_t*)ogs_malloc(sizeof(*(*dst)->min_requested_mbs_bandwidth_downlink));
        *(*dst)->min_requested_mbs_bandwidth_downlink = *src->min_requested_mbs_bandwidth_downlink;
    }
    if (src->codecs[0]) {
        (*dst)->codecs[0] = ogs_strdup(src->codecs[0]);
        if (src->codecs[1]) {
            (*dst)->codecs[1] = ogs_strdup(src->codecs[1]);
        }
    } else if (src->codecs[1]) {
        (*dst)->codecs[0] = ogs_strdup(src->codecs[1]);
    }
}

bool _mbs_media_info_equal(const mb_smf_sc_mbs_media_info_t *a, const mb_smf_sc_mbs_media_info_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    if (a->mbs_media_type != b->mbs_media_type) return false;
    if (!a->max_requested_mbs_bandwidth_downlink != !b->max_requested_mbs_bandwidth_downlink) return false;
    if (!a->min_requested_mbs_bandwidth_downlink != !b->min_requested_mbs_bandwidth_downlink) return false;
    const char *a1 = a->codecs[0];
    const char *a2 = a->codecs[1];
    const char *b1 = b->codecs[0];
    const char *b2 = b->codecs[1];
    if (!a1) {
        a1 = a2;
        a2 = NULL;
    }
    if (!b1) {
        b1 = b2;
        b2 = NULL;
    }
    if (!a1 != !b1 || !a2 != !b2) return false;
    if (a1) {
        if (strcmp(a1, b1)) {
            if (!b2) return false;
            if (strcmp(a1, b2)) return false;
            const char *b_tmp = b1;
            b1 = b2;
            b2 = b_tmp;
        }
    }
    if (a2 && strcmp(a2, b2)) return false;

    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
