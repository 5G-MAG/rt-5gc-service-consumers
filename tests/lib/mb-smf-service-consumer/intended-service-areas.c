/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2026 5G-MAG
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include "ogs-core.h"
#include "ogs-sbi.h"

#include "macros.h"
#include "mbs-service-area.h"
#include "priv_mbs-service-area.h"
#include "json-patch.h"
#include "unit-test.h"

/* NR NTN Intended Service Areas of an MbsServiceArea (TS 29.571 V19.8.0 clause 5.9.4.4): intendedServAreaList is a list of
 * base64 strings of the Intended Service Area Coordinates IE of TS 38.413 clause 9.3.1.273. */

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

static bool test_isa_roundtrip(unit_test_ctx *ctx)
{
    static const uint8_t first[] = {0x01, 0x02, 0x03, 0xfe, 0xff};
    static const uint8_t second[] = {0xa0};
    mb_smf_sc_mbs_service_area_t *area = mb_smf_sc_mbs_service_area_new();
    mb_smf_sc_mbs_service_area_t *copy = NULL, *back = NULL;
    OpenAPI_mbs_service_area_t *api = NULL;
    cJSON *json = NULL;
    cJSON *list;
    bool ok = false;

    UT_PTR_NOT_NULL_GOTO(area, done);
    UT_BOOL_TRUE_GOTO(mb_smf_sc_mbs_service_area_add_intended_service_area(area, first, sizeof(first)), done);
    UT_BOOL_TRUE_GOTO(mb_smf_sc_mbs_service_area_add_intended_service_area(area, second, sizeof(second)), done);
    /* invalid arguments are refused */
    UT_BOOL_TRUE_GOTO(!mb_smf_sc_mbs_service_area_add_intended_service_area(area, NULL, 3), done);
    UT_BOOL_TRUE_GOTO(!mb_smf_sc_mbs_service_area_add_intended_service_area(area, first, 0), done);
    UT_BOOL_TRUE_GOTO(!mb_smf_sc_mbs_service_area_add_intended_service_area(NULL, first, sizeof(first)), done);
    UT_BOOL_TRUE_GOTO(ogs_list_count(&area->intended_service_areas) == 2, done);

    /* SBI JSON: intendedServAreaList of base64 strings */
    json = _mbs_service_area_to_json(area);
    UT_PTR_NOT_NULL_GOTO(json, done);
    list = cJSON_GetObjectItem(json, "intendedServAreaList");
    UT_PTR_NOT_NULL_GOTO(list, done);
    UT_BOOL_TRUE_GOTO(cJSON_GetArraySize(list) == 2, done);
    UT_BOOL_TRUE_GOTO(strcmp(cJSON_GetArrayItem(list, 0)->valuestring, "AQID/v8=") == 0, done); /* 01 02 03 fe ff */
    UT_BOOL_TRUE_GOTO(strcmp(cJSON_GetArrayItem(list, 1)->valuestring, "oA==") == 0, done);     /* a0 */

    /* parsed back it is the same, and a copy is equal and separate */
    api = _mbs_service_area_to_openapi(area);
    UT_PTR_NOT_NULL_GOTO(api, done);
    back = _mbs_service_area_from_openapi(api);
    UT_PTR_NOT_NULL_GOTO(back, done);
    UT_BOOL_TRUE_GOTO(_mbs_service_area_equal(area, back), done);
    _mbs_service_area_copy(&copy, area);
    UT_PTR_NOT_NULL_GOTO(copy, done);
    UT_BOOL_TRUE_GOTO(_mbs_service_area_equal(area, copy), done);
    UT_BOOL_TRUE_GOTO(ogs_list_first(&copy->intended_service_areas) != ogs_list_first(&area->intended_service_areas), done);
    ok = true;
done:
    if (json) cJSON_Delete(json);
    if (api) OpenAPI_mbs_service_area_free(api);
    if (back) mb_smf_sc_mbs_service_area_delete(back);
    if (copy) mb_smf_sc_mbs_service_area_delete(copy);
    if (area) mb_smf_sc_mbs_service_area_delete(area);
    return ok;
}

static bool test_isa_equality_and_patch(unit_test_ctx *ctx)
{
    static const uint8_t x[] = {0x10, 0x20};
    static const uint8_t y[] = {0x10, 0x21};
    mb_smf_sc_mbs_service_area_t *a = mb_smf_sc_mbs_service_area_new();
    mb_smf_sc_mbs_service_area_t *b = mb_smf_sc_mbs_service_area_new();
    ogs_list_t *patches = NULL;
    bool ok = false;

    UT_PTR_NOT_NULL_GOTO(a, done);
    UT_PTR_NOT_NULL_GOTO(b, done);
    UT_BOOL_TRUE_GOTO(mb_smf_sc_mbs_service_area_add_intended_service_area(a, x, sizeof(x)), done);

    /* an area with one Intended Service Area is not equal to one with none, nor to one with other coordinates */
    UT_BOOL_TRUE_GOTO(!_mbs_service_area_equal(a, b), done);
    UT_BOOL_TRUE_GOTO(mb_smf_sc_mbs_service_area_add_intended_service_area(b, y, sizeof(y)), done);
    UT_BOOL_TRUE_GOTO(!_mbs_service_area_equal(a, b), done);

    /* an update from one list to the other replaces the whole list, an unchanged list gives no patch */
    patches = _mbs_service_area_patch_list(a, b);
    UT_PTR_NOT_NULL_GOTO(patches, done);
    UT_BOOL_TRUE_GOTO(ogs_list_count(patches) == 1, done);
    free_patches(patches);
    patches = NULL;
    patches = _mbs_service_area_patch_list(a, a);
    UT_BOOL_TRUE_GOTO(patches == NULL || ogs_list_count(patches) == 0, done);
    ok = true;
done:
    if (patches) free_patches(patches);
    if (a) mb_smf_sc_mbs_service_area_delete(a);
    if (b) mb_smf_sc_mbs_service_area_delete(b);
    return ok;
}

static const unit_test_t test_isa_roundtrip_desc = {
    .name = "mbs-service-area: NR NTN Intended Service Areas are written as base64, parsed back and copied",
    .fn = test_isa_roundtrip
};
static const unit_test_t test_isa_patch_desc = {
    .name = "mbs-service-area: NR NTN Intended Service Areas in equality and update patches",
    .fn = test_isa_equality_and_patch
};

__attribute__ ((constructor))
static void _init_fn()
{
    register_unit_test(&test_isa_roundtrip_desc);
    register_unit_test(&test_isa_patch_desc);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
