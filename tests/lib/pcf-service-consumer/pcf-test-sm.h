/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#ifndef __TESTS_PCF_TEST_SM_H
#define __TESTS_PCF_TEST_SM_H

#include "pcf-test-event.h"

#ifdef __cplusplus
extern "C" {
#endif

void pcf_test_state_initial(ogs_fsm_t *s, pcf_test_event_t *e);
void pcf_test_state_final(ogs_fsm_t *s, pcf_test_event_t *e);

#define pcf_test_sm_debug(__pe) \
    ogs_debug("%s(): %s", __func__, pcf_test_event_get_name(__pe))

#ifdef __cplusplus
}
#endif

#endif /* ifndef __TESTS_pcf_TEST_SM_H */

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
