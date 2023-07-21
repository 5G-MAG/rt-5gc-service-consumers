/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef TOOL_PCF_POLICYAUTH_LOG_H
#define TOOL_PCF_POLICYAUTH_LOG_H

#include "ogs-core.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int __pcf_policyauth_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __pcf_policyauth_log_domain

extern void app_log_init(void);
extern void app_log_final(void);
extern void app_log_change_log_level(ogs_log_level_e);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* TOOL_PCF_POLICYAUTH_LOG_H */
