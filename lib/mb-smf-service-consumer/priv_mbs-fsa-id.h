#ifndef _MB_SMF_SC_PRIV_MBS_FSA_ID_H
#define _MB_SMF_SC_PRIV_MBS_FSA_ID_H
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

#include "mbs-fsa-id.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ogs_list_s ogs_list_t;
typedef struct cJSON cJSON;
typedef struct OpenAPI_list_s OpenAPI_list_t;

/* Library internal mbs_fsa_id methods (protected) */
void _mbs_fsa_ids_copy(ogs_list_t *dst, const ogs_list_t *src);
void _mbs_fsa_ids_clear(ogs_list_t *mbs_fsa_ids);
bool _mbs_fsa_ids_equal(const ogs_list_t *a, const ogs_list_t *b);
ogs_list_t *_mbs_fsa_ids_patch_list(const ogs_list_t *a, const ogs_list_t *b);
OpenAPI_list_t *_mbs_fsa_ids_to_openapi(const ogs_list_t *mbs_fsa_ids);
cJSON *_mbs_fsa_ids_to_json(const ogs_list_t *mbs_fsa_ids);

mb_smf_sc_mbs_fsa_id_t *_mbs_fsa_id_new();
void _mbs_fsa_id_free(mb_smf_sc_mbs_fsa_id_t *mbs_fsa_id);
void _mbs_fsa_id_clear(mb_smf_sc_mbs_fsa_id_t *mbs_fsa_id);
void _mbs_fsa_id_copy(mb_smf_sc_mbs_fsa_id_t **dst, const mb_smf_sc_mbs_fsa_id_t *src);
bool _mbs_fsa_id_equal(const mb_smf_sc_mbs_fsa_id_t *a, const mb_smf_sc_mbs_fsa_id_t *b);
ogs_list_t *_mbs_fsa_id_patch_list(const mb_smf_sc_mbs_fsa_id_t *a, const mb_smf_sc_mbs_fsa_id_t *b);
char *_mbs_fsa_id_to_openapi(const mb_smf_sc_mbs_fsa_id_t *mbs_fsa_id);
cJSON *_mbs_fsa_id_to_json(const mb_smf_sc_mbs_fsa_id_t *mbs_fsa_id);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_MBS_FSA_ID_H */
