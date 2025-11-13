/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include "ogs-core.h"

#include "macros.h"

#include "flow-description.h"
#include "priv_flow-description.h"

/* mb_smf_sc_flow_description Type functions */
MB_SMF_CLIENT_API mb_smf_sc_flow_description_t *mb_smf_sc_flow_description_new()
{
    return _flow_description_new();
}

MB_SMF_CLIENT_API void mb_smf_sc_flow_description_delete(mb_smf_sc_flow_description_t *flow_desc)
{
    _flow_description_free(flow_desc);
}

/* Library internal flow_description methods (protected) */
void _flow_descriptions_copy(ogs_list_t **dst, const ogs_list_t *src)
{
    if (!src) {
        if (*dst) {
            _flow_descriptions_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = (ogs_list_t*)ogs_calloc(1, sizeof(**dst));
    } else {
        _flow_descriptions_clear(*dst);
    }

    mb_smf_sc_flow_description_t *flow;
    ogs_list_for_each(src, flow) {
        mb_smf_sc_flow_description_t *new_flow = NULL;
        _flow_description_copy(&new_flow, flow);
        ogs_list_add(*dst, new_flow);
    }
}

void _flow_descriptions_clear(ogs_list_t *flow_descriptions)
{
    if (!flow_descriptions) return;
    mb_smf_sc_flow_description_t *next, *flow;
    ogs_list_for_each_safe(flow_descriptions, next, flow) {
        ogs_list_remove(flow_descriptions, flow);
        _flow_description_free(flow);
    }
}

void _flow_descriptions_free(ogs_list_t *flow_descriptions)
{
    if (!flow_descriptions) return;
    _flow_descriptions_clear(flow_descriptions);
    ogs_free(flow_descriptions);
}

bool _flow_descriptions_equal(const ogs_list_t *a, const ogs_list_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    if (ogs_list_count(a) != ogs_list_count(b)) return false;

    ogs_list_t *b_copy = NULL;
    _flow_descriptions_copy(&b_copy, b);

    mb_smf_sc_flow_description_t *a_flow;
    ogs_list_for_each(a, a_flow) {
        mb_smf_sc_flow_description_t *next, *b_flow;
        bool found = true;
        ogs_list_for_each_safe(b_copy, next, b_flow) {
            if (_flow_description_equal(a_flow, b_flow)) {
                found = true;
                ogs_list_remove(b_copy, b_flow);
                _flow_description_free(b_flow);
                break;
            }
        }
        if (!found) {
            _flow_descriptions_free(b_copy);
            return false;
        }
    }

    _flow_descriptions_free(b_copy);
    return true;
}

mb_smf_sc_flow_description_t *_flow_description_new()
{
    return (mb_smf_sc_flow_description_t*)ogs_calloc(1,sizeof(mb_smf_sc_flow_description_t));
}

void _flow_description_free(mb_smf_sc_flow_description_t *flow_description)
{
    if (!flow_description) return;
    _flow_description_clear(flow_description);
    ogs_free(flow_description);
}

void _flow_description_clear(mb_smf_sc_flow_description_t *flow_description)
{
    if (!flow_description) return;
    if (flow_description->string) {
        ogs_free(flow_description->string);
        flow_description->string = NULL;
    }
}

void _flow_description_copy(mb_smf_sc_flow_description_t **dst, const mb_smf_sc_flow_description_t *src)
{
    if (!src || !src->string) {
        if (*dst) {
            _flow_description_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = _flow_description_new();
    } else {
        _flow_description_clear(*dst);
    }

    (*dst)->string = ogs_strdup(src->string);
}

bool _flow_description_equal(const mb_smf_sc_flow_description_t *a, const mb_smf_sc_flow_description_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    if (!a->string != !b->string) return false;
    if (a->string && strcmp(a->string, b->string)) return false;

    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
