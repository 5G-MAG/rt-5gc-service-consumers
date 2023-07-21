/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2022 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef PCF_SVC_CONSUMER_H
#define PCF_SVC_CONSUMER_H

#include "ogs-core.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#ifdef BUILD_PCF_CLIENT_LIB
    #if defined _WIN32 || defined __CYGWIN__
        #ifdef __GNUC__
            #define PCF_SVC_CONSUMER_API __attribute__ ((dllexport))
        #else
            #define PCF_SVC_CONSUMER_API __declspec(dllexport)
        #endif
    #else
        #if __GNUC__ >= 4
            #define PCF_SVC_CONSUMER_API __attribute__ ((visibility ("default")))
        #else
            #define PCF_SVC_CONSUMER_API
        #endif
    #endif
#else
    #if defined _WIN32 || defined __CYGWIN__
        #ifdef __GNUC__
            #define PCF_SVC_CONSUMER_API __attribute__ ((dllimport))
        #else
            #define PCF_SVC_CONSUMER_API __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
        #endif
    #else
        #define PCF_SVC_CONSUMER_API
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * PCF connection session
 */
typedef struct pcf_session_s pcf_session_t;

/**
 * Service consumer representation of a PCF AppSessionContext
 */
typedef struct pcf_app_session_s pcf_app_session_t;

/**
 * Callback for PCF events notifications
 *
 * Called when the service consumer receives a notification from the PCF for one of the App Sessions created using
 * pcf_session_create_app_session().
 *
 * @param app_session The App Session the notification came from.
 * @param notifications The notifications that have arrived.
 * @param user_data The user data registered with the `pcf_session_create_app_session()` or `pcf_app_session_subscribe_event()`
 *                  functions.
 * @return `false` if there was an error processing the notifications.
 */
typedef bool (*pcf_app_session_notification_callback)(pcf_app_session_t *app_session,
		const OpenAPI_events_notification_t *notifications, void *user_data);

/**
 * Callback for PCF App Session change events
 *
 * This callback will be called when an AppSessionContext is created or destroyed on the PCF or if there was an error creating an
 * AppSessionContext.
 *
 * @param app_session The service consumer representation of the AppSessionContext or NULL on error or destruction of the
 *                    AppSessionContext.
 * @param user_data The user_data registered in the call to pcf_session_create_app_session().
 */
typedef bool (*pcf_app_session_change_callback)(pcf_app_session_t *app_session, void *user_data);

/**
 * UE Network address information to be used in registering the AppSessionContext
 */
typedef struct ue_network_identifier_s {
    ogs_sockaddr_t *address; /** The IP address of the UE for a PDU session. */
    char *supi;              /** The SUPI of the UE's PDU session. */
    char *gpsi;              /** The GPSI of the UE's PDU session. */
    char *dnn;               /** The network name of the UE's PDU session. */
    char *ip_domain;         /** The IP domain for the UE's PDU session. */
} ue_network_identifier_t;

/**
 * AppSessionContext event notification bits
 *
 * These are ORed together to indicate that the App wants to receive these event types.
 *
 * Two special entries are also included:
 * - PCF_APP_SESSION_EVENT_TYPE_NONE represents the state where no event notifications are wanted.
 * - PCF_APP_SESSION_EVENT_TYPE_ALL is a shorthand for every event type.
 */
typedef enum pcf_app_session_event_type_e {
    PCF_APP_SESSION_EVENT_TYPE_NONE = 0x00000,
    PCF_APP_SESSION_EVENT_TYPE_ACCESS_TYPE_CHANGE = 0x00001,
    PCF_APP_SESSION_EVENT_TYPE_ANI_REPORT = 0x00002,
    PCF_APP_SESSION_EVENT_TYPE_APP_DETECTION = 0x00004,
    PCF_APP_SESSION_EVENT_TYPE_CHARGING_CORRELATION = 0x00008,
    PCF_APP_SESSION_EVENT_TYPE_UP_PATH_CHG_FAILURE = 0x00010,
    PCF_APP_SESSION_EVENT_TYPE_EPS_FALLBACK = 0x00020,
    PCF_APP_SESSION_EVENT_TYPE_FAILED_QOS_UPDATE = 0x00040,
    PCF_APP_SESSION_EVENT_TYPE_FAILED_RESOURCES_ALLOCATION = 0x00080,
    PCF_APP_SESSION_EVENT_TYPE_OUT_OF_CREDIT = 0x00100,
    PCF_APP_SESSION_EVENT_TYPE_PDU_SESSION_STATUS = 0x00200,
    PCF_APP_SESSION_EVENT_TYPE_PLMN_CHG = 0x00400,
    PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF = 0x00800,
    PCF_APP_SESSION_EVENT_TYPE_QOS_MONITORING = 0x01000,
    PCF_APP_SESSION_EVENT_TYPE_RAN_NAS_CAUSE = 0x02000,
    PCF_APP_SESSION_EVENT_TYPE_REALLOCATION_OF_CREDIT = 0x04000,
    PCF_APP_SESSION_EVENT_TYPE_SAT_CATEGORY_CHG = 0x08000,
    PCF_APP_SESSION_EVENT_TYPE_SUCCESSFUL_QOS_UPDATE = 0x10000,
    PCF_APP_SESSION_EVENT_TYPE_SUCCESSFUL_RESOURCES_ALLOCATION = 0x20000,
    PCF_APP_SESSION_EVENT_TYPE_TSN_BRIDGE_INFO = 0x40000,
    PCF_APP_SESSION_EVENT_TYPE_USAGE_REPORT = 0x80000,
    PCF_APP_SESSION_EVENT_TYPE_ALL = 0xFFFFF
} pcf_app_session_event_type_t;

/**
 * Create a new PCF session
 *
 * Create a new PCF session context with 0 associated AppSessionContexts and make a connection to the PCF.
 *
 * May reuse an existing underlying PCF connection if one already exists.
 *
 * @param pcf_address The socket address of the PCF to connect to.
 *
 * @return A PCF connection session for the @pcf_address or NULL on error.
 */
PCF_SVC_CONSUMER_API pcf_session_t *pcf_session_new(const ogs_sockaddr_t *pcf_address);

/**
 * Tidy up PCF service consumer
 *
 * Free all memory allocated by the PCF service consumer library
 */
PCF_SVC_CONSUMER_API void pcf_service_consumer_final(void);

/**
 * Create a new AppSessionContext
 *
 * @param session The PCF connection session to create the new AppSessionContext on.
 * @param ue_connection The address of the UE that this AppSessionContext will be for.
 * @param events The bit mask of the ORed `pcf_app_session_event_type_t` representing the notifications to report to
 *               @notify_callback.
 * @param media_component The requested list of `MediaComponent` entries describing UE connections and requested bitrates that this
 *                        AppSessionContext will manage.
 * @param notify_callback The callback to use when a notification matching one of @events is received for this AppSessionContext.
 * @param notify_user_data The `user_data` to pass to the @notify_callback when it's called.
 * @param change_callback The callback to call when an AppSessionContext is created or destroyed by the PCF.
 * @param change_user_data The `user_data` to pass to the @change_callback when it's called.
 */
PCF_SVC_CONSUMER_API bool pcf_session_create_app_session(pcf_session_t *session,
                const ue_network_identifier_t *ue_connection, int events,
                OpenAPI_list_t *media_component,
                pcf_app_session_notification_callback notify_callback, void *notify_user_data,
                pcf_app_session_change_callback change_callback, void *change_user_data);

/**
 * Update the MediaComponents for an AppSessionContext
 *
 * @param sess The AppSessionContext to update the MediaComponents for.
 * @param media_component The list of MediaComponent entries for the update.
 */
PCF_SVC_CONSUMER_API bool pcf_session_update_app_session(pcf_app_session_t *sess, OpenAPI_list_t *media_component);

/**
 * Release an AppSessionContext
 *
 * This will result in the PCF being asked to destroy the AppSessionContext resource.
 *
 * @param sess The AppSessionContext to destroy.
 */
PCF_SVC_CONSUMER_API void pcf_app_session_free(pcf_app_session_t *sess);

/**
 * Release a PCF connection session
 *
 * This will destroy any AppSessionContexts associated with this PCF session.
 *
 * If this is the last release for the underlying PCF connection then the socket connection is dropped too.
 */
PCF_SVC_CONSUMER_API void pcf_session_free(pcf_session_t *session);

/**
 * Process an ogs_event_t
 *
 * Checks the ogs_event_t to see if it is an event associated with this service consumer and consumes it if it is.
 *
 * @param event The event to check.
 *
 * @return `true` if the service consumer has dealt with the event, no further processing should be done for this event. If the
 *         return value is `false` then the service consumer has not consumed the event and it should be processed by the
 *         application as normal.
 */
PCF_SVC_CONSUMER_API bool pcf_session_process_event(ogs_event_t *event);

/**
 * Subscribe to additional event notfications for an AppSessionContext
 *
 * This allows greater control over subscribed event notifications for an AppSessionContext compared to
 * pcf_session_create_app_session().
 *
 * @param app_session The AppSessionContext to add these notifications to.
 * @param evt_subsc_req The event subscription data, the notification URLs will be overwritten by the service consumer.
 * @param callback The events notification callback to use for the events described in @evt_subsc_req.
 * @param user_data The `user_data` to pass to @callback when it is called.
 */
PCF_SVC_CONSUMER_API bool pcf_app_session_subscribe_event(pcf_app_session_t *app_session,
                OpenAPI_events_subsc_req_data_t *evt_subsc_req, pcf_app_session_notification_callback callback,
                void *user_data);

/**
 * Unsubscribe from event notfications for an AppSessionContext
 *
 * @param app_session The AppSessionContext to remove these notifications from.
 * @param evt_subsc_req The events to unsubscribe.
 */
PCF_SVC_CONSUMER_API bool pcf_app_session_unsubscribe_event(pcf_app_session_t *app_session,
                OpenAPI_events_subsc_req_data_t *evt_subsc_req);

#ifdef __cplusplus
}
#endif

#endif /* PCF_SVC_CONSUMER_H */
