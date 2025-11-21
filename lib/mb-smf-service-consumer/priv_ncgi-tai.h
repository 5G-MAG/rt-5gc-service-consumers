#ifndef _MB_SMF_SC_PRIV_NCGI_TAI_H
#define _MB_SMF_SC_PRIV_NCGI_TAI_H
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include "ncgi-tai.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct ogs_list_s ogs_list_t;
typedef struct cJSON cJSON;
typedef struct OpenAPI_list_s OpenAPI_list_t;
typedef struct OpenAPI_ncgi_tai_s OpenAPI_ncgi_tai_t;

/* Library internal ncgi_tai methods (protected) */
ogs_list_t *_ncgi_tais_patch_list(const ogs_list_t *a, const ogs_list_t *b);
OpenAPI_list_t *_ncgi_tais_to_openapi(const ogs_list_t *ncgi_tais);
cJSON *_ncgi_tais_to_json(const ogs_list_t *ncgi_tais);

mb_smf_sc_ncgi_tai_t *_ncgi_tai_new();
void _ncgi_tai_free(mb_smf_sc_ncgi_tai_t *ncgi_tai);
void _ncgi_tai_clear(mb_smf_sc_ncgi_tai_t *ncgi_tai);
void _ncgi_tai_copy(mb_smf_sc_ncgi_tai_t **dst, const mb_smf_sc_ncgi_tai_t *src);
bool _ncgi_tai_equal(const mb_smf_sc_ncgi_tai_t *a, const mb_smf_sc_ncgi_tai_t *b);
ogs_list_t *_ncgi_tai_patch_list(const mb_smf_sc_ncgi_tai_t *a, const mb_smf_sc_ncgi_tai_t *b);
OpenAPI_ncgi_tai_t *_ncgi_tai_to_openapi(const mb_smf_sc_ncgi_tai_t *ncgi_tai);
cJSON *_ncgi_tai_to_json(const mb_smf_sc_ncgi_tai_t *ncgi_tai);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_NCGI_TAI_H */
