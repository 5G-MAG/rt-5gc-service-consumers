/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2026 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <stdbool.h>
#include <string.h>

#include "ogs-core.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "geographic-area.h"
#include "priv_geographic-area.h"

#include "unit-test.h"

/* Each shape of TS 29.572 GeographicArea is written as the JSON of that shape alone: its shape name and the
 * attributes it requires, and none of the attributes of the other shapes. */

static bool has_keys(const cJSON *json, const char *keys[], int count)
{
    int i;
    if (cJSON_GetArraySize((cJSON*)json) != count) return false;
    for (i = 0; i < count; i++) {
        if (!cJSON_HasObjectItem((cJSON*)json, keys[i])) return false;
    }
    return true;
}

static bool check_shape(mb_smf_sc_geographic_area_t *area, const char *shape, const char *keys[], int count)
{
    cJSON *json = NULL;
    mb_smf_sc_geographic_area_t *copy = NULL;
    bool ok = false;

    UT_PTR_NOT_NULL_GOTO(area, done);
    json = _geographic_area_to_json(area);
    UT_PTR_NOT_NULL_GOTO(json, done);
    UT_BOOL_TRUE_GOTO(cJSON_IsString(cJSON_GetObjectItem(json, "shape")), done);
    UT_BOOL_TRUE_GOTO(strcmp(cJSON_GetObjectItem(json, "shape")->valuestring, shape) == 0, done);
    UT_BOOL_TRUE_GOTO(has_keys(json, keys, count), done);

    /* a copy is equal to its source and is a separate object, and a different area is not equal */
    _geographic_area_copy(&copy, area);
    UT_PTR_NOT_NULL_GOTO(copy, done);
    UT_BOOL_TRUE_GOTO(_geographic_area_equal(area, copy), done);
    ok = true;
done:
    if (json) cJSON_Delete(json);
    if (copy) mb_smf_sc_geographic_area_delete(copy);
    if (area) mb_smf_sc_geographic_area_delete(area);
    return ok;
}

static bool test_shapes(unit_test_ctx *ctx)
{
    const uint8_t v_conf = 90;
    const char *point[] = {"shape", "point"};
    const char *circle[] = {"shape", "point", "uncertainty"};
    const char *ellipse[] = {"shape", "point", "uncertaintyEllipse", "confidence"};
    const char *polygon[] = {"shape", "pointList"};
    const char *altitude[] = {"shape", "point", "altitude"};
    const char *altitude_unc[] = {"shape", "point", "altitude", "uncertaintyEllipse", "uncertaintyAltitude", "confidence", "vConfidence"};
    const char *arc[] = {"shape", "point", "innerRadius", "uncertaintyRadius", "offsetAngle", "includedAngle", "confidence"};
    mb_smf_sc_geographic_area_t *poly = mb_smf_sc_ga_polygon_new();

    UT_BOOL_TRUE(check_shape(mb_smf_sc_ga_point_new(6.1, 46.2), "POINT", point, 2));
    UT_BOOL_TRUE(check_shape(mb_smf_sc_ga_point_uncertainty_circle_new(6.1, 46.2, 500), "POINT_UNCERTAINTY_CIRCLE", circle, 3));
    UT_BOOL_TRUE(check_shape(mb_smf_sc_ga_point_uncertainty_ellipse_new(6.1, 46.2, 300, 200, 45, 68),
                             "POINT_UNCERTAINTY_ELLIPSE", ellipse, 4));
    UT_BOOL_TRUE(mb_smf_sc_ga_polygon_add_point(poly, 6.1, 46.2));
    UT_BOOL_TRUE(mb_smf_sc_ga_polygon_add_point(poly, 6.1, 46.3));
    UT_BOOL_TRUE(mb_smf_sc_ga_polygon_add_point(poly, 6.2, 46.3));
    UT_BOOL_TRUE(check_shape(poly, "POLYGON", polygon, 2));
    UT_BOOL_TRUE(check_shape(mb_smf_sc_ga_point_altitude_new(6.1, 46.2, 420.5), "POINT_ALTITUDE", altitude, 3));
    UT_BOOL_TRUE(check_shape(mb_smf_sc_ga_point_altitude_uncertainty_new(6.1, 46.2, 420.5, 300, 200, 45, 12.5, 68, &v_conf),
                             "POINT_ALTITUDE_UNCERTAINTY", altitude_unc, 7));
    UT_BOOL_TRUE(check_shape(mb_smf_sc_ga_ellipsoid_arc_new(6.1, 46.2, 1000, 50, 90, 120, 68), "ELLIPSOID_ARC", arc, 7));

    {
        /* areas that differ are not equal */
        mb_smf_sc_geographic_area_t *a = mb_smf_sc_ga_point_uncertainty_circle_new(6.1, 46.2, 500);
        mb_smf_sc_geographic_area_t *b = mb_smf_sc_ga_point_uncertainty_circle_new(6.1, 46.2, 501);
        bool differ = !_geographic_area_equal(a, b);
        mb_smf_sc_geographic_area_delete(a);
        mb_smf_sc_geographic_area_delete(b);
        UT_BOOL_TRUE(differ);
    }
    return true;
}

static const unit_test_t test_shapes_desc = {
    .name = "geographic-area: every shape is written as that shape alone",
    .fn = test_shapes
};

__attribute__ ((constructor))
static void _init_fn()
{
    register_unit_test(&test_shapes_desc);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
