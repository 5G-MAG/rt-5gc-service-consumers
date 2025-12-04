#ifndef _MB_SMF_SC_PRIV_MBS_SERVICE_INFO_H
#define _MB_SMF_SC_PRIV_MBS_SERVICE_INFO_H
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

#include "mbs-service-info.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ogs_list_s ogs_list_t;
typedef struct cJSON cJSON;
typedef struct OpenAPI_mbs_service_info_s OpenAPI_mbs_service_info_t;

/* Library internal mbs_service_info methods (protected) */
mb_smf_sc_mbs_service_info_t *_mbs_service_info_new();
void _mbs_service_info_free(mb_smf_sc_mbs_service_info_t *mbs_service_info);
void _mbs_service_info_clear(mb_smf_sc_mbs_service_info_t *mbs_service_info);
void _mbs_service_info_copy(mb_smf_sc_mbs_service_info_t **dst, const mb_smf_sc_mbs_service_info_t *src);
bool _mbs_service_info_equal(const mb_smf_sc_mbs_service_info_t *a, const mb_smf_sc_mbs_service_info_t *b);
ogs_list_t *_mbs_service_info_patch_list(const mb_smf_sc_mbs_service_info_t *a, const mb_smf_sc_mbs_service_info_t *b);
OpenAPI_mbs_service_info_t *_mbs_service_info_to_openapi(const mb_smf_sc_mbs_service_info_t *mbs_service_info);
cJSON *_mbs_service_info_to_json(const mb_smf_sc_mbs_service_info_t *mbs_service_info);
mb_smf_sc_mbs_service_info_t *_mbs_service_info_set_mbs_media_comp(mb_smf_sc_mbs_service_info_t *svc_info,
                                                                   mb_smf_sc_mbs_media_comp_t *media_comp);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_MBS_SERVICE_INFO_H */
