/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef TOOL_MBS_SERVICE_TOOL_INIT_H
#define TOOL_MBS_SERVICE_TOOL_INIT_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise the PCF policyauthorization tool
 */
extern bool mbs_service_tool_init(int argc, char *argv[]);

/**
 * Tidy up while the event loop is still running
 */
extern void mbs_service_tool_event_termination(void);

/**
 * Tidy up the PCF policyauthorization tool
 */
extern void mbs_service_tool_final(void);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* TOOL_MBS_SERVICE_TOOL_INIT_H */
