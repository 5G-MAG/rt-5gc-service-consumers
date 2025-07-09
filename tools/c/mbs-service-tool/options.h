#ifndef TOOL_MBS_SERVICE_TOOL_OPTIONS_H
#define TOOL_MBS_SERVICE_TOOL_OPTIONS_H
/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring <david.waring2@bbc.co.uk>
 * Copyright: (C) 2025 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_options_s {
    /* (M) = Mandatory, (C) = Conditional, (O) = Optional */

    /* at least one of req_tmgi, tmgi_id or ssm_source must be set */

    /* flags */
    bool req_tmgi;         /**< (C) Request a TMGI be allocated (tmgi_id must not be set) */
    bool is_multicast;     /**< (O) MBS Service is multicast (true=Multicast, false=Broadcast; default=false) */
    bool req_tunnel;       /**< (O) Request that a UDP tunnel be assigned (if false ssm.source must be set) */

    /* SSM */
    struct options_ssm {   /* must set both source and dest if this is set */
        char *source;      /**< (C) SSM source port (NULL=not set; must be set if is_multicast==true or req_tunnel==false) */
        char *dest;        /**< (C) SSM multicast destination (NULL=not set) */
    } ssm;

    /* TMGI */
    char *tmgi_id;         /**< (C) TMGI MBS Service Id (NULL=not set; req_tmgi must not be set) */
    struct options_plmn {
        char *mcc;         /**< (C) TMGI PLMN MCC (NULL=not set; must be set when tmgi_id is set; set if plmn.mnc is set) */
        char *mnc;         /**< (C) TMGI PLMN MNC (NULL=not set; must be set when tmgi_id is set; set if plmn.mcc is set) */
    } plmn;

    /* Service addresses */
    char *nrf_address;  /** (C) NRF IP Address (NULL=not set; at least one of nrf_address or scp_address must be set) */
    short int nrf_port; /** (C) NRF TCP Port number for Nnrf interface (0=not set; must be set if nrf_address set) */
    char *scp_address;  /** (C) SCP IP Address (NULL=not set; at least one of nrf_address or scp_address must be set) */
    short int scp_port; /** (C) SCP TCP Port number for Nscp interface (0=not set; must be set if scp_address set) */
} app_options_t;

extern app_options_t* app_options_parse(int *argc, char ***argv);
extern void app_options_final(app_options_t *options, int argc, char **argv);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* TOOL_MBS_SERVICE_TOOL_OPTIONS_H */
