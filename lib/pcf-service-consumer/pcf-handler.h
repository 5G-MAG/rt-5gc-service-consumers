/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2022 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#ifndef PCF_HANDLER_H
#define PCF_HANDLER_H

#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

void pcf_policyauthorization_create(pcf_app_session_t *sess, ogs_sbi_message_t *recvmsg, ogs_sbi_response_t *response);
void pcf_policyauthorization_update(pcf_app_session_t *sess, ogs_sbi_message_t *recvmsg);
void pcf_policyauthorization_delete(pcf_app_session_t *app_sess);

void pcf_policyauthorization_subscribe_event(pcf_app_session_t *sess, ogs_sbi_message_t *recvmsg, ogs_sbi_response_t *response);

void pcf_policyauthorization_event_notification(ogs_sbi_message_t *recvmsg, ogs_sbi_response_t *response);

#ifdef __cplusplus
}
#endif

#endif /* PCF_HANDLER_H */
