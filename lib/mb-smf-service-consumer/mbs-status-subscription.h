#ifndef _MB_SMF_MBS_STATUS_SUBSCRIPTION_H_
#define _MB_SMF_MBS_STATUS_SUBSCRIPTION_H_
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include "macros.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mb_smf_sc_mbs_session_s mb_smf_sc_mbs_session_t;

typedef enum mb_smf_sc_mbs_session_event_type_e {
    MBS_SESSION_EVENT_MBS_REL_TMGI_EXPIRY = 1,
    MBS_SESSION_EVENT_BROADCAST_DELIVERY_STATUS = 2,
    MBS_SESSION_EVENT_INGRESS_TUNNEL_ADD_CHANGE = 4
} mb_smf_sc_mbs_session_event_type_t;

typedef struct mb_smf_sc_mbs_status_subscription_s mb_smf_sc_mbs_status_subscription_t;
typedef struct mb_smf_sc_mbs_status_notification_result_s mb_smf_sc_mbs_status_notification_result_t;

typedef void (*mb_smf_sc_mbs_status_notification_cb)(const mb_smf_sc_mbs_status_notification_result_t *result, void *data);

MB_SMF_CLIENT_API mb_smf_sc_mbs_status_subscription_t *mb_smf_sc_mbs_status_subscription_new(uint16_t area_session_id,
                                                                        int event_type_flags,
                                                                        const char *correlation_id,
                                                                        time_t expiry_time,
									mb_smf_sc_mbs_status_notification_cb notify_cb,
                                                                        void *cb_data);
MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_delete(mb_smf_sc_mbs_status_subscription_t *subscription);

MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_status_subscription_get_id(const mb_smf_sc_mbs_status_subscription_t *subscription);

MB_SMF_CLIENT_API uint16_t mb_smf_sc_mbs_status_subscription_get_area_session_id(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription);
MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_area_session_id(mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        uint16_t area_session_id);

MB_SMF_CLIENT_API int mb_smf_sc_mbs_status_subscription_get_event_type_flags(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription);
MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_event_type_flags(mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        int flags);

MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_status_subscription_get_correlation_id(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription);
MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_correlation_id(mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        const char *correlation_id);

MB_SMF_CLIENT_API time_t mb_smf_sc_mbs_status_subscription_get_expiry_time(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription);
MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_expiry_time(mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        time_t expiry_time);

MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_notification_callback(
                                                                        mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        mb_smf_sc_mbs_status_notification_cb result_cb,
                                                                        void *cb_data);

MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_status_subscription_get_notif_url(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription);


MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_status_subscription_as_string(const mb_smf_sc_mbs_status_subscription_t *subscription);

MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_status_subscription_mbs_session(const mb_smf_sc_mbs_status_subscription_t *subscription);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_MBS_STATUS_SUBSCRIPTION_H_ */
