#ifndef _MB_SMF_MBS_SESSION_H_
#define _MB_SMF_MBS_SESSION_H_
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <stdint.h>

#include "ogs-core.h"

#include "macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct mb_smf_sc_associated_session_id_s mb_smf_sc_associated_session_id_t;
typedef struct mb_smf_sc_ext_mbs_service_area_s mb_smf_sc_ext_mbs_service_area_t;
typedef struct mb_smf_sc_mbs_service_area_s mb_smf_sc_mbs_service_area_t;
typedef struct mb_smf_sc_mbs_service_info_s mb_smf_sc_mbs_service_info_t;
typedef struct mb_smf_sc_mbs_session_s mb_smf_sc_mbs_session_t;
typedef struct mb_smf_sc_mbs_status_subscription_s mb_smf_sc_mbs_status_subscription_t;
typedef struct mb_smf_sc_ssm_addr_s mb_smf_sc_ssm_addr_t;
typedef struct mb_smf_sc_tmgi_s mb_smf_sc_tmgi_t;

typedef struct ogs_s_nssai_s ogs_s_nssai_t;
typedef struct ogs_sockaddr_s ogs_sockaddr_t;

/* Data types */

/** @defgroup mbs_session_class MBS Session management
 * @{
 */

/** MBS Service Type Enum
 *
 * The two types of MBS Service are Broadcast and Multicast. Broadcast will be sent to all UEs and Multicast is identified by
 * an SSM, which UEs can subscribe to.
 */
typedef enum {
    MBS_SERVICE_TYPE_BROADCAST, /**< Broadcast MBS Service */
    MBS_SERVICE_TYPE_MULTICAST  /**< Multicast MBS Service using an SSM */
} mb_smf_sc_mbs_service_type_e;

/** MBS Session activity status enumeration
 */
typedef enum {
    MBS_SESSION_ACTIVITY_STATUS_NONE = 0,
    MBS_SESSION_ACTIVITY_STATUS_ACTIVE,
    MBS_SESSION_ACTIVITY_STATUS_INACTIVE
} mb_smf_sc_activity_status_e;

/** MBS Session creation/destruction callback
 *
 * This callback is used when an MBS Session create has either succeeded or failed or when the MBS Session is destroyed.
 *
 * On creation, @p result will be `OGS_OK` when the creation succeeded, `OGS_ERROR` for an error during creation or `OGS_TIMEOUT`
 * if the creation operation timed out. If the @p result is `OGS_ERROR` then a ProblemDetails associated with the error is passed
 * in the @p problem_details parameter.
 *
 * On destruction, @p result will be `OGS_DONE`.
 *
 * @param session The MBS Session object this callback is about.
 * @param result `OGS_OK` on creation, `OGS_ERROR` for a general error, `OGS_TIMEDOUT` for a timeout failure and `OGS_DONE` when
 *               the MBS Session is destroyed.
 * @param problem_details The ProblemDetails object associated with the error or `NULL` if no ProblemDetails are available.
 * @param data The value of @p data passed to mb_smf_sc_mbs_session_set_callback() when the callback was set.
 *
 * @see mb_smf_sc_mbs_session_set_callback()
 */
typedef void (*mb_smf_sc_mbs_session_create_result_cb)(mb_smf_sc_mbs_session_t *session, int result,
                                                       const OpenAPI_problem_details_t *problem_details, void *data);

/** MBS Session
 *
 * This holds the public parts of the MBS Session Object.
 */
typedef struct mb_smf_sc_mbs_session_s {
    mb_smf_sc_mbs_service_type_e service_type; /**< Service type, broadcast or multicast */
    mb_smf_sc_ssm_addr_t *ssm;         /**< SSM for this session, NULL if no SSM assigned */
    mb_smf_sc_tmgi_t *tmgi;            /**< TMGI for this session, NULL if no TMGI assigned */
    ogs_sockaddr_t *mb_upf_udp_tunnel; /**< Tunnel address assigned by the MB-UPF for this session */
    bool tunnel_req;                   /**< Tunnel required, true to ask the MB-SMF for a UDP tunnel if mb_upf_udp_tunnel is NULL */
    bool tmgi_req;                     /**< Request a TMGI be allocated, must be false if tmgi is not `NULL` */
    bool location_dependent;           /**< `true` if this MBS Session is location dependent */
    bool any_ue_ind;                   /**< `true` if this MBS Session is for any UE */
    bool contact_pcf_ind;              /**< `true` if the MB-SMF should contact the PCF during an update of this MBS Session */
    uint16_t *area_session_id;         /**< The Area Session Identifier when location_dependent is true */
    mb_smf_sc_mbs_service_area_t *mbs_service_area; /**< The optional MBS Service Area */
    mb_smf_sc_ext_mbs_service_area_t *ext_mbs_service_area; /**< The optional External MBS Service Area */
    char *dnn;                         /**< The network name that this MBS Session is for */
    ogs_s_nssai_t *snssai;             /**< The S-NSSAI that this MBS Session is for */
    ogs_time_t *start_time;            /**< The time at which the MBS Session activates */
    ogs_time_t *termination_time;      /**< The time at which the MBS Session will deactivate */
    mb_smf_sc_mbs_service_info_t *mbs_service_info; /**< The media components and QoS parameters for the MBS Session */
    mb_smf_sc_activity_status_e activity_status; /**< The activity status of a multicast MBS Session (service_type == multicast) */
    ogs_list_t mbs_fsa_ids;            /**< The MBS FSA Ids list for a broadcast MBS Session (service_type == broadcast) */
    mb_smf_sc_associated_session_id_t *associated_session_id; /**< The Associated Session Id (for Rel 18, currently unused) */
    /* mb_smf_sc_mbs_security_context_t *mbs_security_context; **< The optional MBS Security Context */
    /* uint16_t *area_session_policy_id; **< The optional Area Session Policy Id (for Rel 18, currently unused) */
} mb_smf_sc_mbs_session_t;

/* mb_smf_sc_mbs_session Type functions */

/** Create an empty MBS Session
 * @memberof mb_smf_sc_mbs_session_s
 * @static
 * @public
 *
 * @return A new, empty, MBS Session.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new();

/** Create an MBS Session using an IPv4 SSM
 * @memberof mb_smf_sc_mbs_session_s
 * @static
 * @public
 *
 * @param source The IPv4 source address for the SSM.
 * @param dest The IPv4 destination address for the SSM.
 *
 * @return A new multicast MBS Session using the IPv4 SSM defined by @p source and @p dest.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new_ipv4(const struct in_addr *source, const struct in_addr *dest);

/** Create an MBS Session using an IPv6 SSM
 * @memberof mb_smf_sc_mbs_session_s
 * @static
 * @public
 *
 * @param source The IPv6 source address for the SSM.
 * @param dest The IPv6 destination address for the SSM.
 *
 * @return A new multicast MBS Session using the IPv6 SSM defined by @p source and @p dest.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new_ipv6(const struct in6_addr *source, const struct in6_addr *dest);

/** Create an MBS Session using a TMGI
 * @memberof mb_smf_sc_mbs_session_s
 * @static
 * @public
 *
 * @param tmgi The TMGI used to identify the MBS Session.
 *
 * @return A new broadcast MBS Session using the TMGI defined by @p tmgi.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new_tmgi(mb_smf_sc_tmgi_t *tmgi);

/** Destroy an MBS Session
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * @param session The MBS Session to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_mbs_session_delete(mb_smf_sc_mbs_session_t *session);

/** Set the TMGI
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * This sets the TMGI that identifies this MBS Session. Pass `NULL` in @p tmgi to unset the TMGI. If a TMGI is set then the
 * @ref mb_smf_sc_mbs_session_s::tmgi_req "TMGI request flag" is cleared. The
 * @ref mb_smf_sc_mbs_session_s::tmgi_req "TMGI request flag" and setting a TMGI are mutually exclusive.
 * This will also direct further MBS Session communications towards the MB-SMF that allocated the TMGI.
 *
 * @param session The MBS Session to set the TMGI for.
 * @param tmgi The TMGI to set.
 *
 * @return `true` if setting the TMGI was successful.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_tmgi(mb_smf_sc_mbs_session_t *session, mb_smf_sc_tmgi_t *tmgi);

/** Add a notification subscription
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * Adds a notification subscription to an MBS Session. Note this just adds the subscription to the internal model, you must call
 * either mb_smf_sc_mbs_session_push_changes() or mb_smf_sc_mbs_session_push_all_changes() to send the subscription requests to the
 * MB-SMF.
 *
 * @param session The MBS Session to add the subscription to.
 * @param subscription The subscription to add.
 *
 * @return `true` if the subscription was successfully added.
 *
 * @see mb_smf_sc_mbs_session_push_changes()
 * @see mb_smf_sc_mbs_session_push_all_changes()
 */
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_add_subscription(mb_smf_sc_mbs_session_t *session, mb_smf_sc_mbs_status_subscription_t *subscription);

/** Remove a notification subscription
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * Removes a notification subscription from an MBS Session. Note this just removes the notification from the internal model, you
 * must call either mb_smf_sc_mbs_session_push_changes() or mb_smf_sc_mbs_session_push_all_changes() to send the subscription
 * removal to the MB-SMF. If the subscription request has never been sent to the MB-SMF then this just removes the subscription
 * from the internal model and there is no need to use mb_smf_sc_mbs_session_push_changes() or
 * mb_smf_sc_mbs_session_push_all_changes(). If the subscription has been registered at the MB-SMF, then this will mark the
 * subscription as pending deletion for the next call to mb_smf_sc_mbs_session_push_changes() or
 * mb_smf_sc_mbs_session_push_all_changes(). The @p subscription will be deleted once removed.
 *
 * @param session The MBS Session to remove the subscription from.
 * @param subscription The subscription to remove.
 *
 * @return `true` if the subscription was successfully removed or marked as pending delete.
 *
 * @see mb_smf_sc_mbs_session_push_changes()
 * @see mb_smf_sc_mbs_session_push_all_changes()
 */
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_remove_subscription(mb_smf_sc_mbs_session_t *session, mb_smf_sc_mbs_status_subscription_t *subscription);

/** Find an active MBS Session status subscription by subscription id
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * @param session The MBS Session to find the subscription in.
 * @param subscription_id The subscription id of the subscription to find.
 *
 * @return The subscription matching @p subscription_id or `NULL` if the subscription could not be found.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_status_subscription_t *mb_smf_sc_mbs_session_find_subscription(const mb_smf_sc_mbs_session_t *session, const char *subscription_id);

/** Find an active MBS Session status subscription by correlation id
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * This will find the first subscription matching the @p correlation_id.
 *
 * @param session The MBS Session to find the subscription in.
 * @param correlation_id The correlation id of the subscription to find.
 *
 * @return The first subscription matching @p correlation_id or `NULL` if no subscription could not be found.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_status_subscription_t *mb_smf_sc_mbs_session_find_subscription_by_correlation(const mb_smf_sc_mbs_session_t *session, const char *correlation_id);

/** Set the callback for the MBS Session
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * The callback will be called when the MBS Session when the result of a create attempt on the MB-SMF is received.
 *
 * @param session The MBS Session to set the callback for.
 * @param callback The callback to set.
 * @param data The @a data parameter to call the callback with.
 *
 * @return `true` if the callback is updated successfully.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_callback(mb_smf_sc_mbs_session_t *session, mb_smf_sc_mbs_session_create_result_cb callback, void *data);

/** Get the resource ID for the MBS Session at the MB-SMF
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * @param session The MBS Session to get the resource ID for.
 * @return The MBS Session resource ID, as provided by the MB-SMF, or `NULL` if the MBS Session has not been created yet.
 */
MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_session_get_resource_id(const mb_smf_sc_mbs_session_t *session);

/** Push pending changes to an MBS Session
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * This will send any pending changes for the MBS Session to the MB-SMF. This may create or delete MBS Sessions and their
 * notification subscriptions.
 *
 * @param session The MBS Session to commit to the MB-SMF.
 *
 * @return `true` if changes were sent.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_push_changes(mb_smf_sc_mbs_session_t *session);

/** Push pending changes for all MBS Sessions known to the library
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * This will send any pending changes for all MBS Session to the MB-SMF. This may create or delete MBS Sessions and their
 * notification subscriptions.
 *
 * @return `true` if changes were sent.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_push_all_changes();

/** Create an OpenAPI MbsSessionId
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * Creates a new OpenAPI MbsSessionId object for this MBS Session. It is the callers responsibility to ensure
 * OpenAPI_mbs_session_id_free() is called with the returned pointer when the object is finished with.
 *
 * @param session The MBS Session to get the MbsSessionId from.
 *
 * @return A new OpenAPI MbsSessionId object with the MBS Session Id for this MBS Session.
 */
MB_SMF_CLIENT_API OpenAPI_mbs_session_id_t *mb_smf_sc_mbs_session_create_mbs_session_id(mb_smf_sc_mbs_session_t *session);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_MBS_SESSION_H_ */
