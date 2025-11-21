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
#include "priv_civic-address.h"
#include "priv_geographic-area.h"

#include "ext-mbs-service-area.h"
#include "priv_ext-mbs-service-area.h"

/* mb_smf_sc_ext_mbs_service_area Type functions */
MB_SMF_CLIENT_API mb_smf_sc_ext_mbs_service_area_t *mb_smf_sc_ext_mbs_service_area_new()
{
    return _ext_mbs_service_area_new();
}

MB_SMF_CLIENT_API void mb_smf_sc_ext_mbs_service_area_delete(mb_smf_sc_ext_mbs_service_area_t *area)
{
    _ext_mbs_service_area_free(area);
}

/* Library internal ext_mbs_service_area methods (protected) */
mb_smf_sc_ext_mbs_service_area_t *_ext_mbs_service_area_new()
{
    return (mb_smf_sc_ext_mbs_service_area_t*)ogs_calloc(1,sizeof(mb_smf_sc_ext_mbs_service_area_t));
}

void _ext_mbs_service_area_free(mb_smf_sc_ext_mbs_service_area_t *svc_areas)
{
    if (!svc_areas) return;
    _ext_mbs_service_area_clear(svc_areas);
    ogs_free(svc_areas);
}

void _ext_mbs_service_area_clear(mb_smf_sc_ext_mbs_service_area_t *svc_areas)
{
    mb_smf_sc_geographic_area_t *area, *next_area;
    mb_smf_sc_civic_address_t *addr, *next_addr;

    ogs_list_for_each_safe(&svc_areas->geographic_areas, next_area, area) {
        ogs_list_remove(&svc_areas->geographic_areas, area);
        _geographic_area_free(area);
    }
    ogs_list_for_each_safe(&svc_areas->civic_addresses, next_addr, addr) {
        ogs_list_remove(&svc_areas->civic_addresses, addr);
        _civic_address_free(addr);
    }
}

void _ext_mbs_service_area_copy(mb_smf_sc_ext_mbs_service_area_t **dst, const mb_smf_sc_ext_mbs_service_area_t *src)
{
    if (!src) {
        _ext_mbs_service_area_free(*dst);
        *dst = NULL;
        return;
    }

    if (!*dst) {
        *dst = _ext_mbs_service_area_new();
    } else {
        /* clear old service areas lists */
        _ext_mbs_service_area_clear(*dst);
    }

    /* copy src to dst */
    ogs_list_t *l = &(*dst)->geographic_areas;
    _geographic_areas_copy(&l, &src->geographic_areas);
    l = &(*dst)->civic_addresses;
    _civic_addresses_copy(&l, &src->civic_addresses);
}

bool _ext_mbs_service_area_equal(const mb_smf_sc_ext_mbs_service_area_t *a, const mb_smf_sc_ext_mbs_service_area_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    /* compare array lengths */
    return _geographic_areas_equal(&a->geographic_areas, &b->geographic_areas) &&
           _civic_addresses_equal(&a->civic_addresses, &b->civic_addresses);
}

ogs_list_t *_ext_mbs_service_area_patch_list(const mb_smf_sc_ext_mbs_service_area_t *a, const mb_smf_sc_ext_mbs_service_area_t *b)
{
    ogs_list_t *patches = NULL;

    if (a != b) {
        if (!a) {
            /* create whole new ExtMbsServiceArea */
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _ext_mbs_service_area_to_json(b));
            patches = (ogs_list_t*)ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            patches = (ogs_list_t*)ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else {
            patches = _json_patches_append_list(patches, _geographic_areas_patch_list(&a->geographic_areas, &b->geographic_areas), "/geographicAreaList");
            patches = _json_patches_append_list(patches, _civic_addresses_patch_list(&a->civic_addresses, &b->civic_addresses), "/civicAddressList");
        }
    }

    return patches;
}

OpenAPI_external_mbs_service_area_t *_ext_mbs_service_area_to_openapi(const mb_smf_sc_ext_mbs_service_area_t *area)
{
    if (!area) return NULL;

    OpenAPI_external_mbs_service_area_t *api_area = OpenAPI_external_mbs_service_area_create(
                                                                            _geographic_areas_to_openapi(&area->geographic_areas),
                                                                            _civic_addresses_to_openapi(&area->civic_addresses));

    return api_area;
}

cJSON *_ext_mbs_service_area_to_json(const mb_smf_sc_ext_mbs_service_area_t *area)
{
    if (!area) return NULL;

    OpenAPI_external_mbs_service_area_t *api_area = _ext_mbs_service_area_to_openapi(area);
    if (!api_area) return NULL;

    cJSON *json = OpenAPI_external_mbs_service_area_convertToJSON(api_area);
    OpenAPI_external_mbs_service_area_free(api_area);

    return json;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
