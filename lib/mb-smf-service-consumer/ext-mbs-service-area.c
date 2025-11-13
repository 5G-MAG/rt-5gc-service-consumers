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

#include "macros.h"
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
    mb_smf_sc_geographic_area_t *area;
    mb_smf_sc_civic_address_t *addr;

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
    ogs_list_for_each(&src->geographic_areas, area) {
        mb_smf_sc_geographic_area_t *new_area = NULL;
        _geographic_area_copy(&new_area, area);
        ogs_list_add(&((*dst)->geographic_areas), new_area);
    }
    ogs_list_for_each(&src->civic_addresses, addr) {
        mb_smf_sc_civic_address_t *new_addr = NULL;
        _civic_address_copy(&new_addr, addr);
        ogs_list_add(&((*dst)->civic_addresses), new_addr);
    }
}

bool _ext_mbs_service_area_equal(const mb_smf_sc_ext_mbs_service_area_t *a, const mb_smf_sc_ext_mbs_service_area_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    /* compare array lengths */
    if (ogs_list_count(&a->geographic_areas) != ogs_list_count(&b->geographic_areas)) return false;
    if (ogs_list_count(&a->civic_addresses) != ogs_list_count(&b->civic_addresses)) return false;

    /* compare array contents (any order matching) */
    mb_smf_sc_ext_mbs_service_area_t *b_copy = NULL;
    _ext_mbs_service_area_copy(&b_copy, b);

    mb_smf_sc_geographic_area_t *a_area;
    ogs_list_for_each(&a->geographic_areas, a_area) {
        mb_smf_sc_geographic_area_t *next, *b_area;
        bool found = false;
        ogs_list_for_each_safe(&b_copy->geographic_areas, next, b_area) {
            if (_geographic_area_equal(a_area, b_area)) {
                ogs_list_remove(&b_copy->geographic_areas, b_area);
                _geographic_area_free(b_area);
                found = true;
                break;
            }
        }
        if (!found) {
            _ext_mbs_service_area_free(b_copy);
            return false;
        }
    }

    mb_smf_sc_civic_address_t *a_addr;
    ogs_list_for_each(&a->civic_addresses, a_addr) {
        mb_smf_sc_civic_address_t *next, *b_addr;
        bool found = false;
        ogs_list_for_each_safe(&b_copy->civic_addresses, next, b_addr) {
            if (_civic_address_equal(a_addr, b_addr)) {
                ogs_list_remove(&b_copy->civic_addresses, b_addr);
                _civic_address_free(b_addr);
                found = true;
                break;
            }
        }
        if (!found) {
            _ext_mbs_service_area_free(b_copy);
            return false;
        }
    }

    _ext_mbs_service_area_free(b_copy);
    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
