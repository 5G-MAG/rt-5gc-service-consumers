#ifndef _MB_SMF_SC_PRIV_GEOGRAPHIC_COORDINATE_H
#define _MB_SMF_SC_PRIV_GEOGRAPHIC_COORDINATE_H
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

#include "geographic-coordinate.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ogs_list_s ogs_list_t;
typedef struct cJSON cJSON;
typedef struct OpenAPI_list_s OpenAPI_list_t;
typedef struct OpenAPI_geographic_coordinate_s OpenAPI_geographic_coordinate_t;

/* Library internal geographic_coordinate methods (protected) */
ogs_list_t *_geographic_coordinates_patch_list(const ogs_list_t *a, const ogs_list_t *b);
OpenAPI_list_t *_geographic_coordinates_to_openapi(const ogs_list_t *geographic_coordinates);
cJSON *_geographic_coordinates_to_json(const ogs_list_t *geographic_coordinates);

mb_smf_sc_geographic_coordinate_t *_geographic_coordinate_new();
void _geographic_coordinate_free(mb_smf_sc_geographic_coordinate_t *geographic_coordinate);
void _geographic_coordinate_clear(mb_smf_sc_geographic_coordinate_t *geographic_coordinate);
void _geographic_coordinate_copy(mb_smf_sc_geographic_coordinate_t **dst, const mb_smf_sc_geographic_coordinate_t *src);
bool _geographic_coordinate_equal(const mb_smf_sc_geographic_coordinate_t *a, const mb_smf_sc_geographic_coordinate_t *b);
void _geographic_coordinate_set(mb_smf_sc_geographic_coordinate_t *geographic_coordinate, double longitude, double latitude);
ogs_list_t *_geographic_coordinate_patch_list(const mb_smf_sc_geographic_coordinate_t *a, const mb_smf_sc_geographic_coordinate_t *b);
OpenAPI_geographic_coordinate_t *_geographic_coordinate_to_openapi(const mb_smf_sc_geographic_coordinate_t *geographic_coordinate);
cJSON *_geographic_coordinate_to_json(const mb_smf_sc_geographic_coordinate_t *geographic_coordinate);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_GEOGRAPHIC_COORDINATE_H */
