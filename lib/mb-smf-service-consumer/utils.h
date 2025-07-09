/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef MB_SMF_CLIENT_UTILS_H
#define MB_SMF_CLIENT_UTILS_H

#include "ogs-core.h"
#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library Internals */
char *_sockaddr_string(const ogs_sockaddr_t *addr);
char *_time_string(ogs_time_t t);
ogs_time_t _response_to_expiry_time(ogs_sbi_response_t *message);
ogs_time_t _cache_control_to_cache_age(const char *cache_control);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* MB_SMF_CLIENT_UTILS_H */
