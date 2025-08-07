#ifndef _MB_SMF_SC_CONTEXT_H_
#define _MB_SMF_SC_CONTEXT_H_
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <stdbool.h>

#include "ogs-core.h"

#include "macros.h"
#include "priv_mbs-session.h"
#include "priv_mbs-status-subscription.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _context_s _context_t;
typedef struct ogs_sbi_object_s ogs_sbi_object_t;

_context_t *_context_new();
int _context_parse_config(const char *local);
void _context_destroy();
bool _context_add_mbs_session(_priv_mbs_session_t *session);
bool _context_remove_mbs_session(_priv_mbs_session_t *session);
ogs_list_t *_context_mbs_sessions();
bool _context_active_sessions_exists(_priv_mbs_session_t *session);
_priv_mbs_session_t *_context_sbi_object_to_session(ogs_sbi_object_t *sbi_object);
const ogs_sockaddr_t *_context_get_notification_address();
bool _context_is_notification_server(ogs_sbi_server_t *server);
_priv_mbs_status_subscription_t *_context_find_subscription(ogs_sbi_server_t *server, const char *url_path);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_SC_CONTEXT_H_ */
