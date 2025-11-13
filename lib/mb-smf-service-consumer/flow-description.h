#ifndef _MB_SMF_FLOW_DESCRIPTION_H_
#define _MB_SMF_FLOW_DESCRIPTION_H_
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

/** @defgroup flow_description_class Flow Description management
 * @{
 */

/** Flow Description
 */
typedef struct mb_smf_sc_flow_description_s {
    ogs_lnode_t node;
    char *string;
} mb_smf_sc_flow_description_t;

/* mb_smf_sc_flow_description Type functions */

/** Create an empty Flow Description
 * @memberof mb_smf_sc_flow_description_s
 * @static
 * @public
 *
 * @return A new, empty, Flow Description.
 */
MB_SMF_CLIENT_API mb_smf_sc_flow_description_t *mb_smf_sc_flow_description_new();

/** Destroy a Flow Description
 * @memberof mb_smf_sc_flow_description_s
 * @public
 *
 * @param flow_desc The Flow Description to destroy.
 */
MB_SMF_CLIENT_API void mb_smf_sc_flow_description_delete(mb_smf_sc_flow_description_t *flow_desc);

/**@}*/

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_FLOW_DESCRIPTION_H_ */
