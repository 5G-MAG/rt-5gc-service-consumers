/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2023 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#include "ogs-app.h"
#include "ogs-proto.h"

#include "context.h"
#include "log.h"
#include "priv_mbs-session.h"

#include "local.h"

bool _local_discover_and_send(_priv_mbs_session_t *sess)
{
    int rv;
    mb_smf_client_event_t *ev;

    ev = ogs_event_size(MB_SMF_CLIENT_LOCAL_EVENT, sizeof(*ev));
    ogs_assert(ev);

    ev->id = MB_SMF_CLIENT_LOCAL_DISCOVER_AND_SEND;
    ev->h.sbi.data = sess;

    ogs_debug("Queueing discover & send event (%p)", ev);
    rv = ogs_queue_push(ogs_app()->queue, &ev->h);
    if (rv != OGS_OK) {
        ogs_error("Failed to push discover and send event onto the queue");
        return false;
    }

    /* process the event queue */
    ogs_pollset_notify(ogs_app()->pollset);

    ogs_debug("event queued");

    return true;
}

bool _local_process_event(ogs_event_t *e)
{
    _priv_mbs_session_t *sess;

    if (!e) return false;
    if (e->id != MB_SMF_CLIENT_LOCAL_EVENT) return false;

    sess = (_priv_mbs_session_t*)e->sbi.data;
    if (_context_active_sessions_exists(sess)) {
        mb_smf_client_event_t *mb_smf_event = ogs_container_of(e, mb_smf_client_event_t, h);
        switch (mb_smf_event->id) {
            case MB_SMF_CLIENT_LOCAL_DISCOVER_AND_SEND:
                ogs_debug("Discover & Send event");
                _local_discover_and_send(sess);
                break;
            default:
                break;
        }
        return true;
    }
    return false;
}

const char *_mb_smf_client_local_get_event_name(ogs_event_t *e)
{
    if (e->id < OGS_MAX_NUM_OF_PROTO_EVENT)
        return ogs_event_get_name(e);
    if (e->id == MB_SMF_CLIENT_LOCAL_EVENT) {
        _priv_mbs_session_t *sess;
        sess = (_priv_mbs_session_t*)e->sbi.data;
        if (_context_active_sessions_exists(sess)) {
            mb_smf_client_event_t *mb_smf_event = ogs_container_of(e, mb_smf_client_event_t, h);
            switch (mb_smf_event->id) {
                case MB_SMF_CLIENT_LOCAL_DISCOVER_AND_SEND:
                    return "MB_SMF_CLIENT_LOCAL_DISCOVER_AND_SEND";
                default:
                    break;
            }
            return "Unknown MB_SMF_CLIENT_LOCAL Event";
        }
    }
    return "Unknown Event Type";
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
