/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2022 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#include "context.h"

static int initialized = 0;

int pcf_initialize(void)
{
    int rv;
    pcf_context_init();
    rv = ogs_sbi_context_parse_config(NULL, "nrf", "scp");
    if (rv != OGS_OK) return rv;
    rv = pcf_parse_config("pcf-service-consumer");
    if (rv != OGS_OK) {
	ogs_error("Failed to configure PCF library");
	return rv;
    }
    rv = ogs_log_config_domain(
            ogs_app()->logger.domain, ogs_app()->logger.level);
    if (rv != OGS_OK) {
	ogs_error("Failed to set log domain");
	return rv;
    }
    initialized = 1;
    return OGS_OK;
}

void pcf_terminate(void)
{
    if (!initialized) return;
    pcf_context_final();
}
