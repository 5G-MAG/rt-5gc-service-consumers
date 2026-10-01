/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include "nmbsmf-mbs-session-handle.c"

/* Link seam, not behaviour under test. The response handlers above reach the session lifecycle and
 * the status-subscription machinery, which this harness does not build: nmbsmf-builders.c carries
 * its own definitions of the session helpers rather than compiling mbs-session.c. Only the Update
 * response handler is exercised; of these, only its two result callbacks are on its path. */
void _mbs_session_do_deleted_callback(_priv_mbs_session_t *sess)
{
    (void)sess;
}

/* Counted, so a test can tell which outcome an Update response was reported as. */
int ut_updated_callbacks = 0;
int ut_update_error_callbacks = 0;
const OpenAPI_problem_details_t *ut_update_error_problem = NULL;

void _mbs_session_do_updated_callback(_priv_mbs_session_t *sess)
{
    (void)sess;
    ut_updated_callbacks++;
}

void _mbs_session_do_update_error_callback(_priv_mbs_session_t *sess, const OpenAPI_problem_details_t *problem_details)
{
    (void)sess;
    ut_update_error_callbacks++;
    ut_update_error_problem = problem_details;
}

bool _context_remove_mbs_session(_priv_mbs_session_t *sess)
{
    (void)sess;
    return true;
}

_priv_mbs_status_subscription_t *_mbs_session_find_subscription(const _priv_mbs_session_t *sess,
                                                                const char *correlation_id)
{
    (void)sess;
    (void)correlation_id;
    return NULL;
}

OpenAPI_status_subscribe_rsp_data_t *OpenAPI_status_subscribe_rsp_data_parseFromJSON(cJSON *json)
{
    (void)json;
    return NULL;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
