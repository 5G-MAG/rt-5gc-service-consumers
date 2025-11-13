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
#include "priv_flow-description.h"
#include "priv_mbs-media-info.h"
#include "priv_mbs-qos-req.h"

#include "mbs-media-comp.h"
#include "priv_mbs-media-comp.h"

/* mb_smf_sc_mbs_media_comp Type functions */
MB_SMF_CLIENT_API mb_smf_sc_mbs_media_comp_t *mb_smf_sc_mbs_media_comp_new()
{
    return _mbs_media_comp_new();
}

MB_SMF_CLIENT_API void mb_smf_sc_mbs_media_comp_delete(mb_smf_sc_mbs_media_comp_t *media_comp)
{
    _mbs_media_comp_free(media_comp);
}

/* Library internal mbs_media_comp methods (protected) */
mb_smf_sc_mbs_media_comp_t *_mbs_media_comp_new()
{
    return (mb_smf_sc_mbs_media_comp_t*)ogs_calloc(1,sizeof(mb_smf_sc_mbs_media_comp_t));
}

void _mbs_media_comp_free(mb_smf_sc_mbs_media_comp_t *media_comp)
{
    if (!media_comp) return;
    _mbs_media_comp_clear(media_comp);
    ogs_free(media_comp);
}

void _mbs_media_comp_clear(mb_smf_sc_mbs_media_comp_t *media_comp)
{
    if (!media_comp) return;

    _flow_descriptions_free(media_comp->flow_descriptions);
    media_comp->flow_descriptions = NULL;

    media_comp->mbs_sdf_reserve_priority = 0;

    _mbs_media_info_free(media_comp->media_info);
    media_comp->media_info = NULL;

    if (media_comp->qos_ref) {
        ogs_free(media_comp->qos_ref);
        media_comp->qos_ref = NULL;
    }

    _mbs_qos_req_free(media_comp->mbs_qos_req);
    media_comp->mbs_qos_req = NULL;
}

void _mbs_media_comp_copy(mb_smf_sc_mbs_media_comp_t **dst, const mb_smf_sc_mbs_media_comp_t *src)
{
    if (!src) {
        _mbs_media_comp_free(*dst);
        *dst = NULL;
        return;
    }

    if (!*dst) {
        *dst = _mbs_media_comp_new();
    } else {
        _mbs_media_comp_clear(*dst);
    }

    mb_smf_sc_mbs_media_comp_t *dest = *dst;

    dest->id = src->id;
    _flow_descriptions_copy(&dest->flow_descriptions, src->flow_descriptions);
    dest->mbs_sdf_reserve_priority = src->mbs_sdf_reserve_priority;
    _mbs_media_info_copy(&dest->media_info, src->media_info);
    if (src->qos_ref) dest->qos_ref = ogs_strdup(src->qos_ref);
    _mbs_qos_req_copy(&dest->mbs_qos_req, src->mbs_qos_req);
}

bool _mbs_media_comp_equal(const mb_smf_sc_mbs_media_comp_t *a, const mb_smf_sc_mbs_media_comp_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    if (a->mbs_sdf_reserve_priority != b->mbs_sdf_reserve_priority) return false;
    if (!a->qos_ref != !b->qos_ref) return false;
    if (a->qos_ref && strcmp(a->qos_ref, b->qos_ref)) return false;
    if (!_flow_descriptions_equal(a->flow_descriptions, b->flow_descriptions)) return false;
    if (!_mbs_media_info_equal(a->media_info, b->media_info)) return false;
    if (!_mbs_qos_req_equal(a->mbs_qos_req, b->mbs_qos_req)) return false;

    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
