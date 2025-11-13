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
#include "openapi/model/mbs_service_type.h"
#include "openapi/model/mbs_session_activity_status.h"
#include "openapi/model/status_subscribe_req_data.h"

#include "macros.h"
#include "context.h"
#include "log.h"
#include "priv_arp.h"
#include "priv_ncgi-tai.h"
#include "priv_ncgi.h"
#include "priv_flow-description.h"
#include "priv_tai.h"
#include "priv_mbs-session.h"
#include "priv_mbs-status-subscription.h"
#include "priv_civic-address.h"
#include "priv_ext-mbs-service-area.h"
#include "priv_mbs-service-area.h"
#include "priv_mbs-service-info.h"
#include "priv_mbs-media-comp.h"
#include "priv_mbs-media-info.h"
#include "priv_mbs-qos-req.h"
#include "utils.h"

#include "nmbsmf-mbs-session-build.h"

extern ogs_sbi_server_actions_t ogs_sbi_server_actions;

static void __allocate_notification_server(_priv_mbs_status_subscription_t *subscription);
static ogs_sbi_server_t *__new_sbi_server(const ogs_sockaddr_t *address);
static OpenAPI_mbs_session_id_t *__make_mbs_session_id(_priv_mbs_session_t *session, OpenAPI_ssm_t **ssm_ptr);

static ogs_sbi_server_t *fixed_port_notifications = NULL;

static OpenAPI_ext_mbs_session_t *__make_ext_mbs_session(_priv_mbs_session_t *session, bool for_update);
static char *__bitrate_to_str(double bitrate);

/* Library Internals */

// MBS Session builders

ogs_sbi_request_t *_nmbsmf_mbs_session_build_create(void *context, void *data)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)context;

    ogs_sbi_message_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.h.method = (char*)OGS_SBI_HTTP_METHOD_POST;
    msg.h.service.name = (char*)OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION;
    msg.h.api.version = (char*)OGS_SBI_API_V1;
    msg.h.resource.component[0] = (char*)OGS_SBI_RESOURCE_NAME_MBS_SESSIONS;
    msg.http.content_type = (char*)OGS_SBI_CONTENT_JSON_TYPE;

    OpenAPI_ext_mbs_session_t *ext_mbs_session = __make_ext_mbs_session(session, false);

    /* Add first subscription */
    _priv_mbs_status_subscription_t *subsc = NULL;
    if (!ogs_list_empty(&session->new_subscriptions)) {
        subsc = ogs_list_first(&session->new_subscriptions);
    } else if (session->active_subscriptions && ogs_hash_count(session->active_subscriptions) > 0) {
        ogs_hash_index_t *it = ogs_hash_first(session->active_subscriptions);
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
                                                     ogs_strdup(NF_INSTANCE_ID(ogs_sbi_self()->nf_instance)), /* nfc_instance_id: write-only */
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

    req->http.content = body;
    req->http.content_length = body?strlen(body):0;

    if (msg.CreateReqData) OpenAPI_create_req_data_free(msg.CreateReqData);

    ogs_debug("CreateReqData: %s", body);

    return req;
}

ogs_sbi_request_t *_nmbsmf_mbs_session_build_update(void *context, void *data)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)context;

    ogs_sbi_message_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.h.method = (char*)OGS_SBI_HTTP_METHOD_PATCH;
    msg.h.service.name = (char*)OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION;
    msg.h.api.version = (char*)OGS_SBI_API_V1;
    msg.http.content_type = (char*)OGS_SBI_CONTENT_PATCH_TYPE;
    msg.h.resource.component[0] = (char*)OGS_SBI_RESOURCE_NAME_MBS_SESSIONS;
    msg.h.resource.component[1] = session->id;
    msg.PatchItemList = OpenAPI_list_create();

    /* TODO: populate the PatchItemList with operations for things that have changed */
    ogs_warn("TODO: complete implementation of the MBS Session update builder");

    ogs_sbi_request_t *req = ogs_sbi_build_request(&msg);
    ogs_expect(req);

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
    msg.h.resource.component[1] = session->id;

    ogs_sbi_request_t *req = ogs_sbi_build_request(&msg);

    return req;
}

// MBS Session Status Subscription builders

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
    static const struct {
        mb_smf_sc_mbs_session_event_type_t t;
        OpenAPI_mbs_session_event_type_e e;
    } flags[] = {
        {MBS_SESSION_EVENT_MBS_REL_TMGI_EXPIRY, OpenAPI_mbs_session_event_type_MBS_REL_TMGI_EXPIRY},
        {MBS_SESSION_EVENT_BROADCAST_DELIVERY_STATUS, OpenAPI_mbs_session_event_type_BROADCAST_DELIVERY_STATUS},
        {MBS_SESSION_EVENT_INGRESS_TUNNEL_ADD_CHANGE, OpenAPI_mbs_session_event_type_INGRESS_TUNNEL_ADD_CHANGE}
    };

    int i;
    for (i = 0; i<sizeof(flags)/sizeof(flags[0]); i++) {
        if (subsc->flags & flags[i].t) {
            if (!event_list) event_list = OpenAPI_list_create();
            OpenAPI_mbs_session_event_t *event = OpenAPI_mbs_session_event_create(flags[i].e);
            OpenAPI_list_add(event_list, event);
        }
    }

    char *notify_uri = (subsc->cache && subsc->cache->notif_url)?ogs_strdup(subsc->cache->notif_url):NULL;

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

    OpenAPI_status_subscribe_req_data_free(req_data);

    return req;
}

ogs_sbi_request_t *_nmbsmf_mbs_session_build_status_subscription_update(void *context, void *data)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)context;
    _priv_mbs_status_subscription_t *subsc = (_priv_mbs_status_subscription_t*)data;

    ogs_sbi_message_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.h.method = (char*)OGS_SBI_HTTP_METHOD_PATCH;
    msg.h.service.name = (char*)OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION;
    msg.h.api.version = (char *)OGS_SBI_API_V1;
    msg.h.resource.component[0] = (char *)OGS_SBI_RESOURCE_NAME_MBS_SESSIONS;
    msg.h.resource.component[1] = (char *)OGS_SBI_RESOURCE_NAME_SUBSCRIPTIONS;
    msg.h.resource.component[2] = subsc->id;
    msg.http.content_type = (char*)OGS_SBI_CONTENT_PATCH_TYPE;

    msg.PatchItemList = OpenAPI_list_create();

    ogs_list_t *patches = _mbs_session_patch_list(session);
    if (patches) {
        _json_patch_t *patch, *next;
        ogs_list_for_each_safe(patches, next, patch) {
            ogs_list_remove(patches, patch);
            OpenAPI_list_add(msg.PatchItemList, patch->item);
            ogs_free(patch);
        }
        ogs_free(patches);
    }

    ogs_sbi_request_t *req = ogs_sbi_build_request(&msg);

    return req;
}

ogs_sbi_request_t *_nmbsmf_mbs_session_build_status_subscription_delete(void *context, void *data)
{
    //_priv_mbs_session_t *session = (_priv_mbs_session_t*)context;
    _priv_mbs_status_subscription_t *subsc = (_priv_mbs_status_subscription_t*)data;

    ogs_sbi_message_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.h.method = (char*)OGS_SBI_HTTP_METHOD_DELETE;
    msg.h.service.name = (char*)OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION;
    msg.h.api.version = (char *)OGS_SBI_API_V1;
    msg.h.resource.component[0] = (char *)OGS_SBI_RESOURCE_NAME_MBS_SESSIONS;
    msg.h.resource.component[1] = (char *)OGS_SBI_RESOURCE_NAME_SUBSCRIPTIONS;
    msg.h.resource.component[2] = subsc->id;

    ogs_sbi_request_t *req = ogs_sbi_build_request(&msg);

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
    OpenAPI_mbs_session_id_t *mbs_session_id = _mbs_session_create_mbs_session_id(session);
    if (mbs_session_id && ssm_ptr) {
        *ssm_ptr = OpenAPI_ssm_copy(*ssm_ptr, mbs_session_id->ssm);
    }
    return mbs_session_id;
}

static OpenAPI_ext_mbs_session_t *__make_ext_mbs_session(_priv_mbs_session_t *session, bool for_update)
{
    OpenAPI_ext_mbs_session_t *ext_mbs_session = NULL;

    OpenAPI_ssm_t *ssm = NULL;
    OpenAPI_mbs_session_id_t *mbs_session_id = __make_mbs_session_id(session, &ssm);
    bool have_tmgi_req = session->session.tmgi_req;
    bool have_locn_dependent = session->session.location_dependent;
    bool have_tun_req = session->session.tunnel_req;
    bool have_any_ue_ind = session->session.any_ue_ind;
    bool have_contact_pcf_ind = (for_update && session->session.contact_pcf_ind); /* only in update */
    int tmgi_req = session->session.tmgi_req?1:0;
    int tun_req = session->session.tunnel_req?1:0;
    int locn_dependent = session->session.location_dependent?1:0;
    int any_ue_ind = session->session.any_ue_ind?1:0;
    int contact_pcf_ind = (for_update && session->session.contact_pcf_ind)?1:0; /* only in update */
    OpenAPI_mbs_session_activity_status_e activity_status = OpenAPI_mbs_session_activity_status_NULL;
    OpenAPI_mbs_service_type_e service_type = ssm?OpenAPI_mbs_service_type_MULTICAST:OpenAPI_mbs_service_type_BROADCAST;
    OpenAPI_mbs_service_area_t *mbs_service_area = NULL;
    OpenAPI_external_mbs_service_area_t *ext_mbs_service_area = NULL;
    char *dnn = session->session.dnn?ogs_strdup(session->session.dnn):NULL;
    OpenAPI_snssai_t *snssai = NULL;
    char *start_time = NULL;
    char *term_time = NULL;
    OpenAPI_mbs_service_info_t *mbs_service_info = NULL;
    OpenAPI_list_t *mbs_fsa_ids = NULL;
    OpenAPI_mbs_security_context_t *mbs_security_ctx = NULL;

    if (session->session.activity_status == MBS_SESSION_ACTIVITY_STATUS_ACTIVE)
        activity_status = OpenAPI_mbs_session_activity_status_ACTIVE;
    else if (session->session.activity_status == MBS_SESSION_ACTIVITY_STATUS_INACTIVE)
        activity_status = OpenAPI_mbs_session_activity_status_INACTIVE;

    if (session->session.mbs_service_area &&
        ogs_list_count(&session->session.mbs_service_area->ncgi_tais) +
                ogs_list_count(&session->session.mbs_service_area->tais) > 0
       ) {
        mbs_service_area = OpenAPI_mbs_service_area_create(NULL, NULL);
        if (ogs_list_count(&session->session.mbs_service_area->ncgi_tais) > 0) {
            mbs_service_area->ncgi_list = OpenAPI_list_create();
            /* copy ncgi_tais array over to OpenAPI structures */
            mb_smf_sc_ncgi_tai_t *ncgi_tai;
            ogs_list_for_each(&session->session.mbs_service_area->ncgi_tais, ncgi_tai) {
                OpenAPI_tai_t *api_tai = OpenAPI_tai_create(ogs_sbi_build_plmn_id(&ncgi_tai->tai.plmn_id),
                                                            _uint32_to_hex_str(ncgi_tai->tai.tac, 4, 6),
                                                            ncgi_tai->tai.nid?_uint64_to_hex_str(*ncgi_tai->tai.nid, 11, 11):NULL);
                OpenAPI_list_t *api_cell_list = NULL;
                mb_smf_sc_ncgi_t *ncgi;
                ogs_list_for_each(&ncgi_tai->ncgis, ncgi) {
                    if (!api_cell_list) api_cell_list = OpenAPI_list_create();
                    OpenAPI_ncgi_t *api_ncgi = OpenAPI_ncgi_create(ogs_sbi_build_plmn_id(&ncgi->plmn_id),
                                                                   _uint64_to_hex_str(ncgi->nr_cell_id, 9, 9),
                                                                   ncgi->nid?_uint64_to_hex_str(*ncgi->nid, 11, 11):NULL);
                    OpenAPI_list_add(api_cell_list, api_ncgi);
                }
                OpenAPI_ncgi_tai_t *api_ncgi_tai = OpenAPI_ncgi_tai_create(api_tai, api_cell_list);
                OpenAPI_list_add(mbs_service_area->ncgi_list, api_ncgi_tai);
            }
        }
        if (ogs_list_count(&session->session.mbs_service_area->tais) > 0) {
            mbs_service_area->tai_list = OpenAPI_list_create();
            /* copy tais array over to OpenAPI structures */
            mb_smf_sc_tai_t *tai;
            ogs_list_for_each(&session->session.mbs_service_area->tais, tai) {
                OpenAPI_tai_t *api_tai = OpenAPI_tai_create(ogs_sbi_build_plmn_id(&tai->plmn_id),
                                                            _uint32_to_hex_str(tai->tac, 4, 6),
                                                            tai->nid?_uint64_to_hex_str(*tai->nid, 11, 11):NULL);
                OpenAPI_list_add(mbs_service_area->tai_list, api_tai);
            }
        }
    }

    if (session->session.ext_mbs_service_area &&
        ogs_list_count(&session->session.ext_mbs_service_area->geographic_areas) +
                ogs_list_count(&session->session.ext_mbs_service_area->civic_addresses) > 0
       ) {
        ext_mbs_service_area = OpenAPI_external_mbs_service_area_create(NULL,NULL);
        if (ogs_list_count(&session->session.ext_mbs_service_area->geographic_areas) > 0) {
            ext_mbs_service_area->geographic_area_list = OpenAPI_list_create();
            /* TODO: copy array over to OpenAPI structures */
        }
        if (ogs_list_count(&session->session.ext_mbs_service_area->civic_addresses) > 0) {
            ext_mbs_service_area->civic_address_list = OpenAPI_list_create();
            /* copy civic_addresses array over to OpenAPI structures */
            mb_smf_sc_civic_address_t *civic_addr;
            ogs_list_for_each(&session->session.ext_mbs_service_area->civic_addresses, civic_addr) {
                OpenAPI_civic_address_t *api_civic_addr = OpenAPI_civic_address_create(
                    civic_addr->country?ogs_strdup(civic_addr->country):NULL,
                    civic_addr->a[0]?ogs_strdup(civic_addr->a[0]):NULL,
                    civic_addr->a[1]?ogs_strdup(civic_addr->a[1]):NULL,
                    civic_addr->a[2]?ogs_strdup(civic_addr->a[2]):NULL,
                    civic_addr->a[3]?ogs_strdup(civic_addr->a[3]):NULL,
                    civic_addr->a[4]?ogs_strdup(civic_addr->a[4]):NULL,
                    civic_addr->a[5]?ogs_strdup(civic_addr->a[5]):NULL,
                    civic_addr->prd?ogs_strdup(civic_addr->prd):NULL,
                    civic_addr->pod?ogs_strdup(civic_addr->pod):NULL,
                    civic_addr->sts?ogs_strdup(civic_addr->sts):NULL,
                    civic_addr->hno?ogs_strdup(civic_addr->hno):NULL,
                    civic_addr->hns?ogs_strdup(civic_addr->hns):NULL,
                    civic_addr->lmk?ogs_strdup(civic_addr->lmk):NULL,
                    civic_addr->loc?ogs_strdup(civic_addr->loc):NULL,
                    civic_addr->nam?ogs_strdup(civic_addr->nam):NULL,
                    civic_addr->pc?ogs_strdup(civic_addr->pc):NULL,
                    civic_addr->bld?ogs_strdup(civic_addr->bld):NULL,
                    civic_addr->unit?ogs_strdup(civic_addr->unit):NULL,
                    civic_addr->flr?ogs_strdup(civic_addr->flr):NULL,
                    civic_addr->room?ogs_strdup(civic_addr->room):NULL,
                    civic_addr->plc?ogs_strdup(civic_addr->plc):NULL,
                    civic_addr->pcn?ogs_strdup(civic_addr->pcn):NULL,
                    civic_addr->pobox?ogs_strdup(civic_addr->pobox):NULL,
                    civic_addr->addcode?ogs_strdup(civic_addr->addcode):NULL,
                    civic_addr->seat?ogs_strdup(civic_addr->seat):NULL,
                    civic_addr->rd?ogs_strdup(civic_addr->rd):NULL,
                    civic_addr->rdsec?ogs_strdup(civic_addr->rdsec):NULL,
                    civic_addr->rdbr?ogs_strdup(civic_addr->rdbr):NULL,
                    civic_addr->rdsubbr?ogs_strdup(civic_addr->rdsubbr):NULL,
                    civic_addr->prm?ogs_strdup(civic_addr->prm):NULL,
                    civic_addr->pom?ogs_strdup(civic_addr->pom):NULL,
                    civic_addr->usage_rules?ogs_strdup(civic_addr->usage_rules):NULL,
                    civic_addr->method?ogs_strdup(civic_addr->method):NULL,
                    civic_addr->provided_by?ogs_strdup(civic_addr->provided_by):NULL
                );
                OpenAPI_list_add(ext_mbs_service_area->civic_address_list, api_civic_addr);
            }
        }
    }

    if (session->session.snssai) {
        char *sd_str = ogs_s_nssai_sd_to_string(session->session.snssai->sd);
        snssai = OpenAPI_snssai_create(session->session.snssai->sst, sd_str);
    }

    if (session->session.start_time) {
        start_time = ogs_sbi_gmtime_string(*session->session.start_time);
    }
    if (session->session.termination_time) {
        term_time = ogs_sbi_gmtime_string(*session->session.termination_time);
    }
    if (session->session.mbs_service_info) {
        OpenAPI_list_t *media_comps = NULL;
        ogs_hash_index_t *idx = ogs_hash_index_make(session->session.mbs_service_info->mbs_media_comps);
        ogs_hash_index_t *it = idx;
        for (it = ogs_hash_next(it); it; it = ogs_hash_next(it)) {
            mb_smf_sc_mbs_media_comp_t *media_comp = (mb_smf_sc_mbs_media_comp_t*)ogs_hash_this_val(it);
            OpenAPI_list_t *api_flow_descs = NULL;
            OpenAPI_mbs_media_info_t *api_mbs_media_info = NULL;
            char *api_qos_ref = NULL;
            OpenAPI_mbs_qo_s_req_t *api_mbs_qo_s_req = NULL;

            if (media_comp->flow_descriptions) {
                api_flow_descs = OpenAPI_list_create();
                mb_smf_sc_flow_description_t *flow_desc;
                ogs_list_for_each(media_comp->flow_descriptions, flow_desc) {
                    OpenAPI_list_add(api_flow_descs, ogs_strdup(flow_desc->string));
                }
            }

            if (media_comp->media_info) {
                OpenAPI_list_t *api_codecs = NULL;
                OpenAPI_media_type_e api_mbs_media_type = OpenAPI_media_type_NULL;
                switch (media_comp->media_info->mbs_media_type) {
                case MEDIA_TYPE_AUDIO:
                    api_mbs_media_type = OpenAPI_media_type_AUDIO;
                    break;
                case MEDIA_TYPE_VIDEO:
                    api_mbs_media_type = OpenAPI_media_type_VIDEO;
                    break;
                case MEDIA_TYPE_DATA:
                    api_mbs_media_type = OpenAPI_media_type_DATA;
                    break;
                case MEDIA_TYPE_APPLICATION:
                    api_mbs_media_type = OpenAPI_media_type_APPLICATION;
                    break;
                case MEDIA_TYPE_CONTROL:
                    api_mbs_media_type = OpenAPI_media_type_CONTROL;
                    break;
                case MEDIA_TYPE_TEXT:
                    api_mbs_media_type = OpenAPI_media_type_TEXT;
                    break;
                case MEDIA_TYPE_MESSAGE:
                    api_mbs_media_type = OpenAPI_media_type_MESSAGE;
                    break;
                case MEDIA_TYPE_OTHER:
                    api_mbs_media_type = OpenAPI_media_type_OTHER;
                    break;
                default:
                    break;
                }
                if (media_comp->media_info->codecs[0]) {
                    if (!api_codecs) api_codecs = OpenAPI_list_create();
                    OpenAPI_list_add(api_codecs, ogs_strdup(media_comp->media_info->codecs[0]));
                }
                if (media_comp->media_info->codecs[1]) {
                    if (!api_codecs) api_codecs = OpenAPI_list_create();
                    OpenAPI_list_add(api_codecs, ogs_strdup(media_comp->media_info->codecs[1]));
                }
                api_mbs_media_info = OpenAPI_mbs_media_info_create(api_mbs_media_type,
                                media_comp->media_info->max_requested_mbs_bandwidth_downlink?
                                        __bitrate_to_str(*media_comp->media_info->max_requested_mbs_bandwidth_downlink):NULL,
                                media_comp->media_info->min_requested_mbs_bandwidth_downlink?
                                        __bitrate_to_str(*media_comp->media_info->min_requested_mbs_bandwidth_downlink):NULL,
                                api_codecs);
            } else if (media_comp->qos_ref) {
                api_qos_ref = ogs_strdup(media_comp->qos_ref);
            } else if (media_comp->mbs_qos_req) {
                OpenAPI_arp_t *req_mbs_arp = NULL;
                if (media_comp->mbs_qos_req->req_mbs_arp) {
                    req_mbs_arp = OpenAPI_arp_create(!!media_comp->mbs_qos_req->req_mbs_arp->priority_level,
                                                    media_comp->mbs_qos_req->req_mbs_arp->priority_level,
                                                    media_comp->mbs_qos_req->req_mbs_arp->preemption_capability,
                                                    media_comp->mbs_qos_req->req_mbs_arp->preemption_vulnerability);
                }
                api_mbs_qo_s_req = OpenAPI_mbs_qo_s_req_create(media_comp->mbs_qos_req->five_qi,
                                media_comp->mbs_qos_req->guarenteed_bit_rate?
                                        __bitrate_to_str(*media_comp->mbs_qos_req->guarenteed_bit_rate):NULL,
                                media_comp->mbs_qos_req->max_bit_rate?
                                        __bitrate_to_str(*media_comp->mbs_qos_req->max_bit_rate):NULL,
                                !!media_comp->mbs_qos_req->averaging_window,
                                media_comp->mbs_qos_req->averaging_window?*media_comp->mbs_qos_req->averaging_window:0,
                                req_mbs_arp);
            }

            OpenAPI_mbs_media_comp_rm_t *api_media_comp = OpenAPI_mbs_media_comp_rm_create(media_comp->id, api_flow_descs,
                                OpenAPI_reserv_priority_NULL + media_comp->mbs_sdf_reserve_priority,
                                api_mbs_media_info, api_qos_ref, api_mbs_qo_s_req);
            if (!media_comps) media_comps = OpenAPI_list_create();
            OpenAPI_map_t *kv_pair = OpenAPI_map_create(ogs_msprintf("%i", api_media_comp->mbs_med_comp_num), api_media_comp);
            OpenAPI_list_add(media_comps, kv_pair);
        }
        ogs_free(idx);
        mbs_service_info = OpenAPI_mbs_service_info_create(media_comps, OpenAPI_reserv_priority_NULL, NULL, NULL);
        if (session->session.mbs_service_info->mbs_sdf_reserve_priority != 0) {
            mbs_service_info->mbs_sdf_res_prio =
                                OpenAPI_reserv_priority_PRIO_1 + session->session.mbs_service_info->mbs_sdf_reserve_priority - 1;
        }
        if (session->session.mbs_service_info->af_app_id) {
            mbs_service_info->af_app_id = ogs_strdup(session->session.mbs_service_info->af_app_id);
        }
        if (session->session.mbs_service_info->mbs_session_ambr) {
            mbs_service_info->mbs_session_ambr = __bitrate_to_str(*session->session.mbs_service_info->mbs_session_ambr);
        }
    }

    if (ogs_list_count(&session->session.mbs_fsa_ids) > 0) {
        mbs_fsa_ids = OpenAPI_list_create();
        /* TODO: copy array values to new structure */
    }

    ext_mbs_session = OpenAPI_ext_mbs_session_create(mbs_session_id, /* mbs_session_id */
                                                     have_tmgi_req, tmgi_req, /* tmgi_alloc_req: write-only */
                                                     NULL, /* tmgi: read-only */
                                                     NULL, /* expiry_time: read-only */
                                                     service_type, /* service_type: write-only */
                                                     have_locn_dependent, locn_dependent, /* location_dependant */
                                                     false, 0, /* area_session_id: read-only */
                                                     have_tun_req, tun_req, /* ingress_tun_addr_req: write-only */
                                                     NULL, /* ingress_tun_addr list: read-only */
                                                     ssm,  /* ssm: write-only */
                                                     mbs_service_area, /* mbs_service_area: write-only */
                                                     ext_mbs_service_area, /* ext_mbs_service_area: write-only */
                                                     /* red_mbs_serv_area: read-only */
                                                     /* ext_red_mbs_serv_area: read-only */
                                                     dnn, /* dnn: write-only */
                                                     snssai, /* snssai: write-only */
                                                     NULL, /* activation_time: deprecated */
                                                     start_time, /* start_time */
                                                     term_time, /* termination_time */
                                                     mbs_service_info, /* mbs_serv_info */
                                                     NULL, /* mbs_session_subsc */
                                                     activity_status, /* activity_status */
                                                     have_any_ue_ind, any_ue_ind, /* any_ue_ind */
                                                     mbs_fsa_ids, /* mbs_fsa_id_list */
                                                     /* associated_session_id */
                                                     mbs_security_ctx, /* mbs_security_context */
                                                     have_contact_pcf_ind, contact_pcf_ind /* contact_pcf_ind */
                                                     /* area_session_policy_id */
                                                    );
    return ext_mbs_session;
}

static char *__bitrate_to_str(double bitrate)
{
    double divisor = 1.0;
    const char *units = "bps";

    if (bitrate >= 1e12) {
        divisor = 1e12;
        units = "Tbps";
    } else if (bitrate >= 1e9) {
        divisor = 1e9;
        units = "Gbps";
    } else if (bitrate >= 1e6) {
        divisor = 1e6;
        units = "Mbps";
    } else if (bitrate >= 1e3) {
        divisor = 1e3;
        units = "Kbps";
    }

    return ogs_msprintf("%.6f %s", bitrate/divisor, units);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
