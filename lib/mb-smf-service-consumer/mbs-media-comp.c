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
#include "ogs-sbi.h"

#include "macros.h"
#include "json-patch.h"
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
ogs_list_t *_mbs_media_comps_patch_list(const ogs_hash_t *a, const ogs_hash_t *b)
{
    if (a && ogs_hash_count((ogs_hash_t*)a) == 0) a = NULL;
    if (b && ogs_hash_count((ogs_hash_t*)b) == 0) b = NULL;
    if (a == b) return NULL;

    ogs_list_t *patches = NULL;

    if (!a) {
        _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _mbs_media_comps_to_json(b));
        patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
        ogs_list_add(patches, patch);
    } else if (!b) {
        _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
        patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
        ogs_list_add(patches, patch);
    } else {
        ogs_hash_index_t *idx = ogs_hash_index_make(a);
        ogs_hash_index_t *it = idx;
        for (it = ogs_hash_next(it); it; it = ogs_hash_next(it)) {
            mb_smf_sc_mbs_media_comp_t *a_comp = (mb_smf_sc_mbs_media_comp_t*)ogs_hash_this_val(it);
            mb_smf_sc_mbs_media_comp_t *b_comp = (mb_smf_sc_mbs_media_comp_t*)ogs_hash_get((ogs_hash_t*)b, &a_comp->id, sizeof(a_comp->id));
            char *path = ogs_msprintf("/%i", a_comp->id);
            if (!b_comp) {
                _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, path, NULL);
                if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            } else {
                patches = _json_patches_append_list(patches, _mbs_media_comp_patch_list(a_comp, b_comp), path);
            }
            ogs_free(path);
        }
        ogs_free(idx);
        idx = ogs_hash_index_make(b);
        it = idx;
        for (it = ogs_hash_next(it); it; it = ogs_hash_next(it)) {
            mb_smf_sc_mbs_media_comp_t *b_comp = (mb_smf_sc_mbs_media_comp_t*)ogs_hash_this_val(it);
            mb_smf_sc_mbs_media_comp_t *a_comp = (mb_smf_sc_mbs_media_comp_t*)ogs_hash_get((ogs_hash_t*)a, &b_comp->id, sizeof(b_comp->id));
            if (!a_comp) {
                char *path = ogs_msprintf("/%i", a_comp->id);
                _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, path, _mbs_media_comp_to_json(b_comp));
                ogs_free(path);
                if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
        }
        ogs_free(idx);
    }

    return patches;
}

OpenAPI_list_t *_mbs_media_comps_to_openapi(const ogs_hash_t *mbs_media_comps)
{
    if (mbs_media_comps && ogs_hash_count((ogs_hash_t*)mbs_media_comps) == 0) mbs_media_comps = NULL;
    if (!mbs_media_comps) return NULL;

    OpenAPI_list_t *list = OpenAPI_list_create();

    mb_smf_sc_mbs_media_comp_t *comp;
    ogs_hash_index_t *idx = ogs_hash_index_make(mbs_media_comps);
    ogs_hash_index_t *it = idx;
    for (it = ogs_hash_next(it); it; it = ogs_hash_next(it)) {
        comp = (mb_smf_sc_mbs_media_comp_t*)ogs_hash_this_val(it);
        char *id = ogs_msprintf("%i", comp->id);
        OpenAPI_map_t *kv = OpenAPI_map_create(id, _mbs_media_comp_to_openapi(comp));
        OpenAPI_list_add(list, kv);
    }
    ogs_free(idx);

    return list;
}

cJSON *_mbs_media_comps_to_json(const ogs_hash_t *mbs_media_comps)
{
    if (mbs_media_comps && ogs_hash_count((ogs_hash_t*)mbs_media_comps) == 0) mbs_media_comps = NULL;
    if (!mbs_media_comps) return NULL;

    cJSON *json = cJSON_CreateObject();

    mb_smf_sc_mbs_media_comp_t *comp;
    ogs_hash_index_t *idx = ogs_hash_index_make(mbs_media_comps);
    ogs_hash_index_t *it = idx;
    for (it = ogs_hash_next(it); it; it = ogs_hash_next(it)) {
        comp = (mb_smf_sc_mbs_media_comp_t*)ogs_hash_this_val(it);
        char *id = ogs_msprintf("%i", comp->id);
        cJSON_AddItemToObject(json, id, _mbs_media_comp_to_json(comp));
        ogs_free(id);
    }

    return json;
}

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

ogs_list_t *_mbs_media_comp_patch_list(const mb_smf_sc_mbs_media_comp_t *a, const mb_smf_sc_mbs_media_comp_t *b)
{
    ogs_list_t *patches = NULL;

    if (a != b) {
        if (!a) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _mbs_media_comp_to_json(b));
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else {
            if (a->id != b->id) {
                _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_replace, "/mbsMedCompNum", cJSON_CreateNumber(b->id));
                patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
            patches = _json_patches_append_list(patches, _flow_descriptions_patch_list(a->flow_descriptions, b->flow_descriptions), "/mbsFlowDescs");
            if (a->mbs_sdf_reserve_priority != b->mbs_sdf_reserve_priority) {
                _json_patch_t *patch = NULL;
                if (!a->mbs_sdf_reserve_priority) {
                    char *prio = ogs_msprintf("PRIO_%i", b->mbs_sdf_reserve_priority);
                    patch = _json_patch_new(OpenAPI_patch_operation_add, "/mbsSdfResPrio", cJSON_CreateString(prio));
                    ogs_free(prio);
                } else if (!b->mbs_sdf_reserve_priority) {
                    patch = _json_patch_new(OpenAPI_patch_operation__remove, "/mbsSdfResPrio", NULL);
                } else {
                    char *prio = ogs_msprintf("PRIO_%i", b->mbs_sdf_reserve_priority);
                    patch = _json_patch_new(OpenAPI_patch_operation_replace, "/mbsSdfResPrio", cJSON_CreateString(prio));
                    ogs_free(prio);
                }
                if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
            patches = _json_patches_append_list(patches, _mbs_media_info_patch_list(a->media_info, b->media_info), "/mbsMediaInfo");
            if (!a->qos_ref != !b->qos_ref) {
                _json_patch_t *patch = NULL;
                if (!a->qos_ref) {
                    patch = _json_patch_new(OpenAPI_patch_operation_add, "/qosRef", cJSON_CreateString(b->qos_ref));
                } else if (!b->qos_ref) {
                    patch = _json_patch_new(OpenAPI_patch_operation__remove, "/qosRef", NULL);
                } else if (strcmp(a->qos_ref, b->qos_ref)) {
                    patch = _json_patch_new(OpenAPI_patch_operation_replace, "/qosRef", cJSON_CreateString(b->qos_ref));
                }
                if (patch) {
                    if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                    ogs_list_add(patches, patch);
                }
            }
            patches = _json_patches_append_list(patches, _mbs_qos_req_patch_list(a->mbs_qos_req, b->mbs_qos_req), "/mbsQoSReq");
        }
    }

    return patches;
}

OpenAPI_mbs_media_comp_rm_t *_mbs_media_comp_to_openapi(const mb_smf_sc_mbs_media_comp_t *mbs_media_comp)
{
    if (!mbs_media_comp) return NULL;

    OpenAPI_list_t *flow_descs = _flow_descriptions_to_openapi(mbs_media_comp->flow_descriptions);
    OpenAPI_mbs_media_info_t *mbs_media_info = _mbs_media_info_to_openapi(mbs_media_comp->media_info);
    char *qos_ref = (!mbs_media_info && mbs_media_comp->qos_ref)?ogs_strdup(mbs_media_comp->qos_ref):NULL;
    OpenAPI_mbs_qo_s_req_t *mbs_qos_req = (!mbs_media_info && !qos_ref)?_mbs_qos_req_to_openapi(mbs_media_comp->mbs_qos_req):NULL;

    return OpenAPI_mbs_media_comp_rm_create(mbs_media_comp->id, flow_descs, mbs_media_comp->mbs_sdf_reserve_priority,
                                            mbs_media_info, qos_ref, mbs_qos_req);
}

cJSON *_mbs_media_comp_to_json(const mb_smf_sc_mbs_media_comp_t *mbs_media_comp)
{
    if (!mbs_media_comp) return NULL;

    OpenAPI_mbs_media_comp_rm_t *api_comp = _mbs_media_comp_to_openapi(mbs_media_comp);
    if (!api_comp) return NULL;

    cJSON *json = OpenAPI_mbs_media_comp_rm_convertToJSON(api_comp);
    OpenAPI_mbs_media_comp_rm_free(api_comp);

    return json;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
