/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2026 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef __TESTS_BSF_TEST_UNIT_H
#define __TESTS_BSF_TEST_UNIT_H

#include "test-common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int bsf_unit_initialise(void);
extern void bsf_unit_terminate(void);

extern abts_suite *test_bsf_unit(abts_suite *suite);

#ifdef __cplusplus
}
#endif

#endif /* ifndef __TESTS_BSF_TEST_UNIT_H */

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
