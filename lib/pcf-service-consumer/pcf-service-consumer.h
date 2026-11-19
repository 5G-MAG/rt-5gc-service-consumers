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
    ogs_sockaddr_t *address; /** The IP address of the UE for a PDU session: the only one of these that the requests carry. */
    /*
     * The four that follow are kept but not sent (TS 29.514 V19.8.0 clause 4.2.2.2 lets the consumer include them, "may"): the PCF matches all that
     * it is given (note 7) and uses the DNN as it is given, without a transformation (note 2 of table 5.6.2.3-1), so a DNN of another form than the
     * PCF's own makes the binding fail (PDU_SESSION_NOT_AVAILABLE). The address binds the session. See CONSUMERS audit O6 (no CR).
     */
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
    PCF_APP_SESSION_EVENT_TYPE_L4S_SUPP = 0x100000,
    PCF_APP_SESSION_EVENT_TYPE_ALL = 0x1FFFFF
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
 * The operation that a result reports
 */
typedef enum pcf_app_session_operation_e {
    PCF_APP_SESSION_OP_NONE = 0,
    PCF_APP_SESSION_OP_CREATE,      /**< the AppSessionContext was (or was not) created in the PCF */
    PCF_APP_SESSION_OP_UPDATE,      /**< the PCF accepted (or refused) the update */
    PCF_APP_SESSION_OP_EVENTS,      /**< the PCF accepted (or refused) the change of the event subscription */
    PCF_APP_SESSION_OP_DELETE,      /**< the AppSessionContext was deleted (the object is released after the callback) */
    PCF_APP_SESSION_OP_TERMINATED   /**< the PCF ended the AppSessionContext by itself (the object is released after the callback) */
} pcf_app_session_operation_t;

/**
 * The outcome of one operation on an AppSessionContext
 */
typedef struct pcf_app_session_result_s {
    pcf_app_session_operation_t operation;
    bool success;
    int http_status;                /**< Status of the PCF's answer; 0 if there was none (no connection, timeout, answer not usable). */
    const char *problem_title;      /**< Title of the ProblemDetails of an error answer, or NULL. Valid during the callback only. */
    const char *problem_detail;     /**< Detail of the ProblemDetails of an error answer, or NULL. Valid during the callback only. */
    const char *problem_cause;      /**< Cause of the ProblemDetails of an error answer, or NULL. Valid during the callback only. */
    /**
     * The Retry-After of an error answer in seconds (TS 29.514 V19.8.0 clause 4.2.2.2: after a 403 with a retry interval the same service
     * information is not to be sent again until the interval has elapsed), or -1. The library does not judge which requests carry "the same
     * service information": the application keeps to the interval.
     */
    int retry_after;
    const char *redirect_location;  /**< The Location of a 303 See Other (the existing Individual Application Session Context), or NULL. Valid during the callback only. */
    const char *termination_cause;  /**< For PCF_APP_SESSION_OP_TERMINATED: the termCause of the TerminationInfo (for example "PDU_SESSION_TERMINATION"), or NULL. Valid during the callback only. */
} pcf_app_session_result_t;

/**
 * Callback for the outcome of an operation
 *
 * Called exactly once for each create, update and delete that was started, and when the PCF terminates an AppSessionContext.
 * After a failed create, after a delete and after a termination the library releases @app_session when the callback returns.
 *
 * @param app_session The AppSessionContext the result is for.
 * @param result What happened.
 * @param user_data The user_data registered in the call to pcf_session_create_app_session_with_result().
 */
typedef void (*pcf_app_session_result_callback)(pcf_app_session_t *app_session, const pcf_app_session_result_t *result, void *user_data);

/**
 * Ownership: what is passed to the library stays the caller's
 *
 * The library copies the UE address and the media components that it is given, so the caller keeps them and releases
 * them itself, whether the call succeeded or not, and can retry. Lists of media components are released with
 * pcf_media_components_free() (create) and pcf_media_components_rm_free() (update).
 */

/**
 * Release a list of `MediaComponent` entries (a map from the component number to the entry) that was built for
 * pcf_session_create_app_session().
 */
PCF_SVC_CONSUMER_API void pcf_media_components_free(OpenAPI_list_t *media_components);

/**
 * Release a list of `MediaComponentRm` entries that was built for pcf_session_update_app_session().
 */
PCF_SVC_CONSUMER_API void pcf_media_components_rm_free(OpenAPI_list_t *media_components_rm);

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
 * Create a new AppSessionContext and be told the outcome of each operation on it
 *
 * Like pcf_session_create_app_session(), with a callback that says which operation finished, with the HTTP status and the
 * ProblemDetails of an error answer. The caller can tell a refused create from a refused update from a deletion from an end
 * of the AppSessionContext by the PCF.
 */
PCF_SVC_CONSUMER_API bool pcf_session_create_app_session_with_result(pcf_session_t *session,
                const ue_network_identifier_t *ue_connection, int events,
                OpenAPI_list_t *media_component,
                pcf_app_session_notification_callback notify_callback, void *notify_user_data,
                pcf_app_session_result_callback result_callback, void *result_user_data);

/**
 * What an AppSessionContext asks for beyond its media components
 *
 * Everything is borrowed: the library copies what it needs and the caller keeps and releases its own objects.
 */
typedef struct pcf_app_session_options_s {
    int events;                     /**< The ORed `pcf_app_session_event_type_t` to subscribe to. */
    /**
     * The details of the event subscription (TS 29.514 V19.8.0 table 5.6.2.5-1): the events with their notification method, the
     * QoS monitoring information and parameters, the usage threshold. Its events are subscribed in addition to `events`, and replace
     * the entry of the same event in it; the notification URI is set by the library. NULL for none.
     */
    const OpenAPI_events_subsc_req_data_t *events_subscription;
    const char *asp_id;             /**< The identity of the application service provider (aspId), or NULL. */
    const char *spon_id;            /**< The sponsor of the data connectivity (sponId), or NULL. */
    bool sponsoring_enabled;        /**< With asp_id and spon_id: the sponsoring status is SPONSOR_ENABLED, else SPONSOR_DISABLED. Without them no sponsoring status is sent. */
    /**
     * The reservation priority (resPrio of TS 29.514 V19.8.0 table 5.6.2.3-1; PRIO_1 is the lowest and PRIO_16 the highest priority), or
     * OpenAPI_reserv_priority_NULL (0, the default) to send none: the text lets the consumer include it ("may") and the PCF takes it into
     * account at its option, so the library does not rank a session above those of other consumers unless the application asks.
     */
    OpenAPI_reserv_priority_e res_prio;
} pcf_app_session_options_t;

/**
 * Create a new AppSessionContext with the options of the application, and be told the outcome of each operation on it
 *
 * Like pcf_session_create_app_session_with_result(), with `options` instead of the event mask.
 */
PCF_SVC_CONSUMER_API bool pcf_session_create_app_session_with_options(pcf_session_t *session,
                const ue_network_identifier_t *ue_connection, const pcf_app_session_options_t *options,
                OpenAPI_list_t *media_component,
                pcf_app_session_notification_callback notify_callback, void *notify_user_data,
                pcf_app_session_result_callback result_callback, void *result_user_data);

/**
 * Update the MediaComponents for an AppSessionContext
 *
 * @param sess The AppSessionContext to update the MediaComponents for.
 * @param media_component The map of MediaComponentRm entries for the update.
 */
PCF_SVC_CONSUMER_API bool pcf_session_update_app_session(pcf_app_session_t *sess, OpenAPI_list_t *media_component_rm_map);

/**
 * The optional features of the Npcf_PolicyAuthorization API that the PCF and the library both support for this AppSessionContext
 * (TS 29.514 V19.8.0 table 5.8-1; feature n is bit n-1), as negotiated when it was created. 0 before the PCF has answered.
 */
PCF_SVC_CONSUMER_API uint64_t pcf_app_session_get_features(const pcf_app_session_t *sess);
/* The negotiated features above number 64 (bit 0 is feature 65) */
PCF_SVC_CONSUMER_API uint64_t pcf_app_session_get_features_hi(const pcf_app_session_t *sess);

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


/**
 * Service consumer representation of an Individual BDT policy of the PCF (TS 29.554 V19.2.1)
 */
typedef struct pcf_bdt_policy_s pcf_bdt_policy_t;

/**
 * The service requirements of a Background Data Transfer (the BdtReqData of TS 29.554 V19.2.1 table 5.6.2.3-1)
 *
 * The strings are RFC 3339 date-times (for example "2030-03-05T00:00:00Z"). A volume that is not given is set to a negative
 * value; at least one of them has to be given (the UsageThreshold of TS 29.122).
 */
typedef struct pcf_bdt_request_s {
    const char *asp_id;             /**< The identity of the application service provider. */
    const char *start_time;         /**< The start of the desired time window. */
    const char *stop_time;          /**< The end of the desired time window. */
    int num_of_ues;                 /**< The expected number of UEs, at least 1. */
    int64_t total_volume;           /**< The volume per UE in bytes, or negative. */
    int64_t downlink_volume;        /**< The downlink volume per UE in bytes, or negative. */
    int64_t uplink_volume;          /**< The uplink volume per UE in bytes, or negative. */
    int64_t duration;               /**< The duration in seconds, or negative. */
    const char *dnn;                /**< The DNN, or NULL. */
    const ogs_s_nssai_t *s_nssai;   /**< The slice, or NULL. */
    const char *inter_group_id;     /**< The internal group identifier of a group of UEs, or NULL. */
} pcf_bdt_request_t;

/**
 * One transfer policy that the PCF offers (the TransferPolicy of TS 29.554 V19.2.1 table 5.6.2.5-1)
 *
 * The strings belong to the pcf_bdt_policy_t that it was read from and are valid as long as that is.
 */
typedef struct pcf_bdt_transfer_policy_s {
    int trans_policy_id;
    int rating_group;
    const char *start_time;         /**< The start of the recommended time window. */
    const char *stop_time;          /**< The end of the recommended time window. */
    const char *max_bit_rate_dl;    /**< The maximum aggregated bit rate downlink, or NULL. */
    const char *max_bit_rate_ul;    /**< The maximum aggregated bit rate uplink, or NULL. */
} pcf_bdt_transfer_policy_t;

/**
 * The operation that a result reports
 */
typedef enum pcf_bdt_operation_e {
    PCF_BDT_OP_NONE = 0,
    PCF_BDT_OP_CREATE,      /**< the Individual BDT policy was (or was not) created in the PCF */
    PCF_BDT_OP_GET,         /**< the Individual BDT policy was (or was not) read from the PCF */
    PCF_BDT_OP_SELECT,      /**< the PCF accepted (or refused) the selection of a transfer policy */
    PCF_BDT_OP_DELETE       /**< the Individual BDT policy was deleted (the object is released after the callback) */
} pcf_bdt_operation_t;

/**
 * The outcome of one operation on an Individual BDT policy
 */
typedef struct pcf_bdt_result_s {
    pcf_bdt_operation_t operation;
    bool success;
    int http_status;                /**< Status of the PCF's answer; 0 if there was none (no connection, timeout, answer not usable). */
    const char *problem_title;      /**< Title of the ProblemDetails of an error answer, or NULL. Valid during the callback only. */
    const char *problem_detail;     /**< Detail of the ProblemDetails of an error answer, or NULL. Valid during the callback only. */
    const char *problem_cause;      /**< Cause of the ProblemDetails of an error answer, or NULL. Valid during the callback only. */
    const char *existing_policy_id; /**< For a create that the PCF answered with 303 See Other: the bdtPolicyId of the existing resource. */
} pcf_bdt_result_t;

/**
 * Callback for the outcome of an operation on an Individual BDT policy
 *
 * Called exactly once for each operation that was started. After a failed create, a failed get and a delete the library releases
 * @policy when the callback returns.
 */
typedef void (*pcf_bdt_result_callback)(pcf_bdt_policy_t *policy, const pcf_bdt_result_t *result, void *user_data);

/**
 * Ask the PCF for the transfer policies of a Background Data Transfer (Npcf_BDTPolicyControl_Create)
 *
 * The request is copied. The library offers the PatchCorrection feature, so a transfer policy is selected in the form of
 * PatchBdtPolicy.
 *
 * @return `false` if the request was not sent; the callback is not called then.
 */
PCF_SVC_CONSUMER_API bool pcf_session_create_bdt_policy(pcf_session_t *session, const pcf_bdt_request_t *request,
                pcf_bdt_result_callback callback, void *user_data);

/**
 * Read an existing Individual BDT policy from the PCF
 *
 * @param bdt_policy_id The bdtPolicyId of the resource, as the last segment of its URI.
 */
PCF_SVC_CONSUMER_API bool pcf_session_get_bdt_policy(pcf_session_t *session, const char *bdt_policy_id,
                pcf_bdt_result_callback callback, void *user_data);

/**
 * Like pcf_session_create_bdt_policy() and pcf_session_get_bdt_policy(), with the PCF found through the NRF (a BDT policy is not
 * tied to a UE, so there is no BSF binding to find it by): the first request goes to a PCF that the NRF names for the service
 * npcf-bdtpolicycontrol, and the later requests of the policy to the PCF that answered. If the NRF names none, or the PCF does not
 * answer in time, the callback gets a result without a policy (policy NULL, no http_status).
 */
PCF_SVC_CONSUMER_API bool pcf_discover_create_bdt_policy(const pcf_bdt_request_t *request,
                pcf_bdt_result_callback callback, void *user_data);
PCF_SVC_CONSUMER_API bool pcf_discover_get_bdt_policy(const char *bdt_policy_id,
                pcf_bdt_result_callback callback, void *user_data);

/** The BDT Reference ID of the policy, or NULL if the PCF has not answered. */
PCF_SVC_CONSUMER_API const char *pcf_bdt_policy_get_ref_id(const pcf_bdt_policy_t *policy);

/** The URI of the Individual BDT policy resource, or NULL if the PCF has not answered. */
PCF_SVC_CONSUMER_API const char *pcf_bdt_policy_get_uri(const pcf_bdt_policy_t *policy);

/** The number of transfer policies that the PCF offers. */
PCF_SVC_CONSUMER_API int pcf_bdt_policy_get_num_transfer_policies(const pcf_bdt_policy_t *policy);

/** Transfer policy @index (from 0) of the policy; `false` if there is none. */
PCF_SVC_CONSUMER_API bool pcf_bdt_policy_get_transfer_policy(const pcf_bdt_policy_t *policy, int index,
                pcf_bdt_transfer_policy_t *transfer_policy);

/** The transPolicyId of the selected transfer policy, or 0 if none is selected. */
PCF_SVC_CONSUMER_API int pcf_bdt_policy_get_selected_transfer_policy_id(const pcf_bdt_policy_t *policy);

/**
 * Select one of the transfer policies that the PCF offered (Npcf_BDTPolicyControl_Update)
 *
 * @return `false` if the request was not sent or @trans_policy_id is not the identity of an offered transfer policy.
 */
PCF_SVC_CONSUMER_API bool pcf_bdt_policy_select(pcf_bdt_policy_t *policy, int trans_policy_id);

/**
 * Delete the Individual BDT policy (Npcf_BDTPolicyControl_Delete)
 *
 * The object is released when the callback of the result has returned. If the delete cannot be sent the object is released at once.
 */
PCF_SVC_CONSUMER_API void pcf_bdt_policy_free(pcf_bdt_policy_t *policy);

#ifdef __cplusplus
}
#endif

#endif /* PCF_SVC_CONSUMER_H */
