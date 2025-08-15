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

typedef struct app_options_s app_options_t; /* see: options.h */
typedef struct mb_smf_sc_mbs_session_s mb_smf_sc_mbs_session_t; /* see: library mbs-session.h */

/**
 * Initialise the PCF policyauthorization tool
 */
extern bool tmgi_tool_init(int argc, char *argv[]);

/**
 * Tidy up while the event loop is still running
 */
extern void tmgi_tool_event_termination(void);

/**
 * Tidy up the PCF policyauthorization tool
 */
extern void tmgi_tool_final(void);

/**
 * Get the MBS Session for this app
 */
extern mb_smf_sc_mbs_session_t *tmgi_tool_get_mbs_session();

/**
 * Set the MBS Session for this app
 *
 * If a previous session had been set it will be deleted and freed.
 */
extern void tmgi_tool_set_mbs_session(mb_smf_sc_mbs_session_t *session);

/**
 * Get the command line options structure for the app
 */
extern const app_options_t *tmgi_tool_get_app_options();

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* TOOL_MBS_SERVICE_TOOL_INIT_H */
