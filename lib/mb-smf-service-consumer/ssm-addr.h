#ifndef _MB_SMF_SSM_ADDR_H_
#define _MB_SMF_SSM_ADDR_H_
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <netinet/in.h>

#include "macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */

/* Data types */

/** @defgroup ssm_addr_class SSM Address management
 * @{
 */

/** SSM Address
 *
 * Object for storing an SSM address.
 */
typedef struct mb_smf_sc_ssm_addr_s {
    int family;               /**< The AF family for the SSM address, either AF_INET or AF_INET6. */
    /** SSM Source Address
     * The source address for the SSM using the address family given by the @ref mb_smf_sc_ssm_addr_s::family "family" member.
     */
    union {
        struct in_addr ipv4;  /**< The IPv4 source address if @ref mb_smf_sc_ssm_addr_s::family "family" is AF_INET. */
        struct in6_addr ipv6; /**< The IPv6 source address if @ref mb_smf_sc_ssm_addr_s::family "family" is AF_INET6. */
    } source;
    /** SSM Destination Address
     * The destination address for the SSM using the address family given by the @ref mb_smf_sc_ssm_addr_s::family "family" member.
     */
    union {
        struct in_addr ipv4;  /**< The IPv4 destination address if @ref mb_smf_sc_ssm_addr_s::family "family" is AF_INET. */
        struct in6_addr ipv6; /**< The IPv6 destination address if @ref mb_smf_sc_ssm_addr_s::family "family" is AF_INET6. */
    } dest_mc;
} mb_smf_sc_ssm_addr_t;

/* mb_smf_sc_ssm Type functions */

/** SSM equality
 * @memberof mb_smf_sc_ssm_addr_s
 * @public
 *
 * Check if SSM @p a is equal to SSM @p b.
 *
 * @param a The first SSM to compare.
 * @param b The second SSM to compare.
 * @return `true` if SSMs @p a and @p b contain the same SSM value.
 */
MB_SMF_CLIENT_API bool mb_smf_sc_ssm_equal(const mb_smf_sc_ssm_addr_t *a, const mb_smf_sc_ssm_addr_t *b);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_SSM_ADDR_H_ */
