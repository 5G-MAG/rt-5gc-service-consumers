/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-core.h"
#include "ogs-sbi.h"
#include "openapi/model/status_subscribe_req_data.h"

#include "macros.h"
#include "context.h"
#include "log.h"
#include "priv_mbs-session.h"
#include "priv_mbs-status-subscription.h"

#include "nmbsmf-mbs-session-build.h"

extern ogs_sbi_server_actions_t ogs_sbi_server_actions;

static OpenAPI_ip_addr_t *__new_OpenAPI_ip_addr_from_inaddr(const struct in_addr *addr);
static OpenAPI_ip_addr_t *__new_OpenAPI_ip_addr_from_in6addr(const struct in6_addr *addr);
static void __allocate_notification_server(_priv_mbs_status_subscription_t *subscription);
static ogs_sbi_server_t *__new_sbi_server(const ogs_sockaddr_t *address);
static OpenAPI_mbs_session_id_t *__make_mbs_session_id(_priv_mbs_session_t *session, OpenAPI_ssm_t **ssm_ptr);

static ogs_sbi_server_t *fixed_port_notifications = NULL;

/* Library Internals */
ogs_sbi_request_t *_nmbsmf_mbs_session_build_create(void *context, void *data)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)context;

    ogs_sbi_message_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.h.method = (char*)OGS_SBI_HTTP_METHOD_POST;
    msg.h.service.name = (char*)OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION;
    msg.h.api.version = (char *)OGS_SBI_API_V1;
    msg.h.resource.component[0] = (char *)OGS_SBI_RESOURCE_NAME_MBS_SESSIONS;

    OpenAPI_ssm_t *ssm = NULL;
    OpenAPI_mbs_session_id_t *mbs_session_id = __make_mbs_session_id(session, &ssm);

    OpenAPI_ext_mbs_session_t *ext_mbs_session = OpenAPI_ext_mbs_session_create(mbs_session_id,
                                                                                session->session.tmgi_req,
                                                                                (session->session.tmgi_req?1:0), /* tmgi_alloc_req: write-only */
                                                                                NULL, /* tmgi: read-only */
                                                                                NULL, /* expiry_time: read-only */
                                                                                ssm?OpenAPI_mbs_service_type_MULTICAST
                                                                                   :OpenAPI_mbs_service_type_BROADCAST, /* service_type: write-only */
                                                                                false, 0, /* location_dependant */
                                                                                false, 0, /* area_session_id: read-only */
                                                                                session->session.tunnel_req, /* ingress_tun_addr_req: write-only */
                                                                                ((session->session.tunnel_req)?1:0),
                                                                                NULL, /* ingress_tun_addr list: read-only */
                                                                                ssm,  /* ssm: write-only */
                                                                                NULL, /* mbs_service_area: write-only */
                                                                                NULL, /* ext_mbs_service_area: write-only */
                                                                                /* red_mbs_serv_area: read-only */
                                                                                /* ext_red_mbs_serv_area: read-only */
                                                                                NULL, /* dnn: write-only */
                                                                                NULL, /* snssai: write-only */
                                                                                NULL, /* activation_time: deprecated */
                                                                                NULL, /* start_time */
                                                                                NULL, /* termination_time */
                                                                                NULL, /* mbs_serv_info */
                                                                                NULL, /* mbs_session_subsc */
                                                                                OpenAPI_mbs_session_activity_status_ACTIVE,
                                                                                true, 1, /* any_ue_ind */
                                                                                NULL, /* mbs_fsa_id_list */
                                                                                NULL, /* mbs_security_context */
                                                                                false, 0 /* contact_pcf_ind */
                                                                                );

    /* Add first subscription */
    _priv_mbs_status_subscription_t *subsc;
    if (!ogs_list_empty(&session->new_subscriptions)) {
        subsc = ogs_list_first(&session->new_subscriptions);
    } else if (session->session.subscriptions && ogs_hash_count(session->session.subscriptions) > 0) {
        ogs_hash_index_t *it = ogs_hash_first(session->session.subscriptions);
        subsc = _priv_mbs_status_subscription_from_public(ogs_hash_this_val(it));
    }
    if (subsc) {
        OpenAPI_list_t *subsc_events = NULL;
        if (subsc->flags & MBS_SESSION_EVENT_MBS_REL_TMGI_EXPIRY) {
            OpenAPI_mbs_session_event_t *event = OpenAPI_mbs_session_event_create(
                                                            OpenAPI_mbs_session_event_type_MBS_REL_TMGI_EXPIRY);
            subsc_events = OpenAPI_list_create();
            OpenAPI_list_add(subsc_events, event);
        }
        if (subsc->flags & MBS_SESSION_EVENT_BROADCAST_DELIVERY_STATUS) {
            OpenAPI_mbs_session_event_t *event = OpenAPI_mbs_session_event_create(
                                                            OpenAPI_mbs_session_event_type_BROADCAST_DELIVERY_STATUS);
            if (!subsc_events) subsc_events = OpenAPI_list_create();
            OpenAPI_list_add(subsc_events, event);
        }
        if (subsc->flags & MBS_SESSION_EVENT_INGRESS_TUNNEL_ADD_CHANGE) {
            OpenAPI_mbs_session_event_t *event = OpenAPI_mbs_session_event_create(
                                                            OpenAPI_mbs_session_event_type_INGRESS_TUNNEL_ADD_CHANGE);
            if (!subsc_events) subsc_events = OpenAPI_list_create();
            OpenAPI_list_add(subsc_events, event);
        }
        if (subsc_events) {
            char *correlation_id = subsc->correlation_id?ogs_strdup(subsc->correlation_id):NULL;
            char *notify_uri = NULL;
            __allocate_notification_server(subsc);
            if (subsc->cache->notif_url) {
                notify_uri = ogs_strdup(subsc->cache->notif_url);
            }
            ext_mbs_session->mbs_session_subsc = OpenAPI_mbs_session_subscription_create(
                                                     NULL, /* mbs_session_id: not in MBS Session create */
                                                     false, 0, /* area_session_id: not in MBS Session create */
                                                     subsc_events, /* event_list: must have at least 1 entry */
                                                     notify_uri, /* notify_uri: write-only */
                                                     correlation_id, /* notify_correlation_id: write-only */
                                                     NULL, /* expiry_time (TODO) */
                                                     NULL, /* nfc_instance_id: write-only (TODO - use app nf_instance if available) */
                                                     NULL /* mbs_session_subsc_uri: read-only */
                                                 );
        }
    }

    /* Create the request */
    msg.CreateReqData = OpenAPI_create_req_data_create(ext_mbs_session);

    /* message => request for CreateReqData is not implemented, so do so here */
    cJSON *json = OpenAPI_create_req_data_convertToJSON(msg.CreateReqData);
    char *body = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    ogs_sbi_request_t *req = ogs_sbi_build_request(&msg);
    ogs_expect(req);

    ogs_sbi_header_set(req->http.headers, OGS_SBI_CONTENT_TYPE, OGS_SBI_CONTENT_JSON_TYPE);
    req->http.content = body;
    req->http.content_length = strlen(body);

    if (msg.CreateReqData) OpenAPI_create_req_data_free(msg.CreateReqData);

    ogs_debug("CreateReqData: %s", body);

    return req;
}

ogs_sbi_request_t *_nmbsmf_mbs_session_build_remove(void *context, void *data)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)context;

    ogs_sbi_message_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.h.method = (char*)OGS_SBI_HTTP_METHOD_DELETE;
    msg.h.service.name = (char*)OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION;
    msg.h.api.version = (char *)OGS_SBI_API_V1;
    msg.h.resource.component[0] = (char *)OGS_SBI_RESOURCE_NAME_MBS_SESSIONS;
    msg.h.resource.component[1] = ogs_strdup(session->id);

    ogs_sbi_request_t *req = ogs_sbi_build_request(&msg);

    return req;
}

ogs_sbi_request_t *_nmbsmf_mbs_session_build_status_subscription_create(void *context, void *data)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)context;
    _priv_mbs_status_subscription_t *subsc = (_priv_mbs_status_subscription_t*)data;

    ogs_sbi_message_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.h.method = (char*)OGS_SBI_HTTP_METHOD_POST;
    msg.h.service.name = (char*)OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION;
    msg.h.api.version = (char *)OGS_SBI_API_V1;
    msg.h.resource.component[0] = (char *)OGS_SBI_RESOURCE_NAME_MBS_SESSIONS;
    msg.h.resource.component[1] = (char *)OGS_SBI_RESOURCE_NAME_SUBSCRIPTIONS;

    char *body = NULL;

    OpenAPI_mbs_session_id_t *mbs_session_id = __make_mbs_session_id(session, NULL);

    bool has_area_session_id = (subsc->area_session_id != 0);
    int area_session_id = subsc->area_session_id;

    OpenAPI_list_t *event_list = NULL;

    char *notify_uri = subsc->cache?subsc->cache->notif_url:NULL;

    char *notify_correlation_id = subsc->correlation_id?ogs_strdup(subsc->correlation_id):NULL;

    char *expiry_time = (subsc->expiry_time!=0)?ogs_sbi_gmtime_string(subsc->expiry_time):NULL;

    char *nfc_instance_id = NF_INSTANCE_ID(ogs_sbi_self()->nf_instance);
    if (nfc_instance_id) nfc_instance_id = ogs_strdup(nfc_instance_id);

    OpenAPI_mbs_session_subscription_t *subscription = OpenAPI_mbs_session_subscription_create(
                                                                mbs_session_id,
                                                                has_area_session_id, area_session_id,
                                                                event_list,
                                                                notify_uri,
                                                                notify_correlation_id,
                                                                expiry_time,
                                                                nfc_instance_id,
                                                                NULL /* Read-Only */);

    ogs_assert(subscription);

    OpenAPI_status_subscribe_req_data_t *req_data = OpenAPI_status_subscribe_req_data_create(subscription);
    ogs_assert(req_data);

    cJSON *json = OpenAPI_status_subscribe_req_data_convertToJSON(req_data);
    if (json) {
        body = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    ogs_sbi_request_t *req = ogs_sbi_build_request(&msg);
    req->http.content = body;
    req->http.content_length = body?strlen(body):0;

    return req;
}

void _notification_server_free(ogs_sbi_server_t *server)
{
    if (!server || fixed_port_notifications == server) return;

    ogs_sbi_server_actions.stop(server);
    ogs_sbi_server_remove(server);
}

void _tidy_fixed_notification_server()
{
    if (!fixed_port_notifications) return;
    ogs_sbi_server_actions.stop(fixed_port_notifications);
    ogs_sbi_server_remove(fixed_port_notifications);
}

/* Private functions */

static OpenAPI_ip_addr_t *__new_OpenAPI_ip_addr_from_inaddr(const struct in_addr *addr)
{
    OpenAPI_ip_addr_t *ret = NULL;
    char addr_str[INET_ADDRSTRLEN];

    if (inet_ntop(AF_INET, addr, addr_str, sizeof(addr_str))) {
        ret = OpenAPI_ip_addr_create(ogs_strdup(addr_str), NULL, NULL);
    }

    return ret;
}

static OpenAPI_ip_addr_t *__new_OpenAPI_ip_addr_from_in6addr(const struct in6_addr *addr)
{
    OpenAPI_ip_addr_t *ret = NULL;
    char addr_str[INET6_ADDRSTRLEN];

    if (inet_ntop(AF_INET6, addr, addr_str, sizeof(addr_str))) {
        ret = OpenAPI_ip_addr_create(NULL, ogs_strdup(addr_str), NULL);
    }

    return ret;
}

static void __allocate_notification_server(_priv_mbs_status_subscription_t *subsc)
{
    if (subsc->cache->notif_server) return; /* Already got a server allocated */

    const ogs_sockaddr_t *notif_address = _context_get_notification_address();

    if (!notif_address) {
        /* if not configured, use ephemeral port on IPv4 any address */
        static const ogs_sockaddr_t any_ephemeral_v4 = { .sa = { .sa_family = AF_INET } };
        notif_address = &any_ephemeral_v4;
    }

    bool is_ephemeral = false;
    if (notif_address->ogs_sa_family == AF_INET) {
        if (notif_address->sin.sin_port == 0) {
            is_ephemeral = true;
        }
    } else if (notif_address->ogs_sa_family == AF_INET6) {
        if (notif_address->sin6.sin6_port == 0) {
            is_ephemeral = true;
        }
    }

    ogs_sbi_header_t header;
    memset(&header, 0, sizeof(header));
    header.service.name = (char*)"mbs-session-notify";
    header.api.version = (char*)"v1";

    if (is_ephemeral) {
        /* ephemeral port, allocate new server, notifications to come in at root */
        subsc->cache->notif_server = __new_sbi_server(notif_address);
        if (subsc->cache->notif_server) {
            header.resource.component[0] = (char*)"notification";
            subsc->cache->notif_url = ogs_sbi_server_uri(subsc->cache->notif_server, &header);
        }
    } else {
        if (!fixed_port_notifications) fixed_port_notifications = __new_sbi_server(notif_address);
        if (fixed_port_notifications) {
            ogs_uuid_t uuid;
            char id[OGS_UUID_FORMATTED_LENGTH + 1];

            ogs_uuid_get(&uuid);
            ogs_uuid_format(id, &uuid);
            header.resource.component[0] = id;
            subsc->cache->notif_server = fixed_port_notifications;
            subsc->cache->notif_url = ogs_sbi_server_uri(subsc->cache->notif_server, &header);
        }
    }
}

static ogs_sbi_server_t *__new_sbi_server(const ogs_sockaddr_t *address)
{
    ogs_sbi_server_t *svr = ogs_sbi_server_add(NULL, OpenAPI_uri_scheme_http, (ogs_sockaddr_t*)address, NULL);
    if (svr) {
        ogs_sbi_server_actions.start(svr, ogs_sbi_server_handler);
        if ((address->ogs_sa_family == AF_INET && address->sin.sin_port == 0) ||
            (address->ogs_sa_family == AF_INET6 && address->sin6.sin6_port == 0)) {
            /* Retrieve bound address to get port number of ephemeral port */
            char buf[OGS_ADDRSTRLEN];
            socklen_t len = ogs_sockaddr_len(&svr->node.sock->local_addr);
            getsockname(svr->node.sock->fd, (struct sockaddr*)&svr->node.sock->local_addr, &len);
            ogs_freeaddrinfo(svr->node.addr);
            ogs_copyaddrinfo(&svr->node.addr, &svr->node.sock->local_addr);
            ogs_info("Ephemeral notification server(%s) [%s://%s]:%u", svr->interface ? svr->interface : "",
                     svr->ssl_ctx ? "https" : "http", OGS_ADDR(svr->node.addr, buf), OGS_PORT(svr->node.addr));
        }
    }
    return svr;
}

static OpenAPI_mbs_session_id_t *__make_mbs_session_id(_priv_mbs_session_t *session, OpenAPI_ssm_t **ssm_ptr)
{
    OpenAPI_mbs_session_id_t *mbs_session_id = NULL;
    if (session->session.ssm || session->session.tmgi) {
        mbs_session_id = OpenAPI_mbs_session_id_create(NULL /*tmgi*/, NULL /*ssm*/, NULL /*nid*/);
    }
    if (session->session.ssm) {
        OpenAPI_ip_addr_t *src = NULL, *dest = NULL;
        if (session->session.ssm->family == AF_INET) {
            src = __new_OpenAPI_ip_addr_from_inaddr(&session->session.ssm->source.ipv4);
            dest = __new_OpenAPI_ip_addr_from_inaddr(&session->session.ssm->dest_mc.ipv4);
        } else {
            src = __new_OpenAPI_ip_addr_from_in6addr(&session->session.ssm->source.ipv6);
            dest = __new_OpenAPI_ip_addr_from_in6addr(&session->session.ssm->dest_mc.ipv6);
        }
        mbs_session_id->ssm = OpenAPI_ssm_create(src, dest);
        if (ssm_ptr) *ssm_ptr = OpenAPI_ssm_copy(*ssm_ptr, mbs_session_id->ssm);
    }
    if (session->session.tmgi) {
        OpenAPI_plmn_id_t *plmn_id = OpenAPI_plmn_id_create(ogs_plmn_id_mcc_string(&session->session.tmgi->plmn),
                                                            ogs_plmn_id_mnc_string(&session->session.tmgi->plmn));
        mbs_session_id->tmgi = OpenAPI_tmgi_create(ogs_strdup(session->session.tmgi->mbs_service_id), plmn_id);
    }
    return mbs_session_id;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
