#ifndef _MB_SMF_MBS_SERVICE_INFO_H_
#define _MB_SMF_MBS_SERVICE_INFO_H_
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

/** @defgroup mbs_service_info_class MBS Service Info management
 * @{
 */

/** MBS Service Info
 */
typedef struct mb_smf_sc_mbs_service_info_s {
    ogs_hash_t *mbs_media_comps;      /**< Media Components, must be at least one entry */
    char *af_app_id;                  /**< AF Application Id */
    uint64_t *mbs_session_ambr;       /**< `NULL` or pointer to MBS Service Info AMBR as a bit rate */
    uint8_t mbs_sdf_reserve_priority; /**< MBS SDF Reserve Priority (1-16 inclusive or 0 to unset) */
} mb_smf_sc_mbs_service_info_t;

/* mb_smf_sc_mbs_service_info Type functions */

/** Create an empty MBS Service Info
 * @memberof mb_smf_sc_mbs_service_info_s
 * @static
 * @public
 *
 * @return A new, empty, MBS Service Info.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_service_info_t *mb_smf_sc_mbs_service_info_new();

/** Destroy an MBS Service Info
 * @memberof mb_smf_sc_mbs_service_info_s
 * @public
 *
 * @param service_info The MBS Service Info to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_mbs_service_info_delete(mb_smf_sc_mbs_service_info_t *service_info);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_MBS_SERVICE_INFO_H_ */
