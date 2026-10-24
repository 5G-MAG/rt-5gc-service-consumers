/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2022 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-core.h"
#include "ogs-sbi.h"

#include "log.h"
#include "utils.h"

#include "pcf-bindings-cache.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library Internals */
void _pcf_bindings_cache_init(ogs_hash_t **cache)
{
    *cache = ogs_hash_make();
}

/*
 * The cache is keyed by the text of the UE address, in a copy that the cache owns: the address of the caller is
 * released when its request completes, and the raw bytes of an ogs_sockaddr_t include pointers that never compare equal.
 */
static char *cache_key(const ogs_sockaddr_t *ue_address)
{
    return ogs_ipstrdup((ogs_sockaddr_t *)ue_address);
}

static void cache_entry_remove(ogs_hash_t *cache, const char *key)
{
    pcf_bindings_cache_t *value = ogs_hash_get(cache, key, OGS_HASH_KEY_STRING);
    const char *stored_key = NULL;
    ogs_hash_index_t *idx;

    if (!value) return;
    for (idx = ogs_hash_first(cache); idx; idx = ogs_hash_next(idx)) {
        if (ogs_hash_this_val(idx) == value) { stored_key = ogs_hash_this_key(idx); break; }
    }
    ogs_hash_set(cache, key, OGS_HASH_KEY_STRING, NULL);
    if (stored_key) ogs_free((void *)stored_key);
    OpenAPI_pcf_binding_free(value->pcf_binding);
    ogs_free(value);
}

void _pcf_bindings_cache_clear(ogs_hash_t **cache)
{
    ogs_hash_index_t *idx;

    if (!cache || !*cache) return;

    for (idx = ogs_hash_first(*cache); idx; idx = ogs_hash_first(*cache)) {
        char *key = ogs_strdup(ogs_hash_this_key(idx));

        cache_entry_remove(*cache, key);
        ogs_free(key);
    }

    ogs_hash_destroy(*cache);
    *cache = NULL;
}

void _pcf_bindings_cache_log_debug(ogs_hash_t *cache, int indent)
{
    ogs_hash_index_t *idx;

    ogs_debug("%*sBindings Cache:", indent, "");
    for (idx = ogs_hash_first(cache); idx ; idx = ogs_hash_next(idx)) {
        const char *key = ogs_hash_this_key(idx);
        pcf_bindings_cache_t *value = ogs_hash_this_val(idx);
        char *json_txt;
        cJSON *json;
        char *expires;

        json = OpenAPI_pcf_binding_convertToJSON(value->pcf_binding);
        json_txt = cJSON_Print(json);
        cJSON_Delete(json);
        expires = _time_string(value->expires);

        ogs_debug("%*s%s [%s] = %s", indent+2, "", key, expires, json_txt);

        ogs_free(json_txt);
        ogs_free(expires);
    }
}

OpenAPI_pcf_binding_t *_pcf_bindings_cache_find(ogs_hash_t *cache, const ogs_sockaddr_t *ue_address)
{
    pcf_bindings_cache_t *value;
    char *key = cache_key(ue_address);
    OpenAPI_pcf_binding_t *found = NULL;

    if (!key) return NULL;
    value = ogs_hash_get(cache, key, OGS_HASH_KEY_STRING);
    if (value) {
        if (value->expires < ogs_time_now()) {
            /* found, but expired */
            cache_entry_remove(cache, key);
        } else {
            found = value->pcf_binding;
        }
    }
    ogs_free(key);
    return found;
}

bool _pcf_bindings_cache_add(ogs_hash_t *cache, const ogs_sockaddr_t *ue_address, const OpenAPI_pcf_binding_t *bindings, ogs_time_t expires)
{
    pcf_bindings_cache_t *value;
    char *key = cache_key(ue_address);

    if (!key) return false;

    /* a binding for the same address replaces the one held */
    cache_entry_remove(cache, key);
    value = ogs_calloc(1, sizeof(*value));
    ogs_assert(value);
    value->pcf_binding = OpenAPI_pcf_binding_copy(NULL, (OpenAPI_pcf_binding_t*)bindings);
    value->expires = expires;
    ogs_hash_set(cache, key, OGS_HASH_KEY_STRING, value);

    return true;
}

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
