#ifndef _MB_SMF_SC_PRIV_SSM_ADDR_H
#define _MB_SMF_SC_PRIV_SSM_ADDR_H
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include "macros.h"

#include "ssm-addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library internal ssm_addr methods (protected) */
mb_smf_sc_ssm_addr_t *_ssm_addr_new();
void _ssm_addr_free(mb_smf_sc_ssm_addr_t *ssm_addr);
void _ssm_addr_clear(mb_smf_sc_ssm_addr_t *ssm_addr);
void _ssm_addr_copy(mb_smf_sc_ssm_addr_t **dst, const mb_smf_sc_ssm_addr_t *src);
bool _ssm_addr_equal(const mb_smf_sc_ssm_addr_t *a, const mb_smf_sc_ssm_addr_t *b);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_SSM_ADDR_H */
