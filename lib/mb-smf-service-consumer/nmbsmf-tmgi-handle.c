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

#include "context.h"
#include "priv_tmgi.h"

#include "nmbsmf-tmgi-handle.h"

/* Library Internals */
int _nmbsmf_tmgi_allocated(ogs_sbi_xact_t *xact, ogs_sbi_message_t *message)
{
    if (message->res_status >= 200 && message->res_status < 300) {
        if (!message->TmgiAllocated) {
            ogs_error("Expected TmgiAllocated structure in response not found");
            return OGS_ERROR;
        }

        if (!message->TmgiAllocated->tmgi_list || message->TmgiAllocated->tmgi_list->count < 1) {
            ogs_error("Missing or empty tmgiList in TmgiAllocated response");
            return OGS_ERROR;
        }

        time_t expiry_time = 0;
        if (message->TmgiAllocated->expiration_time) {
            ogs_time_t expiry_ogs_time;
            if (!ogs_sbi_time_from_string(&expiry_ogs_time, message->TmgiAllocated->expiration_time)) {
                ogs_warn("TmgiAllocated.expirationTime format not understood: %s", message->TmgiAllocated->expiration_time);
            } else {
                expiry_time = ogs_time_sec(expiry_ogs_time);
            }
        }

        OpenAPI_lnode_t *node;
        OpenAPI_list_for_each(message->TmgiAllocated->tmgi_list, node) {
            OpenAPI_tmgi_t *api_tmgi = (OpenAPI_tmgi_t*)node->data;
            if (!api_tmgi) {
                ogs_error("TmgiAllocated.tmgiList contains null entry");
                return OGS_ERROR;
            }
            _priv_tmgi_t *tmgi = _tmgi_find_matching_openapi_type(api_tmgi);
            if (!tmgi) {
                char *api_tmgi_str = ogs_msprintf("%s %s", api_tmgi->plmn_id->mcc, api_tmgi->plmn_id->mnc);
                if (api_tmgi->mbs_service_id) {
                    api_tmgi_str = ogs_mstrcatf(api_tmgi_str, ", mbs-service-id = %s", api_tmgi->mbs_service_id);
                }
                ogs_warn("Could not find new TMGI or existing TMGI matching %s", api_tmgi_str);
                ogs_free(api_tmgi_str);
                continue;
            }
            tmgi->tmgi.expiry_time = expiry_time;
            if (tmgi->callback) {
                tmgi->callback(_priv_tmgi_to_public(tmgi), OGS_OK, NULL, tmgi->callback_data);
            }
        }
    } else {
        /* error response:
         *  - use callbacks to inform app about error
         *    - how do I find the lists of tmgis from the request? xact->id?
         */
    }

    return OGS_OK;
}

int _nmbsmf_tmgi_deallocated(ogs_sbi_xact_t *xact, ogs_sbi_message_t *message)
{
    const char *tmgi_list_param = ogs_hash_get(xact->request->http.params, "tmgi-list", OGS_HASH_KEY_STRING);
    if (tmgi_list_param) {
        cJSON *json = cJSON_Parse(tmgi_list_param);
        if (json) {
            if (cJSON_IsArray(json)) {
                cJSON *elem;
                cJSON_ArrayForEach(elem, json) {
                    OpenAPI_tmgi_t *api_tmgi = OpenAPI_tmgi_parseFromJSON(elem);
                    _priv_tmgi_t *tmgi = _tmgi_find_matching_openapi_type(api_tmgi);
                    if (tmgi) {
                        if (message->res_status >= 200 && message->res_status < 300) {
                            if (tmgi->callback) tmgi->callback(_priv_tmgi_to_public(tmgi), OGS_DONE, NULL, tmgi->callback_data);
                        } else {
                            if (tmgi->callback) tmgi->callback(_priv_tmgi_to_public(tmgi), OGS_ERROR, message->ProblemDetails, tmgi->callback_data);
                        }
                        _context_remove_tmgi(tmgi);
                    }
                }
            } else if (cJSON_IsObject(json)) {
                OpenAPI_tmgi_t *api_tmgi = OpenAPI_tmgi_parseFromJSON(json);
                _priv_tmgi_t *tmgi = _tmgi_find_matching_openapi_type(api_tmgi);
                if (tmgi) {
                    if (message->res_status >= 200 && message->res_status < 300) {
                        if (tmgi->callback) tmgi->callback(_priv_tmgi_to_public(tmgi), OGS_DONE, NULL, tmgi->callback_data);
                    } else {
                        if (tmgi->callback) tmgi->callback(_priv_tmgi_to_public(tmgi), OGS_ERROR, message->ProblemDetails, tmgi->callback_data);
                    }
                    _context_remove_tmgi(tmgi);
                }
            }
        }
    }
    return OGS_OK;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
