#ifndef _MB_SMF_GEOGRAPHIC_COORDINATE_H_
#define _MB_SMF_GEOGRAPHIC_COORDINATE_H_
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

/* Data types */

/** @defgroup geographic_coordinate_class Geographic Coordinate management
 * @{
 */

/** Geographic Coordinate
 *
 * Expressed as a longitude and latitude
 */
typedef struct mb_smf_sc_geographic_coordinate_s {
    ogs_lnode_t node; /**< Can be used in an ogs_list_t list */
    double longitude; /**< Longitude in degrees between -180 and 180 */
    double latitude;  /**< Latitude in degrees -90 to 90 */
} mb_smf_sc_geographic_coordinate_t;

/* mb_smf_sc_geographic_coordinate Type functions */

/** Create an empty Geographic Coordinate
 * @memberof mb_smf_sc_geographic_coordinate_s
 * @static
 * @public
 *
 * Creates a geographic coordinate with longitude 0 degrees and latitude 0 degrees.
 *
 * @return A new, empty, Geographic Coordinate.
 */
MB_SMF_CLIENT_API mb_smf_sc_geographic_coordinate_t *mb_smf_sc_geographic_coordinate_new();

/** Create a Geographic Coordinate using longitude and latitude
 * @memberof mb_smf_sc_geographic_coordinate_s
 * @static
 * @public
 *
 * The coordinates will be rationalised to (-180 .. 180] for longitude and [-90 .. 90] for latitude.
 *
 * @param longitude The longitude for the coordinate.
 * @param latitude The latitude for the coordinate.
 *
 * @return A new Geographic Coordinate using the longitude and latitude given by @p longitude and @p latitude.
 */
MB_SMF_CLIENT_API mb_smf_sc_geographic_coordinate_t *mb_smf_sc_geographic_coordinate_new_values(double longitude, double latitude);

/** Destroy an Geographic Coordinate
 * @memberof mb_smf_sc_geographic_coordinate_s
 * @public
 *
 * @param coord The Geographic Coordinate to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_geographic_coordinate_delete(mb_smf_sc_geographic_coordinate_t *coord);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_GEOGRAPHIC_COORDINATE_H_ */
