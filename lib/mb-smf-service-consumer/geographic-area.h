#ifndef _MB_SMF_GEOGRAPHIC_AREA_H_
#define _MB_SMF_GEOGRAPHIC_AREA_H_
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
#include "geographic-coordinate.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct mb_smf_sc_geographic_area_s mb_smf_sc_geographic_area_t;

/* Data types */

/** @defgroup geographic_area_class Geographic Area management
 * @{
 */

typedef enum {
    GEOGRAPHIC_AREA_SHAPE_NONE = 0,                              /**< No geographic area */
    GEOGRAPHIC_AREA_SHAPE_POINT,                                 /**< Single point */
    GEOGRAPHIC_AREA_SHAPE_POINT_UNCERTAINTY_CIRCLE,              /**< Point with uncertainty circle */
    GEOGRAPHIC_AREA_SHAPE_POINT_UNCERTAINTY_ELLIPSE,             /**< Point with uncertainty ellipse */
    GEOGRAPHIC_AREA_SHAPE_POLYGON,                               /**< Geographic area described by a polygon */
    GEOGRAPHIC_AREA_SHAPE_POINT_ALTITUDE,                        /**< Point with altitude */
    GEOGRAPHIC_AREA_SHAPE_POINT_ALTITUDE_UNCERTAINTY,            /**< Point with altitude and uncertainty ellipsoid */
    GEOGRAPHIC_AREA_SHAPE_ELLIPSOID_ARC,                         /**< Ellipsoid arc around a point */
    GEOGRAPHIC_AREA_SHAPE_LOCAL_2D_POINT_UNCERTAINTY_ELLIPSE,    /**< Relative Point with uncertainty ellipse */
    GEOGRAPHIC_AREA_SHAPE_LOCAL_3D_POINT_UNCERTAINTY_ELLIPSOID/*,*/ /**< Relative Point with altitude and uncertainty ellipsoid */
    /* GEOGRAPHIC_AREA_SHAPE_DISTANCE_DIRECTION, */
    /* GEOGRAPHIC_AREA_SHAPE_RELATIVE_2D_LOCATION_UNCERTAINTY_ELLIPSE, */
    /* GEOGRAPHIC_AREA_SHAPE_RELATIVE_3D_LOCATION_UNCERTAINTY_ELLIPSOID */
} mb_smf_sc_geographic_area_shape_e;

/** Geographic Area
 */
struct mb_smf_sc_geographic_area_s {
    ogs_lnode_t node;
    mb_smf_sc_geographic_area_shape_e shape;
    union {
        mb_smf_sc_geographic_coordinate_t point;
        struct {
            mb_smf_sc_geographic_coordinate_t point;
            float uncertainty; /* 0 or more */
        } point_uncertainty_circle;
        struct {
            mb_smf_sc_geographic_coordinate_t point;
            float semi_major; /* 0 or more */
            float semi_minor; /* 0 or more */
            uint8_t orientation_major; /* 0 <= orientation_major <= 180 */
            uint8_t confidence; /* 0 <= confidence <= 100 */
        } point_uncertainty_ellipse;
        struct {
            ogs_list_t points; /* list of 3 to 15 mb_smf_sc_geographic_coordinate_t */
        } polygon;
        struct {
            mb_smf_sc_geographic_coordinate_t point;
            double altitude; /* -32767 <= altitude <= 32767 */
        } point_altitude;
        struct {
            mb_smf_sc_geographic_coordinate_t point;
            double altitude; /* -32767 <= altitude <= 32767 */
            float semi_major; /* 0 or more */
            float semi_minor; /* 0 or more */
            uint8_t orientation_major; /* 0 <= orientation_major <= 180 */
            float uncertainty_altitude; /* 0 or more */
            uint8_t confidence; /* 0 <= confidence <= 100 */
            uint8_t *v_confidence; /* NULL or point to 0 <= *v_confidence <= 100 */
        } point_altitude_uncertainty;
        struct {
            mb_smf_sc_geographic_coordinate_t point;
            uint32_t inner_radius; /* 0 <= inner_radius <= 327675 */
            float uncertainty_radius; /* 0 or more */
            uint16_t offset_angle; /* degrees 0 <= offset_angle <= 360 */
            uint16_t included_angle; /* degrees 0 <= included_angle <= 360 */
            uint8_t confidence; /* 0 <= confidence <= 100 */
        } ellipsoid_arc;
        struct {
            struct {
                char *coordinate_id;
                mb_smf_sc_geographic_coordinate_t *point;
                struct mb_smf_sc_geographic_area_s *area;
                uint16_t horiz_axes_orientation; /* clockwise 0.1 degrees: 0 <= horiz_axes_orientation <= 3600 */
            } local_origin;
            struct {
                float x;
                float y;
                float *z;
            } point;
            float semi_major; /* 0 or more */
            float semi_minor; /* 0 or more */
            uint8_t orientation_major; /* 0 <= orientation_major <= 180 */
            uint8_t confidence; /* 0 <= confidence <= 100 */
        } local_2d_point_uncertainty_ellipse;
        struct {
            struct {
                char *coordinate_id;
                mb_smf_sc_geographic_coordinate_t *point;
                struct mb_smf_sc_geographic_area_s *area;
                uint16_t horiz_axes_orientation; /* clockwise 0.1 degrees: 0 <= horiz_axes_orientation <= 3600 */
            } local_origin;
            struct {
                float x;
                float y;
                float *z;
            } point;
            float semi_major; /* 0 or more */
            float semi_minor; /* 0 or more */
            uint8_t orientation_major; /* 0 <= orientation_major <= 180 */
            uint8_t confidence; /* 0 <= confidence <= 100 */
            uint8_t *v_confidence; /* NULL or 0 <= *v_confidence <= 100 */
        } local_3d_point_uncertainty_ellipse;
    };
};

/* mb_smf_sc_geographic_area Type functions */

/** Create a new point Geographic Area
 * @memberof mb_smf_sc_geographic_area_s
 * @static
 * @public
 *
 * @param longitude The longitude for the point.
 * @param latitude The latitude for the point..
 *
 * @return A new Point Geographic Area.
 */
MB_SMF_CLIENT_API mb_smf_sc_geographic_area_t *mb_smf_sc_ga_point_new(double longitude, double latitude);

/** Create a point with uncertainty circle Geographic Area
 * @memberof mb_smf_sc_geographic_area_s
 * @static
 * @public
 *
 * @param longitude The longitude for the point.
 * @param latitude The latitude for the point.
 * @param uncertainty The radius of the uncertainty circle (0 or more).
 *
 * @return A new Point with uncertainty circle Geographic Area.
 */
MB_SMF_CLIENT_API mb_smf_sc_geographic_area_t *mb_smf_sc_ga_point_uncertainty_circle_new(double longitude, double latitude, float uncertainty);

/** Create a point with uncertainty ellipse Geographic Area
 * @memberof mb_smf_sc_geographic_area_s
 * @static
 * @public
 *
 * @param orientation_major Degrees, 0 to 180.
 * @param confidence Percent, 0 to 100.
 *
 * @return A new Point with uncertainty ellipse Geographic Area.
 */
MB_SMF_CLIENT_API mb_smf_sc_geographic_area_t *mb_smf_sc_ga_point_uncertainty_ellipse_new(double longitude, double latitude, float semi_major, float semi_minor, uint8_t orientation_major, uint8_t confidence);

/** Create an empty polygon Geographic Area; add its 3 to 15 points with mb_smf_sc_ga_polygon_add_point()
 * @memberof mb_smf_sc_geographic_area_s
 * @static
 * @public
 */
MB_SMF_CLIENT_API mb_smf_sc_geographic_area_t *mb_smf_sc_ga_polygon_new();

/** Add a point to a polygon Geographic Area
 * @memberof mb_smf_sc_geographic_area_s
 * @public
 *
 * @return true if the area is a polygon and the point was added.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_ga_polygon_add_point(mb_smf_sc_geographic_area_t *area, double longitude, double latitude);

/** Create a point with altitude Geographic Area
 * @memberof mb_smf_sc_geographic_area_s
 * @static
 * @public
 *
 * @param altitude Metres, -32767 to 32767.
 */
MB_SMF_CLIENT_API mb_smf_sc_geographic_area_t *mb_smf_sc_ga_point_altitude_new(double longitude, double latitude, double altitude);

/** Create a point with altitude and uncertainty ellipsoid Geographic Area
 * @memberof mb_smf_sc_geographic_area_s
 * @static
 * @public
 *
 * @param v_confidence NULL, or the vertical confidence in percent (0 to 100).
 */
MB_SMF_CLIENT_API mb_smf_sc_geographic_area_t *mb_smf_sc_ga_point_altitude_uncertainty_new(double longitude, double latitude, double altitude, float semi_major, float semi_minor, uint8_t orientation_major, float uncertainty_altitude, uint8_t confidence, const uint8_t *v_confidence);

/** Create an ellipsoid arc Geographic Area
 * @memberof mb_smf_sc_geographic_area_s
 * @static
 * @public
 */
MB_SMF_CLIENT_API mb_smf_sc_geographic_area_t *mb_smf_sc_ga_ellipsoid_arc_new(double longitude, double latitude, uint32_t inner_radius, float uncertainty_radius, uint16_t offset_angle, uint16_t included_angle, uint8_t confidence);

/** Destroy a Geographic Area
 * @memberof mb_smf_sc_geographic_area_s
 * @public
 *
 * @param area The Geographic Area to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_geographic_area_delete(mb_smf_sc_geographic_area_t *area);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_GEOGRAPHIC_AREA_H_ */
