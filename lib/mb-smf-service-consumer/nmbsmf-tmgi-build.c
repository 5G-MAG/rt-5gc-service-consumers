/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-core.h"
#include "ogs-sbi.h"

#include "priv_tmgi.h"

/* Library Internals */
ogs_sbi_request_t *_nmbsmf_tmgi_build_create(void *context, void *data)
{
    ogs_list_t *new_tmgis = (ogs_list_t*)context;
    ogs_list_t *refresh_tmgis = (ogs_list_t*)data;
    ogs_sbi_request_t *req = NULL;

    if (new_tmgis || refresh_tmgis) {
        ogs_sbi_message_t message = {
            .h = {
                .method = OGS_SBI_HTTP_METHOD_POST,
                .service = { .name = OGS_SBI_SERVICE_NAME_NMBSMF_TMGI },
                .api = { .version = OGS_SBI_API_V1 },
                .resource = { .component = { OGS_SBI_RESOURCE_NAME_TMGI } }
            }
        };

        size_t num_of_new_tmgis = 0;
        if (new_tmgis) num_of_new_tmgis = ogs_list_count(new_tmgis);

        OpenAPI_list_t *refresh_list = NULL;

        if (refresh_tmgis) {
            _priv_tmgi_t *node;
            ogs_list_for_each(refresh_tmgis, node) {
                if (!refresh_list) refresh_list = OpenAPI_list_create();
                OpenAPI_list_add(refresh_list, _tmgi_to_openapi_type(node));
            }
        }

        message.TmgiAllocate = OpenAPI_tmgi_allocate_create(num_of_new_tmgis?true:false, num_of_new_tmgis, refresh_list);

        req = ogs_sbi_build_request(&message);

        OpenAPI_tmgi_allocate_free(message.TmgiAllocate);
    }

    return req;
}

ogs_sbi_request_t *_nmbsmf_tmgi_build_remove(void *context, void *data)
{
    ogs_list_t *tmgis = (ogs_list_t*)context;
    ogs_sbi_request_t *req = NULL;

    if (tmgis) {
        ogs_sbi_message_t message = {
            .h = {
                .method = OGS_SBI_HTTP_METHOD_DELETE,
                .service = { .name = OGS_SBI_SERVICE_NAME_NMBSMF_TMGI },
                .api = { .version = OGS_SBI_API_V1 },
                .resource = { .component = { OGS_SBI_RESOURCE_NAME_TMGI } }
            }
        };

        /* tmgi-list query parameter = array(Tmgi) */
        _priv_tmgi_t *node;
        ogs_list_for_each(tmgis, node) {
            if (!message.param.tmgi_list) message.param.tmgi_list = OpenAPI_list_create();
            OpenAPI_list_add(message.param.tmgi_list, _tmgi_to_openapi_type(node));
        }

        req = ogs_sbi_build_request(&message);

        if (message.param.tmgi_list) {
            OpenAPI_lnode_t *api_node;
            OpenAPI_list_for_each(message.param.tmgi_list, api_node) {
                OpenAPI_tmgi_free(api_node->data);
            }
            OpenAPI_list_free(message.param.tmgi_list);
        }
    }

    return req;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
