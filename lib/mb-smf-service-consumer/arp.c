/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <stddef.h>

#include "ogs-core.h"

#include "macros.h"

#include "arp.h"
#include "priv_arp.h"

/* mb_smf_sc_arp Type functions */

MB_SMF_CLIENT_API mb_smf_sc_arp_t *mb_smf_sc_arp_new()
{
    return _arp_new();
}

MB_SMF_CLIENT_API void mb_smf_sc_arp_delete(mb_smf_sc_arp_t *arp)
{
    _arp_free(arp);
}

/* Library internal arp methods (protected) */
mb_smf_sc_arp_t *_arp_new()
{
    return (mb_smf_sc_arp_t*)ogs_calloc(1,sizeof(mb_smf_sc_arp_t));
}

void _arp_free(mb_smf_sc_arp_t *arp)
{
    if (!arp) return;
    ogs_free(arp);
}

void _arp_copy(mb_smf_sc_arp_t **dst, const mb_smf_sc_arp_t *src)
{
    if (!src) {
        if (*dst) {
            _arp_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = _arp_new();
    }

    (*dst)->priority_level = src->priority_level;
    (*dst)->preemption_capability = src->preemption_capability;
    (*dst)->preemption_vulnerability = src->preemption_vulnerability;
}

bool _arp_equal(const mb_smf_sc_arp_t *a, const mb_smf_sc_arp_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    if (a->priority_level != b->priority_level) return false;
    if (a->preemption_capability != b->preemption_capability) return false;
    if (a->preemption_vulnerability != b->preemption_vulnerability) return false;

    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
