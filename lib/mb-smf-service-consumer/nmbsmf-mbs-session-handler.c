/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "ogs-core.h"
#include "ogs-sbi.h"

#include "log.h"
#include "priv_mbs-session.h"

#include "nmbsmf-mbs-session-handler.h"

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
        if (!mbs_session->tmgi) {
            ogs_error("TMGI requested but no TMGI in the response from the MB-SMF");
            return OGS_ERROR;
        }
        /* allocate tmgi if we don't have one already */
        if (!sess->session.tmgi) {
            sess->session.tmgi = ogs_calloc(1, sizeof(*sess->session.tmgi));
        }
        /* replace tmgi contents with the received TMGI */
        if (sess->session.tmgi->mbs_service_id) ogs_free(sess->session.tmgi->mbs_service_id);
        sess->session.tmgi->mbs_service_id = ogs_strdup(mbs_session->tmgi->mbs_service_id);
        ogs_plmn_id_build(&sess->session.tmgi->plmn, atoi(mbs_session->tmgi->plmn_id->mcc),
                                                     atoi(mbs_session->tmgi->plmn_id->mnc),
                                                     strlen(mbs_session->tmgi->plmn_id->mnc));
        /* Copy new TMGI to previous record to detect app changes */
        if (!sess->previous_tmgi) {
            sess->previous_tmgi = ogs_calloc(1, sizeof(*sess->session.tmgi));
        }
        if (sess->previous_tmgi->mbs_service_id) ogs_free(sess->previous_tmgi->mbs_service_id);
        sess->previous_tmgi->mbs_service_id = ogs_strdup(sess->session.tmgi->mbs_service_id);
        memcpy(&sess->previous_tmgi->plmn, &sess->session.tmgi->plmn, sizeof(sess->previous_tmgi->plmn));
    }

    if (mbs_session->ssm && mbs_session->ssm->source_ip_addr && mbs_session->ssm->dest_ip_addr) {
        ogs_sockaddr_t *addr = NULL;
        if (!sess->session.ssm) {
            sess->session.ssm = ogs_calloc(1, sizeof(*sess->session.ssm));
        }
        if (mbs_session->ssm->source_ip_addr->ipv4_addr && mbs_session->ssm->dest_ip_addr->ipv4_addr) {
            sess->session.ssm->family = AF_INET;
            ogs_getaddrinfo(&addr, AF_INET, mbs_session->ssm->source_ip_addr->ipv4_addr, 0, 0);
            sess->session.ssm->source.ipv4 = addr->sin.sin_addr;
            ogs_freeaddrinfo(addr);
            addr = NULL;
            ogs_getaddrinfo(&addr, AF_INET, mbs_session->ssm->dest_ip_addr->ipv4_addr, 0, 0);
            sess->session.ssm->dest_mc.ipv4 = addr->sin.sin_addr;
            ogs_freeaddrinfo(addr);
        } else if (mbs_session->ssm->source_ip_addr->ipv6_addr && mbs_session->ssm->dest_ip_addr->ipv6_addr) {
            sess->session.ssm->family = AF_INET6;
            ogs_getaddrinfo(&addr, AF_INET6, mbs_session->ssm->source_ip_addr->ipv6_addr, 0, 0);
            memcpy(&sess->session.ssm->source.ipv6, &addr->sin6.sin6_addr, sizeof(sess->session.ssm->source.ipv6));
            ogs_freeaddrinfo(addr);
            addr = NULL;
            ogs_getaddrinfo(&addr, AF_INET6, mbs_session->ssm->dest_ip_addr->ipv6_addr, 0, 0);
            memcpy(&sess->session.ssm->dest_mc.ipv6, &addr->sin6.sin6_addr, sizeof(sess->session.ssm->dest_mc.ipv6));
            ogs_freeaddrinfo(addr);
        } else {
            ogs_warn("Unable to extract SSM details from the MbsSession");
        }
    }

    /* TODO: process the events and trigger local notification events for each one */

    return OGS_OK;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
