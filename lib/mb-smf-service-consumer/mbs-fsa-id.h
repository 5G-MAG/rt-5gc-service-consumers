#ifndef _MB_SMF_MBS_FSA_ID_H_
#define _MB_SMF_MBS_FSA_ID_H_
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

#include "macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */

/* Data types */

/** @defgroup mbs_fsa_id_class MBS FSA Id management
 * @{
 */

/** MBS FSA Id
 */
typedef struct mb_smf_sc_mbs_fsa_id_s {
    ogs_lnode_t node;
    uint32_t id; /* 0 <= id <= 16777215 */
} mb_smf_sc_mbs_fsa_id_t;

/* mb_smf_sc_mbs_fsa_id Type functions */

/** Create an empty MBS FSA Id
 * @memberof mb_smf_sc_mbs_fsa_id_s
 * @static
 * @public
 *
 * @return A new, empty, MBS FSA Id.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_fsa_id_t *mb_smf_sc_mbs_fsa_id_new();

/** Destroy an MBS FSA Id
 * @memberof mb_smf_sc_mbs_fsa_id_s
 * @public
 *
 * @param fsa_id The MBS FSA Id to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_mbs_fsa_id_delete(mb_smf_sc_mbs_fsa_id_t *fsa_id);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_MBS_FSA_ID_H_ */
