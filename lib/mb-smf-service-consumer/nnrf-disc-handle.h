#ifndef MB_SMF_CLIENT_NNRF_DISC_HANDLE_H
#define MB_SMF_CLIENT_NNRF_DISC_HANDLE_H
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

int _nnrf_disc_handle(ogs_sbi_xact_t *xact, ogs_sbi_message_t *message, ogs_sbi_response_t *response);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* MB_SMF_CLIENT_NNRF_DISC_HANDLE_H */
