/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2023 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#include "ogs-core.h"
#include "ogs-sbi.h"

#include "context.h"
#include "log.h"
#include "nbsf-management-build.h"
#include "utils.h"

#include "bsf-client-sess.h"

#ifdef __cplusplus
extern "C" {
#endif

bsf_client_sess_t *_bsf_client_sess_new(void)
{
    bsf_client_sess_t *sess;

    sess = ogs_calloc(1, sizeof(*sess));
    ogs_assert(sess);

    _bsf_client_context_active_sessions_add(sess);

    return sess;
}

void _bsf_client_sess_free(bsf_client_sess_t *sess)
{
    if (!sess) return;
    _bsf_client_context_active_sessions_remove(sess);
    ogs_sbi_object_free(&sess->sbi);
    if (sess->ue_address) {
        ogs_freeaddrinfo(sess->ue_address);
        sess->ue_address = NULL;
    }
    if (sess->ipv4addr) {
        ogs_free(sess->ipv4addr);
        sess->ipv4addr = NULL;
    }
    if (sess->dnn) ogs_free(sess->dnn);
    if (sess->supi) ogs_free(sess->supi);
    if (sess->gpsi) ogs_free(sess->gpsi);
    if (sess->ip_domain) ogs_free(sess->ip_domain);
    if (sess->answer_cause) ogs_free(sess->answer_cause);
    if (sess->ipv6prefix) {
        ogs_free(sess->ipv6prefix);
        sess->ipv6prefix = NULL;
    }
    ogs_free(sess);
}

void _bsf_client_sess_log_debug(bsf_client_sess_t *sess, int indent)
{
    char *ue_addr;

    if (!sess) return;

    ogs_debug("%*sSBI object = %p", indent, "", &sess->sbi);
    ue_addr = _sockaddr_string(sess->ue_address);
    ogs_debug("%*sUE Address = %s", indent, "", ue_addr);
    ogs_free(ue_addr);
    ogs_debug("%*sIPv4 Address = %s", indent, "", sess->ipv4addr ? sess->ipv4addr : "<not set>");
    ogs_debug("%*sIPv6 Prefix = %s", indent, "", sess->ipv6prefix ? sess->ipv6prefix : "<not set>");
    ogs_debug("%*sCallback = %p (..., %p)", indent, "", sess->retrieve.callback, sess->retrieve.user_data);
}

bool _bsf_client_sess_ue_address_set(bsf_client_sess_t *sess, const ogs_sockaddr_t *ue_address)
{
    if (!sess) return false;
    if (!ue_address) {
        if (sess->ue_address) {
            ogs_freeaddrinfo(sess->ue_address);
            sess->ue_address = NULL;
        }
    } else {
        if (sess->ue_address) ogs_freeaddrinfo(sess->ue_address);
        ogs_copyaddrinfo(&sess->ue_address, (ogs_sockaddr_t*)ue_address);
    }
    return true;
}

bool _bsf_client_sess_query_set(bsf_client_sess_t *sess, const bsf_pcf_binding_query_t *query)
{
    if (!sess || !query) return false;

    if (query->dnn) sess->dnn = ogs_strdup(query->dnn);
    if (query->s_nssai) {
        sess->s_nssai = *query->s_nssai;
        sess->s_nssai_presence = true;
    }
    if (query->supi) sess->supi = ogs_strdup(query->supi);
    if (query->gpsi) sess->gpsi = ogs_strdup(query->gpsi);
    if (query->ip_domain) sess->ip_domain = ogs_strdup(query->ip_domain);
    sess->narrowed = query->dnn || query->s_nssai || query->supi || query->gpsi || query->ip_domain;

    return true;
}

bool _bsf_client_sess_ipv4addr_set_from_sockaddr(bsf_client_sess_t *sess, const ogs_sockaddr_t *addr)
{
    if (!sess) return false;
    if (!addr) return false;
    if (addr->ogs_sa_family != AF_INET) return false;

    if (sess->ipv4addr) ogs_free(sess->ipv4addr);
    sess->ipv4addr = _sockaddr_string(addr);

    return true;
}

bool _bsf_client_sess_ipv6prefix_set_from_sockaddr(bsf_client_sess_t *sess, const ogs_sockaddr_t *addr)
{
    char *ipv6addr;

    if (!sess) return false;
    if (!addr) return false; 
    if (addr->ogs_sa_family != AF_INET6) return false;

    if (sess->ipv6prefix) ogs_free(sess->ipv6prefix);
    ipv6addr = _sockaddr_string(addr);
    sess->ipv6prefix = ogs_msprintf("%s/128", ipv6addr);
    ogs_free(ipv6addr);

    return true;
}

bool _bsf_client_sess_retrieve_callback_set(bsf_client_sess_t *sess, bsf_retrieve_callback_f cb, void *user_data)
{
    if (!sess) return false;
    
    sess->retrieve.callback = cb;
    sess->retrieve.user_data = user_data;

    return true;
}

bool _bsf_client_sess_result_callback_set(bsf_client_sess_t *sess, bsf_retrieve_result_callback_f cb, void *user_data)
{
    if (!sess) return false;

    sess->result.callback = cb;
    sess->result.user_data = user_data;

    return true;
}

/* What the BSF answered: its status, and the cause of the ProblemDetails of an error (TS 29.521 V19.7.0 table 5.3.2.3.2-3) */
void _bsf_client_sess_set_answer(bsf_client_sess_t *sess, int status, const ogs_sbi_response_t *response)
{
    if (!sess) return;
    sess->answer_status = status;
    if (sess->answer_cause) { ogs_free(sess->answer_cause); sess->answer_cause = NULL; }
    if (response && status >= 400 && response->http.content) {
        cJSON *json = cJSON_Parse(response->http.content);
        OpenAPI_problem_details_t *problem = json ? OpenAPI_problem_details_parseFromJSON(json) : NULL;

        if (problem && problem->cause) sess->answer_cause = ogs_strdup(problem->cause);
        if (problem) OpenAPI_problem_details_free(problem);
        if (json) cJSON_Delete(json);
    }
}

bool _bsf_client_sess_retrieve_callback_call(bsf_client_sess_t *sess, const OpenAPI_pcf_binding_t *binding)
{
    OpenAPI_pcf_binding_t *bind_copy = NULL;
    bool ret;

    if (!sess) return false;
    if (sess->result.callback) {
        bsf_pcf_binding_result_t result;

        memset(&result, 0, sizeof(result));
        result.pcf_binding = binding;
        result.http_status = sess->answer_status;
        result.problem_cause = sess->answer_cause;
        ret = sess->result.callback(&result, sess->result.user_data);
        if (!ret) ogs_error("BSF client callback failed");
        return ret;
    }
    if (!sess->retrieve.callback) return false;

    if (binding)
        bind_copy = OpenAPI_pcf_binding_copy(NULL, (OpenAPI_pcf_binding_t*)binding);

    ret = sess->retrieve.callback(bind_copy, sess->retrieve.user_data);
    if (!ret) {
        ogs_error("BSF client callback failed");
        OpenAPI_pcf_binding_free(bind_copy);
    }

    return ret;
}

bool _bsf_client_sess_discover_and_send(bsf_client_sess_t *sess)
{
    int rv;
    ogs_sbi_xact_t *xact;

    if (!sess) return false;

    xact = ogs_sbi_xact_add(OGS_INVALID_POOL_ID, &sess->sbi, OpenAPI_service_name_nbsf_management, NULL, (ogs_sbi_build_f)_nbsf_management_pcf_binding_build, sess, NULL);
    if (!xact) {
        ogs_error("bsf_client_sess_discover_and_send() failed");
        return false;
    }

    ogs_debug("Sending BSF query");
    rv = ogs_sbi_discover_and_send(xact);
    if (rv != OGS_OK) {
        ogs_error("bsf_client_sess_discover_and_send() failed");
        ogs_sbi_xact_remove(xact);
        return false;
    }

    return true;
}

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
