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
#include "mbs-tmgi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct mb_smf_sc_mbs_status_subscription_s mb_smf_sc_mbs_status_subscription_t;
typedef struct mb_smf_sc_mbs_session_s mb_smf_sc_mbs_session_t;

/* Data types */
typedef enum {
    MBS_SERVICE_TYPE_BROADCAST,
    MBS_SERVICE_TYPE_MULTICAST
} mb_smf_sc_mbs_service_type_e;

typedef struct mb_smf_sc_ssm_addr_s {
    int family;
    union {
        struct in_addr ipv4;
        struct in6_addr ipv6;
    } source;
    union {
        struct in_addr ipv4;
        struct in6_addr ipv6;
    } dest_mc;
} mb_smf_sc_ssm_addr_t;

typedef void (*mb_smf_sc_mbs_session_create_result_cb)(mb_smf_sc_mbs_session_t *session, int result,
                                                       const OpenAPI_problem_details_t *problem_details, void *data);

typedef struct mb_smf_sc_mbs_session_s {
    mb_smf_sc_mbs_service_type_e service_type; /* Service type, broadcast or multicast */
    mb_smf_sc_ssm_addr_t *ssm;         /* SSM for this session, NULL if no SSM assigned */
    mb_smf_sc_tmgi_t *tmgi;            /* TMGI for this session, NULL if no TMGI assigned */
    ogs_sockaddr_t *mb_upf_udp_tunnel; /* Tunnel address assigned by the MB-UPF for this session */
    bool tunnel_req;                   /* Tunnel required, true to ask the MB-SMF for a UDP tunnel if mb_upf_udp_tunnel is NULL */
    bool tmgi_req;                     /* Request a TMGI be allocated, must be false if tmgi is not NULL */
    ogs_hash_t *subscriptions;         /* list of mb_smf_sc_mbs_status_subscription_t indexed by their id */
    mb_smf_sc_mbs_session_create_result_cb create_result_cb; /* Callback for result of requesting MBS Session creation */
    void *create_result_cb_data;       /* data passed to the create_result_cb when it is called */
} mb_smf_sc_mbs_session_t;

/* mb_smf_sc_ssm Type functions */
MB_SMF_CLIENT_API bool mb_smf_sc_ssm_equal(const mb_smf_sc_ssm_addr_t *a, const mb_smf_sc_ssm_addr_t *b);

/* mb_smf_sc_mbs_session Type functions */
MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new();
MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new_ipv4(const struct in_addr *source, const struct in_addr *dest);
MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new_ipv6(const struct in6_addr *source, const struct in6_addr *dest);
MB_SMF_CLIENT_API mb_smf_sc_mbs_session_t *mb_smf_sc_mbs_session_new_tmgi(mb_smf_sc_tmgi_t *tmgi);

MB_SMF_CLIENT_API void mb_smf_sc_mbs_session_delete(mb_smf_sc_mbs_session_t *);

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_add_subscription(mb_smf_sc_mbs_session_t *session, mb_smf_sc_mbs_status_subscription_t *subscription);
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_remove_subscription(mb_smf_sc_mbs_session_t *session, mb_smf_sc_mbs_status_subscription_t *subscription);

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_tmgi_request(mb_smf_sc_mbs_session_t *session, bool request_tmgi);
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_tunnel_request(mb_smf_sc_mbs_session_t *session, bool request_udp_tunnel);
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_service_type(mb_smf_sc_mbs_session_t *session,
                                                              mb_smf_sc_mbs_service_type_e service_type);
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_set_tmgi(mb_smf_sc_mbs_session_t *session, mb_smf_sc_tmgi_t *tmgi);

MB_SMF_CLIENT_API const char *mb_smf_sc_mbs_session_get_resource_id(const mb_smf_sc_mbs_session_t *session);

MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_push_changes(mb_smf_sc_mbs_session_t *);
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_session_push_all_changes();

MB_SMF_CLIENT_API OpenAPI_mbs_session_id_t *mb_smf_sc_mbs_session_create_mbs_session_id(mb_smf_sc_mbs_session_t *);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_MBS_SESSION_H_ */
