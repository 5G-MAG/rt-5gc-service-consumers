/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2022 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef PCF_BDT_H
#define PCF_BDT_H

#include "pcf-service-consumer.h"
#include "pcf-client-sess.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pcf_bdt_policy_s {
    ogs_lnode_t node;
    ogs_sbi_object_t sbi;           /* names the policy in a transaction that finds the PCF through the NRF */
    pcf_session_t *pcf_session;     /* NULL until the PCF is known, for a policy whose first request goes through the NRF */
    ogs_sbi_xact_t *xact;
    char *pending_method;           /* the first request, until it is sent to the PCF that the NRF found */
    char *pending_id;
    char *pending_text;

    char *uri;                      /* the Individual BDT policy resource, from the Location header or the request */
    char *bdt_ref_id;
    cJSON *transf_policies;         /* the transfPolicies of the PCF's answer */
    uint64_t features;              /* negotiated, TS 29.554 V19.2.1 table 5.8-1 */
    int selected;                   /* selTransPolicyId, 0 if none */

    cJSON *pending_body;            /* the request of the operation that waits for the PCF */
    int pending_selected;
    pcf_bdt_operation_t operation;
    pcf_bdt_result_callback callback;
    void *user_data;
};

/* A policy that waits for the answer to an operation; library internal, used by the tests */
pcf_bdt_policy_t *_pcf_bdt_policy_new(pcf_session_t *session, pcf_bdt_operation_t operation, pcf_bdt_result_callback callback, void *user_data);

/* The callback of the client library for every request (the event goes to _pcf_process_event()) */
int client_notify_cb(int status, ogs_sbi_response_t *response, void *data);

/* The event is the answer to a request of a BDT policy: true if it was dealt with (and the response released) */
bool _pcf_bdt_process_client_event(ogs_event_t *e);

/* The timer event of a transaction that finds the PCF through the NRF: true if it is one of the BDT policies' */
bool _pcf_bdt_process_timer_event(ogs_event_t *e);

/* Release every BDT policy without telling the PCF (the library is released) */
void _pcf_bdt_policies_free_all(void);

/* Release the BDT policies of a PCF session without telling the PCF */
void _pcf_bdt_policies_remove_all(pcf_session_t *session);

#ifdef __cplusplus
}
#endif

#endif /* PCF_BDT_H */
