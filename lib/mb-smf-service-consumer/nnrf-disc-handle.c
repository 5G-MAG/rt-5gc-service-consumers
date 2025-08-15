/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-sbi.h"

#include "nnrf-disc-handle.h"

int _nnrf_disc_handle(ogs_sbi_xact_t *xact, ogs_sbi_message_t *message, ogs_sbi_response_t *response)
{
    int ret = OGS_ERROR;
    ogs_debug("NRF Discovery response for MB-SMF API");
    SWITCH(message->h.resource.component[0])
    CASE(OGS_SBI_RESOURCE_NAME_NF_INSTANCES)
        ogs_debug("NRF NF Instance API");
        SWITCH(message->h.method)
        CASE(OGS_SBI_HTTP_METHOD_GET)
            ogs_debug("NRF NF Instance GET response");
            OpenAPI_search_result_t *SearchResult = message->SearchResult;
            if (!SearchResult) {
                char *method = response->h.method;
                ogs_sbi_message_free(message);
                response->h.method = NULL;
                ogs_sbi_header_free(&response->h);
                response->h.method = method;
                ogs_sbi_parse_response(message, response);
                SearchResult = message->SearchResult;
            }
            if (!SearchResult) {
                ogs_error("No SearchResult");
                break;
            }
            ogs_sbi_object_t *object = xact->sbi_object;
            OpenAPI_nf_type_e target_nf_type = ogs_sbi_service_type_to_nf_type(xact->service_type);
            OpenAPI_nf_type_e requester_nf_type = xact->requester_nf_type;
            ogs_sbi_discovery_option_t *discovery_option = xact->discovery_option;
            ogs_nnrf_disc_handle_nf_discover_search_result(SearchResult);
            ogs_sbi_nf_instance_t *nf_instance = ogs_sbi_nf_instance_find_by_discovery_param(target_nf_type, requester_nf_type,
                                                                                             discovery_option);
            if (!nf_instance) {
                ogs_error("(NF discover) No [%s:%s]", ogs_sbi_service_type_to_name(xact->service_type),
                          OpenAPI_nf_type_ToString(requester_nf_type));
                break;
            }
            OGS_SBI_SETUP_NF_INSTANCE(object->service_type_array[xact->service_type], nf_instance);
            ogs_debug("Sending transaction to MB-SMF instance");
            ogs_expect(true == ogs_sbi_send_request_to_nf_instance(nf_instance, xact));
            ret = OGS_OK;
            break;
        DEFAULT
            break;
        END
    DEFAULT
        break;
    END

    return ret;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
