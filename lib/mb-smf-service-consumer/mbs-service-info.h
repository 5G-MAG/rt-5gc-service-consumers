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

typedef struct mb_smf_sc_mbs_media_comp_s mb_smf_sc_mbs_media_comp_t;

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

/** Set Media Component
 *
 * Sets a Media Component to the component given in @p media_comp. The @a id attribute of @p media_comp will provide the id to
 * store the media comp as. The Media Component will be freed when the MBS Service Info is destroyed or when another MBS Media
 * Component is set with the same @a id value.
 *
 * @param service_info The MBS Service Info to set the MBS Media Comp for.
 * @param media_comp The Media Component to set. This will be deleted when the MBS Service Info is destroyed or when another MBS
 *                   Media Component is set with the same @a id value.
 *
 * @return @p service_info.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_service_info_t *mb_smf_sc_mbs_service_info_set_mbs_media_comp(
                                                                            mb_smf_sc_mbs_service_info_t *service_info,
                                                                            mb_smf_sc_mbs_media_comp_t *media_comp);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_MBS_SERVICE_INFO_H_ */
