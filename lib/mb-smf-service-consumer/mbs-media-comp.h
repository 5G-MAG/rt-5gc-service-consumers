#ifndef _MB_SMF_MBS_MEDIA_COMP_H_
#define _MB_SMF_MBS_MEDIA_COMP_H_
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

#include "macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct ogs_list_s ogs_list_t;
typedef struct mb_smf_sc_mbs_media_info_s mb_smf_sc_mbs_media_info_t;
typedef struct mb_smf_sc_mbs_qos_req_s mb_smf_sc_mbs_qos_req_t;

/* Data types */

/** @defgroup mbs_media_comp_class MBS Media Component management
 * @{
 */

/** MBS Media Component
 *
 * The @a media_info, @a qos_ref and @a mbs_qos_req fields are mutally exclusive. If more than one is present then the precedence
 * is @a media_info then @a qos_ref then @a mbs_qos_req.
 */
typedef struct mb_smf_sc_mbs_media_comp_s {
    int id;                                 /**< Component Id */
    ogs_list_t *flow_descriptions;          /**< List of mb_smf_sc_flow_description_t objects */
    uint8_t mbs_sdf_reserve_priority;       /**< MBS SDF Reserve Priority (1-16 inclusive or 0 to unset) */
    mb_smf_sc_mbs_media_info_t *media_info; /**< MBS Media Info */
    char *qos_ref;                          /**< QoS Reference */
    mb_smf_sc_mbs_qos_req_t *mbs_qos_req;   /**< MBS QoS Requirments */
} mb_smf_sc_mbs_media_comp_t;

/* mb_smf_sc_mbs_media_comp Type functions */

/** Create an empty MBS Media Component
 * @memberof mb_smf_sc_mbs_media_comp_s
 * @static
 * @public
 *
 * @return A new, empty, MBS Media Component.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_media_comp_t *mb_smf_sc_mbs_media_comp_new();

/** Destroy an MBS Media Component
 * @memberof mb_smf_sc_mbs_media_comp_s
 * @public
 *
 * @param media_comp The MBS Media Component to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_mbs_media_comp_delete(mb_smf_sc_mbs_media_comp_t *media_comp);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_MBS_MEDIA_COMP_H_ */
