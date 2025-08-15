#ifndef MB_SMF_CLIENT_NMBSMF_MBS_SESSION_HANDLE_H
#define MB_SMF_CLIENT_NMBSMF_MBS_SESSION_HANDLE_H
/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _priv_mbs_session_s _priv_mbs_session_t;

int _nmbsmf_mbs_session_parse(ogs_sbi_message_t *message, _priv_mbs_session_t *sess);

int _nmbsmf_mbs_session_subscription_report_list_handler(_priv_mbs_status_subscription_t *subsc, OpenAPI_list_t *event_report_list);
void _nmbsmf_mbs_session_subscription_response(_priv_mbs_session_t *sess, ogs_sbi_message_t *message, ogs_sbi_response_t *response);


#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* MB_SMF_CLIENT_NMBSMF_MBS_SESSION_HANDLE_H */
