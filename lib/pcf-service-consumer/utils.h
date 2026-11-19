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

/*
 * The authority of the URI of the PCF at the socket address: an IPv6 address is enclosed in square brackets (RFC 3986 clause 3.2.2; TS 29.501 V19.5.0
 * clause 4.4.1), as in "[2001:db8::1]:7777". The caller releases the text.
 */
extern char *_pcf_authority(const ogs_sockaddr_t *addr);

/*
 * The path of the URI that a Location header field value stands for (TS 29.501 V19.5.0 clause 4.6.1.1.1.2: absolute or relative URI reference, resolved
 * against the target URI of the request, RFC 3986 clause 5.2), without query and fragment and without a trailing slash; NULL if there is none. The caller
 * releases the text.
 */
extern char *_location_path(const char *location, const char *request_path);

/* The seconds that a Retry-After header field value asks for (RFC 9110 clause 10.2.3: a number of seconds, or a date), or -1 */
extern int _retry_after_seconds(const char *value, time_t now);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* PCF_CLIENT_UTILS_H */
