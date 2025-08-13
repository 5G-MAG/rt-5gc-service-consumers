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

#include "macros.h"
#include "context.h"
#include "nmbsmf-tmgi-build.h"
#include "utils.h"

#include "mbs-tmgi.h"
#include "priv_mbs-tmgi.h"

/* mb_smf_sc_tmgi Type functions */
MB_SMF_CLIENT_API void mb_smf_sc_tmgi_create(mb_smf_sc_tmgi_create_result_cb callback, void *callback_data)
{
    _priv_tmgi_t *tmgi = _tmgi_create(callback, callback_data);
    _tmgi_send_create(tmgi);
}

MB_SMF_CLIENT_API void mb_smf_sc_tmgi_free(mb_smf_sc_tmgi_t *tmgi)
{
    _tmgi_free(_priv_tmgi_from_public(tmgi));
}

MB_SMF_CLIENT_API bool mb_smf_sc_tmgi_equal(const mb_smf_sc_tmgi_t *a, const mb_smf_sc_tmgi_t *b)
{
    return _tmgi_equal(_priv_tmgi_from_public_const(a), _priv_tmgi_from_public_const(b));
}

MB_SMF_CLIENT_API mb_smf_sc_tmgi_t *mb_smf_sc_tmgi_set_mbs_service_id(mb_smf_sc_tmgi_t *tmgi, const char *mbs_svc_id)
{
    _tmgi_set_mbs_service_id(_priv_tmgi_from_public(tmgi), mbs_svc_id);
    return tmgi;
}

MB_SMF_CLIENT_API mb_smf_sc_tmgi_t *mb_smf_sc_tmgi_set_plmn(mb_smf_sc_tmgi_t *tmgi, uint16_t mcc, uint16_t mnc)
{
    _tmgi_set_plmn(_priv_tmgi_from_public(tmgi), mcc, mnc);
    return tmgi;
}

MB_SMF_CLIENT_API mb_smf_sc_tmgi_t *mb_smf_sc_tmgi_set_expiry_time(mb_smf_sc_tmgi_t *tmgi, time_t expiry_time)
{
    _tmgi_set_expiry_time(_priv_tmgi_from_public(tmgi), expiry_time);
    return tmgi;
}

/* internal library functions */
_priv_tmgi_t *_tmgi_create(mb_smf_sc_tmgi_create_result_cb callback, void *callback_data)
{
    _priv_tmgi_t *tmgi = ogs_calloc(1, sizeof(*tmgi));
    
    tmgi->callback = callback;
    tmgi->callback_data = callback_data;
    tmgi->cache = ogs_calloc(1, sizeof(*tmgi->cache));

    _context_add_tmgi(tmgi);

    return tmgi;
}

void _tmgi_free(_priv_tmgi_t *tmgi)
{
    if (!tmgi) return;

    _context_remove_tmgi(tmgi);

    if (tmgi->tmgi.mbs_service_id) ogs_free(tmgi->tmgi.mbs_service_id);

    if (tmgi->cache) {
        if (tmgi->cache->repr) ogs_free(tmgi->cache->repr);
        ogs_free(tmgi->cache);
    }

    ogs_free(tmgi);
}

_priv_tmgi_t *_tmgi_set_mbs_service_id(_priv_tmgi_t *tmgi, const char *mbs_service_id)
{
    if (tmgi->tmgi.mbs_service_id == mbs_service_id) return tmgi;

    if (!tmgi->tmgi.mbs_service_id || !mbs_service_id || strcmp(tmgi->tmgi.mbs_service_id, mbs_service_id)) {
        if (tmgi->tmgi.mbs_service_id) {
            ogs_free(tmgi->tmgi.mbs_service_id);
            tmgi->tmgi.mbs_service_id = NULL;
        }
        if (mbs_service_id) tmgi->tmgi.mbs_service_id = ogs_strdup(mbs_service_id);

        _tmgi_clear_repr(tmgi);
    }

    return tmgi;
}

_priv_tmgi_t *_tmgi_set_plmn(_priv_tmgi_t *tmgi, uint16_t mcc, uint16_t mnc)
{
    if (ogs_plmn_id_mcc(&tmgi->tmgi.plmn) != mcc || ogs_plmn_id_mnc(&tmgi->tmgi.plmn) != mnc) {
        uint16_t mnc_len = (mnc < 100)?2:3;

        ogs_plmn_id_build(&tmgi->tmgi.plmn, mcc, mnc, mnc_len);

        _tmgi_clear_repr(tmgi);
    }

    return tmgi;
}

_priv_tmgi_t *_tmgi_set_expiry_time(_priv_tmgi_t *tmgi, time_t expiry_time)
{
    if (tmgi->tmgi.expiry_time != expiry_time) {
        _tmgi_clear_repr(tmgi);
        tmgi->tmgi.expiry_time = expiry_time;
    }

    return tmgi;
}

void _tmgi_clear_repr(_priv_tmgi_t *tmgi)
{
    if (!tmgi || !tmgi->cache || !tmgi->cache->repr) return;
    ogs_free(tmgi->cache->repr);
    tmgi->cache->repr = NULL;
}

void _tmgi_send_create(_priv_tmgi_t *tmgi)
{
    if (!tmgi) return;

    ogs_list_t new_list = {};
    ogs_list_add(&new_list, tmgi);
    _tmgi_list_send_create(&new_list, NULL);
}

void _tmgi_send_refresh(_priv_tmgi_t *tmgi)
{
    if (!tmgi) return;

    ogs_list_t refresh_list = {};
    ogs_list_add(&refresh_list, tmgi);
    _tmgi_list_send_create(NULL, &refresh_list);
}

void _tmgi_send_delete(_priv_tmgi_t *tmgi)
{
    if (!tmgi) return;

    ogs_list_t del_list = {};
    ogs_list_add(&del_list, tmgi);
    _tmgi_list_send_delete(&del_list);
}

void _tmgi_list_send_create(const ogs_list_t *new_tmgis, const ogs_list_t *refresh_tmgis)
{
    if (!new_tmgis && !refresh_tmgis) return;

    {
        char *new_tmgi_list_str = _tmgi_list_repr(new_tmgis);
        char *refresh_tmgi_list_str = _tmgi_list_repr(refresh_tmgis);

        ogs_debug("Send create for TMGIs [%s] and refresh for TMGIs [%s]", new_tmgi_list_str, refresh_tmgi_list_str);

        ogs_free(new_tmgi_list_str);
        ogs_free(refresh_tmgi_list_str);
    }

    ogs_sbi_discovery_option_t *discovery_option = NULL;
    ogs_sbi_service_type_e service_type = OGS_SBI_SERVICE_TYPE_NMBSMF_TMGI;
    ogs_sbi_object_t *sbi_object = NULL;

    if (new_tmgis) {
        discovery_option = ogs_sbi_discovery_option_new();
        OGS_SBI_FEATURES_SET(discovery_option->requester_features, OGS_SBI_NNRF_DISC_SERVICE_MAP);
        ogs_sbi_discovery_option_add_service_names(discovery_option, (char *)ogs_sbi_service_type_to_name(service_type));

        _ref_count_sbi_object_t *ref_count_sbi_object = _ref_count_sbi_object_new(); /* new discovery, use new sbi_object */
        _priv_tmgi_t *node;
        ogs_list_for_each(new_tmgis, node) {
            if (node->sbi_object) _ref_count_sbi_object_unref(node->sbi_object);
            node->sbi_object = _ref_count_sbi_object_ref(ref_count_sbi_object);
        }
        sbi_object = _ref_count_sbi_object_ptr(ref_count_sbi_object);
        _ref_count_sbi_object_unref(ref_count_sbi_object);
    } else {
        sbi_object = _ref_count_sbi_object_ptr(((_priv_tmgi_t*)ogs_list_first(refresh_tmgis))->sbi_object);
    }

    ogs_sbi_xact_t *xact = ogs_sbi_xact_add(0, sbi_object, service_type, discovery_option, _nmbsmf_tmgi_build_create,
                                            (void*)new_tmgis, (void*)refresh_tmgis);

    ogs_sbi_discover_and_send(xact);
}

void _tmgi_list_send_delete(const ogs_list_t *tmgis)
{
    if (!tmgis || ogs_list_empty(tmgis)) return;

    {
        char *tmgi_list_str = _tmgi_list_repr(tmgis);
        ogs_debug("Send delete for TMGIs [%s]", tmgi_list_str);
        ogs_free(tmgi_list_str);
    }

    ogs_sbi_service_type_e service_type = OGS_SBI_SERVICE_TYPE_NMBSMF_TMGI;
    _priv_tmgi_t *first_tmgi = ogs_list_first(tmgis);
    ogs_sbi_object_t *sbi_object = _ref_count_sbi_object_ptr(first_tmgi->sbi_object);

    ogs_sbi_xact_t *xact = ogs_sbi_xact_add(0, sbi_object, service_type, NULL, _nmbsmf_tmgi_build_remove,
                                            (void*)tmgis, NULL);

    ogs_sbi_discover_and_send(xact);
}

char *_tmgi_list_repr(const ogs_list_t *tmgis)
{
    char *tmgi_list_str = ogs_strdup("");

    if (tmgis) {
        const char *sep = ""; 
        _priv_tmgi_t *node;
        ogs_list_for_each(tmgis, node) {
            tmgi_list_str = ogs_mstrcatf(tmgi_list_str, "%s{%s}", sep, _tmgi_repr(node));
            sep = ", ";
        }
    }

    return tmgi_list_str;
}

bool _tmgi_equal(const _priv_tmgi_t *a, const _priv_tmgi_t *b)
{
    if (a == b) return true;

    if (!a || !b) return false;

    if (a->tmgi.mbs_service_id || b->tmgi.mbs_service_id) {
        if (!a->tmgi.mbs_service_id || !b->tmgi.mbs_service_id) return false;
        if (strcmp(a->tmgi.mbs_service_id, b->tmgi.mbs_service_id) != 0) return false;
    }

    return memcmp(&a->tmgi.plmn, &b->tmgi.plmn, sizeof(a->tmgi.plmn)) == 0;
}

OpenAPI_tmgi_t *_tmgi_to_openapi_type(const _priv_tmgi_t *tmgi)
{
    if (!tmgi) return NULL;

    char *mbs_service_id = NULL;
    if (tmgi->tmgi.mbs_service_id) mbs_service_id = ogs_strdup(tmgi->tmgi.mbs_service_id);

    OpenAPI_plmn_id_t *plmn = OpenAPI_plmn_id_create(ogs_plmn_id_mcc_string(&tmgi->tmgi.plmn),
                                                     ogs_plmn_id_mnc_string(&tmgi->tmgi.plmn));

    return OpenAPI_tmgi_create(mbs_service_id, plmn);
}

const char *_tmgi_repr(const _priv_tmgi_t *tmgi)
{
    if (!tmgi) return NULL;

    if (!tmgi->cache->repr) {
        char buffer[OGS_PLMNIDSTRLEN];
        tmgi->cache->repr = ogs_msprintf("TMGI[%p (%p)]: plmn=%s",
                                         tmgi, _priv_tmgi_to_public_const(tmgi),
                                         ogs_plmn_id_to_string(&tmgi->tmgi.plmn, buffer));

        if (tmgi->tmgi.mbs_service_id) {
            tmgi->cache->repr = ogs_mstrcatf(tmgi->cache->repr, ", mbs_service_id=\"%s\"", tmgi->tmgi.mbs_service_id);
        }

        if (tmgi->tmgi.expiry_time) {
            char *exp_time_str = _time_string(tmgi->tmgi.expiry_time);
            tmgi->cache->repr = ogs_mstrcatf(tmgi->cache->repr, ", expiry_time=%s", exp_time_str);
            ogs_free(exp_time_str);
        }
    }

    return tmgi->cache->repr;
}

_priv_tmgi_t *_tmgi_copy(_priv_tmgi_t *old, _priv_tmgi_t *src)
{
    /* no change, just return */
    if (old == src) return old;

    if (!src) {
        _tmgi_free(old);
        return NULL;
    }

    if (!old) old = _tmgi_create(src->callback, src->callback_data);

    if (old->tmgi.mbs_service_id) {
        ogs_free(old->tmgi.mbs_service_id);
        old->tmgi.mbs_service_id = NULL;
    }
    if (src->tmgi.mbs_service_id) {
        old->tmgi.mbs_service_id = ogs_strdup(src->tmgi.mbs_service_id);
    }

    memcpy(&old->tmgi.plmn, &src->tmgi.plmn, sizeof(src->tmgi.plmn));

    old->tmgi.expiry_time = src->tmgi.expiry_time;

    _ref_count_sbi_object_unref(old->sbi_object);
    old->sbi_object = _ref_count_sbi_object_ref(src->sbi_object);
    _tmgi_clear_repr(old);

    return old;
}

void _tmgi_replace_sbi_object(_priv_tmgi_t *tmgi, _ref_count_sbi_object_t *sbi_object)
{
    _ref_count_sbi_object_unref(tmgi->sbi_object);
    tmgi->sbi_object = _ref_count_sbi_object_ref(sbi_object);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
