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
#include "utils.h"

#include "mbs-fsa-id.h"
#include "priv_mbs-fsa-id.h"

/* mb_smf_sc_mbs_fsa_id Type functions */
MB_SMF_CLIENT_API mb_smf_sc_mbs_fsa_id_t *mb_smf_sc_mbs_fsa_id_new()
{
    return _mbs_fsa_id_new();
}

MB_SMF_CLIENT_API void mb_smf_sc_mbs_fsa_id_delete(mb_smf_sc_mbs_fsa_id_t *fsa_id)
{
    _mbs_fsa_id_free(fsa_id);
}

/* Library internal mbs_fsa_id methods (protected) */
void _mbs_fsa_ids_copy(ogs_list_t *dst, const ogs_list_t *src)
{
    _mbs_fsa_ids_clear(dst);
    mb_smf_sc_mbs_fsa_id_t *fsa_id; 
    ogs_list_for_each(src, fsa_id) {
        mb_smf_sc_mbs_fsa_id_t *new_fsa_id = NULL;
        _mbs_fsa_id_copy(&new_fsa_id, fsa_id);
        ogs_list_add(dst, new_fsa_id);
    }
}

void _mbs_fsa_ids_clear(ogs_list_t *mbs_fsa_ids)
{
    mb_smf_sc_mbs_fsa_id_t *fsa_id, *next;
    ogs_list_for_each_safe(mbs_fsa_ids, next, fsa_id) {
        ogs_list_remove(mbs_fsa_ids, fsa_id);
        _mbs_fsa_id_free(fsa_id);
    }
}

bool _mbs_fsa_ids_equal(const ogs_list_t *a, const ogs_list_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    if (ogs_list_count(a) != ogs_list_count(b)) return false;

    mb_smf_sc_mbs_fsa_id_t *one = (mb_smf_sc_mbs_fsa_id_t*)ogs_list_first(a);
    mb_smf_sc_mbs_fsa_id_t *two = (mb_smf_sc_mbs_fsa_id_t*)ogs_list_first(b);
    while (one && two) {
        if (!_mbs_fsa_id_equal(one, two)) return false;
        one = (mb_smf_sc_mbs_fsa_id_t*)ogs_list_next(one);
        two = (mb_smf_sc_mbs_fsa_id_t*)ogs_list_next(two);
    }
    return true;
}

ogs_list_t *_mbs_fsa_ids_patch_list(const ogs_list_t *a, const ogs_list_t *b)
{
    ogs_list_t *patches = NULL;
    if (a && ogs_list_count(a) == 0) a = NULL;
    if (b && ogs_list_count(b) == 0) b = NULL;
    if (a == b) return NULL;

    if (!a) {
        _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _mbs_fsa_ids_to_json(b));
        patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
        ogs_list_add(patches, patch);
    } else if (!b) {
        _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
        patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
        ogs_list_add(patches, patch);
    } else {
        int idx = 0;
        mb_smf_sc_mbs_fsa_id_t *a_fsa_id = (mb_smf_sc_mbs_fsa_id_t*)ogs_list_first(a);
        mb_smf_sc_mbs_fsa_id_t *b_fsa_id = (mb_smf_sc_mbs_fsa_id_t*)ogs_list_first(b);
        while (a_fsa_id || b_fsa_id) {
            char *path = ogs_msprintf("/%i", idx);
            _json_patch_t *patch = NULL;
            if (!a_fsa_id) {
                patch = _json_patch_new(OpenAPI_patch_operation_add, path, _mbs_fsa_id_to_json(b_fsa_id));
            } else if (!b_fsa_id) {
                patch = _json_patch_new(OpenAPI_patch_operation__remove, path, NULL);
                idx--;
            } else {
                if (!_mbs_fsa_id_equal(a_fsa_id, b_fsa_id)) {
                    patch = _json_patch_new(OpenAPI_patch_operation_replace, path, _mbs_fsa_id_to_json(b_fsa_id));
                }
            }
            if (patch) {
                if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
            ogs_free(path);
            idx++;
            if (a_fsa_id) a_fsa_id = (mb_smf_sc_mbs_fsa_id_t*)ogs_list_next(a_fsa_id);
            if (b_fsa_id) b_fsa_id = (mb_smf_sc_mbs_fsa_id_t*)ogs_list_next(b_fsa_id);
        }
    }

    return patches;
}

OpenAPI_list_t *_mbs_fsa_ids_to_openapi(const ogs_list_t *fsa_ids)
{
    if (!fsa_ids) return NULL;
    OpenAPI_list_t *list = OpenAPI_list_create();

    mb_smf_sc_mbs_fsa_id_t *fsa_id;
    ogs_list_for_each(fsa_ids, fsa_id) {
        OpenAPI_list_add(list, _mbs_fsa_id_to_openapi(fsa_id));
    }

    return list;
}

cJSON *_mbs_fsa_ids_to_json(const ogs_list_t *fsa_ids)
{
    if (!fsa_ids) return NULL;
    cJSON *json = cJSON_CreateArray();

    mb_smf_sc_mbs_fsa_id_t *fsa_id;
    ogs_list_for_each(fsa_ids, fsa_id) {
        cJSON_AddItemToArray(json, _mbs_fsa_id_to_json(fsa_id));
    }

    return json;
}

mb_smf_sc_mbs_fsa_id_t *_mbs_fsa_id_new()
{
    return (mb_smf_sc_mbs_fsa_id_t*)ogs_calloc(1,sizeof(mb_smf_sc_mbs_fsa_id_t));
}

void _mbs_fsa_id_free(mb_smf_sc_mbs_fsa_id_t *fsa_id)
{
    if (!fsa_id) return;
    //_mbs_fsa_id_clear(fsa_id);
    ogs_free(fsa_id);
}

void _mbs_fsa_id_clear(mb_smf_sc_mbs_fsa_id_t *fsa_id)
{
    if (fsa_id) fsa_id->id = 0;
}

void _mbs_fsa_id_copy(mb_smf_sc_mbs_fsa_id_t **dst, const mb_smf_sc_mbs_fsa_id_t *src)
{
    if (!src) {
        _mbs_fsa_id_free(*dst);
        *dst = NULL;
        return;
    }

    if (!*dst) {
        *dst = _mbs_fsa_id_new();
    } else {
        _mbs_fsa_id_clear(*dst);
    }

    (*dst)->id = src->id;
}

bool _mbs_fsa_id_equal(const mb_smf_sc_mbs_fsa_id_t *a, const mb_smf_sc_mbs_fsa_id_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    return (a->id == b->id);
}

ogs_list_t *_mbs_fsa_id_patch_list(const mb_smf_sc_mbs_fsa_id_t *a, const mb_smf_sc_mbs_fsa_id_t *b)
{
    ogs_list_t *ret = NULL;

    if (a != b) {
        if (!a) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _mbs_fsa_id_to_json(b));
            ret = (typeof(ret))ogs_calloc(1,sizeof(*ret));
            ogs_list_add(ret, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            ret = (typeof(ret))ogs_calloc(1,sizeof(*ret));
            ogs_list_add(ret, patch);
        } else if (a->id != b->id) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_replace, "/", _mbs_fsa_id_to_json(b));
            ret = (typeof(ret))ogs_calloc(1,sizeof(*ret));
            ogs_list_add(ret, patch);
        }
    }

    return ret;
}

char *_mbs_fsa_id_to_openapi(const mb_smf_sc_mbs_fsa_id_t *fsa_id)
{
    if (!fsa_id) return NULL;
    return _uint32_to_hex_str(fsa_id->id, 6, 6);
}

cJSON *_mbs_fsa_id_to_json(const mb_smf_sc_mbs_fsa_id_t *fsa_id)
{
    return cJSON_CreateString(_mbs_fsa_id_to_openapi(fsa_id));
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
