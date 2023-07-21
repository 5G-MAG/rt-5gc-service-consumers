/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef TOOL_PCF_POLICYAUTH_APP_SM_H
#define TOOL_PCF_POLICYAUTH_APP_SM_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "ogs-core.h"
#include "ogs-proto.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void app_state_init(ogs_fsm_t *sm, ogs_event_t *event);
extern void app_state_final(ogs_fsm_t *sm, ogs_event_t *event);

extern void app_local_send_discovered_pcf(void);
extern void app_local_send_pcf_discovery_failed(void);
extern void app_local_send_app_session_created(void);
extern void app_local_send_app_session_destroyed(void);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* TOOL_PCF_POLICYAUTH_APP_SM_H */
