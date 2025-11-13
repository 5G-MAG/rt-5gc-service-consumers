#ifndef _MB_SMF_CIVIC_ADDRESS_H_
#define _MB_SMF_CIVIC_ADDRESS_H_
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

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */

/* Data types */

/** @defgroup civic_address_class Civic Address management
 * @{
 */

/** Civic Address
 */
typedef struct mb_smf_sc_civic_address_s {
    ogs_lnode_t node;
    char *country;
    char *a[6];
    char *prd;
    char *pod;
    char *sts;
    char *hno;
    char *hns;
    char *lmk;
    char *loc;
    char *nam;
    char *pc;
    char *bld;
    char *unit;
    char *flr;
    char *room;
    char *plc;
    char *pcn;
    char *pobox;
    char *addcode;
    char *seat;
    char *rd;
    char *rdsec;
    char *rdbr;
    char *rdsubbr;
    char *prm;
    char *pom;
    char *usage_rules;
    char *method;
    char *provided_by;
} mb_smf_sc_civic_address_t;

/* mb_smf_sc_civic_address Type functions */

/** Create an empty Civic Address
 * @memberof mb_smf_sc_civic_address_s
 * @static
 * @public
 *
 * @return A new, empty, Civic Address.
 */
MB_SMF_CLIENT_API mb_smf_sc_civic_address_t *mb_smf_sc_civic_address_new();

/** Destroy an Civic Address
 * @memberof mb_smf_sc_civic_address_s
 * @public
 *
 * @param address The Civic Address to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_civic_address_delete(mb_smf_sc_civic_address_t *address);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_CIVIC_ADDRESS_H_ */
