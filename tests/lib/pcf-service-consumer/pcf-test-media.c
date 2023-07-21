/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2022 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#include "pcf-test-media.h"

OpenAPI_list_t *media_component_create(pcf_npcf_policyauthorization_param_t *pcf_param)
{
    OpenAPI_list_t *MediaComponentList = NULL;
    OpenAPI_map_t *MediaComponentMap = NULL;
    OpenAPI_media_component_t *MediaComponent = NULL;

    OpenAPI_list_t *SubComponentList = NULL;
    OpenAPI_map_t *SubComponentMap = NULL;
    OpenAPI_media_sub_component_t *SubComponent = NULL;

    OpenAPI_list_t *fDescList = NULL;
    OpenAPI_list_t *codecList = NULL;

    OpenAPI_lnode_t *node = NULL, *node2 = NULL, *node3 = NULL;

    int i = 0, j = 0;

    ogs_assert(pcf_param);
    ogs_assert(pcf_param->med_type);

    MediaComponentList = OpenAPI_list_create();
    ogs_assert(MediaComponentList);

    MediaComponent = ogs_calloc(1, sizeof(*MediaComponent));
    ogs_assert(MediaComponent);

    MediaComponent->med_comp_n = (i++);
    MediaComponent->f_status = OpenAPI_flow_status_ENABLED;
    MediaComponent->med_type = pcf_param->med_type;
    if (pcf_param->qos_type == 1) {
        MediaComponent->mar_bw_dl = ogs_sbi_bitrate_to_string(
                                        96000, OGS_SBI_BITRATE_BPS);
        MediaComponent->mar_bw_ul = ogs_sbi_bitrate_to_string(
                                        96000, OGS_SBI_BITRATE_BPS);
        MediaComponent->rr_bw = ogs_sbi_bitrate_to_string(
                                        2400, OGS_SBI_BITRATE_BPS);
        MediaComponent->rs_bw = ogs_sbi_bitrate_to_string(
                                        2400, OGS_SBI_BITRATE_BPS);
    } else if (pcf_param->qos_type == 2) {
        MediaComponent->mar_bw_dl = ogs_sbi_bitrate_to_string(
                                        96000, OGS_SBI_BITRATE_BPS);
        MediaComponent->mar_bw_ul = ogs_sbi_bitrate_to_string(
                                        96000, OGS_SBI_BITRATE_BPS);
        MediaComponent->rr_bw = ogs_sbi_bitrate_to_string(
                                        88000, OGS_SBI_BITRATE_BPS);
        MediaComponent->rs_bw = ogs_sbi_bitrate_to_string(
                                        88000, OGS_SBI_BITRATE_BPS);
    }
    

    /* Codec */
    codecList = OpenAPI_list_create();
    ogs_assert(codecList);
    OpenAPI_list_add(codecList,
        ogs_strdup("downlink\noffer\n"
            "m=audio 49000 RTP/AVP 116 99 97 105 100\r\nb=AS:41\r\n"
            "b=RS:512\r\nb=RR:1537\r\na=maxptime:240\r\n"
            "a=des:qos mandatory local sendrecv\r\na=curr:qos local none\r\n"
            "a=des:qos option"));
    OpenAPI_list_add(codecList,
        ogs_strdup("uplink\nanswer\nm=audio 50020 RTP/AVP 99 105\r\n"
            "b=AS:41\r\nb=RS:600\r\nb=RR:2000\r\na=rtpmap:99 AMR-WB/16000/1\r\n"
            "a=fmtp:99 mode-change-capability=2;max-red=0\r\n"
            "a=rtpmap:105 telephone-event/16"));
    ogs_assert(codecList->count);
    MediaComponent->codecs = codecList;

    MediaComponentMap = OpenAPI_map_create(
            ogs_msprintf("%d", MediaComponent->med_comp_n), MediaComponent);
    ogs_assert(MediaComponentMap);
    ogs_assert(MediaComponentMap->key);

    OpenAPI_list_add(MediaComponentList, MediaComponentMap);

    /* Sub Component */
    SubComponentList = OpenAPI_list_create();
    ogs_assert(SubComponentList);

    /* Sub Component #1 */
    SubComponent = ogs_calloc(1, sizeof(*SubComponent));
    ogs_assert(SubComponent);

    SubComponent->f_num = (j++);
    SubComponent->flow_usage = OpenAPI_flow_usage_NO_INFO;

    SubComponentMap = OpenAPI_map_create(
            ogs_msprintf("%d", SubComponent->f_num), SubComponent);
    ogs_assert(SubComponentMap);
    ogs_assert(SubComponentMap->key);

    OpenAPI_list_add(SubComponentList, SubComponentMap);

    /* Flow Description */
    fDescList = OpenAPI_list_create();
    ogs_assert(fDescList);
    if (pcf_param->flow_type == 0) {
        /* Nothing */
    } else if (pcf_param->flow_type == 1) {
        OpenAPI_list_add(fDescList,
            ogs_strdup("permit out 17 from 172.20.166.84 to 10.45.0.2 20001"));
        OpenAPI_list_add(fDescList,
            ogs_strdup("permit in 17 from 10.45.0.2 to 172.20.166.84 20360"));
    } else if (pcf_param->flow_type == 2) {
        OpenAPI_list_add(fDescList,
            ogs_strdup("permit out 17 from 172.20.166.84 to 10.45.0.2 30001"));
        OpenAPI_list_add(fDescList,
            ogs_strdup("permit in 17 from 10.45.0.2 to 172.20.166.84 30360"));
    } else if (pcf_param->flow_type == 99) {
        OpenAPI_list_add(fDescList,
            ogs_strdup("permit out icmp from any to any"));
        OpenAPI_list_add(fDescList,
            ogs_strdup("permit in icmp from any to any"));
    } else {
        ogs_assert_if_reached();
    }
    if (fDescList->count)
        SubComponent->f_descs = fDescList;
    else
        OpenAPI_list_free(fDescList);

    /* Sub Component #2 */
    SubComponent = ogs_calloc(1, sizeof(*SubComponent));
    ogs_assert(SubComponent);

    SubComponent->f_num = (j++);
    SubComponent->flow_usage = OpenAPI_flow_usage_RTCP;

    SubComponentMap = OpenAPI_map_create(
            ogs_msprintf("%d", SubComponent->f_num), SubComponent);
    ogs_assert(SubComponentMap);
    ogs_assert(SubComponentMap->key);

    OpenAPI_list_add(SubComponentList, SubComponentMap);

    /* Flow Description */
    fDescList = OpenAPI_list_create();
    ogs_assert(fDescList);
    if (pcf_param->flow_type == 0) {
        /* Nothing */
    } else if (pcf_param->flow_type == 1 || pcf_param->flow_type == 99) {
        OpenAPI_list_add(fDescList,
            ogs_strdup("permit out 17 from 172.20.166.84 to 10.45.0.2 20002"));
        OpenAPI_list_add(fDescList,
            ogs_strdup("permit in 17 from 10.45.0.2 to 172.20.166.84 20361"));
    } else if (pcf_param->flow_type == 2) {
        OpenAPI_list_add(fDescList,
            ogs_strdup("permit out 17 from 172.20.166.84 to 10.45.0.2 30002"));
        OpenAPI_list_add(fDescList,
            ogs_strdup("permit in 17 from 10.45.0.2 to 172.20.166.84 30361"));
    } else {
        ogs_assert_if_reached();
    }

    if (fDescList->count)
        SubComponent->f_descs = fDescList;
    else
        OpenAPI_list_free(fDescList);

    ogs_assert(SubComponentList->count);
    MediaComponent->med_sub_comps = SubComponentList;

    ogs_assert(MediaComponentList->count);

    return MediaComponentList;

}