#ifndef _MB_SMF_MBS_TAI_H_
#define _MB_SMF_MBS_TAI_H_
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
#include "ogs-proto.h"

#include "macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */

/* Data types */
/** @defgroup tai_class TAI management
 * @{
 */

/** TAI
 *
 * TAI structure
 */
typedef struct mb_smf_sc_tai_s {
    ogs_lnode_t node;      /**< This can be part of an ogs_list_t list */
    ogs_plmn_id_t plmn_id; /**< The mandatory PLMN Id */
    uint32_t tac : 24;     /**< The mandatory TAC, 16-bit or 24-bit unsigned */
    uint64_t *nid;         /**< Optional Network ID, `NULL` to omit or pointer to 44-bit unsigned */
} mb_smf_sc_tai_t;

/* mb_smf_sc_tai Type functions */

/** Create a new TAI from values
 * @memberof mb_smf_sc_tai_s
 * @static
 * @public
 *
 * Create a new TAI with the given PLMN and TAC. The Network ID will be set to a copy of @p nid if it is not NULL.
 *
 * @param mcc The PLMN Id MCC for this TAI.
 * @param mnc The PLMN Id MNC for this TAI.
 * @param tac The Tac for this TAI.
 * @param nid The Network Id to copy or NULL to leave unset.
 *
 * @return A new TAI object with the PLMN Id and TAC set.
 */
MB_SMF_CLIENT_API mb_smf_sc_tai_t *mb_smf_sc_tai_new(uint16_t mcc, uint16_t mnc, uint32_t tac, const uint64_t *nid);

/** Create a new TAI as a copy
 * @memberof mb_smf_sc_tai_s
 * @static
 * @public
 *
 * This creates a new TAI from an existing TAI.
 *
 * @param other The TAI to copy.
 *
 * @return A new TAI which is a copy of @p other..
 */
MB_SMF_CLIENT_API mb_smf_sc_tai_t *mb_smf_sc_tai_new_copy(const mb_smf_sc_tai_t *other);

/** Destroy a TAI
 * @memberof mb_smf_sc_tai_s
 * @public
 *
 * This destroys the TAI and frees the resources associated with it.
 *
 * @param tai The TAI to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_tai_free(mb_smf_sc_tai_t *tai);

/** Compare two TAIs for equality
 * @memberof mb_smf_sc_tai_s
 * @public
 *
 * Checks if TAIs @p a and @p b have equal values.
 *
 * @param a The first TAI to use in the comparison.
 * @param b The second TAI to use in the comparison.
 *
 * @return `true` if the two TAIs have the same value.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_tai_equal(const mb_smf_sc_tai_t *a, const mb_smf_sc_tai_t *b);

/** In place copy operator for TAIs
 * @memberof mb_smf_sc_tai_s
 * @public
 *
 * Copies the value from @p src into @p dst. This will free any resources previously associated with @p dst that are no longer
 * needed. If @p src is `NULL` then @p dst will be freed. If @p src is not `NULL` then a new TAI will be created if @p dst is
 * `NULL`.
 *
 * @param dst A pointer to the destination TAI.
 * @param src The source TAI to copy.
 *
 * @return The new TAI object which is a copy of @p src or `NULL` if @p src was `NULL`.
 */
MB_SMF_CLIENT_API mb_smf_sc_tai_t *mb_smf_sc_tai_copy(mb_smf_sc_tai_t *dst, const mb_smf_sc_tai_t *src);

/** Set the PLMN ID
 * @memberof mb_smf_sc_tai_s
 * @public
 *
 * Sets the PLMN ID to the value given by @p mcc and @p mnc.
 *
 * @param tai The TAI to set the PLMN ID on.
 * @param mcc The PLMN MCC value to use.
 * @param mnc The PLMN MNC value to use.
 *
 * @return @p tai.
 */
MB_SMF_CLIENT_API mb_smf_sc_tai_t *mb_smf_sc_tai_set_plmn(mb_smf_sc_tai_t *tai, uint16_t mcc, uint16_t mnc);

/** Set the TAC
 * @memberof mb_smf_sc_tai_s
 * @public
 *
 * Sets the TAI TAC to the value in @p tac.
 *
 * @param tai The TAI to set the expiration time on.
 * @param tac The TAC to set.
 *
 * @return @p tai.
 */
MB_SMF_CLIENT_API mb_smf_sc_tai_t *mb_smf_sc_tai_set_tac(mb_smf_sc_tai_t *tai, uint32_t tac);

/** Set the Network Id
 * @memberof mb_smf_sc_tai_s
 * @public
 *
 * Sets the TAI Network Id.
 *
 * @param tai The TAI to set the Network Id for.
 * @param nid The Network Id to set in the TAI..
 *
 * @return @p tai.
 */
MB_SMF_CLIENT_API mb_smf_sc_tai_t *mb_smf_sc_tai_set_network_id(mb_smf_sc_tai_t *tai, uint64_t nid);

/** Unset the Network Id
 * @memberof mb_smf_sc_tai_s
 * @public
 *
 * Unsets the TAI Network Id.
 *
 * @param tai The TAI to unset the Network Id for.
 *
 * @return @p tai.
 */
MB_SMF_CLIENT_API mb_smf_sc_tai_t *mb_smf_sc_tai_unset_network_id(mb_smf_sc_tai_t *tai);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_MBS_TAI_H_ */
