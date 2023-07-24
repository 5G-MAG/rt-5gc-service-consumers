/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef TOOL_PCF_POLICYAUTH_DUMP_OPENAPI_H
#define TOOL_PCF_POLICYAUTH_DUMP_OPENAPI_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "ogs-core.h"
#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define debug_events_notification(evts_notif) dump_events_notification(OGS_LOG_DEBUG, (evts_notif), 0)
#define info_events_notification(evts_notif) dump_events_notification(OGS_LOG_INFO, (evts_notif), 0)

void dump_events_notification(ogs_log_level_e lvl, const OpenAPI_events_notification_t *notifications, int indent);
void dump_user_location(ogs_log_level_e lvl, const OpenAPI_user_location_t *userlocn, int indent);
void dump_eutra_location(ogs_log_level_e lvl, const OpenAPI_eutra_location_t *eutra_locn, int indent);
void dump_nr_location(ogs_log_level_e lvl, const OpenAPI_nr_location_t *nr_locn, int indent);
void dump_n3ga_location(ogs_log_level_e lvl, const OpenAPI_n3ga_location_t *n3ga_locn, int indent);
void dump_utra_location(ogs_log_level_e lvl, const OpenAPI_utra_location_t *utra_locn, int indent);
void dump_gera_location(ogs_log_level_e lvl, const OpenAPI_gera_location_t *gera_locn, int indent);
void dump_global_ran_node_id(ogs_log_level_e lvl, const OpenAPI_global_ran_node_id_t *grni, int indent, const char *node_type);
void dump_gnb_id(ogs_log_level_e lvl, const OpenAPI_gnb_id_t *gnbid, int indent);
void dump_tai(ogs_log_level_e lvl, const OpenAPI_tai_t *tai, int indent);
void dump_ecgi(ogs_log_level_e lvl, const OpenAPI_ecgi_t *ecgi, int indent);
void dump_additional_access_info(ogs_log_level_e lvl, const OpenAPI_additional_access_info_t *aai, int indent,
                                 const char *info_type);
void dump_an_gw_address(ogs_log_level_e lvl, const OpenAPI_an_gw_address_t *aga, int indent);
void dump_plmn_id(ogs_log_level_e lvl, const OpenAPI_plmn_id_t *plmn_id, int indent);
void dump_plmn_id_nid(ogs_log_level_e lvl, const OpenAPI_plmn_id_nid_t *plmn_id, int indent);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* ifndef TOOL_PCF_POLICYAUTH_DUMP_OPENAPI_H */
