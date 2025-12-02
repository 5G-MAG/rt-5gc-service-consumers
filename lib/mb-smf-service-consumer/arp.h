#ifndef _MB_SMF_ARP_H_
#define _MB_SMF_ARP_H_
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

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */

/* Data types */

/** @defgroup arp_class ARP management
 * @{
 */

/** Preemption Capability enumeration
 */
typedef enum {
    ARP_PREEMPT_CAPABILITY_NULL = 0,    /**< No Preemption Capability */
    ARP_PREEMPT_CAPABILITY_NOT_PREEMPT, /**< Must not preempt */
    ARP_PREEMPT_CAPABILITY_MAY_PREEMPT  /**< May preempt */
} mb_smf_sc_preemption_capability_e;

/** Preemption Vulnerability enumeration
 */
typedef enum {
    ARP_PREEMPT_VULNERABILITY_NULL = 0,        /**< No Preemption Vulnerability */
    ARP_PREEMPT_VULNERABILITY_NOT_PREEMPTABLE, /**< Is not preemptable */
    ARP_PREEMPT_VULNERABILITY_PREEMPTABLE      /**< Is preemptable */
} mb_smf_sc_preemption_vulnerability_e;

/** ARP
 */
typedef struct mb_smf_sc_arp_s {
    uint8_t priority_level;                                        /**< Priority level between 1 and 15 inclusive*/
    mb_smf_sc_preemption_capability_e preemption_capability;       /**< Preemption capability */
    mb_smf_sc_preemption_vulnerability_e preemption_vulnerability; /**< Preemption vulnerability */
} mb_smf_sc_arp_t;

/* mb_smf_sc_arp Type functions */

/** Create an empty ARP
 * @memberof mb_smf_sc_arp_s
 * @static
 * @public
 *
 * @return A new, empty, ARP.
 */
MB_SMF_CLIENT_API mb_smf_sc_arp_t *mb_smf_sc_arp_new();

/** Destroy an ARP
 * @memberof mb_smf_sc_arp_s
 * @public
 *
 * @param arp The ARP to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_arp_delete(mb_smf_sc_arp_t *arp);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_ARP_H_ */
