/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2022 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#include "ogs-sbi.h"

#include "bsf-client-sess.h"

#include "nbsf-management-build.h"

#ifdef __cplusplus
extern "C" {
#endif

ogs_sbi_request_t *_nbsf_management_pcf_binding_build(bsf_client_sess_t *sess, void *data)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request;

    ogs_assert(sess);

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_GET;
    message.h.service.name = (char *)OpenAPI_service_name_ToString(OpenAPI_service_name_nbsf_management);
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] =
        (char *)OGS_SBI_RESOURCE_NAME_PCF_BINDINGS;

    message.param.ipv4addr = sess->ipv4addr;
    message.param.ipv6prefix = sess->ipv6prefix;
    message.param.dnn = sess->dnn;
    if (sess->s_nssai_presence) {
        message.param.snssai_presence = true;
        message.param.s_nssai = sess->s_nssai;
    }
    message.param.bsf_supi = sess->supi;
    message.param.bsf_gpsi = sess->gpsi;
    message.param.ip_domain = sess->ip_domain;

    request = ogs_sbi_build_request(&message);
    ogs_expect(request);

    return request;
}

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
