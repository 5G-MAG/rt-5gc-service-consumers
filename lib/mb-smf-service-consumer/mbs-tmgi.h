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
typedef struct mb_smf_sc_tmgi_s {
    char *mbs_service_id;
    ogs_plmn_id_t plmn;
    time_t expiry_time;
} mb_smf_sc_tmgi_t;

typedef void (*mb_smf_sc_tmgi_create_result_cb)(mb_smf_sc_tmgi_t *tmgi, int result,
                                                const OpenAPI_problem_details_t *problem_details, void *data);

/* mb_smf_sc_tmgi Type functions */
MB_SMF_CLIENT_API void mb_smf_sc_tmgi_create(mb_smf_sc_tmgi_create_result_cb callback, void *callback_data);
MB_SMF_CLIENT_API void mb_smf_sc_tmgi_free(mb_smf_sc_tmgi_t *tmgi);
MB_SMF_CLIENT_API bool mb_smf_sc_tmgi_equal(const mb_smf_sc_tmgi_t *a, const mb_smf_sc_tmgi_t *b);

MB_SMF_CLIENT_API mb_smf_sc_tmgi_t *mb_smf_sc_tmgi_set_mbs_service_id(mb_smf_sc_tmgi_t *tmgi, const char *mbs_service_id);
MB_SMF_CLIENT_API mb_smf_sc_tmgi_t *mb_smf_sc_tmgi_set_plmn(mb_smf_sc_tmgi_t *tmgi, uint16_t mcc, uint16_t mnc);
MB_SMF_CLIENT_API mb_smf_sc_tmgi_t *mb_smf_sc_tmgi_set_expiry_time(mb_smf_sc_tmgi_t *tmgi, time_t expiry_time);

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* _MB_SMF_MBS_TMGI_H_ */
