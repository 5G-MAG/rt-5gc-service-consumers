/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef TOOL_PCF_POLICYAUTH_OPTIONS_H
#define TOOL_PCF_POLICYAUTH_OPTIONS_H

#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_options_s {
    /* (M) = Mandatory, (C) = Conditional, (O) = Optional */

    /* UE Connection details */
    char *ue_address;      /** (M) UE IP Address (v4 or v6) */
    short int ue_port;     /** (O) UE Port to limit the QoS session to (0=not set) */
    char *remote_address;  /** (O) Remote end IPv4 or IPv6 address to limit the QoS session to (NULL=not set)*/
    short int remote_port; /** (O) Remote end Port to limit the QoS session to (0=not set) */
    int protocol;          /** (O) IP Protocol to limit the QoS session to (IPPROTO_IP=not set) */

    /* Media Type */
    OpenAPI_media_type_e media_type; /** (O) MediaType to use when creating the QoS Session (OpenAPI_media_type_NULL=not set) */

    /* QoS */
    double min_dl_bit_rate; /** (O) Minimum Download Bit Rate to set as a QoS parameter (0.0=not set) */
    double max_dl_bit_rate; /** (O) Maximum Download Bit Rate to set as a QoS parameter (0.0=not set) */
    double min_ul_bit_rate; /** (O) Minimum Upload Bit Rate to set as a QoS parameter (0.0=not set) */
    double max_ul_bit_rate; /** (O) Maximum Upload Bit Rate to set as a QoS parameter (0.0=not set) */

    /* Service addresses */
    char *pcf_address;  /** (C) PCF IP Address (NULL=not set; at least one of pcf_address or nrf_address must be set) */
    short int pcf_port; /** (C) PCF TCP Port number for Npcf interface (0=not set; must be set if pcf_address set) */
    char *nrf_address;  /** (C) NRF IP Address (NULL=not set; at least one of pcf_address or nrf_address must be set) */
    short int nrf_port; /** (C) NRF TCP Port number for Nnrf interface (0=not set; must be set if nrf_address set) */
} app_options_t;

extern app_options_t* app_options_parse(int *argc, char ***argv);
extern void app_options_final(app_options_t *options, int argc, char **argv);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* TOOL_PCF_POLICYAUTH_OPTIONS_H */
