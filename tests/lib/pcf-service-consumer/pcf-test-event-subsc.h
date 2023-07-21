/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#ifndef __TESTS_PCF_EVENTS_SUBSC_H
#define __TESTS_PCF_EVENTS_SUBSC_H

#include "ogs-sbi.h"
#include "pcf-service-consumer.h"

#ifdef __cplusplus
extern "C" {
#endif

OpenAPI_list_t *tests_events_subsc_create(int events, OpenAPI_af_notif_method_e notif_method);	

#ifdef __cplusplus
}
#endif

#endif /* ifndef __TESTS_PCF_EVENTS_SUBSC_H */

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
