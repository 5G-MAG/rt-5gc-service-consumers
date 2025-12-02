#ifndef _MB_SMF_NCGI_TAI_H_
#define _MB_SMF_NCGI_TAI_H_
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
#include "tai.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct mb_smf_sc_ncgi_s mb_smf_sc_ncgi_t;

/* Data types */

/** @defgroup ncgi_tai_class NCGI TAI management
 * @{
 */

/** NCGI TAI
 */
typedef struct mb_smf_sc_ncgi_tai_s {
    ogs_lnode_t node;     /**< This can be used in an ogs_list_t list */
    mb_smf_sc_tai_t tai;  /**< The TAI */
    ogs_list_t ncgis;     /**< List of mb_smf_sc_ncgi_t objects */
} mb_smf_sc_ncgi_tai_t;

/* mb_smf_sc_ncgi_tai Type functions */

/** Create an empty NCGI TAI
 * @memberof mb_smf_sc_ncgi_tai_s
 * @static
 * @public
 *
 * The new NCGI TAI will contain an empty TAI and empty NCGIs list.
 *
 * @return A new, empty, NCGI TAI.
 */
MB_SMF_CLIENT_API mb_smf_sc_ncgi_tai_t *mb_smf_sc_ncgi_tai_new();

/** Create an NCGI TAI using a TAI and single NCGI
 * @memberof mb_smf_sc_ncgi_tai_s
 * @static
 * @public
 *
 * This creates a new NCGI TAI with a copy of @p tai and the NCGIs list containing a copy of @p ncgi.
 *
 * @param tai The TAI to use for this new NCGI TAI.
 * @param ncgi The NCGI to use for the first NCGIs list entry for this new NCGI TAI.
 *
 * @return A new NCGI TAI using the TAI and NCGI given by @p tai and @p ncgi.
 */
MB_SMF_CLIENT_API mb_smf_sc_ncgi_tai_t *mb_smf_sc_ncgi_tai_new_values(const mb_smf_sc_tai_t *tai, const mb_smf_sc_ncgi_t *ncgi);

/** Copy an NCGI TAI as a new NCGI TAI
 * @memberof mb_smf_sc_ncgi_tai_s
 * @static
 * @public
 *
 * @param other The NCGI TAI entry to copy.
 *
 * @return A new NCGI TAI as a copy of the @p other NCGI TAI.
 */
MB_SMF_CLIENT_API mb_smf_sc_ncgi_tai_t *mb_smf_sc_ncgi_tai_new_copy(const mb_smf_sc_ncgi_tai_t *other);

/** Destroy an NCGI TAI
 * @memberof mb_smf_sc_ncgi_tai_s
 * @public
 *
 * @param ncgi_tai The NCGI TAI to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_ncgi_tai_delete(mb_smf_sc_ncgi_tai_t *ncgi_tai);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_NCGI_TAI_H_ */
