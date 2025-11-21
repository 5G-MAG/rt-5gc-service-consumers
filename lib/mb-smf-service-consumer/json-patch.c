/*
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

#include "json-patch.h"

/* Library internal data types */

/* Library Internals */
ogs_list_t *_json_patches_append_list(ogs_list_t *dst, const ogs_list_t *src, const char *at_prefix)
{
    if (!src) return dst;

    if (!dst) dst = (ogs_list_t*)ogs_calloc(1, sizeof(*dst));

    _json_patch_t *src_patch;
    ogs_list_for_each(src, src_patch) {
        _json_patch_t *new_patch = _json_patch_new_copy_with_prefix(src_patch, at_prefix);
        ogs_list_add(dst, new_patch);
    }

    return dst;
}

_json_patch_t *_json_patch_new_copy_with_prefix(const _json_patch_t *src, const char *at_prefix)
{
    _json_patch_t *new_patch = (_json_patch_t*)ogs_calloc(1, sizeof(*new_patch));
    new_patch->item = OpenAPI_patch_item_create(src->item->op, NULL, src->item->from?ogs_strdup(src->item->from):NULL, src->item->is_value_null, OpenAPI_any_type_create(src->item->value->json));
    if (src->item->path) {
        if (at_prefix) {
            if (src->item->path[0] == '/' && src->item->path[1] == '\0') {
                new_patch->item->path = ogs_strdup(at_prefix);
            } else {
                new_patch->item->path = ogs_msprintf("%s%s", at_prefix, src->item->path);
            }
        } else {
            new_patch->item->path = ogs_strdup(src->item->path);
        }
    } else if (at_prefix) {
        new_patch->item->path = ogs_strdup(at_prefix);
    }
    return new_patch;
}

_json_patch_t *_json_patch_new(OpenAPI_patch_operation_e op, const char *path, cJSON *value)
{
    _json_patch_t *dst = (_json_patch_t*)ogs_calloc(1, sizeof(*dst));
    dst->item = OpenAPI_patch_item_create(op, ogs_strdup(path), NULL,
                                          (value && cJSON_IsNull(value)), value?OpenAPI_any_type_create(value):NULL);
    return dst;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
