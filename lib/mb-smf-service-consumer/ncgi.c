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
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "macros.h"
#include "json-patch.h"
#include "utils.h"

#include "ncgi.h"
#include "priv_ncgi.h"

/* mb_smf_sc_ncgi Type functions */
MB_SMF_CLIENT_API mb_smf_sc_ncgi_t *mb_smf_sc_ncgi_new()
{
    return _ncgi_new();
}

MB_SMF_CLIENT_API mb_smf_sc_ncgi_t *mb_smf_sc_ncgi_new_values(uint16_t mcc, uint16_t mnc, uint64_t nr_cell_id, const uint64_t *network_id)
{
    mb_smf_sc_ncgi_t *ret = _ncgi_new();
    ogs_plmn_id_build(&ret->plmn_id, mcc, mnc, mnc<100?2:3);
    ret->nr_cell_id = nr_cell_id;
    if (network_id) {
        ret->nid = (uint64_t*)ogs_malloc(sizeof(*ret->nid));
        *ret->nid = *network_id;
    }

    return ret;
}

MB_SMF_CLIENT_API mb_smf_sc_ncgi_t *mb_smf_sc_ncgi_new_copy(const mb_smf_sc_ncgi_t *other)
{
    mb_smf_sc_ncgi_t *ret = NULL;
    _ncgi_copy(&ret, other);
    return ret;
}

MB_SMF_CLIENT_API void mb_smf_sc_ncgi_delete(mb_smf_sc_ncgi_t *ncgi)
{
    _ncgi_free(ncgi);
}

MB_SMF_CLIENT_API mb_smf_sc_ncgi_t *mb_smf_sc_ncgi_set_plmn_id(mb_smf_sc_ncgi_t *ncgi, uint16_t mcc, uint16_t mnc)
{
    if (ncgi) {
        ogs_plmn_id_build(&ncgi->plmn_id, mcc, mnc, mnc<100?2:3);
    }
    return ncgi;
}

/* Library internal ncgi methods (protected) */
ogs_list_t *_ncgis_patch_list(const ogs_list_t *a, const ogs_list_t *b)
{
    ogs_list_t *patches = NULL;
    if (a && ogs_list_count(a) == 0) a = NULL;
    if (b && ogs_list_count(b) == 0) b = NULL;
    if (a != b) {
        if (!a) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _ncgis_to_json(b));
            patches = (ogs_list_t*)ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            patches = (ogs_list_t*)ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else {
            int idx = 0;
            mb_smf_sc_ncgi_t *a_ncgi = (mb_smf_sc_ncgi_t*)ogs_list_first(a);
            mb_smf_sc_ncgi_t *b_ncgi = (mb_smf_sc_ncgi_t*)ogs_list_first(b);
            while (a_ncgi && b_ncgi && _ncgi_equal(a_ncgi, b_ncgi)) {
                idx++;
                a_ncgi = (mb_smf_sc_ncgi_t*)ogs_list_next(a_ncgi);
                b_ncgi = (mb_smf_sc_ncgi_t*)ogs_list_next(b_ncgi);
            }
            if (idx == 0) {
                /* replace/add whole array */
                _json_patch_t *patch;
                if (ogs_list_count(a) == 0) {
                    patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _ncgis_to_json(b));
                } else {
                    patch = _json_patch_new(OpenAPI_patch_operation_replace, "/", _ncgis_to_json(b));
                }
                patches = (ogs_list_t*)ogs_calloc(1,sizeof(*patches));
                ogs_list_add(patches, patch);
            } else {
                /* replace/add/remove array entries */
                while (a_ncgi || b_ncgi) {
                    char *path = ogs_msprintf("/%i", idx);
                    _json_patch_t *patch = NULL;
                    if (!a_ncgi) {
                        patch = _json_patch_new(OpenAPI_patch_operation_add, path, _ncgi_to_json(b_ncgi));
                    } else if (!b_ncgi) {
                        patch = _json_patch_new(OpenAPI_patch_operation__remove, path, NULL);
                        idx--;
                    } else if (!_ncgi_equal(a_ncgi, b_ncgi)) {
                        patch = _json_patch_new(OpenAPI_patch_operation_replace, path, _ncgi_to_json(b_ncgi));
                    }
                    ogs_free(path);
                    if (patch) {
                        if (!patches) patches = (ogs_list_t*)ogs_calloc(1,sizeof(*patches));
                        ogs_list_add(patches, patch);
                    }
                    idx++;
                    if (a_ncgi) a_ncgi = (mb_smf_sc_ncgi_t*)ogs_list_next(a_ncgi);
                    if (b_ncgi) b_ncgi = (mb_smf_sc_ncgi_t*)ogs_list_next(b_ncgi);
                }
            }
        }
    }

    return patches;
}

OpenAPI_list_t *_ncgis_to_openapi(const ogs_list_t *ncgis)
{
    if (!ncgis) return NULL;
    OpenAPI_list_t *list = OpenAPI_list_create();

    mb_smf_sc_ncgi_t *ncgi;
    ogs_list_for_each(ncgis, ncgi) {
        OpenAPI_list_add(list, _ncgi_to_openapi(ncgi));
    }

    return list;
}

cJSON *_ncgis_to_json(const ogs_list_t *ncgis)
{
    if (!ncgis) return NULL;
    cJSON *json = cJSON_CreateArray();

    mb_smf_sc_ncgi_t *ncgi;
    ogs_list_for_each(ncgis, ncgi) {
        cJSON_AddItemToArray(json, _ncgi_to_json(ncgi));
    }

    return json;
}

mb_smf_sc_ncgi_t *_ncgi_new()
{
    return (mb_smf_sc_ncgi_t*)ogs_calloc(1, sizeof(mb_smf_sc_ncgi_t));
}

void _ncgi_free(mb_smf_sc_ncgi_t *ncgi)
{
    if (!ncgi) return;
    _ncgi_clear(ncgi);
    ogs_free(ncgi);
}

void _ncgi_clear(mb_smf_sc_ncgi_t *ncgi)
{
    if (!ncgi) return;
    memset(&ncgi->plmn_id, 0, sizeof(*ncgi) - sizeof(ogs_lnode_t));
}

void _ncgi_copy(mb_smf_sc_ncgi_t **dst, const mb_smf_sc_ncgi_t *src)
{
    if (!src) {
        if (*dst) {
            _ncgi_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = _ncgi_new();
    } else {
        _ncgi_clear(*dst);
    }

    memcpy(&(*dst)->plmn_id, &src->plmn_id, sizeof(*src) - sizeof(ogs_lnode_t));
}

bool _ncgi_equal(const mb_smf_sc_ncgi_t *a, const mb_smf_sc_ncgi_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    if (a->nr_cell_id != b->nr_cell_id) return false;
    if (memcmp(&a->plmn_id, &b->plmn_id, sizeof(b->plmn_id))) return false;
    if (!a->nid != !b->nid) return false;
    if (a->nid && *a->nid != *b->nid) return false;

    return true;
}

ogs_list_t *_ncgi_patch_list(const mb_smf_sc_ncgi_t *a, const mb_smf_sc_ncgi_t *b)
{
    ogs_list_t *patches = NULL;

    if (a != b) {
        if (!a) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _ncgi_to_json(b));
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else {
            if (memcmp(&a->plmn_id, &b->plmn_id, sizeof(b->plmn_id))) {
                OpenAPI_plmn_id_t *plmn_id = ogs_sbi_build_plmn_id(&((mb_smf_sc_ncgi_t*)b)->plmn_id);
                _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_replace, "/plmnId", OpenAPI_plmn_id_convertToJSON(plmn_id));
                OpenAPI_plmn_id_free(plmn_id);
                patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
            if (a->nr_cell_id != b->nr_cell_id) {
                char *nr_cell_id = _uint64_to_hex_str(b->nr_cell_id, 9, 9);
                _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_replace, "/nrCellId", cJSON_CreateString(nr_cell_id));
                ogs_free(nr_cell_id);
                if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
            if (a->nid != b->nid) {
                if (!a->nid) {
                    char *nid = _uint64_to_hex_str(*b->nid, 11, 11);
                    _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/nid", cJSON_CreateString(nid));
                    ogs_free(nid);
                    if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                    ogs_list_add(patches, patch);
                } else if (!b->nid) {
                    _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/nid", NULL);
                    if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                    ogs_list_add(patches, patch);
                } else if (*a->nid != *b->nid) {
                    char *nid = _uint64_to_hex_str(*b->nid, 11, 11);
                    _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_replace, "/nid", cJSON_CreateString(nid));
                    ogs_free(nid);
                    if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                    ogs_list_add(patches, patch);
                }
            }
        }
    }

    return patches;
}

OpenAPI_ncgi_t *_ncgi_to_openapi(const mb_smf_sc_ncgi_t *ncgi)
{
    if (!ncgi) return NULL;

    OpenAPI_plmn_id_t *plmn_id = ogs_sbi_build_plmn_id(&((mb_smf_sc_ncgi_t*)ncgi)->plmn_id);
    char *nr_cell_id = _uint64_to_hex_str(ncgi->nr_cell_id, 9, 9);
    char *nid = NULL;
    if (ncgi->nid) {
        nid = _uint64_to_hex_str(*ncgi->nid, 11, 11);
    }

    return OpenAPI_ncgi_create(plmn_id, nr_cell_id, nid);
}

cJSON *_ncgi_to_json(const mb_smf_sc_ncgi_t *ncgi)
{
    if (!ncgi) return NULL;
    OpenAPI_ncgi_t *api_ncgi = _ncgi_to_openapi(ncgi);
    if (!api_ncgi) return NULL;
    cJSON *json = OpenAPI_ncgi_convertToJSON(api_ncgi);
    OpenAPI_ncgi_free(api_ncgi);
    return json;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
