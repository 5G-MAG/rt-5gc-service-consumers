/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#define _DEFAULT_SOURCE     /* See feature_test_macros(7) */
#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>

#include "ogs-core.h"
#include "ogs-sbi.h"
#include "openapi/model/status_subscribe_rsp_data.h"

#include "context.h"
#include "log.h"
#include "priv_mbs-session.h"
#include "priv_mbs-status-subscription.h"
#include "priv_tmgi.h"

#include "nmbsmf-mbs-session-handle.h"

int _nmbsmf_mbs_session_parse(ogs_sbi_message_t *message, _priv_mbs_session_t *sess)
{
    ogs_assert(sess);
    ogs_assert(message);

    if (message->http.location) {
        int rv;
        ogs_sbi_header_t header;
        ogs_sbi_message_t msg;
        memset(&header, 0, sizeof(header));
        header.uri = message->http.location;
        rv = ogs_sbi_parse_header(&msg, &header);
        if (rv != OGS_OK) {
            ogs_error("Cannot parse http.location [%s]", message->http.location);
            return OGS_ERROR;
        }
        if (!msg.h.resource.component[1]) {
            ogs_error("No MBS Session resource id in http.location [%s]", message->http.location);
            return OGS_ERROR;
        }
        sess->id = ogs_strdup(msg.h.resource.component[1]);
        ogs_sbi_message_free(&msg);
        ogs_sbi_header_free(&header);
    } else {
        ogs_error("No Location header found in the Create response, unable to determine MBS Session identifier");
        return OGS_ERROR;
    }

    OpenAPI_create_rsp_data_t *create_rsp_data = message->CreateRspData;
    if (!create_rsp_data) {
        ogs_error("CreateRspData object not found in response");
        return OGS_ERROR;
    }

    OpenAPI_ext_mbs_session_t *mbs_session = create_rsp_data->mbs_session;
    if (!mbs_session) {
        ogs_error("CreateRspData object does not contain the expected mbsSession object");
        return OGS_ERROR;
    }

    if (sess->session.tunnel_req) {
        if (!mbs_session->ingress_tun_addr) {
            ogs_error("Ingress tunnel requested but no tunnel information in the response from the MB-SMF");
            return OGS_ERROR;
        }
        if (sess->session.mb_upf_udp_tunnel) {
            ogs_freeaddrinfo(sess->session.mb_upf_udp_tunnel);
            sess->session.mb_upf_udp_tunnel = NULL;
        }
        OpenAPI_lnode_t *node;
        OpenAPI_list_for_each(mbs_session->ingress_tun_addr, node) {
            OpenAPI_tunnel_address_t *tun_addr = (OpenAPI_tunnel_address_t*)node->data;
            if (tun_addr) {
                if (tun_addr->ipv4_addr) {
                    ogs_addaddrinfo(&sess->session.mb_upf_udp_tunnel, AF_INET, tun_addr->ipv4_addr, tun_addr->port_number,
                                    AI_NUMERICHOST|AI_ADDRCONFIG);
                } else if (tun_addr->ipv6_addr) {
                    ogs_addaddrinfo(&sess->session.mb_upf_udp_tunnel, AF_INET6, tun_addr->ipv6_addr, tun_addr->port_number,
                                    AI_NUMERICHOST|AI_ADDRCONFIG);
                } else {
                    ogs_error("No address specified in TunnelAddress");
                }
            }
        }
    }

    if (sess->session.tmgi_req) {
        if (!mbs_session->tmgi || !mbs_session->expiration_time) {
            ogs_error("TMGI requested but no TMGI and/or expiry time in the response from the MB-SMF");
            return OGS_ERROR;
        }
        /* create new TMGI */
        _priv_tmgi_t *tmgi = _tmgi_create(NULL, NULL);
        _tmgi_set_mbs_service_id(tmgi, mbs_session->tmgi->mbs_service_id);
        _tmgi_set_plmn(tmgi, atoi(mbs_session->tmgi->plmn_id->mcc), atoi(mbs_session->tmgi->plmn_id->mnc));

        struct tm tmgi_expire_tm = {};
        char *rest = ogs_strptime(mbs_session->expiration_time, "%Y-%m-%dT%H:%M:%SZ", &tmgi_expire_tm);
        if (!rest || rest[0]) {
            ogs_error("MBS Session TMGI expiry time has a bad timestamp: %s", mbs_session->expiration_time);
            _context_remove_tmgi(tmgi);
            return OGS_ERROR;
        }
        _tmgi_set_expiry_time(tmgi, ogs_mktime(&tmgi_expire_tm));
        
        _tmgi_replace_sbi_object(tmgi, sess->sbi_object);
        
        sess->session.tmgi = _priv_tmgi_to_public(tmgi);
        
        /* Copy new TMGI to previous record to detect app changes */
        sess->previous_tmgi = _tmgi_copy(sess->previous_tmgi, tmgi);
    }

    if (mbs_session->mbs_session_id->ssm && mbs_session->mbs_session_id->ssm->source_ip_addr &&
            mbs_session->mbs_session_id->ssm->dest_ip_addr) {
        ogs_sockaddr_t *addr = NULL;
        if (!sess->session.ssm) {
            sess->session.ssm = ogs_calloc(1, sizeof(*sess->session.ssm));
        }
        if (mbs_session->mbs_session_id->ssm->source_ip_addr->ipv4_addr &&
                mbs_session->mbs_session_id->ssm->dest_ip_addr->ipv4_addr) {
            sess->session.ssm->family = AF_INET;
            ogs_getaddrinfo(&addr, AF_INET, mbs_session->mbs_session_id->ssm->source_ip_addr->ipv4_addr, 0, 0);
            sess->session.ssm->source.ipv4 = addr->sin.sin_addr;
            ogs_freeaddrinfo(addr);
            addr = NULL;
            ogs_getaddrinfo(&addr, AF_INET, mbs_session->mbs_session_id->ssm->dest_ip_addr->ipv4_addr, 0, 0);
            sess->session.ssm->dest_mc.ipv4 = addr->sin.sin_addr;
            ogs_freeaddrinfo(addr);
        } else if (mbs_session->mbs_session_id->ssm->source_ip_addr->ipv6_addr &&
                mbs_session->mbs_session_id->ssm->dest_ip_addr->ipv6_addr) {
            sess->session.ssm->family = AF_INET6;
            ogs_getaddrinfo(&addr, AF_INET6, mbs_session->mbs_session_id->ssm->source_ip_addr->ipv6_addr, 0, 0);
            memcpy(&sess->session.ssm->source.ipv6, &addr->sin6.sin6_addr, sizeof(sess->session.ssm->source.ipv6));
            ogs_freeaddrinfo(addr);
            addr = NULL;
            ogs_getaddrinfo(&addr, AF_INET6, mbs_session->mbs_session_id->ssm->dest_ip_addr->ipv6_addr, 0, 0);
            memcpy(&sess->session.ssm->dest_mc.ipv6, &addr->sin6.sin6_addr, sizeof(sess->session.ssm->dest_mc.ipv6));
            ogs_freeaddrinfo(addr);
        } else {
            ogs_warn("Unable to extract SSM details from the MbsSession");
        }
        if (!sess->previous_ssm) {
            sess->previous_ssm = (mb_smf_sc_ssm_addr_t*)ogs_malloc(sizeof(*sess->previous_ssm));
        }
        memcpy(sess->previous_ssm, sess->session.ssm, sizeof(*sess->previous_ssm));
    }

    OpenAPI_mbs_session_event_report_list_t *event_list = create_rsp_data->event_list;
    if (event_list && event_list->event_report_list) {
        /* process the events and trigger local notification events for each one */
        _priv_mbs_status_subscription_t *subsc = _mbs_session_find_subscription(sess, event_list->notify_correlation_id);
        _nmbsmf_mbs_session_subscription_report_list_handler(subsc, event_list->event_report_list);
    }

    return OGS_OK;
}

int _nmbsmf_mbs_session_subscription_report_list_handler(_priv_mbs_status_subscription_t *subsc,
                                                         OpenAPI_list_t *event_report_list)
{
    if (!subsc || !subsc->callback || !event_report_list) return 0;

    OpenAPI_lnode_t *node = NULL;
    OpenAPI_list_for_each(event_report_list, node) {
        OpenAPI_mbs_session_event_report_t *report = (OpenAPI_mbs_session_event_report_t*)node->data;
        /* Parse event time */
        struct tm report_tm = {};
        char *rest = ogs_strptime(report->time_stamp, "%Y-%m-%dT%H:%M:%SZ", &report_tm);
        if (!rest || rest[0]) {
            ogs_warn("Received notification from MB-SMF with bad timestamp: %s", report->time_stamp);
            continue;
        }

        /* Build a notification result to send to the app callback */
        mb_smf_sc_mbs_status_notification_result_t notification = {
            .mbs_session = _priv_mbs_session_to_public(subsc->session),
            .correlation_id = subsc->correlation_id,
            .event_time = ogs_mktime(&report_tm)
        };

        /* Add event specific results to notification result */
        switch (report->event_type) {
        case OpenAPI_mbs_session_event_type_MBS_REL_TMGI_EXPIRY:
            notification.event_type = MBS_SESSION_EVENT_MBS_REL_TMGI_EXPIRY;
            notification.event_type_name = (char*)"MBS_REL_TMGI_EXPIRY";
            break;
        case OpenAPI_mbs_session_event_type_BROADCAST_DELIVERY_STATUS:
            notification.event_type = MBS_SESSION_EVENT_BROADCAST_DELIVERY_STATUS;
            notification.event_type_name = (char*)"BROADCAST_DELIVERY_STATUS";
            switch (report->broadcast_del_status) {
            case OpenAPI_broadcast_delivery_status_STARTED:
                notification.broadcast_delivery_status = BROADCAST_DELIVERY_STARTED;
                break;
            case OpenAPI_broadcast_delivery_status_TERMINATED:
                notification.broadcast_delivery_status = BROADCAST_DELIVERY_TERMINATED;
                break;
            default:
                break;
            }
            break;
        case OpenAPI_mbs_session_event_type_INGRESS_TUNNEL_ADD_CHANGE:
        {
            OpenAPI_lnode_t *node2;
            notification.event_type = MBS_SESSION_EVENT_INGRESS_TUNNEL_ADD_CHANGE;
            notification.event_type_name = (char*)"INGRESS_TUNNEL_ADD_CHANGE";
            OpenAPI_list_for_each(report->ingress_tun_addr_info->ingress_tun_addr, node2) {
                OpenAPI_tunnel_address_t *tun_addr = (OpenAPI_tunnel_address_t*)node2->data;
                mb_smf_sc_mbs_status_notification_ingress_tunnel_addr_t *notif_tun_addr =
                        (mb_smf_sc_mbs_status_notification_ingress_tunnel_addr_t*)ogs_calloc(1,
                                sizeof(mb_smf_sc_mbs_status_notification_ingress_tunnel_addr_t));
                struct in_addr *ipv4 = NULL;
                if (tun_addr->ipv4_addr) {
                    ipv4 = (struct in_addr*)ogs_calloc(1, sizeof(*ipv4));
                    if (inet_pton(AF_INET, tun_addr->ipv4_addr, (void*)ipv4) != 1) {
                        ogs_warn("Received INGRESS_TUNNEL_ADD_CHANGE notification from MB-SMF with bad IPv4 address: %s",
                                 tun_addr->ipv4_addr);
                        ogs_free(ipv4);
                        ipv4 = NULL;
                    }
                }
                notif_tun_addr->ipv4 = ipv4;

                struct in6_addr *ipv6 = NULL;
                if (tun_addr->ipv6_addr) {
                    ipv6 = (struct in6_addr*)ogs_calloc(1, sizeof(*ipv6));
                    if (inet_pton(AF_INET6, tun_addr->ipv6_addr, (void*)ipv6) != 1) {
                        ogs_warn("Received INGRESS_TUNNEL_ADD_CHANGE notification from MB-SMF with bad IPv6 address: %s",
                                 tun_addr->ipv6_addr);
                        ogs_free(ipv6);
                        ipv6 = NULL;
                    }
                }
                notif_tun_addr->ipv6 = ipv6;

                notif_tun_addr->port = tun_addr->port_number;

                ogs_list_add(&notification.ingress_tunnel_add_change, notif_tun_addr);
            }
            break;
        }
        default:
            break;
        }

        if (subsc->flags & notification.event_type) {
            subsc->callback(&notification, subsc->callback_data);
        }

        /* Free memory allocated for the notification */
        if (notification.event_type & MBS_SESSION_EVENT_INGRESS_TUNNEL_ADD_CHANGE) {
            mb_smf_sc_mbs_status_notification_ingress_tunnel_addr_t *ita, *ita_next;
            ogs_list_for_each_safe(&notification.ingress_tunnel_add_change, ita_next, ita) {
                if (ita->ipv4) ogs_free(ita->ipv4);
                if (ita->ipv6) ogs_free(ita->ipv6);
                ogs_list_remove(&notification.ingress_tunnel_add_change, ita);
                ogs_free(ita);
            }
        }
    }
    return 1;
}

void _nmbsmf_mbs_session_subscription_response(_priv_mbs_session_t *sess, ogs_sbi_message_t *message, ogs_sbi_response_t *response)
{
    if (!sess || !message || !response) return;

    cJSON *json = cJSON_Parse(response->http.content);
    bool error = false;

    if (!json) {
        ogs_error("Response from MB-SMF, to subscription creation, is not valid JSON");
        error = true;
    } else {
        if (response->status >= 200 && response->status < 300) {
            OpenAPI_status_subscribe_rsp_data_t *status_subscribe_rsp_data = OpenAPI_status_subscribe_rsp_data_parseFromJSON(json);
            if (!status_subscribe_rsp_data) {
                ogs_error("Response from MB-SMF, to subscription creation, is not a valid StatusSubscribeRspData");
                error = true;
            } else {
                _priv_mbs_status_subscription_t *subsc = _mbs_session_find_subscription(sess,
                                                                status_subscribe_rsp_data->subscription->notify_correlation_id);
                if (!subsc) {
                    ogs_error("Could not find matching subscription for response from MB-SMF to subscription creation");
                    error = true;
                } else {
                    /* Set the resource id */
                    char *location = strrchr(message->http.location, '/');
                    if (subsc->id) ogs_free(subsc->id);
                    if (location) {
                        subsc->id = ogs_sbi_url_decode(location+1);
                    } else {
                        subsc->id = ogs_sbi_url_decode(message->http.location);
                    }

                    /* move from new_subscriptions to established subscriptions */
                    _priv_mbs_status_subscription_t *node;
                    ogs_list_for_each(&sess->new_subscriptions, node) {
                        if (node == subsc) {
                            ogs_list_remove(&sess->new_subscriptions, subsc);
                            if (!sess->session.subscriptions) sess->session.subscriptions = ogs_hash_make();
                            ogs_hash_set(sess->session.subscriptions, subsc->id, OGS_HASH_KEY_STRING,
                                         _priv_mbs_status_subscription_to_public(subsc));
                            break;
                        }
                    }

                    /* send notifications */
                    _nmbsmf_mbs_session_subscription_report_list_handler(subsc,
                                                                         status_subscribe_rsp_data->event_list->event_report_list);
                }
            }
        } else {
            error = true;
        }
    }

    if (error) {
        /* TODO: What do we do on error?
         *       1. Re-mark subscription for another attempt?
         *          - max retries?
         *       2. Add a callback to tell the app the subscription failed?
         *          - delete subscription object?
         *          - move to a list of failed subscriptions?
         */
    }
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
