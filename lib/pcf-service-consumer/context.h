/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2022 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef PCF_CONTEXT_H
#define PCF_CONTEXT_H

#include "ogs-sbi.h"
#include "ogs-app.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int __pcf_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __pcf_log_domain

typedef struct ue_network_identifier_s ue_network_identifier_t;
typedef struct pcf_session_s pcf_session_t;
typedef struct pcf_app_session_s pcf_app_session_t;

typedef struct pcf_notification_listener_s {
    ogs_lnode_t   node;	
    char *addr;
    int port;
} pcf_notification_listener_t;

typedef struct pcf_configuration_s {
    ogs_list_t  pcf_notification_listener_list; // Nodes of this list are of type pcf_notification_listener_t *
    
} pcf_configuration_t;


typedef struct pcf_context_s {
    pcf_configuration_t config;
    ogs_list_t          pcf_sessions; // Nodes of this list are of type pcf_session_t *
} pcf_context_t;

extern void pcf_context_init(void);
extern void pcf_context_final(void);
extern pcf_context_t *pcf_self(void);
extern bool pcf_parse_config(const char *local);
extern bool _pcf_app_session_context_add(const char *af_app_id, const OpenAPI_app_session_context_t *app_session_context, ogs_time_t expires);
OpenAPI_app_session_context_t *_pcf_app_session_context_from_cache(const char *af_app_id);
pcf_app_session_t *_pcf_client_context_active_sessions_exists(pcf_app_session_t *app_sess);



#ifdef __cplusplus
}
#endif

#endif /* PCF_CONTEXT_H */
