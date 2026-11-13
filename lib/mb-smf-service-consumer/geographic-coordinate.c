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

#include "geographic-coordinate.h"
#include "priv_geographic-coordinate.h"

static double __rationalise_longitude(double lon);
static double __trim_latitude(double lat);

/* mb_smf_sc_geographic_coordinate Type functions */
MB_SMF_CLIENT_API mb_smf_sc_geographic_coordinate_t *mb_smf_sc_geographic_coordinate_new()
{
    return _geographic_coordinate_new();
}

MB_SMF_CLIENT_API mb_smf_sc_geographic_coordinate_t *mb_smf_sc_geographic_coordinate_new_values(double longitude, double latitude)
{
    mb_smf_sc_geographic_coordinate_t *coord = _geographic_coordinate_new();
    coord->longitude = __rationalise_longitude(longitude);
    coord->latitude = __trim_latitude(latitude);
    return coord;
}

MB_SMF_CLIENT_API void mb_smf_sc_geographic_coordinate_delete(mb_smf_sc_geographic_coordinate_t *coord)
{
    if (!coord) return;
    ogs_free(coord);
}

/* Library internal geographic_coordinate methods (protected) */
ogs_list_t *_geographic_coordinates_patch_list(const ogs_list_t *a, const ogs_list_t *b)
{
    ogs_list_t *patches = NULL;

    if (a && ogs_list_count(a) == 0) a = NULL;
    if (b && ogs_list_count(b) == 0) b = NULL;

    if (a != b) {
        if (!a) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _geographic_coordinates_to_json(b));
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else {
            int idx = 0;
            mb_smf_sc_geographic_coordinate_t *a_coord = (mb_smf_sc_geographic_coordinate_t*)ogs_list_first(a);
            mb_smf_sc_geographic_coordinate_t *b_coord = (mb_smf_sc_geographic_coordinate_t*)ogs_list_first(b);
            while (a_coord || b_coord) {
                _json_patch_t *patch = NULL;
                if (!a_coord) {
                    patch = _json_patch_new(OpenAPI_patch_operation_add, "/-", _geographic_coordinate_to_json(b_coord));
                } else if (!b_coord) {
                    char *path = ogs_msprintf("/%i", idx);
                    patch = _json_patch_new(OpenAPI_patch_operation__remove, path, NULL);
                    ogs_free(path);
                    idx--;
                } else if (!_geographic_coordinate_equal(a_coord, b_coord)) {
                    char *path = ogs_msprintf("/%i", idx);
                    patch = _json_patch_new(OpenAPI_patch_operation_replace, path, _geographic_coordinate_to_json(b_coord));
                    ogs_free(path);
                }
                if (patch) {
                    if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                    ogs_list_add(patches, patch);
                }
                idx++;
                if (a_coord) a_coord = (mb_smf_sc_geographic_coordinate_t*)ogs_list_next(a_coord);
                if (b_coord) b_coord = (mb_smf_sc_geographic_coordinate_t*)ogs_list_next(b_coord);
            }
        }
    }

    return patches;
}

cJSON *_geographic_coordinates_to_json(const ogs_list_t *geog_coords)
{
    if (!geog_coords) return NULL;

    cJSON *json = cJSON_CreateArray();

    mb_smf_sc_geographic_coordinate_t *coord;
    ogs_list_for_each(geog_coords, coord) {
        cJSON_AddItemToArray(json, _geographic_coordinate_to_json(coord));
    }

    return json;
}

mb_smf_sc_geographic_coordinate_t *_geographic_coordinate_new()
{
    return (mb_smf_sc_geographic_coordinate_t*)ogs_calloc(1, sizeof(mb_smf_sc_geographic_coordinate_t));
}

void _geographic_coordinate_free(mb_smf_sc_geographic_coordinate_t *geog_coord)
{
    if (!geog_coord) return;
    ogs_free(geog_coord);
}

void _geographic_coordinate_clear(mb_smf_sc_geographic_coordinate_t *geog_coord)
{
    if (!geog_coord) return;
    geog_coord->longitude = 0;
    geog_coord->latitude = 0;
}

void _geographic_coordinate_copy(mb_smf_sc_geographic_coordinate_t **dst, const mb_smf_sc_geographic_coordinate_t *src)
{
    if (!src) {
        if (*dst) {
            _geographic_coordinate_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) *dst = _geographic_coordinate_new();

    (*dst)->longitude = __rationalise_longitude(src->longitude);
    (*dst)->latitude = __trim_latitude(src->latitude);
}

void _geographic_coordinate_set(mb_smf_sc_geographic_coordinate_t *geog_coord, double lon, double lat)
{
    if (!geog_coord) return;
    geog_coord->longitude = __rationalise_longitude(lon);
    geog_coord->latitude = __trim_latitude(lat);
}

bool _geographic_coordinate_equal(const mb_smf_sc_geographic_coordinate_t *a, const mb_smf_sc_geographic_coordinate_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    return __rationalise_longitude(a->longitude) == __rationalise_longitude(b->longitude) &&
           __trim_latitude(a->latitude) == __trim_latitude(b->latitude);
}

ogs_list_t *_geographic_coordinate_patch_list(const mb_smf_sc_geographic_coordinate_t *a, const mb_smf_sc_geographic_coordinate_t *b)
{
    ogs_list_t *patches = NULL;

    if (a != b) {
        if (!a) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _geographic_coordinate_to_json(b));
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else {
            if (a->longitude != b->longitude) {
                _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_replace, "/lon", cJSON_CreateNumber(b->longitude));
                patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
            if (a->latitude != b->latitude) {
                _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_replace, "/lat", cJSON_CreateNumber(b->latitude));
                if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                ogs_list_add(patches, patch);
            }
        }
    }

    return patches;
}

cJSON *_geographic_coordinate_to_json(const mb_smf_sc_geographic_coordinate_t *geog_coord)
{
    if (!geog_coord) return NULL;

    cJSON *json = cJSON_CreateObject();

    cJSON_AddItemToObject(json, "lon", cJSON_CreateNumber(geog_coord->longitude));
    cJSON_AddItemToObject(json, "lat", cJSON_CreateNumber(geog_coord->latitude));

    return json;
}

static double __rationalise_longitude(double lon)
{
    while (lon <= -180) lon += 360;
    while (lon > 180) lon -= 360;
    return lon;
}

static double __trim_latitude(double lat)
{
    if (lat<-90) lat = -90;
    if (lat>90) lat = 90;
    return lat;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
