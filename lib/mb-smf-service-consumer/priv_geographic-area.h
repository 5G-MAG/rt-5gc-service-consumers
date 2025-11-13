#ifndef _MB_SMF_SC_PRIV_GEOGRAPHIC_AREA_H
#define _MB_SMF_SC_PRIV_GEOGRAPHIC_AREA_H
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

#include "geographic-area.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library internal geographic_area methods (protected) */
mb_smf_sc_geographic_area_t *_geographic_area_new();
void _geographic_area_free(mb_smf_sc_geographic_area_t *geographic_area);
void _geographic_area_clear(mb_smf_sc_geographic_area_t *geographic_area);
void _geographic_area_copy(mb_smf_sc_geographic_area_t **dst, const mb_smf_sc_geographic_area_t *src);
bool _geographic_area_equal(const mb_smf_sc_geographic_area_t *a, const mb_smf_sc_geographic_area_t *b);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_GEOGRAPHIC_AREA_H */
