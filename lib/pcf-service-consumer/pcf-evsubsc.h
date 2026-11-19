/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#ifndef __PCF_EVENTS_SUBSC_H
#define __PCF_EVENTS_SUBSC_H

#ifdef __cplusplus
extern "C" {
#endif

OpenAPI_list_t *events_subsc_create(int events);
int pcf_app_session_evts(OpenAPI_list_t *evt_subsc_req_events);

/*
 * TS 29.500 V19.7.1 clause 6.6.2: attributes and enumerated values that relate to a feature "shall only be sent" if the feature is supported. The
 * events of the mask, and the events and details of the subscription (either may be empty), are checked against the features (the bits of the
 * numbers of table 5.8-1, feature n is bit n-1; those above 64 in the second word) that the sender may use: those that the library offers before the
 * answer to the creation, those that both sides support after it. Returns true if everything is allowed; else false and @what names the first thing that is not
 * (for the log).
 */
bool pcf_events_subscription_allowed(int events, const OpenAPI_events_subsc_req_data_t *subsc, uint64_t features, uint64_t features_hi,
                char *what, size_t what_size);

#ifdef __cplusplus
}
#endif

#endif /* ifndef __PCF_EVENTS_SUBSC_H */

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
