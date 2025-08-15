#ifndef TOOL_MBS_SERVICE_TOOL_UTILS_H
#define TOOL_MBS_SERVICE_TOOL_UTILS_H

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

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Convert time_t to a string
 *
 * @param t The time to convert to a string.
 * @return A string form of @p t, the caller is responsible for freeing the result using ogs_free().
 */
extern char *time_t_to_str(time_t t);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* TOOL_MBS_SERVICE_TOOL_UTILS_H */
