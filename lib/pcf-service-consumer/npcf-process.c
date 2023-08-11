/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2023 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "pcf-client-sess.h"
#include "pcf-client.h"
#include "context.h"
#include "utils.h"

#include "npcf-process.h"
#include "pcf-handler.h"


#ifdef __cplusplus
extern "C" {
#endif

static const char *_event_get_name(ogs_event_t *e);

bool _pcf_process_event(ogs_event_t *e)
{

    /* check if we're ready */
    if (pcf_self() == NULL) return false;

    ogs_debug("_pcf_process_event: %s", _event_get_name(e));

    switch (e->id) {
        case OGS_EVENT_SBI_SERVER:
            {
                int rv;
                ogs_sbi_request_t *request = e->sbi.request;
                ogs_sbi_message_t message;
                ogs_sbi_stream_t *stream = e->sbi.data;
                ogs_sbi_server_t *server;
                pcf_session_t *pcf_session;

                ogs_assert(request);
                ogs_assert(stream);

                server = ogs_sbi_server_from_stream(stream);
                ogs_assert(server);

                /* Check this event is from one of our notification servers */
                rv = 0;
                ogs_list_for_each(&pcf_self()->pcf_sessions, pcf_session) {
                    if (_pcf_session_has_notification_server(pcf_session, server)) {
                        rv = 1;
                        break;
                    }
                }
                if (!rv) {
                    /* This didn't come in on one of our notification servers so ignore it */
                    return false;
                }

                rv = ogs_sbi_parse_header(&message, &request->h);
                if (rv != OGS_OK) {
                    ogs_error("Failed to parse request headers");
                    ogs_sbi_message_free(&message);
                    break;
                }

                ogs_debug("OGS_EVENT_SBI_SERVER: service=%s, component[0]=%s", message.h.service.name, message.h.resource.component[0]);
                if (message.h.service.name) {
                    SWITCH(message.h.service.name)
                    CASE(OGS_SBI_SERVICE_NAME_NPCF_POLICYAUTHORIZATION)
                        if (message.h.resource.component[0]) {
                            SWITCH(message.h.resource.component[0])
                            CASE("app-session-instance")
                                if (message.h.resource.component[1]) {
                                    char *endptr = NULL;
                                    uintptr_t instance_id;

                                    instance_id = (uintptr_t)strtoull(message.h.resource.component[1], &endptr, 0);
                                    if (endptr && *endptr == '\0') {
                                        pcf_app_session_t *app_session = (pcf_app_session_t*)instance_id;
                                        /* check if app_session is a valid pcf_app_session_t pointer by checking registered ones */
                                        if (_pcf_app_session_exists(app_session)) {
                                            if (message.h.resource.component[2] &&
                                                !strcmp(message.h.resource.component[2], "notify")) {
                                                /* process notifications */
                                                OpenAPI_events_notification_t *notifications;
                                                cJSON *json;

                                                json = cJSON_Parse(request->http.content);
                                                if (!json) {
                                                    /* send error response */
                                                    static const char *error = "Bad JSON in request body";
                                                    ogs_error("%s", error);
                                                    ogs_assert(true == ogs_sbi_server_send_error(stream,
                                                                OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                                                                &message, "Bad content", error));
                                                    break;
                                                }
                                                notifications = OpenAPI_events_notification_parseFromJSON(json);
                                                if (!notifications) {
                                                    /* send error response */
                                                    static const char *error = "Unable to parse EventsNotification body";
                                                    ogs_error("%s", error);
                                                    ogs_assert(true == ogs_sbi_server_send_error(stream,
                                                                OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                                                                &message, "Bad content", error));
                                                } else {
                                                    ogs_sbi_response_t *response;

                                                    ogs_debug("Sending PCF notification to callback");

                                                    /* do notification callbacks */
                                                    _pcf_app_session_notifications_callback_call(app_session, notifications);

                                                    /* send OK response */
                                                    response = ogs_sbi_build_response(&message, OGS_SBI_HTTP_STATUS_NO_CONTENT);
                                                    ogs_assert(response);
                                                    ogs_assert(true == ogs_sbi_server_send_response(stream, response));
                                                }
                                                cJSON_Delete(json);
                                            } else {
                                                char *err = ogs_msprintf("Unknown notification operation \"%s\"",
                                                                         message.h.resource.component[2]);
                                                ogs_error("%s", err);
                                                ogs_assert(true == ogs_sbi_server_send_error(stream,
                                                           OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                                                           &message, "Bad content", err));
                                                ogs_free(err);
                                            }
                                        } else {
                                            char *err = ogs_msprintf("Session instance %p does not exist", app_session);
                                            ogs_warn("%s", err);
                                            ogs_assert(true == ogs_sbi_server_send_error(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND,
                                                       &message, "Not found", err));
                                            ogs_free(err);
                                        }
                                    } else {
                                        char *err = ogs_msprintf("Bad session instance id \"%s\"", message.h.resource.component[1]);
                                        ogs_error("%s", err);
                                        ogs_assert(true == ogs_sbi_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                                                   &message, "Bad request", err));
                                        ogs_free(err);
                                    }
                                } else {
                                    static const char *err = "App session id missing from PCF notification";
                                    ogs_error("%s", err);
                                    ogs_assert(true == ogs_sbi_server_send_error(stream,
                                               OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                                               &message, "Missing path value", err));
                                }
                                break;
                            DEFAULT
                                char *err = ogs_msprintf("Unknown object type \"%s\" in PCF notification",
                                                         message.h.resource.component[0]);
                                ogs_error("%s", err);
                                ogs_assert(true == ogs_sbi_server_send_error(stream,
                                           OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                                           &message, "Bad request", err));
                                ogs_free(err);
                            END
                        } else {
                            static const char *err = "Object type missing from PCF notification";
                            ogs_error("%s", err);
                            ogs_assert(true == ogs_sbi_server_send_error(stream,
                                       OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                                       &message, "Missing path value", err));
                        }
                        break;
                    DEFAULT
                        char *err = ogs_msprintf("Unknown service name \"%s\" in PCF notification", message.h.service.name);
                        ogs_error("%s", err);
                        ogs_assert(true == ogs_sbi_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                                   &message, "Bad request", err));
                        ogs_free(err);
                    END
                } else {
                    static const char *err = "Missing service name from URL path";
                    ogs_error("%s", err);
                    ogs_assert(true == ogs_sbi_server_send_error(stream,
                               OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                               &message, "Missing service name", err));
                }
                ogs_sbi_message_free(&message);
                ogs_sbi_request_free(request);
                return true;
            }
            break;
        case OGS_EVENT_SBI_CLIENT:
        {
            int rv;
            pcf_app_session_t *sess = e->sbi.data;
            ogs_sbi_response_t *response = e->sbi.response;
            ogs_sbi_message_t message;

            if (!_pcf_app_session_exists(sess)) {
                /* not one of our client messages, ignore */
                return false;
            }

            switch (e->sbi.state) {
            case OGS_OK:
                /* normal operation - deal with the client response */
                ogs_assert(response);
                rv = ogs_sbi_parse_response(&message, response);
                if (rv != OGS_OK) {
                    ogs_error("Failed to parse response");
                    ogs_sbi_message_free(&message);
                    break;
                }

                ogs_debug("OGS_EVENT_SBI_CLIENT: service=%s, component[0]=%s", message.h.service.name, message.h.resource.component[0]);

                SWITCH(message.h.service.name)
                CASE(OGS_SBI_SERVICE_NAME_NPCF_POLICYAUTHORIZATION)
                    SWITCH(message.h.resource.component[0])
                    CASE(OGS_SBI_RESOURCE_NAME_APP_SESSIONS)
                        if (message.h.resource.component[1]) {
                            /* URL: .../app-session/<id>... */
                            sess = e->sbi.data;
                            ogs_assert(sess);
                            if (message.h.resource.component[2]) {
                                /* URL: .../app-session/<id>/<subresource> */
                                SWITCH(message.h.resource.component[2])
                                CASE("events-subscription")
	    		            pcf_policyauthorization_subscribe_event(sess, &message, response);
                                    break;
		                		    
                                CASE(OGS_SBI_RESOURCE_NAME_DELETE)
			            pcf_policyauthorization_delete(sess);
                                    break;

                                DEFAULT
                                    ogs_error("Unknown AppSessionContext sub-resource [%s]", message.h.resource.component[2]);
                                END
                            } else {
                                /* URL: .../app-session/<id> */
                                SWITCH(message.h.method)
                                CASE(OGS_SBI_HTTP_METHOD_PATCH)
                                    if (message.res_status == OGS_SBI_HTTP_STATUS_OK) {
                                        pcf_policyauthorization_update(sess, &message);
				    } else {
                                        ogs_error("HTTP response error [%d]", message.res_status);
                                    }
                                    break;
                                DEFAULT
                                    ogs_error("Unknown HTTP method [%s]", message.h.method);
                                END
                            }
                        } else {
                            SWITCH(message.h.method)
                            CASE(OGS_SBI_HTTP_METHOD_POST)
                                ogs_debug("POST response: status = %i", message.res_status);
                                if (message.res_status == OGS_SBI_HTTP_STATUS_CREATED) {
                                    ogs_debug("message.res_status == OGS_SBI_HTTP_STATUS_CREATED");
                                    pcf_policyauthorization_create(sess, &message, response);
                                    ogs_debug("taking event for OGS_EVENT_SBI_CLIENT");
                                } else if (message.res_status == OGS_SBI_HTTP_STATUS_NOT_FOUND) {
                                    ogs_error("UE address not known by the PCF");
                                    if (sess) pcf_app_sess_remove(sess);
                                } else {
                                    ogs_debug("Unknown response code %i when creating an AppSessionContext", message.res_status);
                                    if (sess) pcf_app_sess_remove(sess);
                                }
                                break;
                            DEFAULT
                                ogs_debug("Unknown method [%s] for .../" OGS_SBI_RESOURCE_NAME_APP_SESSIONS, message.h.method);
                            END
                        }
                        break;
                    DEFAULT
                        ogs_error("Unknown PCF Policy Authorization resource [%s]", message.h.resource.component[0]);
                        break;
                    END
                    break;

                DEFAULT
                    ogs_error("Unknown SBI service [%s]", message.h.service.name);
                    break;
                END

                ogs_sbi_message_free(&message);
                ogs_debug("end OGS_EVENT_SBI_CLIENT");
                break;
            case OGS_DONE:
                /* App shutting down - tidy up */
                ogs_debug("App shutting down client request aborted - destroying app session");
                if (sess) pcf_app_sess_remove(sess);
                break;
            default:
                /* Some other error happened - client request failed */
                if (response) {
                    ogs_error("Problem with %s %s request", response->h.method, response->h.uri);
                } else {
                    ogs_error("Problem with an HTTP request, no details given");
                }
                if (sess) pcf_app_sess_remove(sess);
                break;
            }

            if (response) ogs_sbi_response_free(response);
            return true;
        }     
        default:
            break;
    }

    return false;    
}

static const char *_event_get_name(ogs_event_t *e)
{
    if (e->id < OGS_MAX_NUM_OF_PROTO_EVENT)
        return ogs_event_get_name(e);
    return "Unknown Event Type";
}

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
