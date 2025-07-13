/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <time.h>

#include "ogs-core.h"

#include "utils.h"

/**************************/
/**** Public functions ****/
/**************************/

char *time_t_to_str(time_t t)
{
    char *str = (char*)ogs_calloc(1,26);

    if (!ctime_r(&t, str)) {
        ogs_free(str);
        str = NULL;
    }
    
    return str;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
