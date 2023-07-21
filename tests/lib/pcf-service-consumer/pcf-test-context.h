/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#ifndef __TESTS_PCF_TEST_CONTEXT_H
#define __TESTS_PCF_TEST_CONTEXT_H

#include "ogs-sbi.h"
#include "ogs-app.h"

#include "pcf-test-event.h"
#include "pcf-test-sm.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int __pcf_test_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __pcf_test_log_domain

typedef struct pcf_test_context_s {
    ogs_hash_t      *supi_hash;     /* hash table (SUPI) */
    ogs_hash_t      *ipv4_hash;     /* hash table (IPv4 Address) */
    ogs_hash_t      *ipv6_hash;     /* hash table (IPv6 Address) */
    ogs_hash_t      *pcf_app_session_id_hash; /* hash table (AppSessionId) */

    ogs_list_t      sess_list;
} pcf_test_context_t;

typedef struct pcf_test_sess_s {
    ogs_sbi_object_t sbi;

    uint64_t policyauthorization_features;

    char *af_app_session_id;
    char *pcf_app_session_id;

    char *ipv4addr;
    char *ipv6addr;
    char *ipv6prefix;

    char *supi;
    char *gpsi;

    ogs_s_nssai_t s_nssai;
    char *dnn;

    struct {
        char *fqdn;

        int num_of_ip;
        struct {
            ogs_sockaddr_t *addr;
            ogs_sockaddr_t *addr6;
            bool is_port;
            int port;
        } ip[OGS_SBI_MAX_NUM_OF_IP_ADDRESS];

        ogs_sbi_client_t *client;
    } pcf;
} pcf_test_sess_t;
#define af_sess_t pcf_test_sess_t

extern void pcf_test_context_init(void);
extern void pcf_test_context_final(void);

extern pcf_test_context_t *pcf_test_self(void);

extern int pcf_test_context_parse_config(void);

extern pcf_test_sess_t *pcf_test_sess_add_by_ue_address(ogs_ip_t *ue_address);
extern void pcf_test_sess_remove(pcf_test_sess_t *sess);
extern void pcf_test_sess_remove_all(void);

extern bool pcf_test_sess_set_pcf_app_session_id(pcf_test_sess_t *sess, char *pcf_app_session_id);

extern pcf_test_sess_t *pcf_test_sess_find(uint32_t index);
extern pcf_test_sess_t *pcf_test_sess_find_by_af_app_session_id(char *af_app_session_id);
extern pcf_test_sess_t *pcf_test_sess_find_by_pcf_app_session_id(char *pcf_app_session_id);

extern void pcf_test_sess_associate_pcf_client(pcf_test_sess_t *sess);

#ifdef __cplusplus
}
#endif

#endif /* ifndef __TESTS_PCF_TEST_CONTEXT_H */

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
