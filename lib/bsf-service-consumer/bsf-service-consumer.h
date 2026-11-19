/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2023 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#ifndef BSF_CLIENT_H
#define BSF_CLIENT_H

/* Open5GS includes */
#include "ogs-proto.h"
#include "ogs-core.h"
#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUILD_BSF_CLIENT_LIB
    #if defined _WIN32 || defined __CYGWIN__
        #ifdef __GNUC__
            #define BSF_CLIENT_API __attribute__ ((dllexport))
        #else
            #define BSF_CLIENT_API __declspec(dllexport)
        #endif
    #else
        #if __GNUC__ >= 4
            #define BSF_CLIENT_API __attribute__ ((visibility ("default")))
        #else
            #define BSF_CLIENT_API
        #endif
    #endif
#else
    #if defined _WIN32 || defined __CYGWIN__
        #ifdef __GNUC__
            #define BSF_CLIENT_API __attribute__ ((dllimport))
        #else
            #define BSF_CLIENT_API __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
        #endif
    #else
        #define BSF_CLIENT_API
    #endif
#endif

typedef bool (*bsf_retrieve_callback_f)(OpenAPI_pcf_binding_t *pcf_binding, void *user_data);

BSF_CLIENT_API bool bsf_parse_config(const char *bsf_sect, const char *bsf_client_sect);
/* The query keys of TS 29.521 V19.7.0 table 5.3.2.3.2-1 that narrow the match besides the UE address; a key that is NULL is not sent */
typedef struct bsf_pcf_binding_query_s {
    const char *dnn;
    const ogs_s_nssai_t *s_nssai;
    const char *supi;
    const char *gpsi;
    const char *ip_domain;
} bsf_pcf_binding_query_t;

/**
 * The outcome of a PCF binding request to the BSF (Nbsf_Management_Discovery, TS 29.521 V19.7.0 clause 4.2.4.2)
 *
 * Everything is borrowed and valid during the callback only.
 */
typedef struct bsf_pcf_binding_result_s {
    const OpenAPI_pcf_binding_t *pcf_binding;   /**< The PCF binding (200), or NULL if there is none. */
    /**
     * The status of the BSF's answer: 200 with a binding, 204 or 404 for no binding, 400 for a refused request (see problem_cause for
     * MANDATORY_QUERY_PARAM_MISSING or MULTIPLE_BINDING_INFO_FOUND), 307 or 308 for a redirection; 0 if there was no usable answer (no BSF
     * found, no connection, timeout). A redirection is reported and not followed: TS 29.500 V19.7.1 clause 6.10.9.1 has the request redirected
     * in indirect communication, which this library does not use.
     */
    int http_status;
    const char *problem_cause;                  /**< The cause of the ProblemDetails of an error answer, or NULL. */
} bsf_pcf_binding_result_t;

typedef bool (*bsf_retrieve_result_callback_f)(const bsf_pcf_binding_result_t *result, void *user_data);

/**
 * Ask the BSF for the PCF binding of a PDU session and be told how the BSF answered
 *
 * Like bsf_retrieve_pcf_binding_for_pdu_session_narrowed(), with a callback that gets the status of the answer and the cause of an error. @query
 * may be NULL. The callback is called exactly once.
 */
BSF_CLIENT_API bool bsf_retrieve_pcf_binding_with_result(ogs_sockaddr_t *ue_address, const bsf_pcf_binding_query_t *query,
                bsf_retrieve_result_callback_f callback, void *user_data);

BSF_CLIENT_API bool bsf_retrieve_pcf_binding_for_pdu_session(ogs_sockaddr_t *ue_address, bsf_retrieve_callback_f callback, void *user_data);
/* The same with narrowing keys; the cache of bindings is not used for such a query (it is keyed by the address only) */
BSF_CLIENT_API bool bsf_retrieve_pcf_binding_for_pdu_session_narrowed(ogs_sockaddr_t *ue_address, const bsf_pcf_binding_query_t *query, bsf_retrieve_callback_f callback, void *user_data);
BSF_CLIENT_API bool bsf_process_event(ogs_event_t *e);
BSF_CLIENT_API void bsf_terminate(void);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* BSF_CLIENT_H */
