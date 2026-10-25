/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#ifndef PCF_TEST_BDT_H
#define PCF_TEST_BDT_H

#include "abts.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The BDT policy client of the library against the BDT policy control service of the PCF */
void pcf_test_bdt_func(abts_case *tc, void *data);

#ifdef __cplusplus
}
#endif

#endif /* PCF_TEST_BDT_H */
