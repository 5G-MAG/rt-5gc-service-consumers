/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef MB_SMF_JSON_PATCH_H
#define MB_SMF_JSON_PATCH_H

#include "ogs-core.h"
#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library internal data types */
typedef struct _json_patch_s {
    ogs_lnode_t node;
    OpenAPI_patch_item_t *item;
} _json_patch_t;

/* Library Internals */
ogs_list_t *_json_patches_append_list(ogs_list_t *dst, const ogs_list_t *src, const char *at_prefix);
_json_patch_t *_json_patch_new_copy_with_prefix(const _json_patch_t *src, const char *at_prefix);
_json_patch_t *_json_patch_new(OpenAPI_patch_operation_e op, const char *path, cJSON *value);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* MB_SMF_JSON_PATCH_H */
