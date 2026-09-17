/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <stdbool.h>

#include "ogs-core.h"
#include "ogs-proto.h"

#include "priv_ncgi-tai.h"
#include "priv_ncgi.h"
#include "priv_tai.h"

#include "unit-test.h"

static mb_smf_sc_ncgi_t *__cell(uint64_t nr_cell_id, const uint64_t *nid)
{
    mb_smf_sc_ncgi_t *ncgi = mb_smf_sc_ncgi_new_values(234, 1, nr_cell_id, nid);
    return ncgi;
}

static mb_smf_sc_ncgi_tai_t *__ncgi_tai_with_cells(uint64_t first, uint64_t second, const uint64_t *nid)
{
    mb_smf_sc_tai_t *tai = mb_smf_sc_tai_new_len(234, 1, 3, 0xABCD, NULL);
    mb_smf_sc_ncgi_t *cell = __cell(first, nid);
    mb_smf_sc_ncgi_tai_t *ncgi_tai = mb_smf_sc_ncgi_tai_new_values(tai, cell);
    mb_smf_sc_tai_free(tai);
    mb_smf_sc_ncgi_delete(cell);

    if (second) ogs_list_add(&ncgi_tai->ncgis, __cell(second, NULL));

    return ncgi_tai;
}

static bool test_ncgi_tai_copy_carries_cells(unit_test_ctx *ctx)
{
    mb_smf_sc_ncgi_tai_t *src = __ncgi_tai_with_cells(0x11, 0x22, NULL);
    UT_SIZE_T_EQUAL(ogs_list_count(&src->ncgis), 2);

    mb_smf_sc_ncgi_tai_t *dst = NULL;
    _ncgi_tai_copy(&dst, src);

    UT_PTR_NOT_NULL(dst);
    UT_SIZE_T_EQUAL(ogs_list_count(&dst->ncgis), 2);

    /* copied into a list of its own, not aliased into the source's */
    mb_smf_sc_ncgi_t *src_first = ogs_list_first(&src->ncgis);
    mb_smf_sc_ncgi_t *dst_first = ogs_list_first(&dst->ncgis);
    UT_PTR_NOT_NULL(dst_first);
    UT_BOOL_TRUE(src_first != dst_first);
    UT_BOOL_TRUE(_ncgi_equal(src_first, dst_first));

    _ncgi_tai_free(src);
    _ncgi_tai_free(dst);

    return true;
}

static bool test_ncgi_tai_equal_compares_cells(unit_test_ctx *ctx)
{
    mb_smf_sc_ncgi_tai_t *a = __ncgi_tai_with_cells(0x11, 0x22, NULL);
    mb_smf_sc_ncgi_tai_t *same = __ncgi_tai_with_cells(0x11, 0x22, NULL);
    mb_smf_sc_ncgi_tai_t *reordered = __ncgi_tai_with_cells(0x22, 0x11, NULL);
    mb_smf_sc_ncgi_tai_t *different = __ncgi_tai_with_cells(0x11, 0x33, NULL);
    mb_smf_sc_ncgi_tai_t *fewer = __ncgi_tai_with_cells(0x11, 0, NULL);

    UT_BOOL_TRUE(_ncgi_tai_equal(a, same));
    UT_BOOL_TRUE(_ncgi_tai_equal(a, reordered));
    UT_BOOL_FALSE(_ncgi_tai_equal(a, different));
    UT_BOOL_FALSE(_ncgi_tai_equal(a, fewer));

    _ncgi_tai_free(a);
    _ncgi_tai_free(same);
    _ncgi_tai_free(reordered);
    _ncgi_tai_free(different);
    _ncgi_tai_free(fewer);

    return true;
}

static bool test_ncgi_tai_clear_empties_cells(unit_test_ctx *ctx)
{
    mb_smf_sc_ncgi_tai_t *ncgi_tai = __ncgi_tai_with_cells(0x11, 0x22, NULL);
    UT_SIZE_T_EQUAL(ogs_list_count(&ncgi_tai->ncgis), 2);

    _ncgi_tai_clear(ncgi_tai);

    UT_SIZE_T_EQUAL(ogs_list_count(&ncgi_tai->ncgis), 0);

    _ncgi_tai_free(ncgi_tai);

    return true;
}

static const unit_test_t test_ncgi_tai_copy_carries_cells_desc = {
    .name = "ncgi-tai: a copy carries the cell list",
    .fn = test_ncgi_tai_copy_carries_cells
};

static const unit_test_t test_ncgi_tai_equal_compares_cells_desc = {
    .name = "ncgi-tai: equality compares the cell list, in any order",
    .fn = test_ncgi_tai_equal_compares_cells
};

static const unit_test_t test_ncgi_tai_clear_empties_cells_desc = {
    .name = "ncgi-tai: clearing empties the cell list",
    .fn = test_ncgi_tai_clear_empties_cells
};

__attribute__ ((constructor))
static void _init_ncgi_tai_cells_fn()
{
    register_unit_test(&test_ncgi_tai_copy_carries_cells_desc);
    register_unit_test(&test_ncgi_tai_equal_compares_cells_desc);
    register_unit_test(&test_ncgi_tai_clear_empties_cells_desc);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
