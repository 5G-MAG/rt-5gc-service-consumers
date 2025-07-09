/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-app.h"
#include "ogs-core.h"

#ifdef __cplusplus
extern "C" {
#endif

int __mb_smf_log_domain = 0;

/* Library Internals */
void _log_init(void){
    if (!__mb_smf_log_domain) {
        ogs_log_install_domain(&__mb_smf_log_domain, "mb-smf-service-consumer", ogs_core()->log.level);
        ogs_log_config_domain("mb-smf-service-consumer", ogs_app()->logger.level);
    }
}

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
