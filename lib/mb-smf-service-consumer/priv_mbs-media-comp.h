#ifndef _MB_SMF_SC_PRIV_MBS_MEDIA_COMP_H
#define _MB_SMF_SC_PRIV_MBS_MEDIA_COMP_H
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

#include "mbs-media-comp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library internal mbs_media_comp methods (protected) */
mb_smf_sc_mbs_media_comp_t *_mbs_media_comp_new();
void _mbs_media_comp_free(mb_smf_sc_mbs_media_comp_t *mbs_media_comp);
void _mbs_media_comp_clear(mb_smf_sc_mbs_media_comp_t *mbs_media_comp);
void _mbs_media_comp_copy(mb_smf_sc_mbs_media_comp_t **dst, const mb_smf_sc_mbs_media_comp_t *src);
bool _mbs_media_comp_equal(const mb_smf_sc_mbs_media_comp_t *a, const mb_smf_sc_mbs_media_comp_t *b);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* define _MB_SMF_SC_PRIV_MBS_MEDIA_COMP_H */
