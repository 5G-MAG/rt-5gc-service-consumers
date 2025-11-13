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

#include "macros.h"
#include "geographic-coordinate.h"

#include "geographic-area.h"
#include "priv_geographic-area.h"

/* mb_smf_sc_geographic_area Type functions */
MB_SMF_CLIENT_API mb_smf_sc_geographic_area_t *mb_smf_sc_ga_point_new(double longitude, double latitude)
{
    mb_smf_sc_geographic_area_t *ret = _geographic_area_new();
    ret->shape = GEOGRAPHIC_AREA_SHAPE_POINT;

    /* rationalise longitude to be (-180 .. 180] */
    while (longitude > 180) longitude -= 360;
    while (longitude <= -180) longitude += 360;

    ret->point.longitude = longitude;

    /* clip latitude to [-90 .. 90] */
    if (latitude < -90) latitude = -90;
    if (latitude > 90) latitude = 90;

    ret->point.latitude = latitude;
}

MB_SMF_CLIENT_API void mb_smf_sc_geographic_area_delete(mb_smf_sc_geographic_area_t *area)
{
    _geographic_area_free(area);
}

/* Library internal geographic_area methods (protected) */
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

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
