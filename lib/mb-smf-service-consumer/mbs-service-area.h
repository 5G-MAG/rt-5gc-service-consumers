#ifndef _MB_SMF_MBS_SERVICE_AREA_H_
#define _MB_SMF_MBS_SERVICE_AREA_H_
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
typedef struct mb_smf_sc_ncgi_tai_s mb_smf_sc_ncgi_tai_t;
typedef struct mb_smf_sc_tai_s mb_smf_sc_tai_t;

/* Data types */

/** @defgroup mbs_service_area_class MBS Service Area management
 * @{
 */

/** NR NTN Intended Service Area (TS 29.571 clause 5.9.4.4, IntendedServiceArea)
 *
 * The coordinates are the value of the Intended Service Area Coordinates IE of TS 38.413 clause 9.3.1.273, the
 * areaCoordinates of the IntendedServiceAreaInfo of TS 38.331. They are held as received and written to the SBI as
 * base64. The MB-SMF and the AMF transfer them without interpreting them.
 */
typedef struct mb_smf_sc_intended_service_area_s {
    ogs_lnode_t node;
    uint8_t *coordinates;     /**< The coordinates octets (owned) */
    size_t coordinates_len;   /**< The number of octets in @a coordinates */
} mb_smf_sc_intended_service_area_t;

/** MBS Service Area
 *
 * This is the union of a list of NCGI TAI structures and a list of TAI structures. For NR NTN it can also carry NR NTN
 * Intended Service Areas, which only further restrict that union in the NG-RAN.
 */
typedef struct mb_smf_sc_mbs_service_area_s {
    ogs_list_t ncgi_tais; /**< List of mb_smf_sc_ncgi_tai_t objects */
    ogs_list_t tais;      /**< List of mb_smf_sc_tai_t objects */
    ogs_list_t intended_service_areas; /**< List of mb_smf_sc_intended_service_area_t objects */
} mb_smf_sc_mbs_service_area_t;

/* mb_smf_sc_mbs_service_area Type functions */

/** Create an empty MBS Service Area
 * @memberof mb_smf_sc_mbs_service_area_s
 * @static
 * @public
 *
 * Creates an emtpy MBS Service Area with no NCGI TAIs or TAIs listed.
 *
 * @return A new, empty, MBS Service Area.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_service_area_t *mb_smf_sc_mbs_service_area_new();

/** Create an MBS Service Area using an NCGI TAI
 * @memberof mb_smf_sc_mbs_service_area_s
 * @static
 * @public
 *
 * @param ncgi_tai The NCGI TAI to copy as the first NCGI TAIs list entry.
 *
 * @return A new MBS Service Area copying the NCGI TAI in @p ncgi_tai as the first entry in the NCGI TAIs list.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_service_area_t *mb_smf_sc_mbs_service_area_new_ncgi_tai(const mb_smf_sc_ncgi_tai_t *ncgi_tai);

/** Create an MBS Service Area using an TAI
 * @memberof mb_smf_sc_mbs_service_area_s
 * @static
 * @public
 *
 * @param tai The TAI to copy as the first TAIs list entry.
 *
 * @return A new MBS Service Area copying the TAI in @p tai as the first entry in the TAIs list.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_service_area_t *mb_smf_sc_mbs_service_area_new_tai(const mb_smf_sc_tai_t *tai);

/** Add an NR NTN Intended Service Area to an MBS Service Area
 * @memberof mb_smf_sc_mbs_service_area_s
 * @public
 *
 * @param mbs_service_area The MBS Service Area to add to.
 * @param coordinates The octets of the Intended Service Area Coordinates IE (TS 38.413 clause 9.3.1.273), copied.
 * @param coordinates_len The number of octets in @p coordinates (at least one).
 *
 * @return true if the Intended Service Area was added, false if an argument is invalid or memory ran out.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_mbs_service_area_add_intended_service_area(mb_smf_sc_mbs_service_area_t *mbs_service_area, const uint8_t *coordinates, size_t coordinates_len);

/** Destroy an MBS Service Area
 * @memberof mb_smf_sc_mbs_service_area_s
 * @public
 *
 * @param mbs_service_area The MBS Service Area to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_mbs_service_area_delete(mb_smf_sc_mbs_service_area_t *mbs_service_area);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_MBS_SERVICE_AREA_H_ */
