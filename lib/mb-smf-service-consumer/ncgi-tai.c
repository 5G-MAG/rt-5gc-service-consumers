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
#include "ogs-sbi.h"

#include "macros.h"
#include "json-patch.h"
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
ogs_list_t *_ncgi_tais_patch_list(const ogs_list_t *a, const ogs_list_t *b)
{
    ogs_list_t *patches = NULL;
    if (a && ogs_list_count(a) == 0) a = NULL;
    if (b && ogs_list_count(b) == 0) b = NULL;
    if (a != b) {
        if (!a) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _ncgi_tais_to_json(b));
            patches = (ogs_list_t*)ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            patches = (ogs_list_t*)ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else {
            int idx = 0;
            mb_smf_sc_ncgi_tai_t *a_ncgi_tai = (mb_smf_sc_ncgi_tai_t*)ogs_list_first(a);
            mb_smf_sc_ncgi_tai_t *b_ncgi_tai = (mb_smf_sc_ncgi_tai_t*)ogs_list_first(b);
            while (a_ncgi_tai && b_ncgi_tai && _ncgi_tai_equal(a_ncgi_tai, b_ncgi_tai)) {
                idx++;
                a_ncgi_tai = (mb_smf_sc_ncgi_tai_t*)ogs_list_next(a_ncgi_tai);
                b_ncgi_tai = (mb_smf_sc_ncgi_tai_t*)ogs_list_next(b_ncgi_tai);
            }
            if (idx == 0) {
                /* replace/add whole array */
                _json_patch_t *patch;
                if (ogs_list_count(a) == 0) {
                    patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _ncgi_tais_to_json(b));
                } else {
                    patch = _json_patch_new(OpenAPI_patch_operation_replace, "/", _ncgi_tais_to_json(b));
                }
                patches = (ogs_list_t*)ogs_calloc(1,sizeof(*patches));
                ogs_list_add(patches, patch);
            } else {
                /* replace/add/remove array entries */
                while (a_ncgi_tai || b_ncgi_tai) {
                    char *path = ogs_msprintf("/%i", idx);
                    _json_patch_t *patch = NULL;
                    if (!a_ncgi_tai) {
                        patch = _json_patch_new(OpenAPI_patch_operation_add, path, _ncgi_tai_to_json(b_ncgi_tai));
                    } else if (!b_ncgi_tai) {
                        patch = _json_patch_new(OpenAPI_patch_operation__remove, path, NULL);
                        idx--;
                    } else if (!_ncgi_tai_equal(a_ncgi_tai, b_ncgi_tai)) {
                        patch = _json_patch_new(OpenAPI_patch_operation_replace, path, _ncgi_tai_to_json(b_ncgi_tai));
                    }
                    ogs_free(path);
                    if (patch) {
                        if (!patches) patches = (ogs_list_t*)ogs_calloc(1,sizeof(*patches));
                        ogs_list_add(patches, patch);
                    }
                    idx++;
                    if (a_ncgi_tai) a_ncgi_tai = (mb_smf_sc_ncgi_tai_t*)ogs_list_next(a_ncgi_tai);
                    if (b_ncgi_tai) b_ncgi_tai = (mb_smf_sc_ncgi_tai_t*)ogs_list_next(b_ncgi_tai);
                }
            }
        }
    }

    return patches;
}

OpenAPI_list_t *_ncgi_tais_to_openapi(const ogs_list_t *ncgi_tais)
{
    if (!ncgi_tais) return NULL;
    OpenAPI_list_t *list = OpenAPI_list_create();

    mb_smf_sc_ncgi_tai_t *ncgi_tai;
    ogs_list_for_each(ncgi_tais, ncgi_tai) {
        OpenAPI_list_add(list, _ncgi_tai_to_openapi(ncgi_tai));
    }

    return list;
}

cJSON *_ncgi_tais_to_json(const ogs_list_t *ncgi_tais)
{
    if (!ncgi_tais) return NULL;
    cJSON *json = cJSON_CreateArray();

    mb_smf_sc_ncgi_tai_t *ncgi_tai;
    ogs_list_for_each(ncgi_tais, ncgi_tai) {
        cJSON_AddItemToArray(json, _ncgi_tai_to_json(ncgi_tai));
    }

    return json;
}

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

ogs_list_t *_ncgi_tai_patch_list(const mb_smf_sc_ncgi_tai_t *a, const mb_smf_sc_ncgi_tai_t *b)
{
    ogs_list_t *patches = NULL;

    if (a != b) {
        if (!a) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _ncgi_tai_to_json(b));
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else {
            patches = _json_patches_append_list(patches, _tai_patch_list(&a->tai, &b->tai), "/tai");
            patches = _json_patches_append_list(patches, _ncgis_patch_list(&a->ncgis, &b->ncgis), "/cellList");
        }
    }

    return patches;
}

OpenAPI_ncgi_tai_t *_ncgi_tai_to_openapi(const mb_smf_sc_ncgi_tai_t *ncgi_tai)
{
    if (!ncgi_tai) return NULL;
    return OpenAPI_ncgi_tai_create(_tai_to_openapi(&ncgi_tai->tai), _ncgis_to_openapi(&ncgi_tai->ncgis));
}

cJSON *_ncgi_tai_to_json(const mb_smf_sc_ncgi_tai_t *ncgi_tai)
{
    if (!ncgi_tai) return NULL;
    OpenAPI_ncgi_tai_t *api_ncgi_tai = _ncgi_tai_to_openapi(ncgi_tai);
    if (!api_ncgi_tai) return NULL;
    cJSON *json = OpenAPI_ncgi_tai_convertToJSON(api_ncgi_tai);
    OpenAPI_ncgi_tai_free(api_ncgi_tai);
    return json;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
