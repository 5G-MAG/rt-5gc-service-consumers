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
#include <netinet/in.h>
#include <stdint.h>

#include <ogs-core.h>

#include "macros.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mb_smf_sc_mbs_session_s mb_smf_sc_mbs_session_t;

/** @defgroup mbs_status_subscription_class MBS Session Status Subscription management
 * @{
 */

/** MBS Session Status Notification Event Type
 *
 * This is a bitmask enumeration for the event types being subscribed to or reported. This list is derived from the
 * MbsSessionEventType enumeration in TS 29.571 Clause 5.9.3.3.
 */
typedef enum mb_smf_sc_mbs_session_event_type_e {
    MBS_SESSION_EVENT_ALL = -1,                      /**< All bits set to include every event type */
    MBS_SESSION_EVENT_OTHER = 0,                     /**< Signalled in a notification result when type not understood */
    MBS_SESSION_EVENT_MBS_REL_TMGI_EXPIRY = 1,       /**< MBS_REL_TMGI_EXPIRY MbsSessionEventType */
    MBS_SESSION_EVENT_BROADCAST_DELIVERY_STATUS = 2, /**< BROADCAST_DELIVERY_STATUS MbsSessionEventType */
    MBS_SESSION_EVENT_INGRESS_TUNNEL_ADD_CHANGE = 4  /**< INGRESS_TUNNEL_ADD_CHANGE MbsSessionEventType */
} mb_smf_sc_mbs_session_event_type_t;

/** MBS Status Subscription
 *
 * The mb_smf_sc_mbs_status_subscription_t object holds the information about individual MBS Session Status Subscriptions.
 */
typedef struct mb_smf_sc_mbs_status_subscription_s {} mb_smf_sc_mbs_status_subscription_t;

/** MBS Session Broadcast Delivery Status
 *
 * This enumeration is reported in "BROADCAST_DELIVERY_STATUS" type events to indicate how the broadcast state has changed.
 *
 * This is derived from the BroadcastDeliveryStatus enumeration given in TS 29.571 Clause 5.9.3.4.
 */
typedef enum {
    BROADCAST_DELIVERY_STARTED,    /**< The MBS session has been started. */
    BROADCAST_DELIVERY_TERMINATED  /**< The MBS session has been terminated. */
} mb_smf_sc_mbs_status_notification_broadcast_delivery_status_e;

/** MBS Session Ingress Tunnel Address Information
 *
 * This holds the information for each ingressTunAddr entry reported in a "INGRESS_TUNNEL_ADD_CHANGE" event and is passed in the
 * notification structure passed to the notification callback as an ogs_list_t item.
 *
 * This is derived from the TunnelAddress type in TS 29.571 Clause 5.2.4.22.
 */
typedef struct mb_smf_sc_mbs_status_notification_ingress_tunnel_addr_s {
    ogs_lnode_t node;
    struct in_addr *ipv4;
    struct in6_addr *ipv6;
    uint16_t port;
} mb_smf_sc_mbs_status_notification_ingress_tunnel_addr_t;

/** MBS Session Status Notification Result
 *
 * This is used to pass the data from an MBS Session Status Notification to the notification callback.
 */
typedef struct mb_smf_sc_mbs_status_notification_result_s {
    mb_smf_sc_mbs_session_event_type_t event_type; /**< The event type for this notification. */
    char *event_type_name;                         /**< The string name if @ref mb_smf_sc_mbs_status_notification_result_s::event_type "event_type" is `MBS_SESSION_EVENT_OTHER`. */
    mb_smf_sc_mbs_session_t *mbs_session;          /**< The MBS Session this notification is associated with. */
    char *correlation_id;                          /**< The correlation Id from the status notification subscription. */
    time_t event_time;                             /**< The timestamp for when the event was received. */
    union {
        /** When @ref mb_smf_sc_mbs_status_notification_result_s::event_type "event_type" is `MBS_SESSION_EVENT_BROADCAST_DELIVERY_STATUS` this contains the BroadcastDeliveryStatus. */
        mb_smf_sc_mbs_status_notification_broadcast_delivery_status_e  broadcast_delivery_status;
        /** When @ref mb_smf_sc_mbs_status_notification_result_s::event_type "event_type" is `MBS_SESSION_EVENT_INGRESS_TUNNEL_ADD_CHANGE`, this contains a list of mb_smf_sc_mbs_status_notification_ingress_tunnel_addr_t derived from the notification. */
        ogs_list_t                                                     ingress_tunnel_add_change;
    };
} mb_smf_sc_mbs_status_notification_result_t;

/** MBS Session Status Notification callback
 *
 * @param result The notification result received.
 * @param data The callback data registered with the callback.
 */
typedef void (*mb_smf_sc_mbs_status_notification_cb)(const mb_smf_sc_mbs_status_notification_result_t *result, void *data);

/** Create a new MBS Session Status Notification Subscription object
 * @memberof mb_smf_sc_mbs_status_subscription_s
 * @static
 * @public
 *
 * This creates a new MBS Session Status Subscription object which can be configured and then attached to an MBS Session with
 * mb_smf_sc_mbs_session_add_subscription().
 *
 * @param area_session_id The `areaSessionId` to use for this subscription.
 * @param event_type_flags The OR of values from mb_smf_sc_mbs_session_event_type_e (Use `MBS_SESSION_EVENT_ALL` to subscribe to all event types).
 * @param correlation_id The correlation Id string to be reported with notification events for this subscription, can be `NULL`.
 * @param expiry_time The expiry time for this subscription, use `0` for no expiry time.
 * @param notify_cb The notification callback function, can be `NULL` for no callback at this time.
 * @param cb_data The value to pass in the @p data parameter when calling @p notify_cb.
 *
 * @return A new MBS Session Status Notification Subscription object.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_status_subscription_t *mb_smf_sc_mbs_status_subscription_new(uint16_t area_session_id,
                                                                        int event_type_flags,
                                                                        const char *correlation_id,
                                                                        time_t expiry_time,
									mb_smf_sc_mbs_status_notification_cb notify_cb,
                                                                        void *cb_data);

/** Delete an MBS Session Status Subscription
 * @memberof mb_smf_sc_mbs_status_subscription_s
 * @public
 *
 * If the notification subscription has been registered with an MB-SMF, then mark this status notification subscription for
 * deletion at the next call to mb_smf_sc_mbs_session_push_changes() or mb_smf_sc_mbs_session_push_all_changes(). If the
 * subscription was never registered, then its resources are freed immediately.
 *
 * @param subscription The MBS Session Status Notification Subscription to delete.
 */
MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_delete(mb_smf_sc_mbs_status_subscription_t *subscription);

/** Get the subscription Id
 * @memberof mb_smf_sc_mbs_status_subscription_s
 * @public
 *
 * Retrieves the subscription Id of an MBS Session Status Notification Subscription registered with the MB-SMF.
 *
 * @param subscription The MBS Session Status Notification Subscription to get the resource Id from.
 *
 * @return The subscription resouirce Id if this subscription has been registered with MB-SMF, otherwise `NULL`.
 */
MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_status_subscription_get_id(const mb_smf_sc_mbs_status_subscription_t *subscription);

/** Get the areaSessionId
 * @memberof mb_smf_sc_mbs_status_subscription_s
 * @public
 *
 * Gets the areaSessionId set for this subscription.
 *
 * @param subscription The MBS Session Status Notification Subscription to get the areaSessionId from.
 *
 * @return The areaSessionId or 0.
 */
MB_SMF_CLIENT_API uint16_t mb_smf_sc_mbs_status_subscription_get_area_session_id(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription);

/** Set the areaSessionId
 *
 * Sets a new value for the areaSessionId for this subscription.
 *
 * @param subscription The MBS Session Status Notification Subscription to set the areaSessionId for.
 * @param area_session_id The new areaSessionId to set.
 */
MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_area_session_id(mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        uint16_t area_session_id);

/** Get the event type flags
 * @memberof mb_smf_sc_mbs_status_subscription_s
 * @public
 *
 * Returns the event type flags set for the MBS Session Status Notification Subscription.
 *
 * @return The event type flags (an ORed set of mb_smf_sc_mbs_session_event_type_e values).
 *
 * @see mb_smf_sc_mbs_session_event_type_e
 */
MB_SMF_CLIENT_API int mb_smf_sc_mbs_status_subscription_get_event_type_flags(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription);

/** Set the event type flags
 * @memberof mb_smf_sc_mbs_status_subscription_s
 * @public
 *
 * Sets the event type flags for the MBS Session Status Notification Subscription.
 *
 * @param subscription The MBS Session Status Notification Subscription to set the event type flags for.
 * @param flags An ORed set of mb_smf_sc_mbs_session_event_type_e values or `MBS_SESSION_EVENT_ALL` for all events, cannot be set
 *              to `0`.
 */
MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_event_type_flags(mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        int flags);

/** Get the correlation Id string
 * @memberof mb_smf_sc_mbs_status_subscription_s
 * @public
 *
 * @param subscription The MBS Session Status Notification Subscription to get the correlation Id from.
 * @return The correlation Id string for the @p subscription.
 */
MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_status_subscription_get_correlation_id(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription);

/** Set the correlation Id string
 * @memberof mb_smf_sc_mbs_status_subscription_s
 * @public
 *
 * @param subscription The MBS Session Status Notification Subscription to set the correlation Id for.
 * @param correlation_id The new correlation Id string to use for the @p subscription.
 */
MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_correlation_id(mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        const char *correlation_id);

/** Get the subscription expiry time
 * @memberof mb_smf_sc_mbs_status_subscription_s
 * @public
 *
 * @param subscription The MBS Session Status Notification Subscription to get the expiry time from.
 *
 * @return The expiry time for the subscription or `0` for no expiry.
 */
MB_SMF_CLIENT_API time_t mb_smf_sc_mbs_status_subscription_get_expiry_time(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription);

/** Set the subscription expiry time
 * @memberof mb_smf_sc_mbs_status_subscription_s
 * @public
 *
 * @param subscription The MBS Session Status Notification Subscription to set the expiry time for.
 * @param expiry_time The expiry time to set or `0` for no expiry time.
 */
MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_expiry_time(mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        time_t expiry_time);

/** Set the notification callback
 * @memberof mb_smf_sc_mbs_status_subscription_s
 * @public
 *
 * Sets the callback function and @p data parameter value for an MBS Session Status Notification Subscription.
 *
 * @param subscription The MBS Session Status Notification Subscription to set the notification callback function for.
 * @param result_cb The notification callback function to use or `NULL` to remove the callback.
 * @param cb_data The value to pass as the @p data parameter to the @p result_cb callback function.
 */
MB_SMF_CLIENT_API void mb_smf_sc_mbs_status_subscription_set_notification_callback(
                                                                        mb_smf_sc_mbs_status_subscription_t *subscription,
                                                                        mb_smf_sc_mbs_status_notification_cb result_cb,
                                                                        void *cb_data);

/** Get the notification URL
 * @memberof mb_smf_sc_mbs_status_subscription_s
 * @public
 *
 * Gets the notification URL assigned to the @p subscription.
 *
 * @param subscription The MBS Session Status Notification Subscription to get the notification URL for.
 *
 * @return The notification URL string.
 */
MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_status_subscription_get_notif_url(
                                                                        const mb_smf_sc_mbs_status_subscription_t *subscription);

/** Get a string representation
 * @memberof mb_smf_sc_mbs_status_subscription_s
 * @public
 *
 * Get a string representation of the status subscription. This is useful for printing in debug messages or when reporting errors.
 *
 * @param subscription The MBS Session Status Notification Subscription to get the string representation of.
 *
 * @return A string representation of the @p subscription.
 */
MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_status_subscription_as_string(const mb_smf_sc_mbs_status_subscription_t *subscription);

/** Get the MBS Session
 * @memberof mb_smf_sc_mbs_status_subscription_s
 * @public
 *
 * Gets the MBS Session object that this subscription is registered with.
 *
 * @return The MBS Session object or `NULL` if this subscription is not registered with an MBS Session.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_status_subscription_mbs_session(const mb_smf_sc_mbs_status_subscription_t *subscription);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_MBS_STATUS_SUBSCRIPTION_H_ */
