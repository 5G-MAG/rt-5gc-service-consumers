#ifndef _MB_SMF_MBS_MEDIA_INFO_H_
#define _MB_SMF_MBS_MEDIA_INFO_H_
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

/* Data types */

/** @defgroup mbs_media_info_class MBS Media Info management
 * @{
 */

/** MBS Media Type enumeration
 */
typedef enum {
    MEDIA_TYPE_NULL = 0,    /**< No media type */
    MEDIA_TYPE_AUDIO,       /**< Audio media */
    MEDIA_TYPE_VIDEO,       /**< Video media */
    MEDIA_TYPE_DATA,        /**< Data */
    MEDIA_TYPE_APPLICATION, /**< Aplication specific */
    MEDIA_TYPE_CONTROL,     /**< Control signals */
    MEDIA_TYPE_TEXT,        /**< Text */
    MEDIA_TYPE_MESSAGE,     /**< Message */
    MEDIA_TYPE_OTHER        /**< Other media type */
} mb_smf_sc_mbs_media_type_e;

/** MBS Media Info
 */
typedef struct mb_smf_sc_mbs_media_info_s {
    mb_smf_sc_mbs_media_type_e mbs_media_type;      /**< The media type */
    uint64_t *max_requested_mbs_bandwidth_downlink; /**< The maximum guaranteed bit rate requested for downlink traffic */
    uint64_t *min_requested_mbs_bandwidth_downlink; /**< The minimum guaranteed bit rate requested for downlink traffic */
    char *codecs[2];                                /**< The CODEC or CODECs used */
} mb_smf_sc_mbs_media_info_t;

/* mb_smf_sc_mbs_media_info Type functions */

/** Create an empty MBS Media Info
 * @memberof mb_smf_sc_mbs_media_info_s
 * @static
 * @public
 *
 * @return A new, empty, MBS Media Info.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_media_info_t *mb_smf_sc_mbs_media_info_new();

/** Destroy an MBS Media Info
 * @memberof mb_smf_sc_mbs_media_info_s
 * @public
 *
 * @param media_info The MBS Media Info to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_mbs_media_info_delete(mb_smf_sc_mbs_media_info_t *media_info);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_MBS_MEDIA_INFO_H_ */
