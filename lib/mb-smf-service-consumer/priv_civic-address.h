#ifndef _MB_SMF_SC_PRIV_CIVIC_ADDRESS_H
#define _MB_SMF_SC_PRIV_CIVIC_ADDRESS_H
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

#include "civic-address.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ogs_list_s ogs_list_t;
typedef struct cJSON cJSON;
typedef struct OpenAPI_list_s OpenAPI_list_t;
typedef struct OpenAPI_civic_address_s OpenAPI_civic_address_t;

/* Library internal civic_address methods (protected) */
void _civic_addresses_free(ogs_list_t *civic_addresses);
void _civic_addresses_clear(ogs_list_t *civic_addresses);
void _civic_addresses_copy_values(ogs_list_t *dst, const ogs_list_t *src);
void _civic_addresses_copy(ogs_list_t **dst, const ogs_list_t *src);
bool _civic_addresses_equal(const ogs_list_t *a, const ogs_list_t *b);
ogs_list_t *_civic_addresses_patch_list(const ogs_list_t *a, const ogs_list_t *b);
OpenAPI_list_t *_civic_addresses_to_openapi(const ogs_list_t *civic_addresses);
cJSON *_civic_addresses_to_json(const ogs_list_t *civic_addresses);

mb_smf_sc_civic_address_t *_civic_address_new();
void _civic_address_free(mb_smf_sc_civic_address_t *civic_address);
void _civic_address_clear(mb_smf_sc_civic_address_t *civic_address);
void _civic_address_copy(mb_smf_sc_civic_address_t **dst, const mb_smf_sc_civic_address_t *src);
bool _civic_address_equal(const mb_smf_sc_civic_address_t *a, const mb_smf_sc_civic_address_t *b);
ogs_list_t *_civic_address_patch_list(const mb_smf_sc_civic_address_t *a, const mb_smf_sc_civic_address_t *b);
OpenAPI_civic_address_t *_civic_address_to_openapi(const mb_smf_sc_civic_address_t *civic_address);
cJSON *_civic_address_to_json(const mb_smf_sc_civic_address_t *civic_address);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_CIVIC_ADDRESS_H */
