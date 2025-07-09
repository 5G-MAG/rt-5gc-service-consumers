/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef MB_SMF_CLIENT_LOCAL_H
#define MB_SMF_CLIENT_LOCAL_H

#include "ogs-proto.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mb_smf_client_sess_s mb_smf_client_sess_t;

enum {
    MB_SMF_CLIENT_LOCAL_EVENT = OGS_MAX_NUM_OF_PROTO_EVENT
};

typedef enum {
    MB_SMF_CLIENT_LOCAL_NULL = 0,
    MB_SMF_CLIENT_LOCAL_DISCOVER_AND_SEND,
    MB_SMF_CLIENT_LOCAL_MAX
} mb_smf_client_local_event_type_e;

typedef struct mb_smf_client_event_s {
    ogs_event_t h;
    mb_smf_client_local_event_type_e id;
} mb_smf_client_event_t;

bool _mb_smf_client_local_discover_and_send(mb_smf_client_sess_t *sess);

bool _mb_smf_client_local_process_event(ogs_event_t *e);

const char *_mb_smf_client_local_get_event_name(ogs_event_t *e);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* ifndef MB_SMF_CLIENT_LOCAL_H */
