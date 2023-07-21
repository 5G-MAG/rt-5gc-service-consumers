/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2022 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#include "ogs-sbi.h"
#include "core/abts.h"
#include "test-common.h"
#include "pcf-service-consumer.h"

static void pcf_session_new_test(abts_case *tc, void *data);

static void pcf_session_new_test(abts_case *tc, void *data)
{
    char *addr = "127.0.0.13";
    int port = 7777;
    int rv;
    ogs_sockaddr_t *pcf_addr;
    pcf_session_t *session;

    rv = ogs_getaddrinfo(&pcf_addr, AF_UNSPEC, addr, port, 0);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);
    
    session = pcf_session_new(pcf_addr);
    ABTS_PTR_NOTNULL(tc, session);
    ogs_freeaddrinfo(pcf_addr);
}

abts_suite *test_pcf_session_new(abts_suite *suite)
{
    suite = ADD_SUITE(suite)
    abts_run_test(suite, pcf_session_new_test, NULL);
    return suite;
}
