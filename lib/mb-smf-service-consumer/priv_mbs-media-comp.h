#ifndef _MB_SMF_SC_PRIV_MBS_MEDIA_COMP_H
#define _MB_SMF_SC_PRIV_MBS_MEDIA_COMP_H
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

#include "mbs-media-comp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ogs_list_s ogs_list_t;
typedef struct cJSON cJSON;
typedef struct OpenAPI_list_s OpenAPI_list_t;
typedef struct OpenAPI_mbs_media_comp_rm_s OpenAPI_mbs_media_comp_rm_t;

/* Library internal mbs_media_comp methods (protected) */
ogs_list_t *_mbs_media_comps_patch_list(const ogs_hash_t *a, const ogs_hash_t *b);
OpenAPI_list_t *_mbs_media_comps_to_openapi(const ogs_hash_t *mbs_media_comps);
cJSON *_mbs_media_comps_to_json(const ogs_hash_t *mbs_media_comps);

mb_smf_sc_mbs_media_comp_t *_mbs_media_comp_new();
void _mbs_media_comp_free(mb_smf_sc_mbs_media_comp_t *mbs_media_comp);
void _mbs_media_comp_clear(mb_smf_sc_mbs_media_comp_t *mbs_media_comp);
void _mbs_media_comp_copy(mb_smf_sc_mbs_media_comp_t **dst, const mb_smf_sc_mbs_media_comp_t *src);
bool _mbs_media_comp_equal(const mb_smf_sc_mbs_media_comp_t *a, const mb_smf_sc_mbs_media_comp_t *b);
ogs_list_t *_mbs_media_comp_patch_list(const mb_smf_sc_mbs_media_comp_t *a, const mb_smf_sc_mbs_media_comp_t *b);
OpenAPI_mbs_media_comp_rm_t *_mbs_media_comp_to_openapi(const mb_smf_sc_mbs_media_comp_t *mbs_media_comp);
cJSON *_mbs_media_comp_to_json(const mb_smf_sc_mbs_media_comp_t *mbs_media_comp);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_MBS_MEDIA_COMP_H */
