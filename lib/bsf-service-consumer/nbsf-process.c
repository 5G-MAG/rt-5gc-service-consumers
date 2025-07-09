/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2023 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "bsf-client-sess.h"
#include "context.h"
#include "local.h"
#include "log.h"
#include "utils.h"

#include "nbsf-process.h"

#ifdef __cplusplus
extern "C" {
#endif

bool _bsf_process_event(ogs_event_t *e)
{
    /* check if we're ready */
    if (_bsf_client_self() == NULL) return false;

    ogs_debug("_bsf_process_event: %s", _bsf_client_local_get_event_name(e));

    switch (e->id) {
        case OGS_EVENT_SBI_CLIENT:
        {
            int rv;
            ogs_sbi_response_t *response = e->sbi.response;
            ogs_sbi_message_t message;
            ogs_pool_id_t xact_id = OGS_POINTER_TO_UINT(e->sbi.data);
            ogs_sbi_xact_t *xact = ogs_sbi_xact_find_by_id(xact_id);
            bsf_client_sess_t *sess;
 
            if (!xact) return false;

            /* Check if this is one of ours */
            sess = ogs_container_of(xact->sbi_object, bsf_client_sess_t, sbi);
            if (!_bsf_client_context_active_sessions_exists(sess)) return false;

            ogs_assert(response);
            rv = ogs_sbi_parse_header(&message, &response->h);
            if (rv != OGS_OK) {
                ogs_error("Failed to parse response");
                ogs_sbi_message_free(&message);
                break;
            }
            message.res_status = response->status;

            ogs_debug("OGS_EVENT_SBI_CLIENT: service=%s, component[0]=%s", message.h.service.name, message.h.resource.component[0]);

            SWITCH(message.h.service.name)
            CASE(OGS_SBI_SERVICE_NAME_NNRF_DISC)
                SWITCH(message.h.resource.component[0])
                CASE(OGS_SBI_RESOURCE_NAME_NF_INSTANCES)
                    {
                        ogs_debug("Got NF-Instances");
                        _bsf_client_context_log_debug();
                        ogs_debug("bsf_client_sess_t = %p", sess);

                        SWITCH(message.h.method)
                        CASE(OGS_SBI_HTTP_METHOD_GET)
                            if (message.res_status == OGS_SBI_HTTP_STATUS_OK) {
                                char *method = response->h.method; /* save this */
                                ogs_sbi_nf_instance_t *nf_instance;

                                ogs_sbi_message_free(&message);
                                response->h.method = NULL;
                                ogs_sbi_header_free(&response->h);
                                response->h.method = method;

                                rv = ogs_sbi_parse_response(&message, response);
                                if (rv != OGS_OK) {
                                    ogs_error("Failed to parse response");
                                    break;
                                }

                                ogs_nnrf_disc_handle_nf_discover_search_result(message.SearchResult);
                                nf_instance = ogs_sbi_nf_instance_find_by_service_type(xact->service_type, xact->requester_nf_type);
                                ogs_sbi_send_request_to_nf_instance(nf_instance, xact);
                            }
                            break;

                        DEFAULT
                        END

                        ogs_sbi_response_free(response);
                        ogs_sbi_message_free(&message);
                        return true;
                    }
                DEFAULT
                END
                break;
            CASE(OGS_SBI_SERVICE_NAME_NBSF_MANAGEMENT)
                SWITCH(message.h.resource.component[0])
                CASE(OGS_SBI_RESOURCE_NAME_PCF_BINDINGS)
                    {
                        ogs_debug("Got pcfBindings!");
                        _bsf_client_context_log_debug();
                        ogs_debug("bsf_client_sess_t = %p", sess);

                        if (message.res_status == OGS_SBI_HTTP_STATUS_OK) {
                            ogs_time_t expires;
                            char *method = response->h.method; /* save this */

                            ogs_sbi_message_free(&message);
                            response->h.method = NULL;
                            ogs_sbi_header_free(&response->h);
                            response->h.method = method;

                            rv = ogs_sbi_parse_response(&message, response);
                            if (rv != OGS_OK) {
                                ogs_error("Failed to parse response");
                                break;
                            }

                            expires = _response_to_expiry_time(response);
                            _bsf_client_context_add_pcf_binding(sess->ue_address, message.PcfBinding, expires);
                                
                            if (!_bsf_client_sess_retrieve_callback_call(sess, message.PcfBinding)) {
                                ogs_error("_bsf_client_sess_retrieve_callback_call() failed");
                            }
                        } else {
                            char *ip = ogs_ipstrdup(sess->ue_address);
                            ogs_error("Unable to find PCF binding for %s", ip);
                            _bsf_client_sess_retrieve_callback_call(sess, NULL);
                            ogs_free(ip);
                        }
                        ogs_sbi_xact_remove(xact);
                        ogs_sbi_response_free(response);
                        _bsf_client_sess_free(sess);
                        ogs_sbi_message_free(&message);
                        ogs_debug("taking event for OGS_EVENT_SBI_CLIENT");
                        return true;
                    }
                    break;
                DEFAULT
                END
                break;
            DEFAULT
            END

            ogs_sbi_message_free(&message);
            /* ogs_sbi_parse_response leaves allocated strings in the response->h that we need to free */
            {
                char *method = response->h.method; /* save this */
                response->h.method = NULL;
                ogs_sbi_header_free(&response->h);
                response->h.method = method;
            }
            ogs_debug("end OGS_EVENT_SBI_CLIENT");

            break;
        }    
        case OGS_EVENT_SBI_TIMER:
            ogs_assert(e);

            ogs_debug("Got timer event [%s]", ogs_timer_get_name(e->timer_id));
            switch(e->timer_id) {
            case OGS_TIMER_SBI_CLIENT_WAIT:
            {
                /* NRF connection may have timed out */
                ogs_pool_id_t xact_id = OGS_POINTER_TO_UINT(e->sbi.data);
                ogs_sbi_xact_t *xact = ogs_sbi_xact_find_by_id(xact_id);
                bsf_client_sess_t *sess;
                char *ip;

                if (!xact) return false;

                /* Check if this is one of ours */
                sess = ogs_container_of(xact->sbi_object, bsf_client_sess_t, sbi);
                if (!_bsf_client_context_active_sessions_exists(sess)) return false;

                /* inform the callback of the error */
                ip = ogs_ipstrdup(sess->ue_address);
                ogs_error("Timed out trying to find PCF binding for %s", ip);
                _bsf_client_sess_retrieve_callback_call(sess, NULL);
                ogs_free(ip);

                /* destroy this transaction and BSF session */
                ogs_sbi_xact_remove(xact);
                _bsf_client_sess_free(sess);

                return true;
            }
            default:
                break;
            }
            break;
        case BSF_CLIENT_LOCAL_EVENT:
            return _bsf_client_local_process_event(e);
        default:
            break;
    }

    return false;    
}

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
