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
    }

    /* TODO: process the events and trigger local notification events for each one */

    return OGS_OK;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
