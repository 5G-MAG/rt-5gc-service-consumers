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

#include "flow-description.h"
#include "priv_flow-description.h"

/* mb_smf_sc_flow_description Type functions */
MB_SMF_CLIENT_API mb_smf_sc_flow_description_t *mb_smf_sc_flow_description_new()
{
    return _flow_description_new();
}

MB_SMF_CLIENT_API void mb_smf_sc_flow_description_delete(mb_smf_sc_flow_description_t *flow_desc)
{
    _flow_description_free(flow_desc);
}

/* Library internal flow_description methods (protected) */
void _flow_descriptions_copy(ogs_list_t **dst, const ogs_list_t *src)
{
    if (!src) {
        if (*dst) {
            _flow_descriptions_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = (ogs_list_t*)ogs_calloc(1, sizeof(**dst));
    } else {
        _flow_descriptions_clear(*dst);
    }

    mb_smf_sc_flow_description_t *flow;
    ogs_list_for_each(src, flow) {
        mb_smf_sc_flow_description_t *new_flow = NULL;
        _flow_description_copy(&new_flow, flow);
        ogs_list_add(*dst, new_flow);
    }
}

void _flow_descriptions_clear(ogs_list_t *flow_descriptions)
{
    if (!flow_descriptions) return;
    mb_smf_sc_flow_description_t *next, *flow;
    ogs_list_for_each_safe(flow_descriptions, next, flow) {
        ogs_list_remove(flow_descriptions, flow);
        _flow_description_free(flow);
    }
}

void _flow_descriptions_free(ogs_list_t *flow_descriptions)
{
    if (!flow_descriptions) return;
    _flow_descriptions_clear(flow_descriptions);
    ogs_free(flow_descriptions);
}

bool _flow_descriptions_equal(const ogs_list_t *a, const ogs_list_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    if (ogs_list_count(a) != ogs_list_count(b)) return false;

    ogs_list_t *b_copy = NULL;
    _flow_descriptions_copy(&b_copy, b);

    mb_smf_sc_flow_description_t *a_flow;
    ogs_list_for_each(a, a_flow) {
        mb_smf_sc_flow_description_t *next, *b_flow;
        bool found = true;
        ogs_list_for_each_safe(b_copy, next, b_flow) {
            if (_flow_description_equal(a_flow, b_flow)) {
                found = true;
                ogs_list_remove(b_copy, b_flow);
                _flow_description_free(b_flow);
                break;
            }
        }
        if (!found) {
            _flow_descriptions_free(b_copy);
            return false;
        }
    }

    _flow_descriptions_free(b_copy);
    return true;
}

ogs_list_t *_flow_descriptions_patch_list(const ogs_list_t *a, const ogs_list_t *b)
{
    if (a && ogs_list_count(a) == 0) a = NULL;
    if (b && ogs_list_count(b) == 0) b = NULL;
    if (a == b) return NULL;

    ogs_list_t *patches = NULL;

    if (!a) {
        _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _flow_descriptions_to_json(b));
        patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
        ogs_list_add(patches, patch);
    } else if (!b) {
        _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
        patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
        ogs_list_add(patches, patch);
    } else {
        int idx = 0;
        mb_smf_sc_flow_description_t *a_flow = (mb_smf_sc_flow_description_t*)ogs_list_first(a);
        mb_smf_sc_flow_description_t *b_flow = (mb_smf_sc_flow_description_t*)ogs_list_first(b);
        while (a_flow || b_flow) {
            _json_patch_t *patch = NULL;
            if (!a_flow) {
                patch = _json_patch_new(OpenAPI_patch_operation_add, "/-", _flow_description_to_json(b_flow));
            } else if (!b_flow) {
                char *path = ogs_msprintf("/%i", idx);
                patch = _json_patch_new(OpenAPI_patch_operation__remove, path, NULL);
                ogs_free(path);
                idx--;
            } else {
                char *path = ogs_msprintf("/%i", idx);
                patches = _json_patches_append_list(patches, _flow_description_patch_list(a_flow, b_flow), path);
                ogs_free(path);
            }
            if (patch) {
                if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
            idx++;
            if (a_flow) a_flow = (mb_smf_sc_flow_description_t*)ogs_list_next(a_flow);
            if (b_flow) b_flow = (mb_smf_sc_flow_description_t*)ogs_list_next(b_flow);
        }
    }

    return patches;
}

OpenAPI_list_t *_flow_descriptions_to_openapi(const ogs_list_t *flow_descs)
{
    if (flow_descs && ogs_list_count(flow_descs) == 0) flow_descs = NULL;
    if (!flow_descs) return NULL;

    OpenAPI_list_t *list = OpenAPI_list_create();

    mb_smf_sc_flow_description_t *flow;
    ogs_list_for_each(flow_descs, flow) {
        OpenAPI_list_add(list, _flow_description_to_openapi(flow));
    }

    return list;
}

cJSON *_flow_descriptions_to_json(const ogs_list_t *flow_descs)
{
    if (flow_descs && ogs_list_count(flow_descs) == 0) flow_descs = NULL;
    if (!flow_descs) return NULL;

    cJSON *json = cJSON_CreateArray();

    mb_smf_sc_flow_description_t *flow;
    ogs_list_for_each(flow_descs, flow) {
        cJSON_AddItemToArray(json, _flow_description_to_json(flow));
    }

    return json;
}

mb_smf_sc_flow_description_t *_flow_description_new()
{
    return (mb_smf_sc_flow_description_t*)ogs_calloc(1,sizeof(mb_smf_sc_flow_description_t));
}

void _flow_description_free(mb_smf_sc_flow_description_t *flow_description)
{
    if (!flow_description) return;
    _flow_description_clear(flow_description);
    ogs_free(flow_description);
}

void _flow_description_clear(mb_smf_sc_flow_description_t *flow_description)
{
    if (!flow_description) return;
    if (flow_description->string) {
        ogs_free(flow_description->string);
        flow_description->string = NULL;
    }
}

void _flow_description_copy(mb_smf_sc_flow_description_t **dst, const mb_smf_sc_flow_description_t *src)
{
    if (!src || !src->string) {
        if (*dst) {
            _flow_description_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = _flow_description_new();
    } else {
        _flow_description_clear(*dst);
    }

    (*dst)->string = ogs_strdup(src->string);
}

bool _flow_description_equal(const mb_smf_sc_flow_description_t *a, const mb_smf_sc_flow_description_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    if (!a->string != !b->string) return false;
    if (a->string && strcmp(a->string, b->string)) return false;

    return true;
}

ogs_list_t *_flow_description_patch_list(const mb_smf_sc_flow_description_t *a, const mb_smf_sc_flow_description_t *b)
{
    if (a == b) return NULL;

    ogs_list_t *patches = NULL;

    _json_patch_t *patch = NULL;
    if (!a) {
        patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _flow_description_to_json(b));
    } else if (!b) {
        patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
    } else if (a->string != b->string) {
        if (!a->string) {
            patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _flow_description_to_json(b));
        } else if (!b->string) {
            patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
        } else if (strcmp(a->string, b->string)) {
            patch = _json_patch_new(OpenAPI_patch_operation_replace, "/", _flow_description_to_json(b));
        }
    }
    if (patch) {
        patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
        ogs_list_add(patches, patch);
    }

    return patches;
}

char *_flow_description_to_openapi(const mb_smf_sc_flow_description_t *flow_desc)
{
    if (!flow_desc || !flow_desc->string) return NULL;
    return ogs_strdup(flow_desc->string);
}

cJSON *_flow_description_to_json(const mb_smf_sc_flow_description_t *flow_desc)
{
    if (!flow_desc || !flow_desc->string) return NULL;
    return cJSON_CreateString(flow_desc->string);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
