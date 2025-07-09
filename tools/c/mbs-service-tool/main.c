/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring <david.waring2@bbc.co.uk>
 * Copyright: (C) 2025 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <signal.h>
#include <stdlib.h>

#include "ogs-app.h"
#include "ogs-core.h"
#include "ogs-proto.h"

#include "init.h"
#include "app-log.h"
#include "app-sm.h"

static void event_thread(void *);
static int process_signal(int signum);

int main(int argc, char *argv[])
{
    ogs_thread_t *event_loop;

    ogs_signal_init();
    ogs_setup_signal_thread();

    if (!mbs_service_tool_init(argc, argv)) {
	ogs_error("Initialisation failed, exiting.");
	exit(1);
    }

    event_loop = ogs_thread_create(event_thread, NULL);

    ogs_signal_thread(process_signal);

    ogs_info("Terminating...");

    mbs_service_tool_event_termination();

    ogs_thread_destroy(event_loop);

    /* close down */
    mbs_service_tool_final();

    return 0;
}

static void event_thread(void *data)
{
    ogs_fsm_t app_sm;
    int rv;
    bool done = false;

    /* Start App state machine */
    ogs_fsm_init(&app_sm, app_state_init, app_state_final, 0);

    /* main loop */
    while (!done) {
        ogs_pollset_poll(ogs_app()->pollset, ogs_timer_mgr_next(ogs_app()->timer_mgr));
        ogs_timer_mgr_expire(ogs_app()->timer_mgr);

        while (true) {
            ogs_event_t *ev = NULL;

            rv = ogs_queue_trypop(ogs_app()->queue, (void**)&ev);
            ogs_assert(rv != OGS_ERROR);
            if (rv == OGS_DONE) {
                done = true;
                break;
            }
            if (rv == OGS_RETRY) {
                break;
            }

            ogs_assert(ev);
            ogs_fsm_dispatch(&app_sm, ev);
            ogs_event_free(ev);
        }
    }
}

static int process_signal(int signum)
{
    switch (signum) {
        case SIGTERM:
        case SIGINT:
            /* terminate by returning 1 */
            return 1;
        case SIGHUP:
            /* rotate log file */
            ogs_log_cycle();
            break;
        case SIGUSR1:
            /* turn on debug logging for all domains */
            ogs_log_set_mask_level(NULL, OGS_LOG_DEBUG);
            break;
        case SIGUSR2:
            /* reset log level to app default */
            ogs_log_set_mask_level(NULL, OGS_LOG_ERROR);
            ogs_log_config_domain(ogs_app()->logger.domain, ogs_app()->logger.level);
            break;
        default:
            break;
    }
    /* return non-1 to continue processing signals */
    return 0;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
