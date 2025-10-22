#ifndef _MB_SMF_PRIV_MBS_TMGI_H_
#define _MB_SMF_PRIV_MBS_TMGI_H_
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <time.h>

#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "tmgi.h"
#include "ref_count_sbi_object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */

/* Data types */
typedef struct _priv_tmgi_s {
    ogs_lnode_t node;
    ogs_lnode_t context_lnode; /* private lnode used by the mb-smf-sc context */
    mb_smf_sc_tmgi_t tmgi;
    mb_smf_sc_tmgi_result_cb callback;
    void *callback_data;
    _ref_count_sbi_object_t *sbi_object;
    struct {
        char *repr;
    } *cache;
    bool allocated; /* has this TMGI been allocated? */
} _priv_tmgi_t;

/* internal library functions */
static inline _priv_tmgi_t *_priv_tmgi_from_public(mb_smf_sc_tmgi_t *tmgi)
{
    if (tmgi) return ogs_container_of(tmgi, _priv_tmgi_t, tmgi);
    return NULL;
}

static inline const _priv_tmgi_t *_priv_tmgi_from_public_const(const mb_smf_sc_tmgi_t *tmgi)
{
    if (tmgi) return ogs_container_of(tmgi, const _priv_tmgi_t, tmgi);
    return NULL;
}

static inline mb_smf_sc_tmgi_t *_priv_tmgi_to_public(_priv_tmgi_t *tmgi)
{
    if (tmgi) return &tmgi->tmgi;
    return NULL;
}

static inline const mb_smf_sc_tmgi_t *_priv_tmgi_to_public_const(const _priv_tmgi_t *tmgi)
{
    if (tmgi) return &tmgi->tmgi;
    return NULL;
}

static inline ogs_lnode_t *_priv_tmgi_to_private_lnode(_priv_tmgi_t *tmgi)
{
    if (tmgi) return &tmgi->context_lnode;
    return NULL;
}

static inline _priv_tmgi_t *_priv_tmgi_from_private_lnode(ogs_lnode_t *lnode)
{
    if (lnode) return ogs_container_of(lnode, _priv_tmgi_t, context_lnode);
    return NULL;
}

_priv_tmgi_t *_tmgi_create(mb_smf_sc_tmgi_result_cb callback, void *callback_data);
void _tmgi_free(_priv_tmgi_t *tmgi);

_priv_tmgi_t *_tmgi_set_mbs_service_id(_priv_tmgi_t *tmgi, const char *mbs_service_id);
_priv_tmgi_t *_tmgi_set_plmn(_priv_tmgi_t *tmgi, uint16_t mcc, uint16_t mnc);
_priv_tmgi_t *_tmgi_set_expiry_time(_priv_tmgi_t *tmgi, time_t expiry_time);
_priv_tmgi_t *_tmgi_set_callback(_priv_tmgi_t *tmgi, mb_smf_sc_tmgi_result_cb callback, void *callback_data);

void _tmgi_clear_repr(_priv_tmgi_t *tmgi);

void _tmgi_send_allocate(_priv_tmgi_t *tmgi);
void _tmgi_send_allocate_all(); /**< Send allocate request for all unallocated TMGIs in the library context */
void _tmgi_send_refresh(_priv_tmgi_t *tmgi);
void _tmgi_send_refresh_all(); /**< Send allocate request for all allocated and expired TMGIs in the library context */
void _tmgi_send_allocate_and_refresh_all(); /**< Send allocate request for all unallocated TMGIs and expired TMGIs in the library context */
void _tmgi_send_deallocate(_priv_tmgi_t *tmgi);

void _tmgi_list_send_allocate(const ogs_list_t *new_tmgis, const ogs_list_t *refresh_tmgis);
void _tmgi_list_send_deallocate(const ogs_list_t *tmgis);
char *_tmgi_list_repr(const ogs_list_t *tmgis);

bool _tmgi_equal(const _priv_tmgi_t *a, const _priv_tmgi_t *b);
const char *_tmgi_repr(const _priv_tmgi_t *tmgi);

_priv_tmgi_t *_tmgi_copy(_priv_tmgi_t *old, _priv_tmgi_t *src);

OpenAPI_tmgi_t *_tmgi_to_openapi_type(const _priv_tmgi_t *tmgi);
_priv_tmgi_t *_tmgi_find_matching_openapi_type(const OpenAPI_tmgi_t *api_tmgi);
void _tmgi_replace_sbi_object(_priv_tmgi_t *tmgi, _ref_count_sbi_object_t *sbi_object);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_PRIV_MBS_TMGI_H_ */
