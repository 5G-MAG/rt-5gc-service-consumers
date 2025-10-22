#ifndef MB_SMF_SERVICE_CONSUMER_H
#define MB_SMF_SERVICE_CONSUMER_H
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

/* Open5GS includes */
#include "ogs-proto.h"

/* Library public API includes */
#include "macros.h"
#include "tmgi.h"
#include "mbs-session.h"
#include "mbs-status-subscription.h"

#ifdef __cplusplus
extern "C" {
#endif

// service consumer lifecycle

/** 
 * @defgroup Lifecycle MB-SMF service consumer library lifecycle
 * @{
 */

/** Initialise and parse configuration
 *
 * This must be the first function called in the library and must be only called once.
 * This initialises the library context and parsing the configuration.
 *
 * @param mb_smf_client_sect The top level section name in the YAML configuration that holds the configuration for the MB-SMF
 *                           service consumer library.
 * @return `true` if the library was initialised and the configuration parsed without errors.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_parse_config(const char *mb_smf_client_sect);

/** Terminate the MB-SMF service consumer
 *
 * This should be called before the controlling app finishes using the MB-SMF service consumer library. This will release all
 * resources reserved in the library context.
 *
 * mb_smf_sc_parse_config() may be called again after this function has been called to initialise a new library context.
 */
MB_SMF_CLIENT_API void mb_smf_sc_terminate(void);

/** Event processing
 *
 * Process an `ogs_event_t`. This will check if the event is the result of an action from the MB-SMF service consumer library and
 * will handle the event if it is. If this function returns `true` then the app should do no further processing of the event as
 * it has already handled the event and tidied up any event resources.
 *
 * @param e The event to process.
 * @return `true` if the library has handled the event, `false` otherwise.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_process_event(ogs_event_t *e);

/** Event name
 *
 * Get the nul terminated string containing the event name or NULL if the event is unknown. This will also return standard
 * Open5GS event names, as returned by ogs_event_get_name(), if the event is one of the default Open5GS ones.
 *
 * @param e The event to get the name of.
 * @return The event name string or NULL if the event type is unknown to this library.
 */
MB_SMF_CLIENT_API const char *mb_smf_sc_event_get_name(ogs_event_t *e);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* MB_SMF_SERVICE_CONSUMER_H */
