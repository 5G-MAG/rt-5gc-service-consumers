#ifndef _MB_SMF_MBS_QOS_REQ_H_
#define _MB_SMF_MBS_QOS_REQ_H_
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
typedef struct mb_smf_sc_arp_s mb_smf_sc_arp_t;

/* Data types */

/** @defgroup mbs_qos_req_class MBS QoS Requirements management
 * @{
 */

/** MBS QoS Requirements
 */
typedef struct mb_smf_sc_mbs_qos_req_s {
    uint8_t five_qi;
    uint64_t *guarenteed_bit_rate;
    uint64_t *max_bit_rate;
    uint16_t *averaging_window; /* NULL or 1 <= aver_window <= 4095 (default assumed when NULL = 2000) */
    mb_smf_sc_arp_t *req_mbs_arp;
} mb_smf_sc_mbs_qos_req_t;

/* mb_smf_sc_mbs_qos_req Type functions */

/** Create an empty MBS QoS Requirements
 * @memberof mb_smf_sc_mbs_qos_req_s
 * @static
 * @public
 *
 * @return A new, empty, MBS QoS Requirements.
 */
MB_SMF_CLIENT_API mb_smf_sc_mbs_qos_req_t *mb_smf_sc_mbs_qos_req_new();

/** Destroy an MBS QoS Requirements
 * @memberof mb_smf_sc_mbs_qos_req_s
 * @public
 *
 * @param qos_req The MBS QoS Requirements to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_mbs_qos_req_delete(mb_smf_sc_mbs_qos_req_t *qos_req);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_MBS_QOS_REQ_H_ */
