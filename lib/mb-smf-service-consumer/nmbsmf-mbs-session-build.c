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

#include "macros.h"
#include "priv_mbs-session.h"

#include "nmbsmf-mbs-session-build.h"

static OpenAPI_ip_addr_t *__new_OpenAPI_ip_addr_from_inaddr(const struct in_addr *addr);
static OpenAPI_ip_addr_t *__new_OpenAPI_ip_addr_from_in6addr(const struct in6_addr *addr);

/* Library Internals */
ogs_sbi_request_t *_nmbsmf_mbs_session_build_create(void *context, void *data)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)context;

    ogs_sbi_request_t *req = ogs_sbi_request_new();

    OpenAPI_ssm_t *ssm = NULL;
    OpenAPI_tmgi_t *tmgi = NULL;
    OpenAPI_mbs_session_id_t *mbs_session_id = OpenAPI_mbs_session_id_create(NULL /*tmgi*/, NULL /*ssm*/, NULL /*nid*/);
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
        ssm = OpenAPI_ssm_copy(ssm, mbs_session_id->ssm);
    }
    if (session->session.tmgi) {
        OpenAPI_plmn_id_t *plmn_id = OpenAPI_plmn_id_create(ogs_plmn_id_mcc_string(&session->session.tmgi->plmn),
                                                            ogs_plmn_id_mnc_string(&session->session.tmgi->plmn));
        mbs_session_id->tmgi = OpenAPI_tmgi_create(ogs_strdup(session->session.tmgi->mbs_service_id), plmn_id);
        tmgi = OpenAPI_tmgi_copy(tmgi, mbs_session_id->tmgi);
    }
    OpenAPI_ext_mbs_session_t *ext_mbs_session = OpenAPI_ext_mbs_session_create(mbs_session_id,
                                                                                !tmgi, (tmgi?0:1),
                                                                                tmgi,
                                                                                NULL, /* TODO expiry_time as str */
                                                                                ssm?OpenAPI_mbs_service_type_MULTICAST
                                                                                   :OpenAPI_mbs_service_type_BROADCAST,
                                                                                false, 0, /* location_dependant */
                                                                                false, 0, /* area_session_id */
                                                                                session->session.tunnel_req,
                                                                                ((session->session.tunnel_req)?1:0),
                                                                                NULL, /* ingress_tun_addr list */
                                                                                ssm,
                                                                                NULL, /* mbs_service_area */
                                                                                NULL, /* ext_mbs_service_area */
                                                                                NULL, /* dnn */
                                                                                NULL, /* snssai */
                                                                                NULL, /* activation_time */
                                                                                NULL, /* start_time */
                                                                                NULL, /* termination_time */
                                                                                NULL, /* mbs_serv_info */
                                                                                NULL, /* mbs_session_subsc TODO */
                                                                                OpenAPI_mbs_session_activity_status_ACTIVE,
                                                                                true, 1, /* any_ue_ind */
                                                                                NULL, /* mbs_fsa_id_list */
                                                                                NULL, /* mbs_security_context */
                                                                                false, 0 /* contact_pcf_ind */
                                                                                );
    OpenAPI_create_req_data_t *create_req_data = OpenAPI_create_req_data_create(ext_mbs_session);

    cJSON *json = OpenAPI_create_req_data_convertToJSON(create_req_data);
    char *body = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    OpenAPI_create_req_data_free(create_req_data);

    req->h.method = ogs_strdup(OGS_SBI_HTTP_METHOD_POST);
    req->h.service.name = ogs_strdup(OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION);
    req->h.api.version = ogs_strdup(OGS_SBI_API_V1);
    req->h.resource.component[0] = ogs_strdup(OGS_SBI_RESOURCE_NAME_MBS_SESSIONS);

    ogs_hash_set(req->http.headers, "Content-Type", OGS_HASH_KEY_STRING, "application/json");
    req->http.content = body;
    req->http.content_length = strlen(body);

    return req;
}

ogs_sbi_request_t *_nmbsmf_mbs_session_build_remove(void *context, void *data)
{
    _priv_mbs_session_t *session = (_priv_mbs_session_t*)context;

    ogs_sbi_request_t *req = ogs_sbi_request_new();

    req->h.method = ogs_strdup(OGS_SBI_HTTP_METHOD_DELETE);
    req->h.service.name = ogs_strdup(OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION);
    req->h.api.version = ogs_strdup(OGS_SBI_API_V1);
    req->h.resource.component[0] = ogs_strdup(OGS_SBI_RESOURCE_NAME_MBS_SESSIONS);
    req->h.resource.component[1] = ogs_strdup(session->id);

    return req;
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

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
