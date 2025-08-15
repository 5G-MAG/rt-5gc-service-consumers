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
MB_SMF_CLIENT_API bool mb_smf_sc_parse_config(const char *mb_smf_client_sect);
MB_SMF_CLIENT_API void mb_smf_sc_terminate(void);
MB_SMF_CLIENT_API bool mb_smf_sc_process_event(ogs_event_t *e);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* MB_SMF_SERVICE_CONSUMER_H */
