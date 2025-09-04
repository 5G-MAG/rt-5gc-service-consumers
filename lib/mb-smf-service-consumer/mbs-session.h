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
#include <netinet/in.h>

#include "ogs-core.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "macros.h"
#include "tmgi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct mb_smf_sc_mbs_status_subscription_s mb_smf_sc_mbs_status_subscription_t;
typedef struct mb_smf_sc_mbs_session_s mb_smf_sc_mbs_session_t;

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

/** SSM Address
 *
 * Object for storing an SSM address.
 */
typedef struct mb_smf_sc_ssm_addr_s {
    int family;               /**< The AF family for the SSM address, either AF_INET or AF_INET6. */
    /** SSM Source Address
     * The source address for the SSM using the address family given by the @ref mb_smf_sc_ssm_addr_s::family "family" member.
     */
    union {
        struct in_addr ipv4;  /**< The IPv4 source address if @ref mb_smf_sc_ssm_addr_s::family "family" is AF_INET. */
        struct in6_addr ipv6; /**< The IPv6 source address if @ref mb_smf_sc_ssm_addr_s::family "family" is AF_INET6. */
    } source;
    /** SSM Destination Address
     * The destination address for the SSM using the address family given by the @ref mb_smf_sc_ssm_addr_s::family "family" member.
     */
    union {
        struct in_addr ipv4;  /**< The IPv4 destination address if @ref mb_smf_sc_ssm_addr_s::family "family" is AF_INET. */
        struct in6_addr ipv6; /**< The IPv6 destination address if @ref mb_smf_sc_ssm_addr_s::family "family" is AF_INET6. */
    } dest_mc;
} mb_smf_sc_ssm_addr_t;

/** MBS Session creation callback
 *
 * This callback is used when an MBS Session create has either succeeded or failed. On success @p result will be OGS_OK, and will
 * be any other value on failure. If the failure result has a ProblemDetails associated with it then it is passed in the
 * @p problem_details parameter.
 *
 * @param session The MBS Session object this callback is about.
 * @param result `OGS_OK` on success, `OGS_ERROR` for a general error and `OGS_TIMEDOUT` for a timeout failure.
 * @param problem_details The ProblemDetails object associated with the error or `NULL` if no ProblemDetails are available.
 * @param data The value of @ref mb_smf_sc_mbs_session_s::create_result_cb_data "create_result_cb_data" from the MBS Session
 *             associated with this callback.
 */
typedef void (*mb_smf_sc_mbs_session_create_result_cb)(mb_smf_sc_mbs_session_t *session, int result,
                                                       const OpenAPI_problem_details_t *problem_details, void *data);

/** MBS Session
 *
 * This holds the public parts of the MBS Session Object. Most members are read-only and the approriate method should be used to
 * change them. This allows the library to keep track of changes to be committed. The exceptions are
 * @ref mb_smf_sc_mbs_session_s::create_result_cb "create_result_cb" and
 * @ref mb_smf_sc_mbs_session_s::create_result_cb_data "create_result_cb_data", which can be set directly.
 */
typedef struct mb_smf_sc_mbs_session_s {
    mb_smf_sc_mbs_service_type_e service_type; /**< Service type, broadcast or multicast */
    mb_smf_sc_ssm_addr_t *ssm;         /**< SSM for this session, NULL if no SSM assigned */
    mb_smf_sc_tmgi_t *tmgi;            /**< TMGI for this session, NULL if no TMGI assigned */
    ogs_sockaddr_t *mb_upf_udp_tunnel; /**< Tunnel address assigned by the MB-UPF for this session */
    bool tunnel_req;                   /**< Tunnel required, true to ask the MB-SMF for a UDP tunnel if mb_upf_udp_tunnel is NULL */
    bool tmgi_req;                     /**< Request a TMGI be allocated, must be false if tmgi is not NULL */
    ogs_hash_t *subscriptions;         /**< list of mb_smf_sc_mbs_status_subscription_t indexed by their id */
    mb_smf_sc_mbs_session_create_result_cb create_result_cb; /**< Callback for result of requesting MBS Session creation */
    void *create_result_cb_data;       /**< data passed to the create_result_cb when it is called */
} mb_smf_sc_mbs_session_t;

/* mb_smf_sc_ssm Type functions */

/** SSM equality
 * @memberof mb_smf_sc_ssm_addr_s
 * @public
 *
 * Check if SSM @p a is equal to SSM @p b.
 *
 * @param a The first SSM to compare.
 * @param b The second SSM to compare.
 * @return `true` if SSMs @p a and @p b contain the same SSM value.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_ssm_equal(const mb_smf_sc_ssm_addr_t *a, const mb_smf_sc_ssm_addr_t *b);

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
 * mb_smf_sc_mbs_session_push_all_changes().
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

/** Set the TMGI request flag
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * This sets the flag for requesting that a TMGI be allocated to the MBS Session upon creation at the MB-SMF. If set to `true`
 * then any TMGI associated with the MBS Session will be removed. Setting this flag to `true` and setting a TMGI are mutually
 * exclusive.
 *
 * @param session The MBS Session to set the request TMGI flag on.
 * @param request_tmgi The value of the request TMGI flag, `true` to request a TMGI upon creation and `false` if no TMGI should be
 *                     created.
 * @return `true` if setting the flag was successful.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_tmgi_request(mb_smf_sc_mbs_session_t *session, bool request_tmgi);

/** Set the request UDP tunnel flag
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * This sets the flag for requesting that a UDP tunnel be allocated for sending the multicast or broadcast packets to the MB-UPF.
 *
 * @param session The MBS Session to set the request UDP tunnel flag on.
 * @param request_udp_tunnel The value of the request UDP tunnel flag, `true` to request a new UDP tunnel upon creation of the
 *                           MBS Session at the MB-SMF.
 * @return `true` if setting the flag was successful.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_tunnel_request(mb_smf_sc_mbs_session_t *session, bool request_udp_tunnel);

/** Set the MBS Service type
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * Sets the MBS Service to either broadcast or multicast type.
 *
 * @param session The MBS Session to set service type for.
 * @param service_type The MBS Service type to set.
 *
 * @return `true` if setting the MBS Service type was successful.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_service_type(mb_smf_sc_mbs_session_t *session,
                                                              mb_smf_sc_mbs_service_type_e service_type);

/** Set the TMGI
 * @memberof mb_smf_sc_mbs_session_s
 * @public
 *
 * This sets the TMGI that identifies this MBS Session. Pass `NULL` in @p tmgi to unset the TMGI. If a TMGI is set then the
 * @ref mb_smf_sc_mbs_session_s::tmgi_req "TMGI request flag" is cleared. The
 * @ref mb_smf_sc_mbs_session_s::tmgi_req "TMGI request flag" and setting a TMGI are mutually exclusive.
 *
 * @param session The MBS Session to set the TMGI for.
 * @param tmgi The TMGI to set.
 *
 * @return `true` if setting the TMGI was successful.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_tmgi(mb_smf_sc_mbs_session_t *session, mb_smf_sc_tmgi_t *tmgi);

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
