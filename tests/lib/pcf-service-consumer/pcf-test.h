/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#ifndef __TESTS_PCF_TEST_H
#define __TESTS_PCF_TEST_H

#include "test-common.h"
#include "pcf-service-consumer.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int pcf_test_initialise(void);
extern void pcf_test_terminate(void);
extern bool notification_handler_callback(pcf_app_session_t *app_session, const OpenAPI_events_notification_t *notifications, void *user_data);
extern bool ev_notification_handler_callback(pcf_app_session_t *app_session, const OpenAPI_events_notification_t *notifications, void *user_data);
extern bool change_handler_callback(pcf_app_session_t *app_session, void *user_data);

extern abts_suite *test_pcf(abts_suite *suite);

#ifdef __cplusplus
}
#endif

#endif /* ifndef __TESTS_PCF_TEST_H */

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
