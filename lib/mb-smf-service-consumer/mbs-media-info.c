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

ogs_list_t *_mbs_media_info_patch_list(const mb_smf_sc_mbs_media_info_t *a, const mb_smf_sc_mbs_media_info_t *b)
{
    if (a == b) return NULL;

    ogs_list_t *patches = NULL;

    if (!a) {
        _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _mbs_media_info_to_json(b));
        patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
        ogs_list_add(patches, patch);
    } else if (!b) {
        _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
        patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
        ogs_list_add(patches, patch);
    } else {
        if (a->mbs_media_type != b->mbs_media_type) {
            _json_patch_t *patch;
            if (!a->mbs_media_type) {
                patch = _json_patch_new(OpenAPI_patch_operation_add, "/mbsMedType", cJSON_CreateString(OpenAPI_media_type_ToString(b->mbs_media_type)));
            } else if (!b->mbs_media_type) {
                patch = _json_patch_new(OpenAPI_patch_operation__remove, "/mbsMedType", NULL);
            } else {
                patch = _json_patch_new(OpenAPI_patch_operation_replace, "/mbsMedType", cJSON_CreateString(OpenAPI_media_type_ToString(b->mbs_media_type)));
            }
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        }
        if (a->max_requested_mbs_bandwidth_downlink != b->max_requested_mbs_bandwidth_downlink) {
            _json_patch_t *patch = NULL;
            if (!a->max_requested_mbs_bandwidth_downlink) {
                char *br = _bitrate_to_str(*b->max_requested_mbs_bandwidth_downlink);
                patch = _json_patch_new(OpenAPI_patch_operation_add, "/maxReqMbsBwDl", cJSON_CreateString(br));
                ogs_free(br);
            } else if (!b->max_requested_mbs_bandwidth_downlink) {
                patch = _json_patch_new(OpenAPI_patch_operation__remove, "/maxReqMbsBwDl", NULL);
            } else if (*a->max_requested_mbs_bandwidth_downlink != *b->max_requested_mbs_bandwidth_downlink) {
                char *br = _bitrate_to_str(*b->max_requested_mbs_bandwidth_downlink);
                patch = _json_patch_new(OpenAPI_patch_operation_replace, "/maxReqMbsBwDl", cJSON_CreateString(br));
                ogs_free(br);
            }
            if (patch) {
                if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
        }
        if (a->min_requested_mbs_bandwidth_downlink != b->min_requested_mbs_bandwidth_downlink) {
            _json_patch_t *patch = NULL;
            if (!a->min_requested_mbs_bandwidth_downlink) {
                char *br = _bitrate_to_str(*b->min_requested_mbs_bandwidth_downlink);
                patch = _json_patch_new(OpenAPI_patch_operation_add, "/minReqMbsBwDl", cJSON_CreateString(br));
                ogs_free(br);
            } else if (!b->min_requested_mbs_bandwidth_downlink) {
                patch = _json_patch_new(OpenAPI_patch_operation__remove, "/minReqMbsBwDl", NULL);
            } else if (*a->min_requested_mbs_bandwidth_downlink != *b->min_requested_mbs_bandwidth_downlink) {
                char *br = _bitrate_to_str(*b->min_requested_mbs_bandwidth_downlink);
                patch = _json_patch_new(OpenAPI_patch_operation_replace, "/minReqMbsBwDl", cJSON_CreateString(br));
                ogs_free(br);
            }
            if (patch) {
                if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
        }
        const char *a_c1 = a->codecs[0];
        const char *a_c2 = a->codecs[1];
        const char *b_c1 = b->codecs[0];
        const char *b_c2 = b->codecs[1];

        if (!a_c1) {
            a_c1 = a_c2;
            a_c2 = NULL;
        }
        if (!b_c1) {
            b_c1 = b_c2;
            b_c2 = NULL;
        }

        if (a_c1 != b_c1) {
            _json_patch_t *patch = NULL;
            if (!a_c1) {
                patch = _json_patch_new(OpenAPI_patch_operation_add, "/codecs/-", cJSON_CreateString(b_c1));
            } else if (!b_c1) {
                patch = _json_patch_new(OpenAPI_patch_operation__remove, "/codecs", NULL);
            } else if (strcmp(a_c1, b_c1)) {
                patch = _json_patch_new(OpenAPI_patch_operation_replace, "/codecs/0", cJSON_CreateString(b_c1));
            }
            if (patch) {
                if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
        }
        if (a_c2 != b_c2) {
            _json_patch_t *patch = NULL;
            if (!a_c2) {
                patch = _json_patch_new(OpenAPI_patch_operation_add, "/codecs/-", cJSON_CreateString(b_c2));
            } else if (!b_c2) {
                if (b_c1) {
                    patch = _json_patch_new(OpenAPI_patch_operation__remove, "/codecs/1", NULL);
                }
            } else if (strcmp(a_c2, b_c2)) {
                patch = _json_patch_new(OpenAPI_patch_operation_replace, "/codecs/1", cJSON_CreateString(b_c2));
            }
            if (patch) {
                if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
        }
    }

    return patches;
}

OpenAPI_mbs_media_info_t *_mbs_media_info_to_openapi(const mb_smf_sc_mbs_media_info_t *media_info)
{
    if (!media_info) return NULL;

    char *max_br = NULL;
    char *min_br = NULL;
    OpenAPI_list_t *codecs = NULL;

    if (media_info->max_requested_mbs_bandwidth_downlink) {
        max_br = _bitrate_to_str(*media_info->max_requested_mbs_bandwidth_downlink);
    }
    if (media_info->min_requested_mbs_bandwidth_downlink) {
        min_br = _bitrate_to_str(*media_info->min_requested_mbs_bandwidth_downlink);
    }
    if (media_info->codecs[0]) {
        codecs = OpenAPI_list_create();
        OpenAPI_list_add(codecs, ogs_strdup(media_info->codecs[0]));
    }
    if (media_info->codecs[1]) {
        if (!codecs) codecs = OpenAPI_list_create();
        OpenAPI_list_add(codecs, ogs_strdup(media_info->codecs[1]));
    }

    return OpenAPI_mbs_media_info_create(media_info->mbs_media_type, max_br, min_br, codecs);
}

cJSON *_mbs_media_info_to_json(const mb_smf_sc_mbs_media_info_t *media_info)
{
    if (!media_info) return NULL;

    OpenAPI_mbs_media_info_t *api_info = _mbs_media_info_to_openapi(media_info);
    cJSON *json = OpenAPI_mbs_media_info_convertToJSON(api_info);
    OpenAPI_mbs_media_info_free(api_info);
    return json;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
