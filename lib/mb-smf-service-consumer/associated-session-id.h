#ifndef _MB_SMF_ASSOCIATED_SESSION_ID_H_
#define _MB_SMF_ASSOCIATED_SESSION_ID_H_
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include "macros.h"
#include "ssm-addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */

/* Data types */

/** @defgroup associated_session_id_class Associated Session Id management
 * @{
 */

/** External MBS Service Area
 *
 * This is a placeholder to Rel 18 compatibility.
 */
typedef struct mb_smf_sc_associated_session_id_s {
    mb_smf_sc_ssm_addr_t ssm; /**< The source specific multicast address */
    char *string;             /**< The id string */
} mb_smf_sc_associated_session_id_t;

/* mb_smf_sc_associated_session_id Type functions */

/** Create an empty Associated Session Id
 * @memberof mb_smf_sc_associated_session_id_s
 * @static
 * @public
 *
 * @return A new, empty, Associated Session Id.
 */
MB_SMF_CLIENT_API mb_smf_sc_associated_session_id_t *mb_smf_sc_associated_session_id_new();

/** Destroy an Associated Session Id
 * @memberof mb_smf_sc_associated_session_id_s
 * @public
 *
 * @param session_id The Associated Session Id to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_associated_session_id_delete(mb_smf_sc_associated_session_id_t *session_id);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_ASSOCIATED_SESSION_ID_H_ */
