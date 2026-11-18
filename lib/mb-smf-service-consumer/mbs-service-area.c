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
#include "ogs-crypt.h"
#include "ogs-sbi.h"

#include "macros.h"
#include "json-patch.h"
#include "priv_ncgi-tai.h"
#include "priv_tai.h"

#include "mbs-service-area.h"
#include "priv_mbs-service-area.h"

/* mb_smf_sc_mbs_service_area Type functions */
MB_SMF_CLIENT_API mb_smf_sc_mbs_service_area_t *mb_smf_sc_mbs_service_area_new()
{
    return _mbs_service_area_new();
}

MB_SMF_CLIENT_API mb_smf_sc_mbs_service_area_t *mb_smf_sc_mbs_service_area_new_ncgi_tai(const mb_smf_sc_ncgi_tai_t *ncgi_tai)
{
    mb_smf_sc_mbs_service_area_t *ret = _mbs_service_area_new();
    mb_smf_sc_ncgi_tai_t *new_ncgi_tai = NULL;
    _ncgi_tai_copy(&new_ncgi_tai, ncgi_tai);
    if (new_ncgi_tai) ogs_list_add(&ret->ncgi_tais, new_ncgi_tai);
    return ret;
}

MB_SMF_CLIENT_API mb_smf_sc_mbs_service_area_t *mb_smf_sc_mbs_service_area_new_tai(const mb_smf_sc_tai_t *tai)
{
    mb_smf_sc_mbs_service_area_t *ret = _mbs_service_area_new();
    mb_smf_sc_tai_t *new_tai = NULL;
    _tai_copy(&new_tai, tai);
    if (new_tai) ogs_list_add(&ret->tais, new_tai);
    return ret;
}

MB_SMF_CLIENT_API void mb_smf_sc_mbs_service_area_delete(mb_smf_sc_mbs_service_area_t *mbs_service_area)
{
    _mbs_service_area_free(mbs_service_area);
}

/* Intended Service Areas: the coordinates are kept as octets and travel as base64 (TS 29.571 clause 5.9.4.4) */
static mb_smf_sc_intended_service_area_t *_intended_service_area_new(const uint8_t *coordinates, size_t len)
{
    mb_smf_sc_intended_service_area_t *isa;
    if (!coordinates || len == 0) return NULL;
    isa = (mb_smf_sc_intended_service_area_t*)ogs_calloc(1, sizeof(*isa));
    if (!isa) return NULL;
    isa->coordinates = (uint8_t*)ogs_malloc(len);
    if (!isa->coordinates) {
        ogs_free(isa);
        return NULL;
    }
    memcpy(isa->coordinates, coordinates, len);
    isa->coordinates_len = len;
    return isa;
}

static void _intended_service_area_free(mb_smf_sc_intended_service_area_t *isa)
{
    if (!isa) return;
    if (isa->coordinates) ogs_free(isa->coordinates);
    ogs_free(isa);
}

static bool _intended_service_area_equal(const mb_smf_sc_intended_service_area_t *a, const mb_smf_sc_intended_service_area_t *b)
{
    return a->coordinates_len == b->coordinates_len && memcmp(a->coordinates, b->coordinates, a->coordinates_len) == 0;
}

/* returns a newly allocated base64 string, or NULL */
static char *_intended_service_area_to_base64(const mb_smf_sc_intended_service_area_t *isa)
{
    int size = ogs_base64_encoded_size(isa->coordinates_len);
    char *out;
    if (size <= 0) return NULL;
    out = (char*)ogs_calloc(1, size);
    if (!out) return NULL;
    if (ogs_base64_encode_from_buffer(out, size, isa->coordinates, isa->coordinates_len) <= 0) {
        ogs_free(out);
        return NULL;
    }
    return out;
}

static mb_smf_sc_intended_service_area_t *_intended_service_area_from_base64(const char *b64)
{
    int size;
    int len;
    uint8_t *buf;
    mb_smf_sc_intended_service_area_t *isa;

    if (!b64) return NULL;
    size = ogs_base64_decoded_size(b64);
    if (size <= 0) return NULL;
    buf = (uint8_t*)ogs_calloc(1, size);
    if (!buf) return NULL;
    len = ogs_base64_decode_to_buffer(buf, size, b64);
    if (len <= 0) {
        ogs_free(buf);
        return NULL;
    }
    isa = _intended_service_area_new(buf, len);
    ogs_free(buf);
    return isa;
}

static void _intended_service_areas_clear(ogs_list_t *list)
{
    mb_smf_sc_intended_service_area_t *isa, *next;
    ogs_list_for_each_safe(list, next, isa) {
        ogs_list_remove(list, isa);
        _intended_service_area_free(isa);
    }
}

static cJSON *_intended_service_areas_to_json(const ogs_list_t *list)
{
    cJSON *array = cJSON_CreateArray();
    mb_smf_sc_intended_service_area_t *isa;
    if (!array) return NULL;
    ogs_list_for_each(list, isa) {
        char *b64 = _intended_service_area_to_base64(isa);
        if (b64) {
            cJSON_AddItemToArray(array, cJSON_CreateString(b64));
            ogs_free(b64);
        }
    }
    return array;
}

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_service_area_add_intended_service_area(mb_smf_sc_mbs_service_area_t *mbs_service_area, const uint8_t *coordinates, size_t coordinates_len)
{
    mb_smf_sc_intended_service_area_t *isa;
    if (!mbs_service_area) return false;
    isa = _intended_service_area_new(coordinates, coordinates_len);
    if (!isa) return false;
    ogs_list_add(&mbs_service_area->intended_service_areas, isa);
    return true;
}

/* Library internal mbs_service_area methods (protected) */
mb_smf_sc_mbs_service_area_t *_mbs_service_area_new()
{
    return (mb_smf_sc_mbs_service_area_t*)ogs_calloc(1,sizeof(mb_smf_sc_mbs_service_area_t));
}

void _mbs_service_area_free(mb_smf_sc_mbs_service_area_t *svc_areas)
{
    if (!svc_areas) return;
    _mbs_service_area_clear(svc_areas);
    ogs_free(svc_areas);
}

void _mbs_service_area_clear(mb_smf_sc_mbs_service_area_t *svc_areas)
{
    if (!svc_areas) return;

    mb_smf_sc_ncgi_tai_t *ncgi_tai, *next_ncgi_tai;
    mb_smf_sc_tai_t *tai, *next_tai;

    ogs_list_for_each_safe(&svc_areas->ncgi_tais, next_ncgi_tai, ncgi_tai) {
        ogs_list_remove(&svc_areas->ncgi_tais, ncgi_tai);
        _ncgi_tai_free(ncgi_tai);
    }
    ogs_list_for_each_safe(&svc_areas->tais, next_tai, tai) {
        ogs_list_remove(&svc_areas->tais, tai);
        _tai_free(tai);
    }
    _intended_service_areas_clear(&svc_areas->intended_service_areas);
}

void _mbs_service_area_copy(mb_smf_sc_mbs_service_area_t **dst, const mb_smf_sc_mbs_service_area_t *src)
{
    mb_smf_sc_ncgi_tai_t *ncgi_tai;
    mb_smf_sc_tai_t *tai;
    mb_smf_sc_intended_service_area_t *isa;

    if (!src) {
        _mbs_service_area_free(*dst);
        *dst = NULL;
        return;
    }

    if (!*dst) {
        *dst = _mbs_service_area_new();
    } else {
        /* clear old service areas lists */
        _mbs_service_area_clear(*dst);
    }

    /* copy src to dst */
    ogs_list_for_each(&src->ncgi_tais, ncgi_tai) {
        mb_smf_sc_ncgi_tai_t *new_ncgi_tai = NULL;
        _ncgi_tai_copy(&new_ncgi_tai, ncgi_tai);
        ogs_list_add(&((*dst)->ncgi_tais), new_ncgi_tai);
    }
    ogs_list_for_each(&src->tais, tai) {
        mb_smf_sc_tai_t *new_tai = NULL;
        _tai_copy(&new_tai, tai);
        ogs_list_add(&((*dst)->tais), new_tai);
    }
    ogs_list_for_each(&src->intended_service_areas, isa) {
        mb_smf_sc_intended_service_area_t *new_isa = _intended_service_area_new(isa->coordinates, isa->coordinates_len);
        if (new_isa) ogs_list_add(&((*dst)->intended_service_areas), new_isa);
    }
}

bool _mbs_service_area_equal(const mb_smf_sc_mbs_service_area_t *a, const mb_smf_sc_mbs_service_area_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    /* check array sizes */
    if (ogs_list_count(&a->ncgi_tais) != ogs_list_count(&b->ncgi_tais)) return false;
    if (ogs_list_count(&a->tais) != ogs_list_count(&b->tais)) return false;
    if (ogs_list_count(&a->intended_service_areas) != ogs_list_count(&b->intended_service_areas)) return false;

    /* check array contents (any order matching) */
    mb_smf_sc_mbs_service_area_t *b_copy = NULL;
    _mbs_service_area_copy(&b_copy, b);
    mb_smf_sc_ncgi_tai_t *a_ncgi_tai;
    ogs_list_for_each(&a->ncgi_tais, a_ncgi_tai) {
        mb_smf_sc_ncgi_tai_t *next, *b_ncgi_tai;
        bool found = false;
        ogs_list_for_each_safe(&b_copy->ncgi_tais, next, b_ncgi_tai) {
            if (_ncgi_tai_equal(a_ncgi_tai, b_ncgi_tai)) {
                ogs_list_remove(&b_copy->ncgi_tais, b_ncgi_tai);
                _ncgi_tai_free(b_ncgi_tai);
                found = true;
                break;
            }
        }
        if (!found) {
            _mbs_service_area_free(b_copy);
            return false;
        }
    }
    mb_smf_sc_tai_t *a_tai;
    ogs_list_for_each(&a->tais, a_tai) {
        mb_smf_sc_tai_t *next, *b_tai;
        bool found = false;
        ogs_list_for_each_safe(&b_copy->tais, next, b_tai) {
            if (_tai_equal(a_tai, b_tai)) {
                ogs_list_remove(&b_copy->tais, b_tai);
                _tai_free(b_tai);
                found = true;
                break;
            }
        }
        if (!found) {
            _mbs_service_area_free(b_copy);
            return false;
        }
    }
    mb_smf_sc_intended_service_area_t *a_isa;
    ogs_list_for_each(&a->intended_service_areas, a_isa) {
        mb_smf_sc_intended_service_area_t *next, *b_isa;
        bool found = false;
        ogs_list_for_each_safe(&b_copy->intended_service_areas, next, b_isa) {
            if (_intended_service_area_equal(a_isa, b_isa)) {
                ogs_list_remove(&b_copy->intended_service_areas, b_isa);
                _intended_service_area_free(b_isa);
                found = true;
                break;
            }
        }
        if (!found) {
            _mbs_service_area_free(b_copy);
            return false;
        }
    }
    _mbs_service_area_free(b_copy);
    return true;
}

/* The Intended Service Areas of an update replace the stored list as a whole when they differ (the order is not significant) */
static ogs_list_t *_intended_service_areas_patch_list(const ogs_list_t *a, const ogs_list_t *b)
{
    ogs_list_t *patches = NULL;
    mb_smf_sc_intended_service_area_t *x, *y;
    bool same = ogs_list_count(a) == ogs_list_count(b);
    OpenAPI_patch_operation_e op;

    if (same) {
        ogs_list_for_each(a, x) {
            bool found = false;
            ogs_list_for_each(b, y) {
                if (_intended_service_area_equal(x, y)) { found = true; break; }
            }
            if (!found) { same = false; break; }
        }
    }
    if (same) return NULL;

    if (ogs_list_count(b) == 0) {
        op = OpenAPI_patch_operation__remove;
    } else if (ogs_list_count(a) == 0) {
        op = OpenAPI_patch_operation_add;
    } else {
        op = OpenAPI_patch_operation_replace;
    }
    patches = (ogs_list_t*)ogs_calloc(1, sizeof(*patches));
    ogs_list_add(patches, _json_patch_new(op, "/", op == OpenAPI_patch_operation__remove ? NULL : _intended_service_areas_to_json(b)));
    return patches;
}

ogs_list_t *_mbs_service_area_patch_list(const mb_smf_sc_mbs_service_area_t *a, const mb_smf_sc_mbs_service_area_t *b)
{
    ogs_list_t *patches = NULL;

    if (a != b) {
        if (!a) {
            /* create whole new MbsServiceArea */
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _mbs_service_area_to_json(b));
            patches = (ogs_list_t*)ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            patches = (ogs_list_t*)ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else {
            patches = _json_patches_append_list(patches, _ncgi_tais_patch_list(&a->ncgi_tais, &b->ncgi_tais), "/ncgiList");
            patches = _json_patches_append_list(patches, _tais_patch_list(&a->tais, &b->tais), "/taiList");
            patches = _json_patches_append_list(patches, _intended_service_areas_patch_list(&a->intended_service_areas, &b->intended_service_areas), "/intendedServAreaList");
        }
    }

    return patches;
}

cJSON *_mbs_service_area_to_json(const mb_smf_sc_mbs_service_area_t *area)
{
    if (!area) return NULL;

    OpenAPI_mbs_service_area_t *api_area = _mbs_service_area_to_openapi(area);
    if (!api_area) return NULL;
    cJSON *json = OpenAPI_mbs_service_area_convertToJSON(api_area);
    OpenAPI_mbs_service_area_free(api_area);
    return json;
}

OpenAPI_mbs_service_area_t *_mbs_service_area_to_openapi(const mb_smf_sc_mbs_service_area_t *area)
{
    OpenAPI_mbs_service_area_t *api_area = OpenAPI_mbs_service_area_create(NULL, NULL, NULL);

    if (ogs_list_count(&area->ncgi_tais) > 0) {
        api_area->ncgi_list = OpenAPI_list_create();
        mb_smf_sc_ncgi_tai_t *ncgi_tai;
        ogs_list_for_each(&area->ncgi_tais, ncgi_tai) {
            OpenAPI_ncgi_tai_t *api_ncgi_tai = _ncgi_tai_to_openapi(ncgi_tai);
            if (api_ncgi_tai) OpenAPI_list_add(api_area->ncgi_list, api_ncgi_tai);
        }
    }

    if (ogs_list_count(&area->tais) > 0) {
        api_area->tai_list = OpenAPI_list_create();
        mb_smf_sc_tai_t *tai;
        ogs_list_for_each(&area->tais, tai) {
            OpenAPI_tai_t *api_tai = _tai_to_openapi(tai);
            if (api_tai) OpenAPI_list_add(api_area->tai_list, api_tai);
        }
    }

    if (ogs_list_count(&area->intended_service_areas) > 0) {
        mb_smf_sc_intended_service_area_t *isa;
        api_area->intended_serv_area_list = OpenAPI_list_create();
        ogs_list_for_each(&area->intended_service_areas, isa) {
            char *b64 = _intended_service_area_to_base64(isa);
            if (b64) OpenAPI_list_add(api_area->intended_serv_area_list, b64);
        }
    }

    return api_area;
}


mb_smf_sc_mbs_service_area_t *_mbs_service_area_from_openapi(const OpenAPI_mbs_service_area_t *api_area)
{
    if (!api_area) return NULL;

    mb_smf_sc_mbs_service_area_t *area = _mbs_service_area_new();
    if (!area) return NULL;

    _ncgi_tais_from_openapi(&area->ncgi_tais, api_area->ncgi_list);
    _tais_from_openapi(&area->tais, api_area->tai_list);
    if (api_area->intended_serv_area_list) {
        OpenAPI_lnode_t *node;
        OpenAPI_list_for_each(api_area->intended_serv_area_list, node) {
            mb_smf_sc_intended_service_area_t *isa = _intended_service_area_from_base64((const char*)node->data);
            if (isa) ogs_list_add(&area->intended_service_areas, isa);
        }
    }

    return area;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
