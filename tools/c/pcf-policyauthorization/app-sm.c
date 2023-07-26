/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <signal.h>
#include <unistd.h>

#include "ogs-core.h"
#include "ogs-proto.h"

#include "bsf-service-consumer.h"
#include "pcf-service-consumer.h"

#include "app-log.h"
#include "init.h"

#include "app-sm.h"

typedef enum ogs_event_id_extension_e {
    APP_LOCAL = OGS_MAX_NUM_OF_PROTO_EVENT + 1500
} ogs_event_id_extension_t;

typedef enum app_local_event_id_e {
    APP_LOCAL_EVENT_NONE = 0,

    APP_LOCAL_EVENT_DISCOVERED_PCF,
    APP_LOCAL_EVENT_PCF_DISCOVERY_FAILED,
    APP_LOCAL_EVENT_APP_SESSION_CREATED,
    APP_LOCAL_EVENT_APP_SESSION_DESTROYED,

    APP_LOCAL_EVENT_MAX
} app_local_event_id_t;

typedef struct app_local_event_s {
    ogs_event_t event;

    app_local_event_id_t id;

} app_local_event_t;

static void app_state_discover_pcf(ogs_fsm_t *sm, ogs_event_t *event);
static void app_state_create_app_session(ogs_fsm_t *sm, ogs_event_t *event);
static void app_state_running(ogs_fsm_t *sm, ogs_event_t *event);

static void app_local_send_event(app_local_event_id_t event_id);
static const char *app_event_get_name(ogs_event_t *event);
static const char *app_local_get_name(app_local_event_t *app_event);

void app_state_init(ogs_fsm_t *sm, ogs_event_t *event)
{
    const char *pcf_address;

    ogs_assert(sm);

    pcf_address = pcf_policyauth_get_pcf_address();
    if (!pcf_address) {
        OGS_FSM_TRAN(sm, app_state_discover_pcf);
    } else {
        OGS_FSM_TRAN(sm, app_state_create_app_session);
    }
}

void app_state_final(ogs_fsm_t *sm, ogs_event_t *event)
{
    ogs_assert(sm);
}

void app_local_send_discovered_pcf(void)
{
    app_local_send_event(APP_LOCAL_EVENT_DISCOVERED_PCF);
}

void app_local_send_pcf_discovery_failed(void)
{
    app_local_send_event(APP_LOCAL_EVENT_PCF_DISCOVERY_FAILED);
}

void app_local_send_app_session_created(void)
{
    app_local_send_event(APP_LOCAL_EVENT_APP_SESSION_CREATED);
}

void app_local_send_app_session_destroyed(void)
{
    app_local_send_event(APP_LOCAL_EVENT_APP_SESSION_DESTROYED);
}

static void app_state_discover_pcf(ogs_fsm_t *sm, ogs_event_t *event)
{
    ogs_assert(sm);

    if (bsf_process_event(event))
        return;

    if (pcf_session_process_event(event))
        return;

    ogs_debug("Processing event %p [%s]", event, app_event_get_name(event));

    switch (event->id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("Discovering PCF for UE...");
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case APP_LOCAL:
        {
            app_local_event_t *app_event = ogs_container_of(event, app_local_event_t, event);

            switch (app_event->id) {
            case APP_LOCAL_EVENT_DISCOVERED_PCF:
                ogs_info("Found PCF...");
                OGS_FSM_TRAN(sm, app_state_create_app_session);
                break;
            case APP_LOCAL_EVENT_PCF_DISCOVERY_FAILED:
                ogs_error("PCF Discovery failed, exiting!");
                kill(getpid(), SIGTERM);
                break;
            default:
                ogs_error("Unexpected local event: %s", app_event_get_name(event));
                break;
            }
        }
        break;

    default:
        break;
    }
}

static void app_state_create_app_session(ogs_fsm_t *sm, ogs_event_t *event)
{
    ogs_assert(sm);

    if (bsf_process_event(event))
        return;

    if (pcf_session_process_event(event))
        return;

    ogs_debug("Processing event %p [%s]", event, app_event_get_name(event));

    switch (event->id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("Requesting AppSessionContext...");
        pcf_policyauth_get_app_session_context();
        break;

    case OGS_FSM_EXIT_SIG:
        break;


    case APP_LOCAL:
        {
            app_local_event_t *app_event = ogs_container_of(event, app_local_event_t, event);

            switch (app_event->id) {
            case APP_LOCAL_EVENT_APP_SESSION_CREATED:
                ogs_info("AppSessionContext created...");
                OGS_FSM_TRAN(sm, app_state_running);
                break;

            case APP_LOCAL_EVENT_APP_SESSION_DESTROYED:
                ogs_info("Failed to create AppSessionContext, exiting");
                kill(getpid(), SIGTERM);
                break;

            default:
                ogs_error("Unexpected local event: %s", app_event_get_name(event));
                break;
            }
        }
        break;

    default:
        break;
    }
}

static void app_state_running(ogs_fsm_t *sm, ogs_event_t *event)
{
    ogs_assert(sm);

    if (bsf_process_event(event))
        return;

    if (pcf_session_process_event(event))
        return;

    ogs_debug("Processing event %p [%s]", event, app_event_get_name(event));

    switch (event->id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("Awaiting notifications...");
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case APP_LOCAL:
        {
            app_local_event_t *app_event = ogs_container_of(event, app_local_event_t, event);

            switch (app_event->id) {
            case APP_LOCAL_EVENT_APP_SESSION_DESTROYED:
                ogs_info("AppSessionContext destroyed by PCF, exiting");
                kill(getpid(), SIGTERM);
                break;

            default:
                ogs_error("Unexpected local event: %s", app_event_get_name(event));
                break;
            }
        }
        break;

    default:
        break;
    }
}

static void app_local_send_event(app_local_event_id_t event_id)
{
    int rv;
    app_local_event_t *event;

    event = ogs_calloc(1, sizeof(*event));
    ogs_assert(event);

    event->id = event_id;
    event->event.id = APP_LOCAL;

    rv = ogs_queue_push(ogs_app()->queue, &event->event);
    if (rv != OGS_OK) {
        ogs_error("Failed to push app local event onto the evet queue");
        return;
    }
    /* process the event queue */
    ogs_pollset_notify(ogs_app()->pollset);
}

static const char *app_event_get_name(ogs_event_t *event)
{
    app_local_event_t *app_event;

    if (ogs_unlikely(!event)) return "*** No Event ***";

    if (event->id != APP_LOCAL) return ogs_event_get_name(event);

    app_event = ogs_container_of(event, app_local_event_t, event);

    return app_local_get_name(app_event);
}

static const char *app_local_get_name(app_local_event_t *app_event)
{
    if (ogs_unlikely(!app_event)) return "*** No Event ***";

    switch (app_event->id) {
    case APP_LOCAL_EVENT_DISCOVERED_PCF:
        return "APP_LOCAL_EVENT_DISCOVERED_PCF";
    case APP_LOCAL_EVENT_PCF_DISCOVERY_FAILED:
        return "APP_LOCAL_EVENT_PCF_DISCOVERY_FAILED";
    case APP_LOCAL_EVENT_APP_SESSION_CREATED:
        return "APP_LOCAL_EVENT_APP_SESSION_CREATED";
    case APP_LOCAL_EVENT_APP_SESSION_DESTROYED:
        return "APP_LOCAL_EVENT_APP_SESSION_DESTROYED";
    default:
        break;
    }

    return "APP_LOCAL_EVENT Unknown";
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
