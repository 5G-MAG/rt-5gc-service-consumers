#ifndef _MB_SMF_MBS_TMGI_H_
#define _MB_SMF_MBS_TMGI_H_
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <time.h>

#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */

/* Data types */
/** @defgroup tmgi_class TMGI management
 * @{
 */

/** TMGI
 *
 * This holds the public members of a TMGI object. Do not set any of the parameters directly, please use the setter methods.
 */
typedef struct mb_smf_sc_tmgi_s {
    char *mbs_service_id;  /**< The MBS-Service-ID for this TMGI */
    ogs_plmn_id_t plmn;    /**< The PLMN for this TMGI */
    time_t expiry_time;    /**< The expiry time for this TMGI */
} mb_smf_sc_tmgi_t;

/** TMGI result callback
 *
 * @param tmgi The TMGI the callback refers to.
 * @param result `OGS_OK` on successful allocation or re-allocation of a TMGI, `OGS_TIMEDOUT` if the request timed out and
 *               `OGS_ERROR` for other general errors.
 * @param problem_details If a ProblemDetails is available for the error or `NULL` if not available.
 * @param data The value passed as @p callback_data in the call to mb_smf_sc_tmgi_create().
 *
 * @see mb_smf_sc_tmgi_create()
 */
typedef void (*mb_smf_sc_tmgi_result_cb)(mb_smf_sc_tmgi_t *tmgi, int result,
                                                const OpenAPI_problem_details_t *problem_details, void *data);

/** TMGI allocation callback (backward compatibility)
 * @deprecated
 * 
 * This is maintained for backward compatibility. New code should use mb_smf_sc_tmgi_result_cb instead.
 *
 * @see mb_smf_sc_tmgi_result_cb
 */
typedef mb_smf_sc_tmgi_result_cb mb_smf_sc_tmgi_create_result_cb;

/* mb_smf_sc_tmgi Type functions */

/** Allocate a TMGI
 * @memberof mb_smf_sc_tmgi_s
 * @static
 * @public
 *
 * This will send a request to the MB-SMF for a new TMGI. The result of the request will be reported back via the @p callback
 * function.
 *
 * @param callback The callback to use to report the result of the new allocation.
 * @param callback_data The pointer to pass to @p callback in the @p data parameter.
 *
 * @see mb_smf_sc_tmgi_result_cb
 */
MB_SMF_CLIENT_API mb_smf_sc_tmgi_t *mb_smf_sc_tmgi_create(mb_smf_sc_tmgi_result_cb callback, void *callback_data);

/** Create a new TMGI
 * @memberof mb_smf_sc_tmgi_s
 * @static
 * @public
 *
 * This creates a new empty TMGI that has no @ref mb_smf_sc_tmgi_s::mbs_service_id "mbs_service_id" and a PLMN of 000-00.
 *
 * @return A new, empty, TMGI.
 */
MB_SMF_CLIENT_API mb_smf_sc_tmgi_t *mb_smf_sc_tmgi_new();

/** Destroy a TMGI
 * @memberof mb_smf_sc_tmgi_s
 * @public
 *
 * This destroys the TMGI and frees the resources associated with it. This will call mb_smf_sc_tmgi_send_deallocate() if the TMGI
 * has been allocated previously.
 *
 * @param tmgi The TMGI to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_tmgi_free(mb_smf_sc_tmgi_t *tmgi);

/** Compare two TMGIs for equality
 * @memberof mb_smf_sc_tmgi_s
 * @public
 *
 * Checks if TMGIs @p a and @p b have equal values.
 *
 * @param a The first TMGI to use in the comparison.
 * @param b The second TMGI to use in the comparison.
 *
 * @return `true` if the two TMGIs have the same value.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_tmgi_equal(const mb_smf_sc_tmgi_t *a, const mb_smf_sc_tmgi_t *b);

/** Set the MBS Service ID
 * @memberof mb_smf_sc_tmgi_s
 * @public
 *
 * Sets the MBS Service ID to the value given in @p mbs_service_id.
 *
 * @param tmgi The TMGI to set the MBS Service ID on.
 * @param mbs_service_id The MBS Service ID to set.
 *
 * @return @p tmgi.
 */
MB_SMF_CLIENT_API mb_smf_sc_tmgi_t *mb_smf_sc_tmgi_set_mbs_service_id(mb_smf_sc_tmgi_t *tmgi, const char *mbs_service_id);

/** Set the PLMN ID
 * @memberof mb_smf_sc_tmgi_s
 * @public
 *
 * Sets the PLMN ID to the value given by @p mcc and @p mnc.
 *
 * @param tmgi The TMGI to set the PLMN ID on.
 * @param mcc The PLMN MCC value to use.
 * @param mnc The PLMN MNC value to use.
 *
 * @return @p tmgi.
 */
MB_SMF_CLIENT_API mb_smf_sc_tmgi_t *mb_smf_sc_tmgi_set_plmn(mb_smf_sc_tmgi_t *tmgi, uint16_t mcc, uint16_t mnc);

/** Set the expiration time
 * @memberof mb_smf_sc_tmgi_s
 * @public
 *
 * Sets the TMGI expiration time to the value in @p expiry_time.
 *
 * @param tmgi The TMGI to set the expiration time on.
 * @param expiry_time The expiration time to set.
 *
 * @return @p tmgi.
 */
MB_SMF_CLIENT_API mb_smf_sc_tmgi_t *mb_smf_sc_tmgi_set_expiry_time(mb_smf_sc_tmgi_t *tmgi, time_t expiry_time);

/** Set the callback function for operation results
 * @memberof mb_smf_sc_tmgi_s
 * @public
 *
 * Sets the TMGI callback function for results of create, refresh or delete operations
 * @param tmgi The TMGI to set the callback for.
 * @param callback The callback to use to report the result of the new allocation.
 * @param callback_data The pointer to pass to @p callback in the @p data parameter.
 *
 * @return @p tmgi.
 */
MB_SMF_CLIENT_API mb_smf_sc_tmgi_t *mb_smf_sc_tmgi_set_callback(mb_smf_sc_tmgi_t *tmgi,
                                                                mb_smf_sc_tmgi_result_cb callback, void *callback_data);

/** Get a string representation of the TMGI
 * @memberof mb_smf_sc_tmgi_s
 * @public
 *
 * This is useful for debug messages to report the TMGI value(s).
 *
 * @param tmgi The TMGI to get the representation of.
 *
 * @return A string representation of the TMGI value.
 */
MB_SMF_CLIENT_API const char *mb_smf_sc_tmgi_repr(mb_smf_sc_tmgi_t *tmgi);

/** Send an allocate request for a TMGI to the MB-SMF
 * @memberof mb_smf_sc_tmgi_s
 * @public
 *
 * This triggers the sending of a TMGI request to the MB-SMF. This is done automatically, to allocate a new TMGI, by the
 * mb_smf_sc_tmgi_create() method. If called on an existing TMGI, or one created with mb_smf_sc_tmgi_new(), it will try to refresh
 * the expiration time of the TMGI.
 *
 * @param tmgi The TMGI to send an allocate request for.
 */
MB_SMF_CLIENT_API void mb_smf_sc_tmgi_send_allocate(mb_smf_sc_tmgi_t *tmgi);

/** Send a deallocate request for a TMGI to the MB-SMF
 * @memberof mb_smf_sc_tmgi_s
 * @public
 *
 * This triggers the sending of a TMGI deallocation request to the MB-SMF.
 *
 * @param tmgi The TMGI to deallocate.
 */
MB_SMF_CLIENT_API void mb_smf_sc_tmgi_send_deallocate(mb_smf_sc_tmgi_t *tmgi);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_MBS_TMGI_H_ */
