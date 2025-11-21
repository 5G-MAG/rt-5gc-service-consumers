#ifndef _MB_SMF_SC_PRIV_NCGI_H
#define _MB_SMF_SC_PRIV_NCGI_H
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

#include "ncgi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ogs_list_s ogs_list_t;
typedef struct cJSON cJSON;
typedef struct OpenAPI_list_s OpenAPI_list_t;
typedef struct OpenAPI_ncgi_s OpenAPI_ncgi_t;

/* Library internal ncgi methods (protected) */
ogs_list_t *_ncgis_patch_list(const ogs_list_t *a, const ogs_list_t *b);
OpenAPI_list_t *_ncgis_to_openapi(const ogs_list_t *ncgis);
cJSON *_ncgis_to_json(const ogs_list_t *ncgis);

mb_smf_sc_ncgi_t *_ncgi_new();
void _ncgi_free(mb_smf_sc_ncgi_t *ncgi);
void _ncgi_clear(mb_smf_sc_ncgi_t *ncgi);
void _ncgi_copy(mb_smf_sc_ncgi_t **dst, const mb_smf_sc_ncgi_t *src);
bool _ncgi_equal(const mb_smf_sc_ncgi_t *a, const mb_smf_sc_ncgi_t *b);
ogs_list_t *_ncgi_patch_list(const mb_smf_sc_ncgi_t *a, const mb_smf_sc_ncgi_t *b);
OpenAPI_ncgi_t *_ncgi_to_openapi(const mb_smf_sc_ncgi_t *ncgi);
cJSON *_ncgi_to_json(const mb_smf_sc_ncgi_t *ncgi);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_NCGI_H */
