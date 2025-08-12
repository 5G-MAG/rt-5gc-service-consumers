#ifndef MB_SMF_CLIENT_NMBSMF_MBS_SESSION_BUILD_H
#define MB_SMF_CLIENT_NMBSMF_MBS_SESSION_BUILD_H
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

#ifdef __cplusplus
extern "C" {
#endif

/* Library Internals */
ogs_sbi_request_t *_nmbsmf_mbs_session_build_create(void *context, void *data);
ogs_sbi_request_t *_nmbsmf_mbs_session_build_update(void *context, void *data);
ogs_sbi_request_t *_nmbsmf_mbs_session_build_remove(void *context, void *data);

ogs_sbi_request_t *_nmbsmf_mbs_session_build_status_subscription_create(void *context, void *data);
ogs_sbi_request_t *_nmbsmf_mbs_session_build_status_subscription_update(void *context, void *data);
ogs_sbi_request_t *_nmbsmf_mbs_session_build_status_subscription_delete(void *context, void *data);

void _notification_server_free(ogs_sbi_server_t *server);
void _tidy_fixed_notification_server();

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* MB_SMF_CLIENT_NMBSMF_MBS_SESSION_BUILD_H */
