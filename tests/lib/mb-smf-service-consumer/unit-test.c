/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk> 
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "unit-test.h"

static size_t next_free_test = 0;
const unit_test_t *unit_tests[128];

bool register_unit_test(const unit_test_t *test)
{
    if (next_free_test == 0) memset(&unit_tests, 0, sizeof(unit_tests));
    unit_tests[next_free_test++] = test;
    assert(next_free_test < sizeof(unit_tests)/sizeof(*unit_tests));
    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
