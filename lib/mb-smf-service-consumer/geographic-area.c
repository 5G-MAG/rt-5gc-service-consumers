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
#include "priv_geographic-coordinate.h"

#include "geographic-area.h"
#include "priv_geographic-area.h"

/* mb_smf_sc_geographic_area Type functions */
MB_SMF_CLIENT_API mb_smf_sc_geographic_area_t *mb_smf_sc_ga_point_new(double longitude, double latitude)
{
    mb_smf_sc_geographic_area_t *ret = _geographic_area_new();
    ret->shape = GEOGRAPHIC_AREA_SHAPE_POINT;
    _geographic_coordinate_set(&ret->point, longitude, latitude);

    return ret;
}

MB_SMF_CLIENT_API void mb_smf_sc_geographic_area_delete(mb_smf_sc_geographic_area_t *area)
{
    _geographic_area_free(area);
}

/* Library internal geographic_area methods (protected) */
void _geographic_areas_free(ogs_list_t *geog_areas)
{
    if (!geog_areas) return;

    _geographic_areas_clear(geog_areas);
    ogs_free(geog_areas);
}

void _geographic_areas_clear(ogs_list_t *geog_areas)
{
    if (!geog_areas) return;

    mb_smf_sc_geographic_area_t *next, *area;
    ogs_list_for_each_safe(geog_areas, next, area) {
        ogs_list_remove(geog_areas, area);
        _geographic_area_free(area);
    }
}

void _geographic_areas_copy(ogs_list_t **dst, const ogs_list_t *src)
{
    if (src && ogs_list_count(src) == 0) src = NULL;
    if (!src) {
        if (*dst) {
            _geographic_areas_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = (typeof(*dst))ogs_calloc(1, sizeof(**dst));
    } else {
        _geographic_areas_clear(*dst);
    }

    mb_smf_sc_geographic_area_t *area;
    ogs_list_for_each(src, area) {
        mb_smf_sc_geographic_area_t *new_area = NULL;
        _geographic_area_copy(&new_area, area);
        ogs_list_add(*dst, new_area);
    }
}

bool _geographic_areas_equal(const ogs_list_t *a, const ogs_list_t *b)
{
    if (a && ogs_list_count(a) == 0) a = NULL;
    if (b && ogs_list_count(b) == 0) b = NULL;
    if (a == b) return true;
    if (!a || !b) return false;
    if (ogs_list_count(a) != ogs_list_count(b)) return false;

    ogs_list_t *b_copy = NULL;
    _geographic_areas_copy(&b_copy, b);

    mb_smf_sc_geographic_area_t *a_area;
    ogs_list_for_each(a, a_area) {
        mb_smf_sc_geographic_area_t *next, *b_area;
        bool found = false;
        ogs_list_for_each_safe(b_copy, next, b_area) {
            if (_geographic_area_equal(a_area, b_area)) {
                ogs_list_remove(b_copy, b_area);
                _geographic_area_free(b_area);
                found = true;
                break;
            }
        }
        if (!found) {
            _geographic_areas_free(b_copy);
            return false;
        }
    }

    _geographic_areas_free(b_copy);
    return true;
}

ogs_list_t *_geographic_areas_patch_list(const ogs_list_t *a, const ogs_list_t *b)
{
    ogs_list_t *patches = NULL;

    if (a && ogs_list_count(a) == 0) a = NULL;
    if (b && ogs_list_count(b) == 0) b = NULL;
    if (a != b) {
        if (!a) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _geographic_areas_to_json(b));
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else {
            int idx = 0;
            mb_smf_sc_geographic_area_t *a_geog_area = (mb_smf_sc_geographic_area_t*)ogs_list_first(a);
            mb_smf_sc_geographic_area_t *b_geog_area = (mb_smf_sc_geographic_area_t*)ogs_list_first(b);
            while (a_geog_area || b_geog_area) {
                _json_patch_t *patch = NULL;
                if (!a_geog_area) {
                    patch = _json_patch_new(OpenAPI_patch_operation_add, "/-", _geographic_area_to_json(b_geog_area));
                } else if (!b_geog_area) {
                    char *path = ogs_msprintf("/%i", idx);
                    patch = _json_patch_new(OpenAPI_patch_operation__remove, path, NULL);
                    ogs_free(path);
                    idx--;
                } else {
                    char *path = ogs_msprintf("/%i", idx);
                    patch = _json_patch_new(OpenAPI_patch_operation_replace, path, _geographic_area_to_json(b_geog_area));
                    ogs_free(path);
                }
                if (patch) {
                    if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                    ogs_list_add(patches, patch);
                }
                idx++;
                if (a_geog_area) a_geog_area = (mb_smf_sc_geographic_area_t*)ogs_list_next(a_geog_area);
                if (b_geog_area) b_geog_area = (mb_smf_sc_geographic_area_t*)ogs_list_next(b_geog_area);
            }
        }
    }

    return patches;
}

OpenAPI_list_t *_geographic_areas_to_openapi(const ogs_list_t *geog_areas)
{
    if (!geog_areas || ogs_list_count(geog_areas) == 0) return NULL;
    OpenAPI_list_t *list = OpenAPI_list_create();

    mb_smf_sc_geographic_area_t *geog_area;
    ogs_list_for_each(geog_areas, geog_area) {
        OpenAPI_list_add(list, _geographic_area_to_openapi(geog_area));
    }

    return list;
}

cJSON *_geographic_areas_to_json(const ogs_list_t *geog_areas)
{
    if (!geog_areas) return NULL;
    cJSON *json = cJSON_CreateArray();

    mb_smf_sc_geographic_area_t *geog_area;
    ogs_list_for_each(geog_areas, geog_area) {
        cJSON_AddItemToArray(json, _geographic_area_to_json(geog_area));
    }

    return json;
}

mb_smf_sc_geographic_area_t *_geographic_area_new()
{
    return (mb_smf_sc_geographic_area_t*)ogs_calloc(1,sizeof(mb_smf_sc_geographic_area_t));
}

void _geographic_area_free(mb_smf_sc_geographic_area_t *geog_area)
{
    if (!geog_area) return;
    _geographic_area_clear(geog_area);
    ogs_free(geog_area);
}

void _geographic_area_clear(mb_smf_sc_geographic_area_t *geog_area)
{
    if (!geog_area) return;

    switch (geog_area->shape) {
    case GEOGRAPHIC_AREA_SHAPE_NONE:
    case GEOGRAPHIC_AREA_SHAPE_POINT:
    case GEOGRAPHIC_AREA_SHAPE_POINT_UNCERTAINTY_CIRCLE:
    case GEOGRAPHIC_AREA_SHAPE_POINT_UNCERTAINTY_ELLIPSE:
    case GEOGRAPHIC_AREA_SHAPE_POINT_ALTITUDE:
    case GEOGRAPHIC_AREA_SHAPE_ELLIPSOID_ARC:
        /* nothing to free */
        break;
    case GEOGRAPHIC_AREA_SHAPE_POLYGON:
        /* TODO: loop through polygon and free coords */
        break;
    case GEOGRAPHIC_AREA_SHAPE_POINT_ALTITUDE_UNCERTAINTY:
        if (geog_area->point_altitude_uncertainty.v_confidence) {
            ogs_free(geog_area->point_altitude_uncertainty.v_confidence);
            geog_area->point_altitude_uncertainty.v_confidence = NULL;
        }
        break;
    case GEOGRAPHIC_AREA_SHAPE_LOCAL_2D_POINT_UNCERTAINTY_ELLIPSE:
        if (geog_area->local_2d_point_uncertainty_ellipse.local_origin.coordinate_id) {
            ogs_free(geog_area->local_2d_point_uncertainty_ellipse.local_origin.coordinate_id);
            geog_area->local_2d_point_uncertainty_ellipse.local_origin.coordinate_id = NULL;
        }
        if (geog_area->local_2d_point_uncertainty_ellipse.local_origin.point) {
            ogs_free(geog_area->local_2d_point_uncertainty_ellipse.local_origin.point);
            geog_area->local_2d_point_uncertainty_ellipse.local_origin.point = NULL;
        }
        if (geog_area->local_2d_point_uncertainty_ellipse.local_origin.area) {
            _geographic_area_free(geog_area->local_2d_point_uncertainty_ellipse.local_origin.area);
            geog_area->local_2d_point_uncertainty_ellipse.local_origin.area = NULL;
        }
        if (geog_area->local_2d_point_uncertainty_ellipse.point.z) {
            ogs_free(geog_area->local_2d_point_uncertainty_ellipse.point.z);
            geog_area->local_2d_point_uncertainty_ellipse.point.z = NULL;
        }
        break;
    case GEOGRAPHIC_AREA_SHAPE_LOCAL_3D_POINT_UNCERTAINTY_ELLIPSOID:
        if (geog_area->local_3d_point_uncertainty_ellipse.local_origin.coordinate_id) {
            ogs_free(geog_area->local_3d_point_uncertainty_ellipse.local_origin.coordinate_id);
            geog_area->local_3d_point_uncertainty_ellipse.local_origin.coordinate_id = NULL;
        }
        if (geog_area->local_3d_point_uncertainty_ellipse.local_origin.point) {
            ogs_free(geog_area->local_3d_point_uncertainty_ellipse.local_origin.point);
            geog_area->local_3d_point_uncertainty_ellipse.local_origin.point = NULL;
        }
        if (geog_area->local_3d_point_uncertainty_ellipse.local_origin.area) {
            _geographic_area_free(geog_area->local_3d_point_uncertainty_ellipse.local_origin.area);
            geog_area->local_3d_point_uncertainty_ellipse.local_origin.area = NULL;
        }
        if (geog_area->local_3d_point_uncertainty_ellipse.point.z) {
            ogs_free(geog_area->local_3d_point_uncertainty_ellipse.point.z);
            geog_area->local_3d_point_uncertainty_ellipse.point.z = NULL;
        }
        if (geog_area->local_3d_point_uncertainty_ellipse.v_confidence) {
            ogs_free(geog_area->local_3d_point_uncertainty_ellipse.v_confidence);
            geog_area->local_3d_point_uncertainty_ellipse.v_confidence = NULL;
        }
        break;
    default:
        ogs_warn("Unknown shape when clearing geographic area");
        break;
    }

    geog_area->shape = GEOGRAPHIC_AREA_SHAPE_NONE;
}

void _geographic_area_copy(mb_smf_sc_geographic_area_t **dst, const mb_smf_sc_geographic_area_t *src)
{
    if (!src) {
        if (*dst) {
            _geographic_area_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = _geographic_area_new();
    } else {
        _geographic_area_clear(*dst);
    }

    memcpy(*dst, src, sizeof(*src));
    switch (src->shape) {
    case GEOGRAPHIC_AREA_SHAPE_NONE:
    case GEOGRAPHIC_AREA_SHAPE_POINT:
    case GEOGRAPHIC_AREA_SHAPE_POINT_UNCERTAINTY_CIRCLE:
    case GEOGRAPHIC_AREA_SHAPE_POINT_UNCERTAINTY_ELLIPSE:
    case GEOGRAPHIC_AREA_SHAPE_POINT_ALTITUDE:
    case GEOGRAPHIC_AREA_SHAPE_ELLIPSOID_ARC:
        /* no pointers or special types, just copy memory, nothing more to do */
        break;
    case GEOGRAPHIC_AREA_SHAPE_POLYGON:
        ogs_list_init(&(*dst)->polygon.points);
        /* TODO: loop through polygon and copy coords */
        break;
    case GEOGRAPHIC_AREA_SHAPE_POINT_ALTITUDE_UNCERTAINTY:
        if (src->point_altitude_uncertainty.v_confidence) {
            (*dst)->point_altitude_uncertainty.v_confidence = (uint8_t*)ogs_malloc(sizeof(uint8_t));
            *(*dst)->point_altitude_uncertainty.v_confidence = *src->point_altitude_uncertainty.v_confidence;
        }
        break;
    case GEOGRAPHIC_AREA_SHAPE_LOCAL_2D_POINT_UNCERTAINTY_ELLIPSE:
        if (src->local_2d_point_uncertainty_ellipse.local_origin.coordinate_id) {
            (*dst)->local_2d_point_uncertainty_ellipse.local_origin.coordinate_id = ogs_strdup(src->local_2d_point_uncertainty_ellipse.local_origin.coordinate_id);
        }
        if (src->local_2d_point_uncertainty_ellipse.local_origin.point) {
            (*dst)->local_2d_point_uncertainty_ellipse.local_origin.point = (mb_smf_sc_geographic_coordinate_t*)ogs_malloc(sizeof(mb_smf_sc_geographic_coordinate_t));
            memcpy((*dst)->local_2d_point_uncertainty_ellipse.local_origin.point, src->local_2d_point_uncertainty_ellipse.local_origin.point, sizeof(mb_smf_sc_geographic_coordinate_t));
        }
        if (src->local_2d_point_uncertainty_ellipse.local_origin.area) {
            (*dst)->local_2d_point_uncertainty_ellipse.local_origin.area = NULL;
            _geographic_area_copy(&(*dst)->local_2d_point_uncertainty_ellipse.local_origin.area, src->local_2d_point_uncertainty_ellipse.local_origin.area);
        }
        if (src->local_2d_point_uncertainty_ellipse.point.z) {
            (*dst)->local_2d_point_uncertainty_ellipse.point.z = (float*)ogs_malloc(sizeof(float));
            *(*dst)->local_2d_point_uncertainty_ellipse.point.z = *src->local_2d_point_uncertainty_ellipse.point.z;
        }
        break;
    case GEOGRAPHIC_AREA_SHAPE_LOCAL_3D_POINT_UNCERTAINTY_ELLIPSOID:
        if (src->local_3d_point_uncertainty_ellipse.local_origin.coordinate_id) {
            (*dst)->local_3d_point_uncertainty_ellipse.local_origin.coordinate_id = ogs_strdup(src->local_3d_point_uncertainty_ellipse.local_origin.coordinate_id);
        }
        if (src->local_3d_point_uncertainty_ellipse.local_origin.point) {
            (*dst)->local_3d_point_uncertainty_ellipse.local_origin.point = (mb_smf_sc_geographic_coordinate_t*)ogs_malloc(sizeof(mb_smf_sc_geographic_coordinate_t));
            memcpy((*dst)->local_3d_point_uncertainty_ellipse.local_origin.point, src->local_3d_point_uncertainty_ellipse.local_origin.point, sizeof(mb_smf_sc_geographic_coordinate_t));
        }
        if (src->local_3d_point_uncertainty_ellipse.local_origin.area) {
            (*dst)->local_3d_point_uncertainty_ellipse.local_origin.area = NULL;
            _geographic_area_copy(&(*dst)->local_3d_point_uncertainty_ellipse.local_origin.area, src->local_3d_point_uncertainty_ellipse.local_origin.area);
        }
        if (src->local_3d_point_uncertainty_ellipse.point.z) {
            (*dst)->local_3d_point_uncertainty_ellipse.point.z = (float*)ogs_malloc(sizeof(float));
            *(*dst)->local_3d_point_uncertainty_ellipse.point.z = *src->local_3d_point_uncertainty_ellipse.point.z;
        }
        if (src->local_3d_point_uncertainty_ellipse.v_confidence) {
            (*dst)->local_3d_point_uncertainty_ellipse.v_confidence = (uint8_t*)ogs_malloc(sizeof(uint8_t));
            *(*dst)->local_3d_point_uncertainty_ellipse.v_confidence = *src->local_3d_point_uncertainty_ellipse.v_confidence;
        }
        break;
    default:
        ogs_warn("Unknown shape when copying geographic area");
        break;
    }
}

bool _geographic_area_equal(const mb_smf_sc_geographic_area_t *a, const mb_smf_sc_geographic_area_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    /* TODO: compare geographic areas */

    return false;
}

ogs_list_t *_geographic_area_patch_list(const mb_smf_sc_geographic_area_t *a, const mb_smf_sc_geographic_area_t *b)
{
    ogs_list_t *patches = NULL;

    if (a != b) {
        if (!a) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _geographic_area_to_json(b));
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (a->shape != b->shape) {
            /* if the shape changes, then replace the whole lot */
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_replace, "/", _geographic_area_to_json(b));
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else {
            switch (a->shape) {
            case GEOGRAPHIC_AREA_SHAPE_POINT:
                patches = _json_patches_append_list(patches, _ga_point_patch_list(a, b), NULL);
                break;
            default:
                ogs_warn("Update patch list function not implemented for geographic area type %i", a->shape);
                break;
            }
        }
    }

    return patches;
}

OpenAPI_geographic_area_t *_geographic_area_to_openapi(const mb_smf_sc_geographic_area_t *geog_area)
{
    if (!geog_area) return NULL;

    /* There is a problem with the Open5GS OpenAPI_geographic_area_t in that it does not handle the shape enumeration properly */

    return NULL;
}

cJSON *_geographic_area_to_json(const mb_smf_sc_geographic_area_t *geog_area)
{
    if (!geog_area) return NULL;
#if 0
    /* If OpenAPI_geographic_area_t worked properly */
    OpenAPI_geographic_area_t *api_geog_area = _geographic_area_to_openapi(geog_area);
    if (!api_geog_area) return NULL;
    cJSON *json = OpenAPI_geographic_area_convertToJSON(api_geog_area);
    OpenAPI_geographic_area_free(api_geog_area);
#else
    /* because OpenAPI_geographic_area_t does not work properly, construct the JSON directly */
    cJSON *json = cJSON_CreateObject();
    switch (geog_area->shape) {
    case GEOGRAPHIC_AREA_SHAPE_POINT:
        cJSON_AddItemToObject(json, "shape", cJSON_CreateString("POINT"));
        cJSON_AddItemToObject(json, "point", _geographic_coordinate_to_json(&geog_area->point));
        break;
    default:
        ogs_warn("Attempt to create JSON not implemented for geographic area type %i", geog_area->shape);
        break;
    }
#endif
    return json;
}

ogs_list_t *_ga_point_patch_list(const mb_smf_sc_geographic_area_t *a, const mb_smf_sc_geographic_area_t *b)
{
    if (!a || !b) return NULL;
    if (a->shape != GEOGRAPHIC_AREA_SHAPE_POINT || b->shape != GEOGRAPHIC_AREA_SHAPE_POINT) return NULL;

    ogs_list_t *patches = NULL;

    patches = _json_patches_append_list(patches, _geographic_coordinate_patch_list(&a->point, &b->point), "/point");

    return patches;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
