/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring <david.waring2@bbc.co.uk>
 * Copyright: (C) 2025 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include "ogs-app.h"
#include "ogs-core.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "mb-smf-service-consumer.h"

#include "app-log.h"
#include "init.h"
#include "options.h"
#include "utils.h"

#include "app-sm.h"

typedef enum ogs_event_id_extension_e {
    APP_LOCAL = OGS_MAX_NUM_OF_PROTO_EVENT + 1500
} ogs_event_id_extension_t;

typedef enum app_local_event_id_e {
    APP_LOCAL_EVENT_NONE = 0,

    APP_LOCAL_EVENT_MBS_SESSION_CREATE,
    APP_LOCAL_EVENT_MBS_SESSION_CREATE_RESULT,

    APP_LOCAL_EVENT_MAX
} app_local_event_id_t;

typedef struct app_local_event_s {
    ogs_event_t event;

    app_local_event_id_t id;

    mb_smf_sc_mbs_session_t *mbs_session;
    int result;
    OpenAPI_problem_details_t *problem_details;

} app_local_event_t;

static void app_state_create_session(ogs_fsm_t *sm, ogs_event_t *event);
static void app_state_running(ogs_fsm_t *sm, ogs_event_t *event);
static void app_state_session_failed(ogs_fsm_t *sm, ogs_event_t *event);

static void app_local_send_mbs_session_create();
static void app_local_send_mbs_session_create_result(mb_smf_sc_mbs_session_t *session, int result,
                                                     const OpenAPI_problem_details_t *problem_details);
static void app_local_send_event(app_local_event_id_t event_id, mb_smf_sc_mbs_session_t *session, int result,
                                 const OpenAPI_problem_details_t *problem_details);
static void app_local_clean(app_local_event_t *app_event);
static const char *app_event_get_name(ogs_event_t *event);
static const char *app_local_get_name(app_local_event_t *app_event);

static void app_mbs_session_created_cb(mb_smf_sc_mbs_session_t *session, int result,
                                       const OpenAPI_problem_details_t *problem_details, void *data);
static void app_mbs_session_notify_cb(const mb_smf_sc_mbs_status_notification_result_t *notification, void *data);
static void app_mbs_session_create();

void app_state_init(ogs_fsm_t *sm, ogs_event_t *event)
{
    ogs_assert(sm);

    OGS_FSM_TRAN(sm, app_state_create_session);
}

void app_state_final(ogs_fsm_t *sm, ogs_event_t *event)
{
    ogs_assert(sm);
}

static void app_state_create_session(ogs_fsm_t *sm, ogs_event_t *event)
{
    ogs_assert(sm);

    if (mb_smf_sc_process_event(event))
        return;

    ogs_debug("Processing event %p [%s]", event, app_event_get_name(event));

    switch (event->id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("Requesting MBS Session...");
        app_local_send_mbs_session_create();
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case APP_LOCAL:
        {
            app_local_event_t *app_event = ogs_container_of(event, app_local_event_t, event);

            switch (app_event->id) {
            case APP_LOCAL_EVENT_MBS_SESSION_CREATE:
                app_mbs_session_create();
                break;
            case APP_LOCAL_EVENT_MBS_SESSION_CREATE_RESULT:
                if (app_event->result == OGS_OK) {
                    ogs_info("MBS Session %s [%p] created", mb_smf_sc_mbs_session_get_resource_id(app_event->mbs_session),
                             app_event->mbs_session);
                    if (app_event->mbs_session->tmgi_req) {
                        if (app_event->mbs_session->tmgi) {
                            char buf[OGS_PLMNIDSTRLEN];
                            mb_smf_sc_tmgi_t *tmgi = app_event->mbs_session->tmgi;
                            ogs_info("  TMGI = %s (PLMN = %s)", tmgi->mbs_service_id, ogs_plmn_id_to_string(&tmgi->plmn, buf));
                        } else {
                            ogs_error("TMGI request failed");
                        }
                    }
                    if (app_event->mbs_session->tunnel_req) {
                        if (app_event->mbs_session->mb_upf_udp_tunnel) {
                            char buf[OGS_ADDRSTRLEN + 1];
                            ogs_sockaddr_t *sa;
                            for (sa = app_event->mbs_session->mb_upf_udp_tunnel; sa; sa = sa->next) {
                                OGS_ADDR(sa, buf);
                                ogs_info("  UDP Tunnel = %s:%u", buf, OGS_PORT(sa));
                            }
                        } else {
                            ogs_error("UDP tunnel request failed");
                        }
                    }
                    OGS_FSM_TRAN(sm, app_state_running);
                } else {
                    if (app_event->problem_details) {
                        if (app_event->problem_details->cause) {
                            ogs_warn("MBS Session creation failed, caused by %s: %s: %s", app_event->problem_details->cause,
                                     app_event->problem_details->title, app_event->problem_details->detail);
                        } else {
                            ogs_warn("MBS Session creation failed: %s: %s",
                                     app_event->problem_details->title, app_event->problem_details->detail);
                        }
                        if (app_event->problem_details->invalid_params) {
                            ogs_warn("    Parameter errors:");
                            OpenAPI_lnode_t *node;
                            OpenAPI_list_for_each(app_event->problem_details->invalid_params, node) {
                                OpenAPI_invalid_param_t *param = (OpenAPI_invalid_param_t*)(node->data);
                                ogs_warn("        %s: %s", param->param, param->reason);
                            }
                        }
                    } else {
                        ogs_warn("MBS Session creation failed, no reason given");
                    }
                    OGS_FSM_TRAN(sm, app_state_session_failed);
                }
                break;
            default:
                ogs_error("Unexpected local event: %s", app_event_get_name(event));
                break;
            }
            app_local_clean(app_event);
        }
        break;

    default:
        break;
    }
}

static void app_state_running(ogs_fsm_t *sm, ogs_event_t *event)
{
    ogs_assert(sm);

    if (mb_smf_sc_process_event(event))
        return;

    ogs_debug("Processing event %p [%s]", event, app_event_get_name(event));

    switch (event->id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("Awaiting notifications...");
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case APP_LOCAL:
        {
            app_local_event_t *app_event = ogs_container_of(event, app_local_event_t, event);

            switch (app_event->id) {
            default:
                ogs_error("Unexpected local event: %s", app_event_get_name(event));
                break;
            }

            app_local_clean(app_event);
        }
        break;

    default:
        break;
    }
}

static void app_state_session_failed(ogs_fsm_t *sm, ogs_event_t *event)
{
    ogs_assert(sm);

    ogs_debug("Processing event %p [%s]", event, app_event_get_name(event));

    switch (event->id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("MBS Session registration failed, exiting...");
        _exit(1);
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case APP_LOCAL:
        {
            app_local_event_t *app_event = ogs_container_of(event, app_local_event_t, event);

            switch (app_event->id) {
            default:
                ogs_error("Unexpected local event: %s", app_event_get_name(event));
                break;
            }

            app_local_clean(app_event);
        }
        break;

    default:
        break;
    }
}

static void app_local_send_mbs_session_create_result(mb_smf_sc_mbs_session_t *session, int result,
                                                     const OpenAPI_problem_details_t *problem_details)
{
    /* send APP_LOCAL_EVENT_MBS_SESSION_CREATE_RESULT event */
    app_local_send_event(APP_LOCAL_EVENT_MBS_SESSION_CREATE_RESULT, session, result, problem_details);
}

static void app_local_send_mbs_session_create()
{
    /* send APP_LOCAL_EVENT_MBS_SESSION_CREATE event */
    app_local_send_event(APP_LOCAL_EVENT_MBS_SESSION_CREATE, NULL, 0, NULL);
}


static void app_local_send_event(app_local_event_id_t event_id, mb_smf_sc_mbs_session_t *session, int result,
                                 const OpenAPI_problem_details_t *problem_details)
{
    int rv;
    app_local_event_t *event;

    event = ogs_calloc(1, sizeof(*event));
    ogs_assert(event);

    event->id = event_id;
    event->event.id = APP_LOCAL;
    event->mbs_session = session;
    event->result = result;
    event->problem_details = OpenAPI_problem_details_copy(NULL, problem_details);

    rv = ogs_queue_push(ogs_app()->queue, &event->event);
    if (rv != OGS_OK) {
        ogs_error("Failed to push app local event onto the evet queue");
        return;
    }
    /* process the event queue */
    ogs_pollset_notify(ogs_app()->pollset);
}

static void app_local_clean(app_local_event_t *event)
{
    if (!event) return;

    if (event->problem_details) OpenAPI_problem_details_free(event->problem_details);
}

static const char *app_event_get_name(ogs_event_t *event)
{
    app_local_event_t *app_event;

    if (ogs_unlikely(!event)) return "*** No Event ***";

    if (event->id != APP_LOCAL) return ogs_event_get_name(event);

    app_event = ogs_container_of(event, app_local_event_t, event);

    return app_local_get_name(app_event);
}

static const char *app_local_get_name(app_local_event_t *app_event)
{
    if (ogs_unlikely(!app_event)) return "*** No Event ***";

    switch (app_event->id) {
    case APP_LOCAL_EVENT_MBS_SESSION_CREATE:
        return "APP_LOCAL_EVENT_MBS_SESSION_CREATE";
    case APP_LOCAL_EVENT_MBS_SESSION_CREATE_RESULT:
        return "APP_LOCAL_EVENT_MBS_SESSION_CREATE_RESULT";
    default:
        break;
    }

    return "APP_LOCAL_EVENT Unknown";
}

static void app_mbs_session_created_cb(mb_smf_sc_mbs_session_t *session, int result,
                                       const OpenAPI_problem_details_t *problem_details, void *data)
{
    /* callback for result of app_mbs_session_create() */

    /* queue result event */
    app_local_send_mbs_session_create_result(session, result, problem_details);
}

static void app_mbs_session_notify_cb(const mb_smf_sc_mbs_status_notification_result_t *notification, void *data)
{
    /* callback for  mb-smf service consumer library receiving an MBS Session notification */
    char *event_time = time_t_to_str(notification->event_time);
    switch (notification->event_type) {
    case MBS_SESSION_EVENT_MBS_REL_TMGI_EXPIRY:
        ogs_info("MBS_REL_TMGI_EXPIRY notification:\n"
                 "    Event time: %s",
                 event_time);
        break;
    case MBS_SESSION_EVENT_BROADCAST_DELIVERY_STATUS:
        ogs_info("BROADCAST_DELIVERY_STATUS notification:\n"
                 "    Event time: %s\n"
                 "    Status: %s",
                 event_time,
                 (notification->broadcast_delivery_status==BROADCAST_DELIVERY_STARTED)?"started":"terminated");
        break;
    case MBS_SESSION_EVENT_INGRESS_TUNNEL_ADD_CHANGE:
    {
        char *tunnels = NULL;
        mb_smf_sc_mbs_status_notification_ingress_tunnel_addr_t *node;
        ogs_list_for_each(&notification->ingress_tunnel_add_change, node) {
            const char *sep = "";
            tunnels = ogs_mstrcatf(tunnels, "\n      ");
            if (node->ipv4) {
                char buf[INET_ADDRSTRLEN];
                tunnels = ogs_mstrcatf(tunnels, "%s:%u", inet_ntop(AF_INET, node->ipv4, buf, sizeof(buf)), node->port);
                sep = ", ";
            }
            if (node->ipv6) {
                char buf[INET6_ADDRSTRLEN];
                tunnels = ogs_mstrcatf(tunnels, "%s[%s]:%u", sep, inet_ntop(AF_INET6, node->ipv6, buf, sizeof(buf)), node->port);
            }
        }
        ogs_info("INGRESS_TUNNEL_ADD_CHANGE notification:\n"
                 "    Event time: %s\n"
                 "    Tunnels:"
                 "%s",
                 event_time,
                 tunnels);
        ogs_free(tunnels);
    }
        break;
    default:
        ogs_warn("Unknown MBS notification '%s' received", notification->event_type_name);
        break;
    }
    ogs_free(event_time);
}

static bool find_family_pair(int family, struct addrinfo *src_ai, struct addrinfo *dst_ai, void **src_addr, void **dst_addr)
{
    while (src_ai && src_ai->ai_family != family) src_ai = src_ai->ai_next;
    if (!src_ai) return false;

    while (dst_ai && dst_ai->ai_family != family) dst_ai = dst_ai->ai_next;
    if (!dst_ai) return false;

    if (family == AF_INET) {
        struct sockaddr_in *addr = (struct sockaddr_in*)src_ai->ai_addr;
        *src_addr = &addr->sin_addr;
        addr = (struct sockaddr_in*)dst_ai->ai_addr;
        *dst_addr =  &addr->sin_addr;
    } else if (family == AF_INET6) {
        struct sockaddr_in6 *addr = (struct sockaddr_in6*)src_ai->ai_addr;
        *src_addr = &addr->sin6_addr;
        addr = (struct sockaddr_in6*)dst_ai->ai_addr;
        *dst_addr = &addr->sin6_addr;
    } else {
        *src_addr = NULL;
        *dst_addr = NULL;
    }

    return true;
}

static void app_mbs_session_create(ogs_fsm_t *sm)
{
    /* create an mb_smf_sc_mbs_session_t object and push changes */
    const app_options_t *options = mbs_service_tool_get_app_options();
    mb_smf_sc_mbs_session_t *session = NULL;

    ogs_debug("app_mbs_session_create");

    mb_smf_sc_mbs_status_subscription_t *subsc = mb_smf_sc_mbs_status_subscription_new(
            0 /* area_session_id */,
            -1 /* event_type_flags */,
            NULL /* correlation_id */,
            time(NULL)+3600 /* expiry_time */,
            app_mbs_session_notify_cb /* notify_cb */,
            NULL /* cb_data */);

    if (options->ssm.source) {
        /* resolve ssm.source and ssm.dest to addresses in the same family (AF_INET or AF_INET6) */
        ogs_debug("app_mbs_session_create: resolving SSM");
        struct addrinfo *ai_src = NULL, *ai_dst = NULL;
        int result;
        void *source_addr, *dest_addr;

        result = getaddrinfo(options->ssm.source, NULL, NULL, &ai_src);
        if (result) {
            ogs_error("Unable to resolve SSM source address '%s': %s", options->ssm.source, gai_strerror(result));
            if (ai_src) freeaddrinfo(ai_src);
            _exit(1);
        }

        result = getaddrinfo(options->ssm.dest, NULL, NULL, &ai_dst);
        if (result) {
            ogs_error("Unable to resolve SSM multicast destination address '%s': %s", options->ssm.dest, gai_strerror(result));
            freeaddrinfo(ai_src);
            if (ai_dst) freeaddrinfo(ai_dst);
            _exit(1);
        }

        /* create a session with the SSM set */
        if (find_family_pair(AF_INET6, ai_src, ai_dst, &source_addr, &dest_addr)) {
            // Found IPv6 pair
            session = mb_smf_sc_mbs_session_new_ipv6((const struct in6_addr*)source_addr,
                                                     (const struct in6_addr*)dest_addr);
        } else if (find_family_pair(AF_INET, ai_src, ai_dst, &source_addr, &dest_addr)) {
            // Found IPv4 pair
            session = mb_smf_sc_mbs_session_new_ipv4((const struct in_addr*)source_addr,
                                                     (const struct in_addr*)dest_addr);
        } else {
            ogs_error("Unable to resolve SSM addresses to the same address family");
            freeaddrinfo(ai_src);
            freeaddrinfo(ai_dst);
            _exit(1);
        }
        freeaddrinfo(ai_src);
        freeaddrinfo(ai_dst);
    } else {
        session = mb_smf_sc_mbs_session_new();
    }

    if (options->tmgi_id) {
        /* set TMGI and PLMN in mbs session */
        ogs_warn("app_mbs_session_create: TODO: TMGI handling");
    }

    /* set the request tmgi flag */
    mb_smf_sc_mbs_session_set_tmgi_request(session, options->req_tmgi);

    /* set the ingressTunAddrReq flag in the MBS session */
    mb_smf_sc_mbs_session_set_tunnel_request(session, options->req_tunnel);

    /* set the service type */
    mb_smf_sc_mbs_session_set_service_type(session, options->is_multicast?MBS_SERVICE_TYPE_MULTICAST:MBS_SERVICE_TYPE_BROADCAST);

    /* Add the subscribe all */
    mb_smf_sc_mbs_session_add_subscription(session, subsc);

    /* set the callback */
    session->create_result_cb = app_mbs_session_created_cb;
    session->create_result_cb_data = (void*)sm;

    mbs_service_tool_set_mbs_session(session);

    mb_smf_sc_mbs_session_push_changes(session);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
