#ifndef _MB_SMF_REF_COUNT_SBI_OBJECT_H_
#define _MB_SMF_REF_COUNT_SBI_OBJECT_H_
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <stdint.h>

#include "ogs-core.h"
#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Data types */
typedef struct _ref_count_sbi_object_s {
    size_t ref_count;
    ogs_sbi_object_t sbi_object;
} _ref_count_sbi_object_t;

/* internal library functions */
static inline _ref_count_sbi_object_t *_ref_count_sbi_object_new()
{
    _ref_count_sbi_object_t *obj = (_ref_count_sbi_object_t*)ogs_calloc(1, sizeof(*obj));
    obj->ref_count++;
    return obj;
}

static inline void _ref_count_sbi_object_unref(_ref_count_sbi_object_t *ref_obj)
{
    if (ref_obj) {
        ref_obj->ref_count--;
        if (!ref_obj->ref_count) ogs_free(ref_obj);
    }
}

static inline _ref_count_sbi_object_t *_ref_count_sbi_object_ref(_ref_count_sbi_object_t *ref_obj)
{
    if (ref_obj) ref_obj->ref_count++;
    return ref_obj;
}

static inline ogs_sbi_object_t *_ref_count_sbi_object_ptr(_ref_count_sbi_object_t *ref_obj)
{
    return &ref_obj->sbi_object;
}

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_REF_COUNT_SBI_OBJECT_H_ */
