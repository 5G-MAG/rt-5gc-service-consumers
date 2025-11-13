#ifndef _MB_SMF_PRIV_MBS_TAI_H_
#define _MB_SMF_PRIV_MBS_TAI_H_
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "tai.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */

/* Data types */

/* internal library functions */
mb_smf_sc_tai_t *_tai_new(const ogs_plmn_id_t *plmn_id, uint32_t tac, const uint64_t *nid);
mb_smf_sc_tai_t *_tai_create();
void _tai_free(mb_smf_sc_tai_t *tai);
void _tai_clear(mb_smf_sc_tai_t *tai);
void _tai_copy(mb_smf_sc_tai_t **dst, const mb_smf_sc_tai_t *src);
bool _tai_equal(const mb_smf_sc_tai_t *a, const mb_smf_sc_tai_t *b);
ogs_list_t *_tai_patch_list(const mb_smf_sc_tai_t *a, const mb_smf_sc_tai_t *b);
void _tai_set_plmn_id(mb_smf_sc_tai_t *tai, const ogs_plmn_id_t *plmn_id);
void _tai_set_tac(mb_smf_sc_tai_t *tai, uint32_t tac);
void _tai_set_network_id(mb_smf_sc_tai_t *tai, const uint64_t *nid);
OpenAPI_tai_t *_tai_to_openapi(const mb_smf_sc_tai_t *tai);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_PRIV_MBS_TAI_H_ */
