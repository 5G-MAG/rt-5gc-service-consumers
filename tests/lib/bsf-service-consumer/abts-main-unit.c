/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2026 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "test-app.h"
#include "bsf-test-unit.h"

/* The requests and the answers of the library are checked without a core network: no NF is started */

const struct testlist {
    abts_suite *(*func)(abts_suite *suite);
} alltests[] = {
    {test_bsf_unit},
    {NULL},
};

static void terminate(void)
{
    bsf_unit_terminate();
    ogs_app_terminate();
}

static void initialize(const char *const argv[])
{
    int rv;

    rv = ogs_app_initialize(NULL, NULL, argv);
    ogs_assert(rv == OGS_OK);

    rv = bsf_unit_initialise();
    ogs_assert(rv == OGS_OK);
}

int main(int argc, const char *const argv[])
{
    int i;
    abts_suite *suite = NULL;

    atexit(terminate);
    test_app_run(argc, argv, "sample.yaml", initialize);

    for (i = 0; alltests[i].func; i++)
        suite = alltests[i].func(suite);

    return abts_report(suite);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
