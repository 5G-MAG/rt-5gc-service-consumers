/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef AF_NPCF_BUILD_H
#define AF_NPCF_BUILD_H

#include "context.h"
#include "pcf-client.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ogs_sbi_request_t *pcf_policyauthorization_request_create(pcf_app_session_t *sess, OpenAPI_list_t *media_component, int events);

extern ogs_sbi_request_t *pcf_policyauthorization_request_update(pcf_app_session_t *sess, OpenAPI_list_t *media_component);

extern ogs_sbi_request_t *pcf_policyauthorization_request_delete(pcf_app_session_t *sess);

extern ogs_sbi_request_t *pcf_policyauthorization_request_subscribe_event(pcf_app_session_t *sess, int events);

extern ogs_sbi_request_t *pcf_policyauthorization_req_subscribe_event(pcf_app_session_t *sess);

extern ogs_sbi_request_t *pcf_policyauthorization_subscribe_request_event(pcf_app_session_t *sess);

extern ogs_sbi_request_t *pcf_policyauthorization_req_unsubscribe_event(pcf_app_session_t *sess);

#ifdef __cplusplus
}
#endif

#endif /* AF_NPCF_BUILD_H */
