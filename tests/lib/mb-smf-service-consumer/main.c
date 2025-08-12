/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk> 
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <stdio.h>

#include "ogs-app.h"
#include "ogs-core.h"
#include "ogs-sbi.h"
#include "openapi/model/nf_type.h"

#include "unit-test.h"

int __mb_smf_log_domain;

int main(int argc, char *argv[])
{
    const unit_test_t **tests_it;
    unit_test_ctx ctx;
    size_t success=0, failed=0, total=0;

    ogs_app_initialize("0.0.1", "unit-test.yaml", (const char* const*)argv);
    ogs_app_parse_local_conf("unit-test");
    ogs_log_add_domain("mb-smf-sc-unit-tests", OGS_LOG_WARN);
    __mb_smf_log_domain = ogs_log_get_domain_id("mb-smf-sc-unit-tests");
    //printf("__mb_smf_log_domain = %i\n", __mb_smf_log_domain);

    ogs_sbi_context_init(OpenAPI_nf_type_AF);

    for (tests_it=unit_tests; *tests_it; tests_it++) {
        const unit_test_t *test = *tests_it;

        total++;
        printf("%.3zi - %s: ", total, test->name);
        if (test->fn(&ctx)) {
            success++;
            printf("OK\n");
        } else {
            failed++;
            printf("FAILED\n");
        }
    }

    printf("%zi/%zi tests passed\n", success, total);

    if (success != total) return 1;
    return 0;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
