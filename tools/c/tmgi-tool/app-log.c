/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring <david.waring2@bbc.co.uk>
 * Copyright: (C) 2025 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-app.h"
#include "ogs-core.h"

#include "app-log.h"

int __tmgi_tool_log_domain;

void app_log_init(void)
{
    ogs_log_install_domain(&__tmgi_tool_log_domain, "tmgi-tool", ogs_core()->log.level);
    ogs_log_config_domain("tmgi-tool", ogs_app()->logger.level);
}

void app_log_final(void)
{
}

void app_log_change_log_level(ogs_log_level_e level)
{
    ogs_log_set_domain_level(__tmgi_tool_log_domain, level);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
