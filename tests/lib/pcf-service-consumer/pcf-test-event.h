/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#ifndef __TESTS_BSF_TEST_EVENT_H
#define __TESTS_BSF_TEST_EVENT_H

#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "pcf-service-consumer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pcf_test_sess_s pcf_test_sess_t;

typedef enum {
    PCF_TEST_EVENT_BASE = OGS_MAX_NUM_OF_PROTO_EVENT,

    PCF_TEST_EVENT_SBI_LOCAL,

    MAX_NUM_OF_PCF_TEST_EVENT,

} pcf_test_event_e;

typedef struct pcf_test_event_s {

    ogs_event_t h;
    int local_id;
    ogs_pkbuf_t *pkbuf;
    pcf_app_session_t *pcf_app_session;
    ue_network_identifier_t *ue_connection;

} pcf_test_event_t;

OGS_STATIC_ASSERT(OGS_EVENT_SIZE >= sizeof(pcf_test_event_t));

extern pcf_test_event_t *pcf_test_event_new(int id);
extern const char *pcf_test_event_get_name(pcf_test_event_t *e);

#ifdef __cplusplus
}
#endif

#endif /* ifndef __TESTS_BSF_TEST_EVENT_H */

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
