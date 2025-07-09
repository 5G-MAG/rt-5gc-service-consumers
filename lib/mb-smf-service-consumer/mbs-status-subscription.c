/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include "ogs-sbi.h"

#include "macros.h"
#include "mbs-status-notification-result.h"
#include "priv_mbs-session.h"
#include "priv_mbs-status-subscription.h"

#include "mbs-status-notification.h"

MB_SMF_CLIENT_API mb_smf_sc_mbs_status_subscription_t *mb_smf_sc_mbs_status_subscription_new(
                                                                            uint16_t area_session_id,
                                                                            int event_type_flags,
                                                                            const char *correlation_id,
                                                                            time_t expiry_time,
                                                                            mb_smf_sc_mbs_status_notification_cb callback,
                                                                            void *cb_data)
{
    _priv_mbs_status_subscription_t *subsc = (_priv_mbs_status_subscription_t*)ogs_calloc(1, sizeof(*subsc));
    subsc->cache = ogs_calloc(1, sizeof(*subsc->cache));

    if (subsc) {
        subsc->area_session_id = area_session_id;
        subsc->flags = event_type_flags;
        if (correlation_id) subsc->correlation_id = ogs_strdup(correlation_id);
        subsc->expiry_time = expiry_time;
        subsc->callback = callback;
        subsc->callback_data = cb_data;
    }

    return _priv_mbs_status_subscription_to_public(subsc);
}

MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_delete(mb_smf_sc_mbs_status_subscription_t *subscription)
{
    _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public(subscription);

    if (!subsc) return;

    if (subsc->id || subsc->session) {
        // Still attached to an MBS session, so go through proper removal process
        mb_smf_sc_mbs_session_remove_subscription(_priv_mbs_session_to_public(subsc->session),
                                                  _priv_mbs_status_subscription_to_public(subsc));
    } else {
        // Not attached, delete status subscription object
        _mbs_status_subscription_delete(subsc);
    } 
}

MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_status_subscription_get_id(const mb_smf_sc_mbs_status_subscription_t *subscription)
{
    const _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public_const(subscription);
    if (!subsc) return NULL;
    return subsc->id;
}

MB_SMF_CLIENT_API uint16_t mb_smf_sc_mbs_status_subscription_get_area_session_id(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription)
{
    const _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public_const(subscription);
    if (!subsc) return 0;
    return subsc->area_session_id;
}

MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_area_session_id(mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        uint16_t area_session_id)
{
    _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public(subscription);
    if (!subsc) return;
    if (subsc->area_session_id == area_session_id) return;

    subsc->area_session_id = area_session_id;

    if (subsc->cache->repr_string) {
        ogs_free(subsc->cache->repr_string);
        subsc->cache->repr_string = NULL;
    }

    subsc->changed = true;
    if (subsc->session) subsc->session->changed = true;
}

MB_SMF_CLIENT_API int mb_smf_sc_mbs_status_subscription_get_event_type_flags(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription)
{
    const _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public_const(subscription);
    if (!subsc) return 0;
    return subsc->flags;
}

MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_event_type_flags(mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        int flags)
{
    _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public(subscription);
    if (!subsc) return;
    if (subsc->flags == flags) return;

    subsc->flags = flags;

    if (subsc->cache->repr_string) {
        ogs_free(subsc->cache->repr_string);
        subsc->cache->repr_string = NULL;
    }

    subsc->changed = true;
    if (subsc->session) subsc->session->changed = true;
}

MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_status_subscription_get_correlation_id(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription)
{
    const _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public_const(subscription);
    if (!subsc) return 0;
    return subsc->correlation_id;
}

MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_correlation_id(mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        const char *correlation_id)
{
    _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public(subscription);
    if (!subsc) return;
    if (correlation_id && subsc->correlation_id && !strcmp(subsc->correlation_id, correlation_id)) return;
    if (!correlation_id && !subsc->correlation_id) return;

    if (subsc->correlation_id) {
        ogs_free(subsc->correlation_id);
        subsc->correlation_id = NULL;
    }
    if (correlation_id) subsc->correlation_id = ogs_strdup(correlation_id);
    
    if (subsc->cache->repr_string) {
        ogs_free(subsc->cache->repr_string);
        subsc->cache->repr_string = NULL;
    }

    subsc->changed = true;
    if (subsc->session) subsc->session->changed = true;
}

MB_SMF_CLIENT_API time_t mb_smf_sc_mbs_status_subscription_get_expiry_time(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription)
{
    const _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public_const(subscription);
    if (!subsc) return 0;
    return subsc->expiry_time;
}

MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_expiry_time(mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        time_t expiry_time)
{
    _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public(subscription);
    if (!subsc) return;
    if (expiry_time == subsc->expiry_time) return;

    subsc->expiry_time = expiry_time;

    if (subsc->cache->repr_string) {
        ogs_free(subsc->cache->repr_string);
        subsc->cache->repr_string = NULL;
    }

    subsc->changed = true;
    if (subsc->session) subsc->session->changed = true;    
}

MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_notification_callback(
                                                                        mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        mb_smf_sc_mbs_status_notification_cb notify_cb,
                                                                        void *cb_data)
{
    _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public(subscription);
    if (!subsc) return;
    subsc->callback = notify_cb;
    subsc->callback_data = cb_data;
}

MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_status_subscription_get_notif_url(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription)
{
    const _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public_const(subscription);
    if (!subsc) return NULL;
    return subsc->cache->notif_url;
}

MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_status_subscription_as_string(const mb_smf_sc_mbs_status_subscription_t *subscription)
{
    const _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public_const(subscription);
    if (!subsc) return NULL;
    
    if (!subsc->cache->repr_string) {
        const char *sep = "";
        size_t i;
        static const struct {
            mb_smf_sc_mbs_session_event_type_t t;
            const char * const s;
        } flags[] = {
            {MBS_SESSION_EVENT_MBS_REL_TMGI_EXPIRY, "MBS_REL_TMGI_EXPIRY"},
            {MBS_SESSION_EVENT_BROADCAST_DELIVERY_STATUS, "BROADCAST_DELIVERY_STATUS"},
            {MBS_SESSION_EVENT_INGRESS_TUNNEL_ADD_CHANGE, "INGRESS_TUNNEL_ADD_CHANGE"}
        };
        subsc->cache->repr_string = ogs_msprintf("MBS Session Status Subscription(area_session_id=%u, event_type_flags=%i (", subsc->area_session_id, subsc->flags);
        for (i = 0; i<sizeof(flags)/sizeof(flags[0]); i++) {
            if (subsc->flags & flags[i].t) {
                subsc->cache->repr_string = ogs_mstrcatf(subsc->cache->repr_string, "%s%s", sep, flags[i].s);
                sep = "|";
            }
        }
        subsc->cache->repr_string = ogs_mstrcatf(subsc->cache->repr_string, "), correlation_id=%s, expiry_time=", subsc->correlation_id);
        char *dt = ogs_sbi_localtime_string(ogs_time_from_sec(subsc->expiry_time + OGS_1970_1900_SEC_DIFF));
        subsc->cache->repr_string = ogs_mstrcatf(subsc->cache->repr_string, "%s", dt);
        ogs_free(dt);
        subsc->cache->repr_string = ogs_mstrcatf(subsc->cache->repr_string, "): id=%s, url=%s", subsc->id, subsc->cache->notif_url);
    }

    return subsc->cache->repr_string;
}

MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_status_subscription_mbs_session(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription)
{
    const _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public_const(subscription);
    if (!subsc) return NULL;

    return _priv_mbs_session_to_public(subsc->session);
}

/* Protected functions */

mb_smf_sc_mbs_status_subscription_t *_priv_mbs_status_subscription_to_public(_priv_mbs_status_subscription_t *subscription) {
    return (mb_smf_sc_mbs_status_subscription_t*)subscription;
}

_priv_mbs_status_subscription_t *_priv_mbs_status_subscription_from_public(mb_smf_sc_mbs_status_subscription_t *subscription) {
    if (!subscription) return NULL;
    return (_priv_mbs_status_subscription_t*)subscription;
}

const _priv_mbs_status_subscription_t *_priv_mbs_status_subscription_from_public_const(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription) {
    if (!subscription) return NULL;
    return (const _priv_mbs_status_subscription_t*)subscription;
}

void _mbs_status_subscription_delete(_priv_mbs_status_subscription_t *subsc)
{
    if (subsc->correlation_id) ogs_free(subsc->correlation_id);
    if (subsc->id) ogs_free(subsc->id);
    if (subsc->cache->repr_string) ogs_free(subsc->cache->repr_string);
    if (subsc->cache->notif_url) ogs_free(subsc->cache->notif_url);
    if (subsc->cache->notif_server) ogs_sbi_server_remove(subsc->cache->notif_server);
    ogs_free(subsc->cache);
    ogs_free(subsc);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
