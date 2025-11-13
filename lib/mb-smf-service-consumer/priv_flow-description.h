#ifndef _MB_SMF_SC_PRIV_FLOW_DESCRIPTION_H
#define _MB_SMF_SC_PRIV_FLOW_DESCRIPTION_H
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

#include "flow-description.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library internal flow_description methods (protected) */
void _flow_descriptions_copy(ogs_list_t **dst, const ogs_list_t *src);
void _flow_descriptions_clear(ogs_list_t *flow_descriptions);
void _flow_descriptions_free(ogs_list_t *flow_descriptions);
bool _flow_descriptions_equal(const ogs_list_t *a, const ogs_list_t *b);

mb_smf_sc_flow_description_t *_flow_description_new();
void _flow_description_free(mb_smf_sc_flow_description_t *flow_description);
void _flow_description_clear(mb_smf_sc_flow_description_t *flow_description);
void _flow_description_copy(mb_smf_sc_flow_description_t **dst, const mb_smf_sc_flow_description_t *src);
bool _flow_description_equal(const mb_smf_sc_flow_description_t *a, const mb_smf_sc_flow_description_t *b);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_FLOW_DESCRIPTION_H */
