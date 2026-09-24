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
#include "ogs-sbi.h"
#include "openapi/model/mbs_service_area.h"
#include "openapi/model/ncgi_tai.h"
#include "openapi/model/ncgi.h"
#include "openapi/model/plmn_id.h"
#include "openapi/model/tai.h"

#include "openapi/model/update_rsp_data.h"
#include "openapi/model/ext_mbs_session.h"

#include "priv_mbs-service-area.h"
#include "priv_mbs-session.h"
#include "priv_mbs-status-subscription.h"

#include "nmbsmf-mbs-session-handle.h"
#include "priv_ncgi-tai.h"
#include "priv_ncgi.h"
#include "priv_tai.h"

#include "unit-test.h"

/* MBS Service Area values shared by the cases below.
 *
 * MNC "001" is deliberate: it is three digits but numerically below 100, the case a converter that
 * derives the digit count from the numeric value gets wrong. */
#define UT_TAI_MCC          234
#define UT_TAI_MNC          1
#define UT_TAI_MNC_LEN      3
#define UT_TAI_TAC          0x00abcd
#define UT_NCGI_TAI_MCC     310
#define UT_NCGI_TAI_MNC     26
#define UT_NCGI_TAI_MNC_LEN 2
#define UT_NCGI_TAI_TAC     0x112233
#define UT_NR_CELL_ID       0x123456789ULL
#define UT_NID              0x0000000002aULL

static OpenAPI_mbs_service_area_t *__build_api_area()
{
    /* tai_list: one TAI, no Nid */
    OpenAPI_tai_t *api_tai = OpenAPI_tai_create(OpenAPI_plmn_id_create(ogs_strdup("234"), ogs_strdup("001")),
                                                ogs_strdup("00abcd"), NULL);
    OpenAPI_list_t *tai_list = OpenAPI_list_create();
    OpenAPI_list_add(tai_list, api_tai);

    /* ncgi_list: one NcgiTai holding one cell that carries a Nid */
    OpenAPI_ncgi_t *api_ncgi = OpenAPI_ncgi_create(OpenAPI_plmn_id_create(ogs_strdup("310"), ogs_strdup("26")),
                                                  ogs_strdup("123456789"), ogs_strdup("0000000002a"));
    OpenAPI_list_t *cell_list = OpenAPI_list_create();
    OpenAPI_list_add(cell_list, api_ncgi);

    OpenAPI_tai_t *api_ncgi_tai_tai = OpenAPI_tai_create(OpenAPI_plmn_id_create(ogs_strdup("310"), ogs_strdup("26")),
                                                        ogs_strdup("112233"), NULL);
    OpenAPI_list_t *ncgi_list = OpenAPI_list_create();
    OpenAPI_list_add(ncgi_list, OpenAPI_ncgi_tai_create(api_ncgi_tai_tai, cell_list));

    return OpenAPI_mbs_service_area_create(ncgi_list, tai_list);
}

static bool test_mbs_service_area_from_openapi(unit_test_ctx *ctx)
{
    OpenAPI_mbs_service_area_t *api_area = __build_api_area();

    mb_smf_sc_mbs_service_area_t *area = _mbs_service_area_from_openapi(api_area);
    OpenAPI_mbs_service_area_free(api_area);

    UT_PTR_NOT_NULL(area);
    UT_SIZE_T_EQUAL(ogs_list_count(&area->tais), 1);
    UT_SIZE_T_EQUAL(ogs_list_count(&area->ncgi_tais), 1);

    /* the plain TAI */
    mb_smf_sc_tai_t *tai = ogs_list_first(&area->tais);
    UT_PTR_NOT_NULL(tai);
    UT_INT_EQUAL(ogs_plmn_id_mcc(&tai->plmn_id), UT_TAI_MCC);
    UT_INT_EQUAL(ogs_plmn_id_mnc(&tai->plmn_id), UT_TAI_MNC);
    UT_INT_EQUAL(ogs_plmn_id_mnc_len(&tai->plmn_id), UT_TAI_MNC_LEN);
    UT_INT_EQUAL(tai->tac, UT_TAI_TAC);
    UT_PTR_NULL(tai->nid);

    /* the NCGI TAI and its single cell */
    mb_smf_sc_ncgi_tai_t *ncgi_tai = ogs_list_first(&area->ncgi_tais);
    UT_PTR_NOT_NULL(ncgi_tai);
    UT_INT_EQUAL(ogs_plmn_id_mcc(&ncgi_tai->tai.plmn_id), UT_NCGI_TAI_MCC);
    UT_INT_EQUAL(ogs_plmn_id_mnc(&ncgi_tai->tai.plmn_id), UT_NCGI_TAI_MNC);
    UT_INT_EQUAL(ogs_plmn_id_mnc_len(&ncgi_tai->tai.plmn_id), UT_NCGI_TAI_MNC_LEN);
    UT_INT_EQUAL(ncgi_tai->tai.tac, UT_NCGI_TAI_TAC);
    UT_SIZE_T_EQUAL(ogs_list_count(&ncgi_tai->ncgis), 1);

    mb_smf_sc_ncgi_t *ncgi = ogs_list_first(&ncgi_tai->ncgis);
    UT_PTR_NOT_NULL(ncgi);
    UT_INT_EQUAL(ogs_plmn_id_mcc(&ncgi->plmn_id), UT_NCGI_TAI_MCC);
    UT_INT_EQUAL(ogs_plmn_id_mnc(&ncgi->plmn_id), UT_NCGI_TAI_MNC);
    UT_BOOL_TRUE(ncgi->nr_cell_id == UT_NR_CELL_ID);
    UT_PTR_NOT_NULL(ncgi->nid);
    UT_BOOL_TRUE(*ncgi->nid == UT_NID);

    _mbs_service_area_free(area);

    return true;
}

static bool test_mbs_service_area_openapi_round_trip(unit_test_ctx *ctx)
{
    OpenAPI_mbs_service_area_t *api_area = __build_api_area();
    mb_smf_sc_mbs_service_area_t *area = _mbs_service_area_from_openapi(api_area);
    OpenAPI_mbs_service_area_free(api_area);
    UT_PTR_NOT_NULL(area);

    /* out and back again: the hex encoding of Tac, NrCellId and Nid has to survive both directions */
    OpenAPI_mbs_service_area_t *api_area2 = _mbs_service_area_to_openapi(area);
    UT_PTR_NOT_NULL(api_area2);
    mb_smf_sc_mbs_service_area_t *area2 = _mbs_service_area_from_openapi(api_area2);
    OpenAPI_mbs_service_area_free(api_area2);
    UT_PTR_NOT_NULL(area2);

    UT_BOOL_TRUE(_mbs_service_area_equal(area, area2));

    /* _tai_equal() compares the MNC numerically, so assert the digit count separately */
    mb_smf_sc_tai_t *tai2 = ogs_list_first(&area2->tais);
    UT_PTR_NOT_NULL(tai2);
    UT_INT_EQUAL(ogs_plmn_id_mnc_len(&tai2->plmn_id), UT_TAI_MNC_LEN);

    _mbs_service_area_free(area);
    _mbs_service_area_free(area2);

    return true;
}

static bool test_mbs_service_area_from_openapi_absent(unit_test_ctx *ctx)
{
    /* An MBS Session response without a redMbsServArea leaves the consumer with no reduced area */
    UT_PTR_NULL(_mbs_service_area_from_openapi(NULL));

    /* An empty MbsServiceArea is a statement by the MB-SMF, not an absent one: keep it distinct */
    OpenAPI_mbs_service_area_t *api_area = OpenAPI_mbs_service_area_create(NULL, NULL);
    mb_smf_sc_mbs_service_area_t *area = _mbs_service_area_from_openapi(api_area);
    OpenAPI_mbs_service_area_free(api_area);

    UT_PTR_NOT_NULL(area);
    UT_SIZE_T_EQUAL(ogs_list_count(&area->tais), 0);
    UT_SIZE_T_EQUAL(ogs_list_count(&area->ncgi_tais), 0);

    _mbs_service_area_free(area);

    return true;
}

static ogs_sbi_message_t *__update_response(OpenAPI_mbs_service_area_t *reduced, bool with_body)
{
    ogs_sbi_message_t *message = (ogs_sbi_message_t*)ogs_calloc(1, sizeof(*message));
    message->res_status = OGS_SBI_HTTP_STATUS_OK;

    if (with_body) {
        OpenAPI_ext_mbs_session_t *ext =
                (OpenAPI_ext_mbs_session_t*)ogs_calloc(1, sizeof(*ext));
        ext->red_mbs_service_area = reduced;
        message->UpdateRspData = OpenAPI_update_rsp_data_create(ext);
    }

    return message;
}

static bool test_patch_response_reduced_service_area(unit_test_ctx *ctx)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)ogs_calloc(1, sizeof(*session));
    _mbs_session_public_copy(&session->previous_session, &session->session);

    ogs_sbi_response_t response;
    memset(&response, 0, sizeof(response));

    /* An Update the MB-SMF could not cover in full answers 200 OK with the part it kept. */
    ogs_sbi_message_t *message = __update_response(__build_api_area(), true);
    _nmbsmf_mbs_session_patch_response(session, message, &response);
    OpenAPI_update_rsp_data_free(message->UpdateRspData);
    ogs_free(message);

    UT_PTR_NOT_NULL(session->session.red_mbs_service_area);
    UT_SIZE_T_EQUAL(ogs_list_count(&session->session.red_mbs_service_area->tais), 1);
    UT_SIZE_T_EQUAL(ogs_list_count(&session->session.red_mbs_service_area->ncgi_tais), 1);

    /* A success carrying no representation, the 204 of step 2a, says nothing about the service
     * area and must not discard what the MB-SMF reported earlier. */
    message = __update_response(NULL, false);
    message->res_status = OGS_SBI_HTTP_STATUS_NO_CONTENT;
    _nmbsmf_mbs_session_patch_response(session, message, &response);
    ogs_free(message);

    UT_PTR_NOT_NULL(session->session.red_mbs_service_area);
    UT_SIZE_T_EQUAL(ogs_list_count(&session->session.red_mbs_service_area->tais), 1);

    /* A representation that carries no reduced area is the MB-SMF saying the session is no longer
     * reduced, which is not the same as saying nothing. */
    message = __update_response(NULL, true);
    _nmbsmf_mbs_session_patch_response(session, message, &response);
    OpenAPI_update_rsp_data_free(message->UpdateRspData);
    ogs_free(message);

    UT_PTR_NULL(session->session.red_mbs_service_area);

    _mbs_session_public_clear(&session->session);
    if (session->previous_session) {
        _mbs_session_public_clear(session->previous_session);
        ogs_free(session->previous_session);
    }
    ogs_free(session);

    return true;
}

static const unit_test_t test_mbs_service_area_from_openapi_desc = {
    .name = "mbs-service-area: parse an MbsServiceArea from OpenAPI",
    .fn = test_mbs_service_area_from_openapi
};

static const unit_test_t test_patch_response_reduced_service_area_desc = {
    .name = "mbs-session: an Update response reports a reduced MBS service area",
    .fn = test_patch_response_reduced_service_area
};

static const unit_test_t test_mbs_service_area_openapi_round_trip_desc = {
    .name = "mbs-service-area: MbsServiceArea OpenAPI round trip",
    .fn = test_mbs_service_area_openapi_round_trip
};

static const unit_test_t test_mbs_service_area_from_openapi_absent_desc = {
    .name = "mbs-service-area: parse an absent and an empty MbsServiceArea",
    .fn = test_mbs_service_area_from_openapi_absent
};

__attribute__ ((constructor))
static void _init_parsers_fn()
{
    register_unit_test(&test_mbs_service_area_from_openapi_desc);
    register_unit_test(&test_mbs_service_area_openapi_round_trip_desc);
    register_unit_test(&test_patch_response_reduced_service_area_desc);
    register_unit_test(&test_mbs_service_area_from_openapi_absent_desc);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
