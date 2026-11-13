#ifndef _MB_SMF_SC_PRIV_ARP_H
#define _MB_SMF_SC_PRIV_ARP_H
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <stdbool.h>

#include "macros.h"

#include "arp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cJSON cJSON;
typedef struct ogs_list_s ogs_list_t;
typedef struct OpenAPI_arp_s OpenAPI_arp_t;

/* Library internal arp methods (protected) */
mb_smf_sc_arp_t *_arp_new();
void _arp_free(mb_smf_sc_arp_t *arp);
void _arp_clear(mb_smf_sc_arp_t *arp);
void _arp_copy(mb_smf_sc_arp_t **dst, const mb_smf_sc_arp_t *src);
bool _arp_equal(const mb_smf_sc_arp_t *a, const mb_smf_sc_arp_t *b);
ogs_list_t *_arp_patch_list(const mb_smf_sc_arp_t *a, const mb_smf_sc_arp_t *b);
OpenAPI_arp_t *_arp_to_openapi(const mb_smf_sc_arp_t *arp);
cJSON *_arp_to_json(const mb_smf_sc_arp_t *arp);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_ARP_H */
