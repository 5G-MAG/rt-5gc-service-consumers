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
    ogs_lnode_t node;           /**< This type can be placed into an ogs_list_t list. */
    char *country;              /**< The country for the address */
    char *a[6];                 /**< The address lines: 0=State/County, 1=Province, 2=City/Town, 3=Borough, 4=Neighbourhood, 5=Group of streets */
    char *prd;                  /**< Street direction prefix, e.g. "N." */
    char *pod;                  /**< Street trailing suffix, e.g. "SW" */
    char *sts;                  /**< Street suffix (type), e.g. "Ave" */
    char *hno;                  /**< House number, e.g. "123" */
    char *hns;                  /**< House number suffix, e.g. "A" */
    char *lmk;                  /**< Landmark or vanity address, e.g. "Columbia University" */
    char *loc;                  /**< Additional location information, e.g. "South Wing" */
    char *nam;                  /**< Name, e.g. "Joe Bloggs" */
    char *pc;                   /**< Postal code, e.g. "AB12 3CD" */
    char *bld;                  /**< Building, e.g. "Library" */
    char *unit;                 /**< Unit, e.g. "Appt. 12" */
    char *flr;                  /**< Floor, e.g. "4" */
    char *room;                 /**< Room, e.g. "B505" */
    char *plc;                  /**< Type of place, e.g. "office" */
    char *pcn;                  /**< Postal community name, e.g. "Leonia" */
    char *pobox;                /**< Post office box, e.g. "12345" */
    char *addcode;              /**< Additional code, e.g. "13203000003" */
    char *seat;                 /**< Seat/desk/cubicle/workstation, e.g. "WS 181" */
    char *rd;                   /**< Primary road name, e.g. "Broadway" */
    char *rdsec;                /**< Road section, e.g. "14" */
    char *rdbr;                 /**< Branch road name, e.g. "Lane 7" */
    char *rdsubbr;              /**< Sub-branch road name, e.g. "Alley 8" */
    char *prm;                  /**< Street name pre-modifier, e.g. "Old" */
    char *pom;                  /**< Street name post-modifier, e.g. "Service" */
    char *usage_rules;          /**< XML Usage Rules element contents, see RFC 4119 section 2.2.2 */
    char *method;               /**< Method by which the address was obtained, e.g. "GPS" */
    char *provided_by;          /**< Who provided the address, e.g. "Provider Name" */
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
