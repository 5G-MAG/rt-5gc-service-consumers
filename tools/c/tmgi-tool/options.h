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

    /* flags */
    bool delete_tmgi;      /**< (O) Request the tool deallocates a TMGI instead of allocating */

    /* TMGI */
    char *mbs_service_id;  /**< (C) TMGI MBS Service Id (NULL=not set; can only be set if delete_tmgi is true) */
    struct options_plmn {
        char *mcc;         /**< (C) TMGI PLMN MCC (NULL=not set; must be set when delete_tmgi is true; set if plmn.mnc is set) */
        char *mnc;         /**< (C) TMGI PLMN MNC (NULL=not set; must be set when delete_tmgi is true; set if plmn.mcc is set) */
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
