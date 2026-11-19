/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2022 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

/* strptime() and timegm() for the Retry-After header field */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <ctype.h>
#include <limits.h>
#include <time.h>

#include "ogs-core.h"
#include "ogs-sbi.h"

#include "pcf-service-consumer.h"

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library Internals */
char *_sockaddr_to_string(const ogs_sockaddr_t *addr)
{
    return ogs_ipstrdup((ogs_sockaddr_t*)addr); /* safe to remove const as OGS routines just use it read-only */
}

char *_pcf_authority(const ogs_sockaddr_t *addr)
{
    char host[OGS_ADDRSTRLEN];
    ogs_sockaddr_t *address = (ogs_sockaddr_t *)addr; /* only read by the Open5GS macros */

    if (!addr) return NULL;
    OGS_ADDR(address, host);
    return addr->ogs_sa_family == AF_INET6 ? ogs_msprintf("[%s]:%d", host, OGS_PORT(address)) : ogs_msprintf("%s:%d", host, OGS_PORT(address));
}

/* RFC 3986 clause 5.2.4: the dot segments of an absolute path are removed (in place) */
static void remove_dot_segments(char *path)
{
    char *out = path, *in = path;

    while (*in) {
        char *next = strchr(in + 1, '/');
        size_t len = next ? (size_t)(next - in) : strlen(in);

        if (len == 2 && in[0] == '/' && in[1] == '.') {
            /* "/." is dropped (a path that ends with it keeps its slash) */
            if (!next) *out++ = '/';
        } else if (len == 3 && in[0] == '/' && in[1] == '.' && in[2] == '.') {
            while (out > path && *--out != '/');
            if (!next) *out++ = '/';
        } else {
            memmove(out, in, len);
            out += len;
        }
        in += len;
    }
    *out = '\0';
}

char *_location_path(const char *location, const char *request_path)
{
    char *copy, *path, *end, *result;

    if (!location || !*location) return NULL;
    copy = ogs_strdup(location);
    if ((end = strpbrk(copy, "?#")) != NULL) *end = '\0';
    if (strstr(copy, "://") != NULL || (copy[0] == '/' && copy[1] == '/')) {
        /* absolute URI, or a network-path reference: the path follows the authority */
        char *authority = strstr(copy, "//") + 2;

        path = strchr(authority, '/');
        if (!path) { ogs_free(copy); return NULL; }
        result = ogs_strdup(path);
    } else if (copy[0] == '/') {
        result = ogs_strdup(copy);
    } else {
        /* a relative reference: merged with the path of the request (RFC 3986 clause 5.2.3) */
        char *slash = request_path ? strrchr(request_path, '/') : NULL;
        size_t dir = slash ? (size_t)(slash - request_path) + 1 : 1;

        result = ogs_calloc(1, dir + strlen(copy) + 2);
        if (slash) memcpy(result, request_path, dir); else result[0] = '/';
        strcat(result, copy);
    }
    ogs_free(copy);
    remove_dot_segments(result);
    /* the identifier of the resource is the last segment: a trailing slash does not name another one */
    end = result + strlen(result);
    while (end > result + 1 && end[-1] == '/') *--end = '\0';
    return result;
}

int _retry_after_seconds(const char *value, time_t now)
{
    if (!value) return -1;
    while (*value == ' ' || *value == '\t') value++;
    if (!*value) return -1;
    if (isdigit((unsigned char)*value)) {
        long long seconds = 0;

        for (; isdigit((unsigned char)*value); value++) {
            seconds = seconds * 10 + (*value - '0');
            if (seconds > INT_MAX) seconds = INT_MAX;
        }
        while (*value == ' ' || *value == '\t') value++;
        return *value ? -1 : (int)seconds;
    } else {
        struct tm tm;
        const char *end;
        long long delta;

        memset(&tm, 0, sizeof(tm));
        end = strptime(value, "%a, %d %b %Y %H:%M:%S GMT", &tm);
        if (!end || *end) return -1;
        delta = (long long)timegm(&tm) - (long long)now;
        return delta < 0 ? 0 : (delta > INT_MAX ? INT_MAX : (int)delta);
    }
}

int events_notification_to_events_mask(const OpenAPI_events_notification_t *events_notif)
{
    int events_mask = 0;

    if (events_notif) {
        OpenAPI_lnode_t *node;
        OpenAPI_list_for_each(events_notif->ev_notifs, node) {
            OpenAPI_af_event_notification_t *af_event_notif = (OpenAPI_af_event_notification_t*)node->data;
            events_mask |= _npcf_af_event_to_event_mask(af_event_notif->event);
        }
    }

    return events_mask;
}

int events_subsc_req_data_to_events_mask(const OpenAPI_events_subsc_req_data_t *evt_subsc_req)
{
    int events_mask = 0;
    OpenAPI_lnode_t *node;

    if (evt_subsc_req) {
        OpenAPI_list_for_each(evt_subsc_req->events, node) {
            OpenAPI_af_event_subscription_t *af_event_subsc = (OpenAPI_af_event_subscription_t*)node->data;
            events_mask |= _npcf_af_event_to_event_mask(af_event_subsc->event);
        }
    }

    return events_mask;
}

int _npcf_af_event_to_event_mask(const OpenAPI_npcf_af_event_e event_type)
{
    switch (event_type) {
    case OpenAPI_npcf_af_event_ACCESS_TYPE_CHANGE:
        return PCF_APP_SESSION_EVENT_TYPE_ACCESS_TYPE_CHANGE;
    case OpenAPI_npcf_af_event_ANI_REPORT:
        return PCF_APP_SESSION_EVENT_TYPE_ANI_REPORT;
    case OpenAPI_npcf_af_event_APP_DETECTION:
        return PCF_APP_SESSION_EVENT_TYPE_APP_DETECTION;
    case OpenAPI_npcf_af_event_CHARGING_CORRELATION:
        return PCF_APP_SESSION_EVENT_TYPE_CHARGING_CORRELATION;
    case OpenAPI_npcf_af_event_EPS_FALLBACK:
        return PCF_APP_SESSION_EVENT_TYPE_EPS_FALLBACK;
    case OpenAPI_npcf_af_event_FAILED_QOS_UPDATE:
        return PCF_APP_SESSION_EVENT_TYPE_FAILED_QOS_UPDATE;
    case OpenAPI_npcf_af_event_FAILED_RESOURCES_ALLOCATION:
        return PCF_APP_SESSION_EVENT_TYPE_FAILED_RESOURCES_ALLOCATION;
    case OpenAPI_npcf_af_event_OUT_OF_CREDIT:
        return PCF_APP_SESSION_EVENT_TYPE_OUT_OF_CREDIT;
    case OpenAPI_npcf_af_event_PDU_SESSION_STATUS:
        return PCF_APP_SESSION_EVENT_TYPE_PDU_SESSION_STATUS;
    case OpenAPI_npcf_af_event_PLMN_CHG:
        return PCF_APP_SESSION_EVENT_TYPE_PLMN_CHG;
    case OpenAPI_npcf_af_event_QOS_MONITORING:
        return PCF_APP_SESSION_EVENT_TYPE_QOS_MONITORING;
    case OpenAPI_npcf_af_event_QOS_NOTIF:
        return PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF;
    case OpenAPI_npcf_af_event_RAN_NAS_CAUSE:
        return PCF_APP_SESSION_EVENT_TYPE_RAN_NAS_CAUSE;
    case OpenAPI_npcf_af_event_REALLOCATION_OF_CREDIT:
        return PCF_APP_SESSION_EVENT_TYPE_REALLOCATION_OF_CREDIT;
    case OpenAPI_npcf_af_event_SAT_CATEGORY_CHG:
        return PCF_APP_SESSION_EVENT_TYPE_SAT_CATEGORY_CHG;
    case OpenAPI_npcf_af_event_SUCCESSFUL_QOS_UPDATE:
        return PCF_APP_SESSION_EVENT_TYPE_SUCCESSFUL_QOS_UPDATE;
    case OpenAPI_npcf_af_event_SUCCESSFUL_RESOURCES_ALLOCATION:
        return PCF_APP_SESSION_EVENT_TYPE_SUCCESSFUL_RESOURCES_ALLOCATION;
    case OpenAPI_npcf_af_event_TSN_BRIDGE_INFO:
        return PCF_APP_SESSION_EVENT_TYPE_TSN_BRIDGE_INFO;
    case OpenAPI_npcf_af_event_UP_PATH_CHG_FAILURE:
        return PCF_APP_SESSION_EVENT_TYPE_UP_PATH_CHG_FAILURE;
    case OpenAPI_npcf_af_event_USAGE_REPORT:
        return PCF_APP_SESSION_EVENT_TYPE_USAGE_REPORT;
    case OpenAPI_npcf_af_event_L4S_SUPP:
        return PCF_APP_SESSION_EVENT_TYPE_L4S_SUPP;
    default:
        break;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
