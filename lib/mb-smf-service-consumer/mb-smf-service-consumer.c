/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

/* Open5GS includes */
#include "ogs-core.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"
#include "openapi/model/status_notify_req_data.h"

/* Library includes */
#include "macros.h"
#include "context.h"
#include "log.h"
#include "nmbsmf-mbs-session-build.h"
#include "nmbsmf-mbs-session-handler.h"

/* Header for this implementation */
#include "mb-smf-service-consumer.h"

static int __upgrade_to_full_response_parse(ogs_sbi_message_t *message, ogs_sbi_response_t *response);

// service consumer lifecycle
MB_SMF_CLIENT_API bool mb_smf_sc_parse_config(const char *mb_smf_client_sect)
{
    _context_new();
    _log_init();

    _context_parse_config(mb_smf_client_sect);

    return false;
}

MB_SMF_CLIENT_API void mb_smf_sc_terminate(void)
{
    _tidy_fixed_notification_server();
    _context_destroy();
}

MB_SMF_CLIENT_API bool mb_smf_sc_process_event(ogs_event_t *e)
{
    ogs_sbi_xact_t *xact = NULL;
    ogs_sbi_response_t *response = NULL;

    if (!e) return false;

    ogs_debug("Processing event %p [%s]", e, ogs_event_get_name(e));

    switch (e->id) {
    case OGS_EVENT_SBI_SERVER:
        /* possible notification */
        ogs_sbi_request_t *request = e->sbi.request;
        if (!request) return false;

        ogs_pool_id_t stream_id = OGS_POINTER_TO_UINT(e->sbi.data);
        ogs_sbi_stream_t *stream = ogs_sbi_stream_find_by_id(stream_id);
        if (!stream) return false;

        ogs_sbi_server_t *server = ogs_sbi_server_from_stream(stream);
        if (!server) return false;

        if (!_context_is_notification_server(server)) return false;

        ogs_sbi_message_t message;
        if (ogs_sbi_parse_header(&message, &request->h) != OGS_OK) return false;

        if (!message.http.content_type || strcmp(message.http.content_type, OGS_SBI_CONTENT_JSON_TYPE)) {
            ogs_warn("Received notification from MB-SMF of wrong Content-Type: %s", message.http.content_type);
            ogs_sbi_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, &message, "Bad Request", "Unsupported Content-Type", "INVALID_MSG_FORMAT");
            break;
        }

        if (!message.h.method || strcmp(message.h.method, OGS_SBI_HTTP_METHOD_POST)) {
            ogs_warn("Received notification from MB-SMF with wrong HTTP method: %s", message.h.method);
            ogs_sbi_server_send_error(stream, OGS_SBI_HTTP_STATUS_METHOD_NOT_ALLOWED, &message, "Method Not Allowed", "Unsupported HTTP method", NULL);
            break;
        }

        cJSON *json = cJSON_Parse(request->http.content);
        if (!json) {
            ogs_warn("Received notification from MB-SMF with malformed body");
            ogs_debug("malformed body:\n%s", request->http.content);
            ogs_sbi_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, &message, "Bad Request", "Unsupported Content-Type", "INVALID_MSG_FORMAT");
            break;
        }

        OpenAPI_status_notify_req_data_t *notif_req = OpenAPI_status_notify_req_data_parseFromJSON(json);
        if (!notif_req) {
            ogs_warn("Received notification from MB-SMF with malformed body");
            ogs_debug("JSON not StatusNotifyReqData:\n%s", request->http.content);
            ogs_sbi_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, &message, "Bad Request", "Unsupported Content-Type", "INVALID_MSG_FORMAT");
            break;
        }

        _priv_mbs_status_subscription_t *subsc = _context_find_subscription(server, request->h.uri);
        _nmbsmf_mbs_session_subscription_report_list_handler(subsc, notif_req->event_list->event_report_list);

        response = ogs_sbi_build_response(&message, OGS_SBI_HTTP_STATUS_OK);
        ogs_sbi_server_send_response(stream, response);

        ogs_sbi_message_free(&message);

        break;
    case OGS_EVENT_SBI_CLIENT:
        response = e->sbi.response;

        ogs_pool_id_t xact_id = OGS_POINTER_TO_UINT(e->sbi.data);
        if (xact_id < OGS_MIN_POOL_ID || xact_id > OGS_MAX_POOL_ID) return false;

        xact = ogs_sbi_xact_find_by_id(xact_id);
        if (!xact) return false;

        _priv_mbs_session_t *sess = _context_sbi_object_to_session(xact->sbi_object);
        if (!sess) return false;

        /* This is one of ours, handle it */

        ogs_debug("Client response for session [%p (%p)]", sess, _priv_mbs_session_to_public(sess));

        switch (xact->service_type) {
        case OGS_SBI_SERVICE_TYPE_NMBSMF_MBS_SESSION:
            /* response to MB-SMF MBS SESSION API request or discovery */
            ogs_sbi_message_t message = {};
            if (ogs_sbi_parse_header(&message, &response->h) != OGS_OK) {
                ogs_error("Failed to parse header from client response for MB-SMF MBS Session transaction");
                if (sess->session.create_result_cb) {
                    sess->session.create_result_cb(_priv_mbs_session_to_public(sess), OGS_ERROR,
                                                   sess->session.create_result_cb_data);
                }
                ogs_sbi_message_free(&message);
                break;
            }
            SWITCH(message.h.service.name)
            CASE(OGS_SBI_SERVICE_NAME_NNRF_DISC)
                /* Process NRF discovery and forward transaction to found nf_instance */
                ogs_debug("NRF Discovery response for MB-SMF MBS Session API");
                SWITCH(message.h.resource.component[0])
                CASE(OGS_SBI_RESOURCE_NAME_NF_INSTANCES)
                    ogs_debug("NRF NF Instance API");
                    SWITCH(message.h.method)
                    CASE(OGS_SBI_HTTP_METHOD_GET)
                        ogs_debug("NRF NF Instance GET response");
                        __upgrade_to_full_response_parse(&message, response);
                        ogs_sbi_object_t *object = xact->sbi_object;
                        OpenAPI_nf_type_e target_nf_type = ogs_sbi_service_type_to_nf_type(xact->service_type);
                        OpenAPI_nf_type_e requester_nf_type = xact->requester_nf_type;
                        ogs_sbi_discovery_option_t *discovery_option = xact->discovery_option;
                        OpenAPI_search_result_t *SearchResult = message.SearchResult;
                        if (!SearchResult) {
                            ogs_error("No SearchResult");
                            break;
                        }
                        ogs_nnrf_disc_handle_nf_discover_search_result(SearchResult);
                        ogs_sbi_nf_instance_t *nf_instance = ogs_sbi_nf_instance_find_by_discovery_param(
                                                                        target_nf_type, requester_nf_type, discovery_option);
                        if (!nf_instance) {
                            ogs_error("(NF discover) No [%s:%s]", ogs_sbi_service_type_to_name(xact->service_type),
                                          OpenAPI_nf_type_ToString(requester_nf_type));
                            break;
                        }
                        OGS_SBI_SETUP_NF_INSTANCE(object->service_type_array[xact->service_type], nf_instance);
                        ogs_debug("Sending transaction to SMF instance");
                        ogs_expect(true == ogs_sbi_send_request_to_nf_instance(nf_instance, xact));
                        xact = NULL; /* preserve xact for next request */
                        break;
                    DEFAULT
                        break;
                    END
                    break;
                DEFAULT
                    break;
                END
                break;
            CASE(OGS_SBI_SERVICE_NAME_NMBSMF_MBS_SESSION)
                /* process response to MBS Session request */
                SWITCH(xact->request->h.resource.component[0])
                CASE(OGS_SBI_RESOURCE_NAME_MBS_SESSIONS)
                    SWITCH(xact->request->h.resource.component[1])
                    CASE("OGS_SWITCH_NULL")
                        /* .../mbs-sessions */
                        SWITCH(xact->request->h.method)
                        CASE(OGS_SBI_HTTP_METHOD_POST)
                            /* Create MBS Session */
                            if (response->status >= 200 && response->status < 300) {
                                ogs_debug("Client response for Create MBS Session received");
                                __upgrade_to_full_response_parse(&message, response);
                                if (_nmbsmf_mbs_session_parse(&message, sess) == OGS_OK) {
                                    if (sess->session.create_result_cb) {
                                        ogs_debug("Forwarding creation result to calling application");
                                        sess->session.create_result_cb(_priv_mbs_session_to_public(sess), OGS_OK,
                                                                       sess->session.create_result_cb_data);
                                    }
                                    /* push pending changes (further subscriptions, etc) */
                                    _mbs_session_push_changes(sess);
                                } else {
                                    ogs_warn("Errors in response from MB-SMF");
                                    if (sess->session.create_result_cb) {
                                        ogs_debug("Forwarding creation failure to calling application");
                                        sess->session.create_result_cb(_priv_mbs_session_to_public(sess), OGS_ERROR,
                                                                       sess->session.create_result_cb_data);
                                    }
                                }
                            } else {
                                ogs_error("MB-SMF responded with a %i status code", response->status);
                                if (sess->session.create_result_cb) {
                                    ogs_debug("Forwarding creation error to calling application");
                                    sess->session.create_result_cb(_priv_mbs_session_to_public(sess), OGS_ERROR,
                                                                   sess->session.create_result_cb_data);
                                }
                            }
                            break;
                        DEFAULT
                            break;
                        END
                        break;
                    CASE(OGS_SBI_RESOURCE_NAME_SUBSCRIPTIONS)
                        SWITCH(xact->request->h.resource.component[2])
                        CASE("OGS_SWITCH_NULL")
                            /* .../mbs-sessions/subscriptions */
                            SWITCH(xact->request->h.method)
                            CASE(OGS_SBI_HTTP_METHOD_POST)
                                if (response->status >= 200 && response->status < 300) {
                                    /* TODO: process the response to a create MBS Status Subscription request */
                                    ogs_warn("TODO: react to the OK MBS Status Subscription create response");
                                } else {
                                    /* TODO: Process unsuccessful MBS Status Subscription join */
                                    ogs_warn("TODO: Process unsuccessful MBS Status Subscription create");
                                }
                                break;
                            DEFAULT
                                break;
                            END
                            break;
                        DEFAULT
                            /* .../mbs-sessions/subscriptions/... */
                            break;
                        END
                        break;
                    DEFAULT
                        /* .../mbs-sessions/... */
                        break;
                    END
                    break;
                DEFAULT
                    /* .../... */
                    break;
                END
                break;
            DEFAULT
                /* unknown service */
                break;
            END
            ogs_sbi_message_free(&message);
            break;
        default: /* xact->service_type */
            break;
        }

        if (response) ogs_sbi_response_free(response);

        break;

    case OGS_EVENT_SBI_TIMER:
        ogs_debug("Timer id = %i (%s)", e->timer_id, ogs_timer_get_name(e->timer_id));

        switch (e->timer_id) {
        case OGS_TIMER_SBI_CLIENT_WAIT:
            /* client call timed out */
            ogs_pool_id_t xact_id = OGS_POINTER_TO_UINT(e->sbi.data);
            if (xact_id < OGS_MIN_POOL_ID || xact_id > OGS_MAX_POOL_ID) return false;

            xact = ogs_sbi_xact_find_by_id(xact_id);
            if (!xact) return false;

            _priv_mbs_session_t *sess = _context_sbi_object_to_session(xact->sbi_object);
            if (!sess) return false;

            ogs_debug("Client response timeout for MBS Session [%p (%p)]", sess, _priv_mbs_session_to_public(sess));

            switch (xact->service_type) {
            case OGS_SBI_SERVICE_TYPE_NMBSMF_MBS_SESSION:
                SWITCH(xact->request->h.resource.component[0])
                CASE(OGS_SBI_RESOURCE_NAME_MBS_SESSIONS)
                    SWITCH(xact->request->h.resource.component[1])
                    CASE("OGS_SWITCH_NULL")
                        /* .../mbs-sessions */
                        SWITCH(xact->request->h.method)
                        CASE(OGS_SBI_HTTP_METHOD_POST)
                            /* Create MBS Session */
                            ogs_warn("Create MBS Session request timed out");
                            if (sess->session.create_result_cb) {
                                ogs_debug("Calling application callback for MBS Session creation with ETIMEDOUT");
                                sess->session.create_result_cb(_priv_mbs_session_to_public(sess), OGS_ETIMEDOUT,
                                                               sess->session.create_result_cb_data);
                            }
                            break;
                        DEFAULT
                            break;
                        END
                        break;
                    DEFAULT
                        break;
                    END
                    break;
                DEFAULT
                    break;
                END
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        break;
    default:
        return false;
    }

    if (xact) ogs_sbi_xact_remove(xact);

    return true;
}

/* Private functions */

static int __upgrade_to_full_response_parse(ogs_sbi_message_t *message, ogs_sbi_response_t *response)
{
    char *method = response->h.method;
    ogs_sbi_message_free(message);
    response->h.method = NULL;
    ogs_sbi_header_free(&response->h);
    response->h.method = method;
    return ogs_sbi_parse_response(message, response);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
