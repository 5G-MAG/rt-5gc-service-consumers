#ifndef _MB_SMF_SC_PRIV_GEOGRAPHIC_AREA_H
#define _MB_SMF_SC_PRIV_GEOGRAPHIC_AREA_H
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

#include "geographic-area.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ogs_list_s ogs_list_t;
typedef struct cJSON cJSON;
typedef struct OpenAPI_list_s OpenAPI_list_t;
typedef struct OpenAPI_geographic_area_s OpenAPI_geographic_area_t;

/* Library internal geographic_area methods (protected) */
void _geographic_areas_free(ogs_list_t *geographic_areas);
void _geographic_areas_clear(ogs_list_t *geographic_areas);
void _geographic_areas_copy_values(ogs_list_t *dst, const ogs_list_t *src);
void _geographic_areas_copy(ogs_list_t **dst, const ogs_list_t *src);
bool _geographic_areas_equal(const ogs_list_t *a, const ogs_list_t *b);
ogs_list_t *_geographic_areas_patch_list(const ogs_list_t *a, const ogs_list_t *b);
OpenAPI_list_t *_geographic_areas_to_openapi(const ogs_list_t *geographic_areas);
cJSON *_geographic_areas_to_json(const ogs_list_t *geographic_areas);

mb_smf_sc_geographic_area_t *_geographic_area_new();
void _geographic_area_free(mb_smf_sc_geographic_area_t *geographic_area);
void _geographic_area_clear(mb_smf_sc_geographic_area_t *geographic_area);
void _geographic_area_copy(mb_smf_sc_geographic_area_t **dst, const mb_smf_sc_geographic_area_t *src);
bool _geographic_area_equal(const mb_smf_sc_geographic_area_t *a, const mb_smf_sc_geographic_area_t *b);
ogs_list_t *_geographic_area_patch_list(const mb_smf_sc_geographic_area_t *a, const mb_smf_sc_geographic_area_t *b);
OpenAPI_geographic_area_t *_geographic_area_to_openapi(const mb_smf_sc_geographic_area_t *geographic_area);
cJSON *_geographic_area_to_json(const mb_smf_sc_geographic_area_t *geographic_area);
ogs_list_t *_ga_point_patch_list(const mb_smf_sc_geographic_area_t *a, const mb_smf_sc_geographic_area_t *b);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_GEOGRAPHIC_AREA_H */
