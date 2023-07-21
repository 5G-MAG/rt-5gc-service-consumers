/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef TOOL_PCF_POLICYAUTH_INIT_H
#define TOOL_PCF_POLICYAUTH_INIT_H

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
extern bool pcf_policyauth_init(int argc, char *argv[]);

/**
 * Tidy up while the event loop is still running
 */
extern void pcf_policyauth_event_termination(void);

/**
 * Tidy up the PCF policyauthorization tool
 */
extern void pcf_policyauth_final(void);

/**
 * Get the current known PCF address
 *
 * Returns immediately may trigger a BSF lookup
 */
extern const char *pcf_policyauth_get_pcf_address(void);

/**
 * Get the PCF address, waiting if necessary
 *
 * This call will get the PCF address and will block the thread while waiting
 * for discovery.
 *
 * Returns the PCF address or NULL if there was a problem retrieving the PCF
 * address.
 */
extern const char *pcf_policyauth_wait_for_pcf_address(void);

/**
 * Get the current known PCF port number
 *
 * Returns immediately with 0 if the PCF address is currently unknown.
 */
extern short unsigned int pcf_policyauth_get_pcf_port(void);

/**
 * Request an AppSessionContext from the PCF
 */
extern void pcf_policyauth_get_app_session_context(void);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* TOOL_PCF_POLICYAUTH_INIT_H */
