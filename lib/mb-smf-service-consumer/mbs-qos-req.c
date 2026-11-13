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
#include "priv_arp.h"
#include "utils.h"

#include "mbs-qos-req.h"
#include "priv_mbs-qos-req.h"

/* mb_smf_sc_mbs_qos_req Type functions */
MB_SMF_CLIENT_API mb_smf_sc_mbs_qos_req_t *mb_smf_sc_mbs_qos_req_new()
{
    return _mbs_qos_req_new();
}

MB_SMF_CLIENT_API void mb_smf_sc_mbs_qos_req_delete(mb_smf_sc_mbs_qos_req_t *qos_req)
{
    _mbs_qos_req_free(qos_req);
}

/* Library internal mbs_qos_req methods (protected) */
mb_smf_sc_mbs_qos_req_t *_mbs_qos_req_new()
{
    return (mb_smf_sc_mbs_qos_req_t*)ogs_calloc(1,sizeof(mb_smf_sc_mbs_qos_req_t));
}

void _mbs_qos_req_free(mb_smf_sc_mbs_qos_req_t *mbs_qos_req)
{
    if (!mbs_qos_req) return;
    _mbs_qos_req_clear(mbs_qos_req);
    ogs_free(mbs_qos_req);
}

void _mbs_qos_req_clear(mb_smf_sc_mbs_qos_req_t *qos_req)
{
    if (!qos_req) return;
    qos_req->five_qi = 0;
    if (qos_req->guarenteed_bit_rate) {
        ogs_free(qos_req->guarenteed_bit_rate);
        qos_req->guarenteed_bit_rate = NULL;
    }
    if (qos_req->max_bit_rate) {
        ogs_free(qos_req->max_bit_rate);
        qos_req->max_bit_rate = NULL;
    }
    if (qos_req->averaging_window) {
        ogs_free(qos_req->averaging_window);
        qos_req->averaging_window = NULL;
    }
    _arp_free(qos_req->req_mbs_arp);
    qos_req->req_mbs_arp = NULL;
}

void _mbs_qos_req_copy(mb_smf_sc_mbs_qos_req_t **dst, const mb_smf_sc_mbs_qos_req_t *src)
{
    if (!src) {
        if (*dst) {
            _mbs_qos_req_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = _mbs_qos_req_new();
    } else {
        _mbs_qos_req_clear(*dst);
    }

    (*dst)->five_qi = src->five_qi;
    if (src->guarenteed_bit_rate) {
        (*dst)->guarenteed_bit_rate = (uint64_t*)ogs_malloc(sizeof(*(*dst)->guarenteed_bit_rate));
        *(*dst)->guarenteed_bit_rate = *src->guarenteed_bit_rate;
    }
    if (src->max_bit_rate) {
        (*dst)->max_bit_rate = (uint64_t*)ogs_malloc(sizeof(*(*dst)->max_bit_rate));
        *(*dst)->max_bit_rate= *src->max_bit_rate;
    }
    if (src->averaging_window) {
        (*dst)->averaging_window = (uint16_t*)ogs_malloc(sizeof(*(*dst)->averaging_window));
        *(*dst)->averaging_window = *src->averaging_window;
    }
    _arp_copy(&(*dst)->req_mbs_arp, src->req_mbs_arp);
}

bool _mbs_qos_req_equal(const mb_smf_sc_mbs_qos_req_t *a, const mb_smf_sc_mbs_qos_req_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    if (a->five_qi != b->five_qi) return false;
    if (!a->guarenteed_bit_rate != !b->guarenteed_bit_rate) return false;
    if (a->guarenteed_bit_rate && *a->guarenteed_bit_rate != *b->guarenteed_bit_rate) return false;
    if (!a->max_bit_rate != !b->max_bit_rate) return false;
    if (a->max_bit_rate && *a->max_bit_rate != *b->max_bit_rate) return false;
    if (!a->averaging_window != !b->averaging_window) return false;
    if (a->averaging_window && *a->averaging_window != *b->averaging_window) return false;

    return _arp_equal(a->req_mbs_arp, b->req_mbs_arp);
}

ogs_list_t *_mbs_qos_req_patch_list(const mb_smf_sc_mbs_qos_req_t *a, const mb_smf_sc_mbs_qos_req_t *b)
{
    if (a == b) return NULL;

    ogs_list_t *patches = NULL;

    if (!a) {
        _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _mbs_qos_req_to_json(b));
        patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
        ogs_list_add(patches, patch);
    } else if (!b) {
        _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
        patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
        ogs_list_add(patches, patch);
    } else {
        if (a->five_qi != b->five_qi) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_replace, "/5Qi", cJSON_CreateNumber(b->five_qi));
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        }
        if (a->guarenteed_bit_rate != b->guarenteed_bit_rate) {
            _json_patch_t *patch = NULL;
            if (!a->guarenteed_bit_rate) {
                char *br = _bitrate_to_str(*b->guarenteed_bit_rate);
                patch = _json_patch_new(OpenAPI_patch_operation_add, "/guarBitRate", cJSON_CreateString(br));
                ogs_free(br);
            } else if (!b->guarenteed_bit_rate) {
                patch = _json_patch_new(OpenAPI_patch_operation__remove, "/guarBitRate", NULL);
            } else if (*a->guarenteed_bit_rate != *b->guarenteed_bit_rate) {
                char *br = _bitrate_to_str(*b->guarenteed_bit_rate);
                patch = _json_patch_new(OpenAPI_patch_operation_replace, "/guarBitRate", cJSON_CreateString(br));
                ogs_free(br);
            }
            if (patch) {
                if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
        }
        if (a->max_bit_rate != b->max_bit_rate) {
            _json_patch_t *patch = NULL;
            if (!a->max_bit_rate) {
                char *br = _bitrate_to_str(*b->max_bit_rate);
                patch = _json_patch_new(OpenAPI_patch_operation_add, "/maxBitRate", cJSON_CreateString(br));
                ogs_free(br);
            } else if (!b->max_bit_rate) {
                patch = _json_patch_new(OpenAPI_patch_operation__remove, "/maxBitRate", NULL);
            } else if (*a->max_bit_rate != *b->max_bit_rate) {
                char *br = _bitrate_to_str(*b->max_bit_rate);
                patch = _json_patch_new(OpenAPI_patch_operation_replace, "/maxBitRate", cJSON_CreateString(br));
                ogs_free(br);
            }
            if (patch) {
                if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
        }
        if (a->averaging_window != b->averaging_window) {
            _json_patch_t *patch = NULL;
            if (!a->averaging_window) {
                patch = _json_patch_new(OpenAPI_patch_operation_add, "/averWindow", cJSON_CreateNumber(*b->averaging_window));
            } else if (!b->averaging_window) {
                patch = _json_patch_new(OpenAPI_patch_operation__remove, "/averWindow", NULL);
            } else if (*a->averaging_window != *b->averaging_window) {
                patch = _json_patch_new(OpenAPI_patch_operation_replace, "/averWindow", cJSON_CreateNumber(*b->averaging_window));
            }
            if (patch) {
                if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
        }
        patches = _json_patches_append_list(patches, _arp_patch_list(a->req_mbs_arp, b->req_mbs_arp), "/regMbsArp");
    }

    return patches;
}

OpenAPI_mbs_qo_s_req_t *_mbs_qos_req_to_openapi(const mb_smf_sc_mbs_qos_req_t *qos_req)
{
    if (!qos_req) return NULL;

    char *guar_bit_rate = NULL;
    char *max_bit_rate = NULL;
    OpenAPI_arp_t *req_mbs_arp = _arp_to_openapi(qos_req->req_mbs_arp);

    if (qos_req->guarenteed_bit_rate) {
        guar_bit_rate = _bitrate_to_str(*qos_req->guarenteed_bit_rate);
    }
    if (qos_req->max_bit_rate) {
        max_bit_rate  = _bitrate_to_str(*qos_req->max_bit_rate);
    }

    return OpenAPI_mbs_qo_s_req_create(qos_req->five_qi, guar_bit_rate, max_bit_rate, !!qos_req->averaging_window,
                                        qos_req->averaging_window?*qos_req->averaging_window:0, req_mbs_arp);
}

cJSON *_mbs_qos_req_to_json(const mb_smf_sc_mbs_qos_req_t *qos_req)
{
    if (!qos_req) return NULL;

    OpenAPI_mbs_qo_s_req_t *api_qos_req = _mbs_qos_req_to_openapi(qos_req);
    cJSON *json = OpenAPI_mbs_qo_s_req_convertToJSON(api_qos_req);
    OpenAPI_mbs_qo_s_req_free(api_qos_req);

    return json;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
