#include "ogs-core.h"
#include "ogs-sbi.h"

#include "macros.h"
#include "ext-mbs-service-area.h"
#include "priv_ext-mbs-service-area.h"
#include "geographic-area.h"
#include "priv_geographic-area.h"
#include "json-patch.h"
#include "unit-test.h"

/* The update of an external MBS service area sends what changed, as it does for the civic addresses: an area that is the same in
 * the old and the new session, written out again from separate objects, gives no patch. */

static void free_patches(ogs_list_t *patches)
{
    _json_patch_t *patch, *next;
    if (!patches) return;
    ogs_list_for_each_safe(patches, next, patch) {
        ogs_list_remove(patches, patch);
        OpenAPI_patch_item_free(patch->item);
        ogs_free(patch);
    }
    ogs_free(patches);
}

/* a polygon of three points and a circle, the circle with the given uncertainty */
static mb_smf_sc_ext_mbs_service_area_t *make_area(float uncertainty)
{
    mb_smf_sc_ext_mbs_service_area_t *area = mb_smf_sc_ext_mbs_service_area_new();
    mb_smf_sc_geographic_area_t *polygon = mb_smf_sc_ga_polygon_new();
    mb_smf_sc_geographic_area_t *circle = mb_smf_sc_ga_point_uncertainty_circle_new(12.5, 41.9, uncertainty);

    mb_smf_sc_ga_polygon_add_point(polygon, 12.5, 41.9);
    mb_smf_sc_ga_polygon_add_point(polygon, 13.0, 41.9);
    mb_smf_sc_ga_polygon_add_point(polygon, 12.75, 42.5);
    ogs_list_init(&area->geographic_areas);
    ogs_list_add(&area->geographic_areas, polygon);
    ogs_list_add(&area->geographic_areas, circle);
    return area;
}

static bool test_unchanged_areas_give_no_patch(unit_test_ctx *ctx)
{
    mb_smf_sc_ext_mbs_service_area_t *a = make_area(120.0f);
    mb_smf_sc_ext_mbs_service_area_t *b = make_area(120.0f);
    mb_smf_sc_ext_mbs_service_area_t *c = make_area(150.0f);
    ogs_list_t *patches = NULL;
    bool ok = false;

    UT_BOOL_TRUE_GOTO(_ext_mbs_service_area_equal(a, b), done);

    patches = _ext_mbs_service_area_patch_list(a, b);
    UT_BOOL_TRUE_GOTO(patches == NULL || ogs_list_count(patches) == 0, done);
    free_patches(patches);
    patches = NULL;

    /* only the circle changed: one patch, for the second area */
    UT_BOOL_TRUE_GOTO(!_ext_mbs_service_area_equal(a, c), done);
    patches = _ext_mbs_service_area_patch_list(a, c);
    UT_PTR_NOT_NULL_GOTO(patches, done);
    UT_BOOL_TRUE_GOTO(ogs_list_count(patches) == 1, done);
    {
        _json_patch_t *patch = (_json_patch_t*)ogs_list_first(patches);
        UT_BOOL_TRUE_GOTO(patch->item->op == OpenAPI_patch_operation_replace, done);
        UT_BOOL_TRUE_GOTO(strstr(patch->item->path, "geographicAreaList/1") != NULL, done);
    }
    ok = true;
done:
    if (patches) free_patches(patches);
    if (a) mb_smf_sc_ext_mbs_service_area_delete(a);
    if (b) mb_smf_sc_ext_mbs_service_area_delete(b);
    if (c) mb_smf_sc_ext_mbs_service_area_delete(c);
    return ok;
}

static const unit_test_t test_ext_area_patch_desc = {
    .name = "ext-mbs-service-area: an unchanged geographic area gives no update patch, a changed one gives one",
    .fn = test_unchanged_areas_give_no_patch
};

__attribute__ ((constructor))
static void _init_fn()
{
    register_unit_test(&test_ext_area_patch_desc);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
