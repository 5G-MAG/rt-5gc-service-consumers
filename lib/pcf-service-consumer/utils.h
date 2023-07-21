/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2022 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef PCF_CLIENT_UTILS_H
#define PCF_CLIENT_UTILS_H

#include "ogs-core.h"
#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library Internals */
char *_sockaddr_to_string(const ogs_sockaddr_t *addr);

extern int events_notification_to_events_mask(const OpenAPI_events_notification_t *events_notif);
extern int events_subsc_req_data_to_events_mask(const OpenAPI_events_subsc_req_data_t *evt_subsc_req);
extern int _npcf_af_event_to_event_mask(const OpenAPI_npcf_af_event_e event_type);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* PCF_CLIENT_UTILS_H */
