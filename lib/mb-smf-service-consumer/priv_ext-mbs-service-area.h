#ifndef _MB_SMF_SC_PRIV_EXT_MBS_SERVICE_AREA_H
#define _MB_SMF_SC_PRIV_EXT_MBS_SERVICE_AREA_H
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

#include "ext-mbs-service-area.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ogs_list_s ogs_list_t;
typedef struct cJSON cJSON;
typedef struct OpenAPI_external_mbs_service_area_s OpenAPI_external_mbs_service_area_t;

/* Library internal ext_mbs_service_area methods (protected) */
mb_smf_sc_ext_mbs_service_area_t *_ext_mbs_service_area_new();
void _ext_mbs_service_area_free(mb_smf_sc_ext_mbs_service_area_t *ext_mbs_service_area);
void _ext_mbs_service_area_clear(mb_smf_sc_ext_mbs_service_area_t *ext_mbs_service_area);
void _ext_mbs_service_area_copy(mb_smf_sc_ext_mbs_service_area_t **dst, const mb_smf_sc_ext_mbs_service_area_t *src);
bool _ext_mbs_service_area_equal(const mb_smf_sc_ext_mbs_service_area_t *a, const mb_smf_sc_ext_mbs_service_area_t *b);
ogs_list_t *_ext_mbs_service_area_patch_list(const mb_smf_sc_ext_mbs_service_area_t *a, const mb_smf_sc_ext_mbs_service_area_t *b);
OpenAPI_external_mbs_service_area_t *_ext_mbs_service_area_to_openapi(const mb_smf_sc_ext_mbs_service_area_t *ext_mbs_service_area);
cJSON *_ext_mbs_service_area_to_json(const mb_smf_sc_ext_mbs_service_area_t *ext_mbs_service_area);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_EXT_MBS_SERVICE_AREA_H */
