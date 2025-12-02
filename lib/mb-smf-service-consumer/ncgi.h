#ifndef _MB_SMF_NCGI_H_
#define _MB_SMF_NCGI_H_
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
#include "ogs-proto.h"

#include "macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */

/* Data types */

/** @defgroup ncgi_class NCGI management
 * @{
 */

/** NCGI
 */
typedef struct mb_smf_sc_ncgi_s {
    ogs_lnode_t node;         /**< This can be used in an ogs_list_t list */
    ogs_plmn_id_t plmn_id;    /**< PLMN Id */
    uint64_t nr_cell_id : 36; /**< Nr Cell Id (36-bit unsigned value) */
    uint64_t *nid;            /**< `NULL` or pointer to 44-bit unsigned Network Id value */
} mb_smf_sc_ncgi_t;

/* mb_smf_sc_ncgi Type functions */

/** Create an empty NCGI
 * @memberof mb_smf_sc_ncgi_s
 * @static
 * @public
 *
 * Creates an empty NCGI with PLMN set to 000-00, NR Cell Id set to 0 and no Network Id.
 *
 * @return A new, empty, NCGI.
 */
MB_SMF_CLIENT_API mb_smf_sc_ncgi_t *mb_smf_sc_ncgi_new();

/** Create an NCGI using an IPv4 SSM
 * @memberof mb_smf_sc_ncgi_s
 * @static
 * @public
 *
 * @param mcc The MCC for the PLMN Id.
 * @param mnc The MNC for the PLMN Id.
 * @param nr_cell_id The Cell Id number for the NCGI.
 * @param network_id The optional Network Id for the NCGI. Use `NULL` for no Network Id or a pointer to a 44-bit unsigned integer
 *                   to set the network Id. The value will be copied into the new NCGI.
 *
 * @return A new NCGI with the values provided.
 */
MB_SMF_CLIENT_API mb_smf_sc_ncgi_t *mb_smf_sc_ncgi_new_values(uint16_t mcc, uint16_t mnc, uint64_t nr_cell_id, const uint64_t *network_id);

/** Create a new NCGI as a copy of another NCGI
 * @memberof mb_smf_sc_ncgi_s
 * @static
 * @public
 *
 * @param other The other NCGI to copy.
 *
 * @return A new NCGI which is a copy of @p other.
 */
MB_SMF_CLIENT_API mb_smf_sc_ncgi_t *mb_smf_sc_ncgi_new_copy(const mb_smf_sc_ncgi_t *other);

/** Destroy an NCGI
 * @memberof mb_smf_sc_ncgi_s
 * @public
 *
 * @param ncgi The NCGI to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_ncgi_delete(mb_smf_sc_ncgi_t *ncgi);

/** Set the PLMN Id
 * @memberof mb_smf_sc_ncgi_s
 * @public
 *
 * This sets the PLMN Id that identifies this NCGI.
 *
 * @param ncgi The NCGI to set the PLMN Id for.
 * @param mcc The PLMN MCC to set.
 * @param mnc The PLMN MNC to set.
 *
 * @return @p ncgi.
 */
MB_SMF_CLIENT_API mb_smf_sc_ncgi_t *mb_smf_sc_ncgi_set_plmn_id(mb_smf_sc_ncgi_t *ncgi, uint16_t mcc, uint16_t mnc);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_NCGI_H_ */
