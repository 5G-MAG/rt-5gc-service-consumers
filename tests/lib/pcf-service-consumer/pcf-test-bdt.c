/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#include "ogs-sbi.h"
#include "test-common.h"

#include "pcf-service-consumer.h"

#include "pcf-test-bdt.h"

/* What the last result callback was given; the callbacks run in the thread of the event loop */
static struct {
    volatile int count;
    pcf_bdt_policy_t *policy;
    pcf_bdt_operation_t operation;
    bool success;
    int http_status;
    char cause[64];
    char existing_policy_id[64];
} last;

static void result_cb(pcf_bdt_policy_t *policy, const pcf_bdt_result_t *result, void *user_data)
{
    (void)user_data;
    last.policy = policy;
    last.operation = result->operation;
    last.success = result->success;
    last.http_status = result->http_status;
    ogs_cpystrn(last.cause, result->problem_cause ? result->problem_cause : "", sizeof(last.cause));
    ogs_cpystrn(last.existing_policy_id, result->existing_policy_id ? result->existing_policy_id : "", sizeof(last.existing_policy_id));
    last.count++;
}

/*
 * The library is used from the thread of the event loop; the test calls it from its own thread, so it wakes the loop for the
 * request to go out.
 */
static bool call(bool started)
{
    if (started)
        ogs_pollset_notify(ogs_app()->pollset);
    return started;
}

/* Wait for the next result; false if none comes */
static bool wait_result(int *count)
{
    int i;

    for (i = 0; i < 100; i++) {
        if (last.count > *count) {
            *count = last.count;
            return true;
        }
        ogs_msleep(50);
    }
    return false;
}

/*
 * TS 29.554 V19.2.1 clauses 4.2.2.2, 4.2.3.2 and 4.2.5.2: the library asks for the transfer policies of a Background Data Transfer,
 * reads the Individual BDT policy, selects an offered transfer policy, is told of an equivalent resource (303 See Other) and of a
 * resource that does not exist (404 BDT_POLICY_NOT_FOUND), and deletes the policy.
 */
void pcf_test_bdt_func(abts_case *tc, void *data)
{
    pcf_session_t *session = NULL;
    ogs_sockaddr_t *pcf_address = NULL;
    pcf_bdt_request_t request;
    pcf_bdt_transfer_policy_t t;
    pcf_bdt_policy_t *policy = NULL;
    char id[64], uri[200];
    int count = 0, rv;
    const char *slash;

    rv = ogs_getaddrinfo(&pcf_address, AF_UNSPEC, "127.0.0.13", 7777, AI_PASSIVE);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);
    session = pcf_session_new(pcf_address);
    ABTS_PTR_NOTNULL(tc, session);
    ogs_freeaddrinfo(pcf_address);

    memset(&request, 0, sizeof(request));
    request.asp_id = "consumer-test";
    request.start_time = "2030-03-05T00:00:00Z";
    request.stop_time = "2030-03-05T12:00:00Z";
    request.num_of_ues = 5;
    request.total_volume = 1000000;
    request.downlink_volume = request.uplink_volume = request.duration = -1;

    /* a request that is not complete is not sent */
    request.num_of_ues = 0;
    ABTS_TRUE(tc, !pcf_session_create_bdt_policy(session, &request, result_cb, NULL));
    request.num_of_ues = 5;

    /* create: the PCF offers a transfer policy for the desired window */
    ABTS_TRUE(tc, call(pcf_session_create_bdt_policy(session, &request, result_cb, NULL)));
    ABTS_TRUE(tc, wait_result(&count));
    ABTS_INT_EQUAL(tc, PCF_BDT_OP_CREATE, last.operation);
    ABTS_TRUE(tc, last.success);
    ABTS_INT_EQUAL(tc, OGS_SBI_HTTP_STATUS_CREATED, last.http_status);
    policy = last.policy;
    ABTS_PTR_NOTNULL(tc, policy);
    ABTS_PTR_NOTNULL(tc, pcf_bdt_policy_get_ref_id(policy));
    ABTS_PTR_NOTNULL(tc, strstr(pcf_bdt_policy_get_uri(policy), "/npcf-bdtpolicycontrol/v1/bdtpolicies/"));
    ogs_cpystrn(uri, pcf_bdt_policy_get_uri(policy), sizeof(uri));
    slash = strrchr(uri, '/');
    ogs_cpystrn(id, slash + 1, sizeof(id));
    /* the configuration of the PCF (configs/vonr.yaml.in): a policy for the window 01:00 to 05:00 UTC and one without a window */
    ABTS_INT_EQUAL(tc, 2, pcf_bdt_policy_get_num_transfer_policies(policy));
    ABTS_TRUE(tc, pcf_bdt_policy_get_transfer_policy(policy, 0, &t));
    ABTS_INT_EQUAL(tc, 1, t.trans_policy_id);
    ABTS_INT_EQUAL(tc, 10, t.rating_group);
    ABTS_STR_EQUAL(tc, "2030-03-05T01:00:00Z", t.start_time);
    ABTS_STR_EQUAL(tc, "2030-03-05T05:00:00Z", t.stop_time);
    ABTS_STR_EQUAL(tc, "10000000 bps", t.max_bit_rate_dl);
    ABTS_STR_EQUAL(tc, "2000000 bps", t.max_bit_rate_ul);
    ABTS_TRUE(tc, pcf_bdt_policy_get_transfer_policy(policy, 1, &t));
    ABTS_INT_EQUAL(tc, 2, t.trans_policy_id);
    ABTS_INT_EQUAL(tc, 20, t.rating_group);
    ABTS_STR_EQUAL(tc, "2030-03-05T00:00:00Z", t.start_time);
    ABTS_STR_EQUAL(tc, "2030-03-05T12:00:00Z", t.stop_time);
    ABTS_PTR_EQUAL(tc, NULL, t.max_bit_rate_dl);
    ABTS_TRUE(tc, !pcf_bdt_policy_get_transfer_policy(policy, 2, &t));
    ABTS_INT_EQUAL(tc, 0, pcf_bdt_policy_get_selected_transfer_policy_id(policy));

    /* a transfer policy that was not offered is not selected */
    ABTS_TRUE(tc, !pcf_bdt_policy_select(policy, 7));
    ABTS_TRUE(tc, !pcf_bdt_policy_select(policy, 0));

    /* select the offered one */
    ABTS_TRUE(tc, call(pcf_bdt_policy_select(policy, 2)));
    ABTS_TRUE(tc, wait_result(&count));
    ABTS_INT_EQUAL(tc, PCF_BDT_OP_SELECT, last.operation);
    ABTS_TRUE(tc, last.success);
    ABTS_INT_EQUAL(tc, OGS_SBI_HTTP_STATUS_OK, last.http_status);
    ABTS_INT_EQUAL(tc, 2, pcf_bdt_policy_get_selected_transfer_policy_id(policy));

    /* the same service requirements: 303 See Other, with the id of the existing resource; nothing is kept */
    ABTS_TRUE(tc, call(pcf_session_create_bdt_policy(session, &request, result_cb, NULL)));
    ABTS_TRUE(tc, wait_result(&count));
    ABTS_INT_EQUAL(tc, PCF_BDT_OP_CREATE, last.operation);
    ABTS_TRUE(tc, !last.success);
    ABTS_INT_EQUAL(tc, OGS_SBI_HTTP_STATUS_SEE_OTHER, last.http_status);
    ABTS_STR_EQUAL(tc, id, last.existing_policy_id);

    /* read the existing resource by its id */
    ABTS_TRUE(tc, call(pcf_session_get_bdt_policy(session, id, result_cb, NULL)));
    ABTS_TRUE(tc, wait_result(&count));
    ABTS_INT_EQUAL(tc, PCF_BDT_OP_GET, last.operation);
    ABTS_TRUE(tc, last.success);
    ABTS_INT_EQUAL(tc, OGS_SBI_HTTP_STATUS_OK, last.http_status);
    ABTS_STR_EQUAL(tc, pcf_bdt_policy_get_ref_id(policy), pcf_bdt_policy_get_ref_id(last.policy));
    ABTS_INT_EQUAL(tc, 2, pcf_bdt_policy_get_selected_transfer_policy_id(last.policy));
    pcf_bdt_policy_free(last.policy);
    ogs_pollset_notify(ogs_app()->pollset);
    ABTS_TRUE(tc, wait_result(&count));
    ABTS_INT_EQUAL(tc, PCF_BDT_OP_DELETE, last.operation);
    ABTS_TRUE(tc, last.success);
    ABTS_INT_EQUAL(tc, OGS_SBI_HTTP_STATUS_NO_CONTENT, last.http_status);

    /* it is gone: the other object finds out by reading it */
    ABTS_TRUE(tc, call(pcf_session_get_bdt_policy(session, id, result_cb, NULL)));
    ABTS_TRUE(tc, wait_result(&count));
    ABTS_INT_EQUAL(tc, PCF_BDT_OP_GET, last.operation);
    ABTS_TRUE(tc, !last.success);
    ABTS_INT_EQUAL(tc, OGS_SBI_HTTP_STATUS_NOT_FOUND, last.http_status);
    ABTS_STR_EQUAL(tc, "BDT_POLICY_NOT_FOUND", last.cause);

    pcf_session_free(session);
}
