/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <netinet/in.h>

#include "ogs-core.h"

#include "macros.h"

#include "ssm-addr.h"
#include "priv_ssm-addr.h"

/* mb_smf_sc_ssm Type functions */
MB_SMF_CLIENT_API bool mb_smf_sc_ssm_equal(const mb_smf_sc_ssm_addr_t *a, const mb_smf_sc_ssm_addr_t *b)
{
    return _ssm_addr_equal(a, b);
}

/* Library internal ssm_addr methods (protected) */
mb_smf_sc_ssm_addr_t *_ssm_addr_new()
{
    return (mb_smf_sc_ssm_addr_t*)ogs_calloc(1, sizeof(mb_smf_sc_ssm_addr_t));
}

void _ssm_addr_free(mb_smf_sc_ssm_addr_t *ssm_addr)
{
    if (!ssm_addr) return;
    ogs_free(ssm_addr);
}

void _ssm_addr_clear(mb_smf_sc_ssm_addr_t *ssm_addr)
{
    if (!ssm_addr) return;
    memset(ssm_addr, 0, sizeof(*ssm_addr));
}

void _ssm_addr_copy(mb_smf_sc_ssm_addr_t **dst, const mb_smf_sc_ssm_addr_t *src)
{
    if (!src) {
        if (*dst) {
            _ssm_addr_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = _ssm_addr_new();
    } else {
        _ssm_addr_clear(*dst);
    }

    memcpy(*dst, src, sizeof(*src));
}

bool _ssm_addr_equal(const mb_smf_sc_ssm_addr_t *a, const mb_smf_sc_ssm_addr_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    if (a->family != b->family) return false;
    switch (a->family) {
    case AF_INET:
        if (a->source.ipv4.s_addr != b->source.ipv4.s_addr) return false;
        if (a->dest_mc.ipv4.s_addr != b->dest_mc.ipv4.s_addr) return false;
        break;
    case AF_INET6:
        if (memcmp(&a->source.ipv6, &b->source.ipv6, sizeof(a->source.ipv6))) return false;
        if (memcmp(&a->dest_mc.ipv6, &b->dest_mc.ipv6, sizeof(a->dest_mc.ipv6))) return false;
        break;
    default:
        return false;
    }

    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
