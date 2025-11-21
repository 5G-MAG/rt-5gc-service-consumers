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
#include "ogs-proto.h"

#include "macros.h"
#include "json-patch.h"
#include "utils.h"

#include "priv_tai.h"
#include "tai.h"

/* Forward declarations */

/* Data types */

/* mb_smf_sc_tai Type functions */
MB_SMF_CLIENT_API mb_smf_sc_tai_t *mb_smf_sc_tai_new(uint16_t mcc, uint16_t mnc, uint32_t tac, const uint64_t *nid)
{
    ogs_plmn_id_t plmn_id;
    ogs_plmn_id_build(&plmn_id, mcc, mnc, (mnc>99)?3:2);
    return _tai_new(&plmn_id, tac, nid);
}

MB_SMF_CLIENT_API mb_smf_sc_tai_t *mb_smf_sc_tai_new_copy(const mb_smf_sc_tai_t *other)
{
    mb_smf_sc_tai_t *dst = NULL;
    _tai_copy(&dst, other);
    return dst;
}

MB_SMF_CLIENT_API void mb_smf_sc_tai_free(mb_smf_sc_tai_t *tai)
{
    _tai_free(tai);
}

MB_SMF_CLIENT_API bool mb_smf_sc_tai_equal(const mb_smf_sc_tai_t *a, const mb_smf_sc_tai_t *b)
{
    return _tai_equal(a, b);
}

MB_SMF_CLIENT_API mb_smf_sc_tai_t *mb_smf_sc_tai_copy(mb_smf_sc_tai_t *dst, const mb_smf_sc_tai_t *src)
{
    _tai_copy(&dst, src);
    return dst;
}

MB_SMF_CLIENT_API mb_smf_sc_tai_t *mb_smf_sc_tai_set_plmn(mb_smf_sc_tai_t *tai, uint16_t mcc, uint16_t mnc)
{
    ogs_plmn_id_t plmn_id;
    ogs_plmn_id_build(&plmn_id, mcc, mnc, (mnc>99)?3:2);
    _tai_set_plmn_id(tai, &plmn_id);
    return tai;
}

MB_SMF_CLIENT_API mb_smf_sc_tai_t *mb_smf_sc_tai_set_tac(mb_smf_sc_tai_t *tai, uint32_t tac)
{
    _tai_set_tac(tai, tac);
    return tai;
}

MB_SMF_CLIENT_API mb_smf_sc_tai_t *mb_smf_sc_tai_set_network_id(mb_smf_sc_tai_t *tai, uint64_t nid)
{
    _tai_set_network_id(tai, &nid);
    return tai;
}

MB_SMF_CLIENT_API mb_smf_sc_tai_t *mb_smf_sc_tai_unset_network_id(mb_smf_sc_tai_t *tai)
{
    _tai_set_network_id(tai, NULL);
    return tai;
}

/* internal library functions */
ogs_list_t *_tais_patch_list(const ogs_list_t *a, const ogs_list_t *b)
{
    ogs_list_t *patches = NULL;

    if (a && ogs_list_count(a) == 0) a = NULL;
    if (b && ogs_list_count(b) == 0) b = NULL;
    if (a != b) {
        if (!a) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _tais_to_json(b));
            patches = (ogs_list_t*)ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            patches = (ogs_list_t*)ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else {
            int idx = 0;
            mb_smf_sc_tai_t *a_tai = (mb_smf_sc_tai_t*)ogs_list_first(a);
            mb_smf_sc_tai_t *b_tai = (mb_smf_sc_tai_t*)ogs_list_first(b);
            while (a_tai && b_tai && _tai_equal(a_tai, b_tai)) {
                idx++;
                a_tai = (mb_smf_sc_tai_t*)ogs_list_next(a_tai);
                b_tai = (mb_smf_sc_tai_t*)ogs_list_next(b_tai);
            }
            if (idx == 0) {
                /* replace/add whole array */
                _json_patch_t *patch;
                if (ogs_list_count(a) == 0) {
                    patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _tais_to_json(b));
                } else {
                    patch = _json_patch_new(OpenAPI_patch_operation_replace, "/", _tais_to_json(b));
                }
                patches = (ogs_list_t*)ogs_calloc(1,sizeof(*patches));
                ogs_list_add(patches, patch);
            } else {
                /* replace/add/remove array entries */
                while (a_tai || b_tai) {
                    _json_patch_t *patch = NULL;
                    if (!a_tai) {
                        patch = _json_patch_new(OpenAPI_patch_operation_add, "/-", _tai_to_json(b_tai));
                    } else if (!b_tai) {
                        char *path = ogs_msprintf("/%i", idx);
                        patch = _json_patch_new(OpenAPI_patch_operation__remove, path, NULL);
                        ogs_free(path);
                        idx--;
                    } else if (!_tai_equal(a_tai, b_tai)) {
                        char *path = ogs_msprintf("/%i", idx);
                        patch = _json_patch_new(OpenAPI_patch_operation_replace, path, _tai_to_json(b_tai));
                        ogs_free(path);
                    }
                    if (patch) {
                        if (!patches) patches = (ogs_list_t*)ogs_calloc(1,sizeof(*patches));
                        ogs_list_add(patches, patch);
                    }
                    idx++;
                    if (a_tai) a_tai = (mb_smf_sc_tai_t*)ogs_list_next(a_tai);
                    if (b_tai) b_tai = (mb_smf_sc_tai_t*)ogs_list_next(b_tai);
                }
            }
        }
    }

    return patches;
}

OpenAPI_list_t *_tais_to_openapi(const ogs_list_t *tais)
{
    if (!tais) return NULL;
    OpenAPI_list_t *list = OpenAPI_list_create();

    mb_smf_sc_tai_t *tai;
    ogs_list_for_each(tais, tai) {
        OpenAPI_list_add(list, _tai_to_openapi(tai));
    }

    return list;
}

cJSON *_tais_to_json(const ogs_list_t *tais)
{
    if (!tais) return NULL;
    cJSON *json = cJSON_CreateArray();

    mb_smf_sc_tai_t *tai;
    ogs_list_for_each(tais, tai) {
        cJSON_AddItemToArray(json, _tai_to_json(tai));
    }

    return json;
}

mb_smf_sc_tai_t *_tai_new(const ogs_plmn_id_t *plmn_id, uint32_t tac, const uint64_t *nid)
{
    mb_smf_sc_tai_t *ret = _tai_create();
    _tai_set_plmn_id(ret, plmn_id);
    _tai_set_tac(ret, tac);
    _tai_set_network_id(ret, nid);
    return ret;
}

mb_smf_sc_tai_t *_tai_create()
{
    mb_smf_sc_tai_t *ret = (mb_smf_sc_tai_t*)ogs_calloc(1, sizeof(*ret));
    return ret;
}

void _tai_free(mb_smf_sc_tai_t *tai)
{
    if (!tai) return;
    _tai_clear(tai);
    ogs_free(tai);
}

void _tai_clear(mb_smf_sc_tai_t *tai)
{
    if (!tai) return;
    memset(&tai->plmn_id, 0, sizeof(tai->plmn_id));
    tai->tac = 0;
    if (tai->nid) {
        ogs_free(tai->nid);
        tai->nid = NULL;
    }
}

void _tai_copy(mb_smf_sc_tai_t **dst, const mb_smf_sc_tai_t *src)
{
    if (!src) {
        if (*dst) {
            _tai_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = _tai_create();
    } else {
        _tai_clear(*dst);
    }

    memcpy(&(*dst)->plmn_id, &src->plmn_id, sizeof(src->plmn_id));
    (*dst)->tac = src->tac;
    if (src->nid) {
        (*dst)->nid = (uint64_t*)ogs_malloc(sizeof(*(*dst)->nid));
        *(*dst)->nid = *src->nid;
    }
}

bool _tai_equal(const mb_smf_sc_tai_t *a, const mb_smf_sc_tai_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    if (a->tac != b->tac) return false;
    if (!a->nid != !b->nid) return false;
    if (a->nid && *a->nid != *b->nid) return false;
    if (ogs_plmn_id_mcc(&a->plmn_id) != ogs_plmn_id_mcc(&b->plmn_id)) return false;
    if (ogs_plmn_id_mnc(&a->plmn_id) != ogs_plmn_id_mnc(&b->plmn_id)) return false;

    return true;
}

ogs_list_t *_tai_patch_list(const mb_smf_sc_tai_t *a, const mb_smf_sc_tai_t *b)
{
    ogs_list_t *ret = NULL;

    if (a != b) {
        if (!a) {
            /* create entire TAI */
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _tai_to_json(b));
            ret = (ogs_list_t*)ogs_calloc(1, sizeof(*ret));
            ogs_list_add(ret, patch);
        } else if (!b) {
            /* destroy TAI */
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            ret = (ogs_list_t*)ogs_calloc(1, sizeof(*ret));
            ogs_list_add(ret, patch);
        } else {
            if (memcmp(&a->plmn_id, &b->plmn_id, sizeof(a->plmn_id))) {
                OpenAPI_plmn_id_t *plmn_id = ogs_sbi_build_plmn_id(&((mb_smf_sc_tai_t*)b)->plmn_id);
                _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_replace, "/plmnId", OpenAPI_plmn_id_convertToJSON(plmn_id));
                OpenAPI_plmn_id_free(plmn_id);
                ret = (ogs_list_t*)ogs_calloc(1, sizeof(*ret));
                ogs_list_add(ret, patch);
            }
            if (a->tac != b->tac) {
                char *tac = _uint32_to_hex_str(b->tac, 4, 6);
                _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_replace, "/tac", cJSON_CreateString(tac));
                ogs_free(tac);
                if (!ret) ret = (ogs_list_t*)ogs_calloc(1, sizeof(*ret));
                ogs_list_add(ret, patch);
            }
            if (a->nid != b->nid) {
                if (!a->nid) {
                    char *nid = _uint64_to_hex_str(*b->nid, 11, 11);
                    _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/nid", cJSON_CreateString(nid));
                    ogs_free(nid);
                    if (!ret) ret = (ogs_list_t*)ogs_calloc(1, sizeof(*ret));
                    ogs_list_add(ret, patch);
                } else if (!b->nid) {
                    _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/nid", NULL);
                    if (!ret) ret = (ogs_list_t*)ogs_calloc(1, sizeof(*ret));
                    ogs_list_add(ret, patch);
                } else if (*a->nid != *b->nid) {
                    char *nid = _uint64_to_hex_str(*b->nid, 11, 11);
                    _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_replace, "/nid", cJSON_CreateString(nid));
                    ogs_free(nid);
                    if (!ret) ret = (ogs_list_t*)ogs_calloc(1, sizeof(*ret));
                    ogs_list_add(ret, patch);
                }
            }
        }
    }

    return ret;
}

void _tai_set_plmn_id(mb_smf_sc_tai_t *tai, const ogs_plmn_id_t *plmn_id)
{
    if (!tai || !plmn_id) return;
    memcpy(&tai->plmn_id, plmn_id, sizeof(*plmn_id));
}

void _tai_set_tac(mb_smf_sc_tai_t *tai, uint32_t tac)
{
    if (!tai) return;
    tai->tac = tac & 0xFFFFFF;
}

void _tai_set_network_id(mb_smf_sc_tai_t *tai, const uint64_t *nid)
{
    if (!tai) return;
    if (nid) {
        if (!tai->nid) tai->nid = (uint64_t*)ogs_malloc(sizeof(*tai->nid));
        *tai->nid = *nid & 0xFFFFFFFFFFFULL;
    } else {
        if (tai->nid) {
            ogs_free(tai->nid);
            tai->nid = NULL;
        }
    }
}

OpenAPI_tai_t *_tai_to_openapi(const mb_smf_sc_tai_t *tai)
{
    if (!tai) return NULL;

    OpenAPI_plmn_id_t *plmn_id = ogs_sbi_build_plmn_id(&((mb_smf_sc_tai_t*)tai)->plmn_id);
    char *tac = _uint32_to_hex_str(tai->tac, 4, 6);
    char *nid = NULL;
    if (tai->nid) nid = _uint64_to_hex_str(*tai->nid, 11, 11);
    OpenAPI_tai_t *api_tai = OpenAPI_tai_create(plmn_id, tac, nid);
    return api_tai;
}

cJSON *_tai_to_json(const mb_smf_sc_tai_t *tai)
{
    if (!tai) return NULL;

    OpenAPI_tai_t *api_tai = _tai_to_openapi(tai);
    cJSON *json = OpenAPI_tai_convertToJSON(api_tai);
    OpenAPI_tai_free(api_tai);
    return json;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
