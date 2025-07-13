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
#include "ogs-proto.h"
#include "ogs-sbi.h"

/* Library includes */
#include "macros.h"
#include "context.h"
#include "log.h"
#include "nmbsmf-mbs-session-handler.h"

/* Header for this implementation */
#include "mb-smf-service-consumer.h"

// service consumer lifecycle
MB_SMF_CLIENT_API bool mb_smf_sc_parse_config(const char *mb_smf_client_sect)
{
    _context_new();
    _log_init();

    // TODO: parse the configuration sections

    return false;
}

MB_SMF_CLIENT_API void mb_smf_sc_terminate(void)
{
    _context_destroy();
}

MB_SMF_CLIENT_API bool mb_smf_sc_process_event(ogs_event_t *e)
{
    if (!e) return false;

    ogs_pool_id_t xact_id = OGS_POINTER_TO_UINT(e->sbi.data);
    if (xact_id < OGS_MIN_POOL_ID || xact_id > OGS_MAX_POOL_ID) return false;

    ogs_sbi_xact_t *xact = ogs_sbi_xact_find_by_id(xact_id);
    if (!xact) return false;

    _priv_mbs_session_t *sess = _context_sbi_object_to_session(xact->sbi_object);
    if (!sess) return false;

    /* This is one of ours, handle it */

    ogs_debug("Processing event %p [%s] for session [%p (%p)]", e, ogs_event_get_name(e), sess, _priv_mbs_session_to_public(sess));

    switch (e->id) {
    case OGS_EVENT_SBI_CLIENT:
        /* response from MB-SMF */
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
                        ogs_sbi_response_t *response = e->sbi.response;
                        ogs_sbi_message_t message;
                        ogs_debug("Client response for Create MBS Session received");
                        ogs_assert(response);
                        ogs_debug("Parsing response");
                        ogs_sbi_parse_response(&message, response);
                        if (_nmbsmf_mbs_session_parse(&message, sess) == OGS_OK) {
                            if (sess->session.create_result_cb) {
                                ogs_debug("Forwarding creation result to calling application");
                                sess->session.create_result_cb(_priv_mbs_session_to_public(sess), OGS_OK,
                                                               sess->session.create_result_cb_data);
                            }
                        } else {
                            ogs_warn("Errors in response from MB-SMF");
                        }
                        ogs_sbi_response_free(response);
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
        default: /* xact->service_type */
            break;
        }
        break;

    case OGS_EVENT_SBI_TIMER:
        ogs_debug("Timer id = %i (%s)", e->timer_id, ogs_timer_get_name(e->timer_id));
        switch (e->timer_id) {
        case OGS_TIMER_SBI_CLIENT_WAIT:
            /* client call timed out */
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
        break;
    }

    if (xact) ogs_sbi_xact_remove(xact);

    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
