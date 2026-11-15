/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include "nmbsmf-tmgi-handle.c"

#include "openapi/model/plmn_id.h"
#include "openapi/model/tmgi.h"
#include "openapi/model/tmgi_allocated.h"

#include "unit-test.h"

static int ut_tmgi_results = 0;

static void ut_tmgi_cb(mb_smf_sc_tmgi_t *tmgi, int result, const OpenAPI_problem_details_t *problem, void *data)
{
    (void)tmgi;
    (void)problem;
    (void)data;
    if (result == OGS_OK) ut_tmgi_results++;
}

/* The MCC and MNC of a TmgiAllocated response are decimal digit strings (TS 29.571 Mcc: Pattern '^[0-9]{3}$'),
 * kept in the library as the numbers ogs_plmn_id_mcc() and ogs_plmn_id_mnc() give. Values below are chosen so
 * that reading the digits as hexadecimal gives a different number. */
static bool __allocated_plmn_case(const char *mcc, const char *mnc, const char *want_mcc, const char *want_mnc)
{
    _priv_tmgi_t *tmgi = _tmgi_create(ut_tmgi_cb, NULL);
    UT_BOOL_TRUE(tmgi != NULL);
    UT_BOOL_TRUE(_context_add_tmgi(tmgi));

    OpenAPI_list_t *tmgi_list = OpenAPI_list_create();
    OpenAPI_list_add(tmgi_list, OpenAPI_tmgi_create(ogs_strdup("0a1b2c"),
                                                    OpenAPI_plmn_id_create(ogs_strdup(mcc), ogs_strdup(mnc))));
    OpenAPI_tmgi_allocated_t *allocated = OpenAPI_tmgi_allocated_create(tmgi_list, NULL, NULL);

    ogs_sbi_message_t message;
    memset(&message, 0, sizeof(message));
    message.res_status = OGS_SBI_HTTP_STATUS_CREATED;
    message.TmgiAllocated = allocated;

    ut_tmgi_results = 0;
    int rv = _nmbsmf_tmgi_allocated(NULL, &message);
    UT_INT_EQUAL(rv, OGS_OK);
    UT_INT_EQUAL(ut_tmgi_results, 1);

    UT_BOOL_TRUE(tmgi->allocated);
    UT_BOOL_TRUE(strcmp(ogs_plmn_id_mcc_string(&tmgi->tmgi.plmn), want_mcc) == 0);
    UT_BOOL_TRUE(strcmp(ogs_plmn_id_mnc_string(&tmgi->tmgi.plmn), want_mnc) == 0);

    _context_remove_tmgi(tmgi);
    _tmgi_free(tmgi);
    OpenAPI_tmgi_allocated_free(allocated);
    return true;
}

static bool test_tmgi_allocated_plmn_is_decimal(unit_test_ctx *ctx)
{
    (void)ctx;
    UT_BOOL_TRUE(__allocated_plmn_case("262", "01", "262", "01"));
    UT_BOOL_TRUE(__allocated_plmn_case("310", "410", "310", "410"));
    UT_BOOL_TRUE(__allocated_plmn_case("001", "01", "001", "01"));
    return true;
}

static const unit_test_t test_tmgi_allocated_plmn_is_decimal_desc = {
    .name = "nmbsmf-tmgi: the MCC and MNC of a TmgiAllocated response are read as decimal digits",
    .fn = test_tmgi_allocated_plmn_is_decimal
};

__attribute__ ((constructor))
static void _init_tmgi_handle_fn()
{
    register_unit_test(&test_tmgi_allocated_plmn_is_decimal_desc);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
