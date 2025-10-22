/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring <david.waring2@bbc.co.uk>
 * Copyright: (C) 2025 British Broadcasting Corporation
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include "ogs-app.h"
#include "ogs-core.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "mb-smf-service-consumer.h"

#include "app-log.h"
#include "init.h"
#include "options.h"
#include "utils.h"

#include "app-sm.h"

typedef enum ogs_event_id_extension_e {
    APP_LOCAL = OGS_MAX_NUM_OF_PROTO_EVENT + 1500
} ogs_event_id_extension_t;

typedef enum app_local_event_id_e {
    APP_LOCAL_EVENT_NONE = 0,

    APP_LOCAL_EVENT_TMGI_ALLOCATE,
    APP_LOCAL_EVENT_TMGI_ALLOCATED,
    APP_LOCAL_EVENT_TMGI_DEALLOCATE,
    APP_LOCAL_EVENT_TMGI_DEALLOCATED,

    APP_LOCAL_EVENT_MAX
} app_local_event_id_t;

typedef struct app_local_event_s {
    ogs_event_t event;

    app_local_event_id_t id;

    mb_smf_sc_tmgi_t *tmgi;
    int result;
    OpenAPI_problem_details_t *problem_details;

} app_local_event_t;

static void app_state_create_tmgi(ogs_fsm_t *sm, ogs_event_t *event);
static void app_state_delete_tmgi(ogs_fsm_t *sm, ogs_event_t *event);
static void app_state_exit(ogs_fsm_t *sm, ogs_event_t *event);

static void app_local_send_tmgi_allocate();
static void app_local_send_tmgi_allocated(mb_smf_sc_tmgi_t *tmgi, int result,
                                                     const OpenAPI_problem_details_t *problem_details);
static void app_local_send_tmgi_deallocate();
static void app_local_send_tmgi_deallocated(mb_smf_sc_tmgi_t *tmgi, int result,
                                                     const OpenAPI_problem_details_t *problem_details);
static void app_local_send_event(app_local_event_id_t event_id, mb_smf_sc_tmgi_t *tmgi, int result,
                                 const OpenAPI_problem_details_t *problem_details);

static void app_local_clean(app_local_event_t *app_event);

static const char *app_event_get_name(ogs_event_t *event);
static const char *app_local_get_name(app_local_event_t *app_event);

static void app_tmgi_cb(mb_smf_sc_tmgi_t *session, int result,
                        const OpenAPI_problem_details_t *problem_details, void *data);

static void app_tmgi_allocate();
static void app_tmgi_deallocate();

void app_state_init(ogs_fsm_t *sm, ogs_event_t *event)
{
    ogs_assert(sm);

    const app_options_t *options = tmgi_tool_get_app_options(); 
    if (options->delete_tmgi) {
        OGS_FSM_TRAN(sm, app_state_delete_tmgi);
    } else {
        OGS_FSM_TRAN(sm, app_state_create_tmgi);
    }
}

void app_state_final(ogs_fsm_t *sm, ogs_event_t *event)
{
    ogs_assert(sm);
}

static void app_state_create_tmgi(ogs_fsm_t *sm, ogs_event_t *event)
{
    ogs_assert(sm);

    if (mb_smf_sc_process_event(event))
        return;

    ogs_debug("Processing event %p [%s]", event, app_event_get_name(event));

    switch (event->id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("Requesting TMGI...");
        app_local_send_tmgi_allocate();
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case APP_LOCAL:
        {
            app_local_event_t *app_event = ogs_container_of(event, app_local_event_t, event);

            switch (app_event->id) {
            case APP_LOCAL_EVENT_TMGI_ALLOCATE:
                app_tmgi_allocate();
                break;
            case APP_LOCAL_EVENT_TMGI_ALLOCATED:
                if (app_event->result == OGS_OK) {
                    ogs_info("%s created", mb_smf_sc_tmgi_repr(app_event->tmgi));
                } else {
                    if (app_event->problem_details) {
                        if (app_event->problem_details->cause) {
                            ogs_warn("TMGI allocation failed, caused by %s: %s: %s", app_event->problem_details->cause,
                                     app_event->problem_details->title, app_event->problem_details->detail);
                        } else {
                            ogs_warn("TMGI allocation failed: %s: %s",
                                     app_event->problem_details->title, app_event->problem_details->detail);
                        }
                        if (app_event->problem_details->invalid_params) {
                            ogs_warn("    Parameter errors:");
                            OpenAPI_lnode_t *node;
                            OpenAPI_list_for_each(app_event->problem_details->invalid_params, node) {
                                OpenAPI_invalid_param_t *param = (OpenAPI_invalid_param_t*)(node->data);
                                ogs_warn("        %s: %s", param->param, param->reason);
                            }
                        }
                    } else {
                        ogs_warn("TMGI allocation failed, no reason given");
                    }
                }
                OGS_FSM_TRAN(sm, app_state_exit);
                break;
            default:
                ogs_error("Unexpected local event: %s", app_event_get_name(event));
                break;
            }
            app_local_clean(app_event);
        }
        break;

    default:
        break;
    }
}

static void app_state_delete_tmgi(ogs_fsm_t *sm, ogs_event_t *event)
{
    ogs_assert(sm);

    if (mb_smf_sc_process_event(event))
        return;

    ogs_debug("Processing event %p [%s]", event, app_event_get_name(event));

    switch (event->id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("Removing TMGI...");
        app_local_send_tmgi_deallocate();
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case APP_LOCAL:
        {
            app_local_event_t *app_event = ogs_container_of(event, app_local_event_t, event);

            switch (app_event->id) {
            case APP_LOCAL_EVENT_TMGI_DEALLOCATE:
                app_tmgi_deallocate();
                break;
            case APP_LOCAL_EVENT_TMGI_DEALLOCATED:
                if (app_event->result == OGS_OK) {
                    ogs_info("%s deleted", mb_smf_sc_tmgi_repr(app_event->tmgi));
                } else {
                    if (app_event->problem_details) {
                        if (app_event->problem_details->cause) {
                            ogs_warn("TMGI deallocation failed, caused by %s: %s: %s", app_event->problem_details->cause,
                                     app_event->problem_details->title, app_event->problem_details->detail);
                        } else {
                            ogs_warn("TMGI deallocation failed: %s: %s",
                                     app_event->problem_details->title, app_event->problem_details->detail);
                        }
                        if (app_event->problem_details->invalid_params) {
                            ogs_warn("    Parameter errors:");
                            OpenAPI_lnode_t *node;
                            OpenAPI_list_for_each(app_event->problem_details->invalid_params, node) {
                                OpenAPI_invalid_param_t *param = (OpenAPI_invalid_param_t*)(node->data);
                                ogs_warn("        %s: %s", param->param, param->reason);
                            }
                        }
                    } else {
                        ogs_warn("TMGI deallocation failed, no reason given");
                    }
                }
                OGS_FSM_TRAN(sm, app_state_exit);
                break;
            default:
                ogs_error("Unexpected local event: %s", app_event_get_name(event));
                break;
            }
            app_local_clean(app_event);
        }
        break;

    default:
        break;
    }
}

static void app_state_exit(ogs_fsm_t *sm, ogs_event_t *event)
{
    ogs_assert(sm);

    ogs_debug("Processing event %p [%s]", event, app_event_get_name(event));

    switch (event->id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("TMGI operation finished, exiting...");
        _exit(1);
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case APP_LOCAL:
        {
            app_local_event_t *app_event = ogs_container_of(event, app_local_event_t, event);

            switch (app_event->id) {
            default:
                ogs_error("Unexpected local event: %s", app_event_get_name(event));
                break;
            }

            app_local_clean(app_event);
        }
        break;

    default:
        break;
    }
}

static void app_local_send_tmgi_allocated(mb_smf_sc_tmgi_t *tmgi, int result, const OpenAPI_problem_details_t *problem_details)
{
    /* send APP_LOCAL_EVENT_TMGI_ALLOCATED event */
    app_local_send_event(APP_LOCAL_EVENT_TMGI_ALLOCATED, tmgi, result, problem_details);
}

static void app_local_send_tmgi_allocate()
{
    /* send APP_LOCAL_EVENT_TMGI_ALLOCATE event */
    app_local_send_event(APP_LOCAL_EVENT_TMGI_ALLOCATE, NULL, 0, NULL);
}

static void app_local_send_tmgi_deallocated(mb_smf_sc_tmgi_t *tmgi, int result, const OpenAPI_problem_details_t *problem_details)
{
    /* send APP_LOCAL_EVENT_TMGI_DEALLOCATED event */
    app_local_send_event(APP_LOCAL_EVENT_TMGI_ALLOCATED, tmgi, result, problem_details);
}

static void app_local_send_tmgi_deallocate()
{
    /* send APP_LOCAL_EVENT_TMGI_DEALLOCATE event */
    app_local_send_event(APP_LOCAL_EVENT_TMGI_DEALLOCATE, NULL, 0, NULL);
}

static void app_local_send_event(app_local_event_id_t event_id, mb_smf_sc_tmgi_t *tmgi, int result,
                                 const OpenAPI_problem_details_t *problem_details)
{
    int rv;
    app_local_event_t *event;

    event = ogs_calloc(1, sizeof(*event));
    ogs_assert(event);

    event->id = event_id;
    event->event.id = APP_LOCAL;
    event->tmgi = tmgi;
    event->result = result;
    if (problem_details) {
        event->problem_details = OpenAPI_problem_details_copy(NULL, (OpenAPI_problem_details_t*)problem_details);
    }

    rv = ogs_queue_push(ogs_app()->queue, &event->event);
    if (rv != OGS_OK) {
        ogs_error("Failed to push app local event onto the evet queue");
        return;
    }
    /* process the event queue */
    ogs_pollset_notify(ogs_app()->pollset);
}

static void app_local_clean(app_local_event_t *event)
{
    if (!event) return;

    if (event->problem_details) OpenAPI_problem_details_free(event->problem_details);
}

static const char *app_event_get_name(ogs_event_t *event)
{
    if (ogs_unlikely(!event)) return "*** No Event ***";

    if (event->id == APP_LOCAL) {
        app_local_event_t *app_event;
        app_event = ogs_container_of(event, app_local_event_t, event);
        return app_local_get_name(app_event);
    } else {
        /* Ask the MB-SMF service consumer library for the event name */
        const char *name;
        name = mb_smf_sc_event_get_name(event);
        if (name) return name;
    }

    return "Unknown Event in App context";
}

static const char *app_local_get_name(app_local_event_t *app_event)
{
    if (ogs_unlikely(!app_event)) return "*** No Event ***";

    switch (app_event->id) {
    case APP_LOCAL_EVENT_TMGI_ALLOCATE:
        return "APP_LOCAL_EVENT_TMGI_ALLOCATE";
    case APP_LOCAL_EVENT_TMGI_ALLOCATED:
        return "APP_LOCAL_EVENT_TMGI_ALLOCATED";
    case APP_LOCAL_EVENT_TMGI_DEALLOCATE:
        return "APP_LOCAL_EVENT_TMGI_DEALLOCATE";
    case APP_LOCAL_EVENT_TMGI_DEALLOCATED:
        return "APP_LOCAL_EVENT_TMGI_DEALLOCATED";
    default:
        break;
    }

    return "APP_LOCAL_EVENT Unknown";
}

static void app_tmgi_cb(mb_smf_sc_tmgi_t *tmgi, int result, const OpenAPI_problem_details_t *problem_details, void *data)
{
    /* callback for result of app_tmgi_allocate() */

    /* queue result event */
    if (result == OGS_DONE) { /* OGS_DONE is used to confirm deletions */
        app_local_send_tmgi_deallocated(tmgi, result, problem_details);
    } else {
        app_local_send_tmgi_allocated(tmgi, result, problem_details);
    }
}

static void app_tmgi_allocate(ogs_fsm_t *sm)
{
    ogs_debug("app_tmgi_allocate");

    /* create an mb_smf_sc_tmgi_t object and send allocate request*/
    mb_smf_sc_tmgi_create(app_tmgi_cb, NULL);
}

static void app_tmgi_deallocate(ogs_fsm_t *sm)
{
    ogs_debug("app_tmgi_deallocate");

    /* make mb_smf_sc_tmgi_t object from options */
    const app_options_t *options = tmgi_tool_get_app_options();

    /* create new empty mb_smf_sc_tmgi_t object */
    mb_smf_sc_tmgi_t *tmgi = mb_smf_sc_tmgi_new();

    /* Set the MBS Service ID */
    if (options->mbs_service_id)
        mb_smf_sc_tmgi_set_mbs_service_id(tmgi, options->mbs_service_id);

    /* Set the PLMN */
    mb_smf_sc_tmgi_set_plmn(tmgi, atoi(options->plmn.mcc), atoi(options->plmn.mnc));

    /* Send the deallocate request */
    mb_smf_sc_tmgi_send_deallocate(tmgi);
}


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
