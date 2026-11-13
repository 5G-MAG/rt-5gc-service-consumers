#ifndef _MB_SMF_EXT_MBS_SERVICE_AREA_H_
#define _MB_SMF_EXT_MBS_SERVICE_AREA_H_
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

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct mb_smf_sc_geographic_area_s mb_smf_sc_geographic_area_t;
typedef struct mb_smf_sc_civic_address_s mb_smf_sc_civic_address_t;

/* Data types */

/** @defgroup ext_mbs_service_area_class External MBS Service Area management
 * @{
 */

/** External MBS Service Area
 *
 * The external MBS Service area is the union of the @a geographic_areas and @a civic_addresses lists.
 */
typedef struct mb_smf_sc_ext_mbs_service_area_s {
    ogs_list_t geographic_areas; /**< The list of mb_smf_sc_geographic_area_t objects */
    ogs_list_t civic_addresses;  /**< The list of mb_smf_sc_civic_address_t objects */
} mb_smf_sc_ext_mbs_service_area_t;

/* mb_smf_sc_ext_mbs_service_area Type functions */

/** Create an empty External MBS Service Area
 * @memberof mb_smf_sc_ext_mbs_service_area_s
 * @static
 * @public
 *
 * @return A new, empty, External MBS Service Area.
 */
MB_SMF_CLIENT_API mb_smf_sc_ext_mbs_service_area_t *mb_smf_sc_ext_mbs_service_area_new();

/** Destroy an External MBS Service Area
 * @memberof mb_smf_sc_ext_mbs_service_area_s
 * @public
 *
 * @param area The External MBS Service Area to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_ext_mbs_service_area_delete(mb_smf_sc_ext_mbs_service_area_t *area);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_EXT_MBS_SERVICE_AREA_H_ */
