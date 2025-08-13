/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <stdbool.h>
#include <netdb.h>

#include "ogs-core.h"
#include "ogs-sbi.h"
#include "yuarel.h"

#include "macros.h"
#include "priv_mbs-session.h"
#include "priv_mbs-status-subscription.h"

#include "context.h"

typedef struct _context_s {
    ogs_list_t mbs_sessions; /* item type is _priv_mbs_session_t */
    ogs_hash_t *tmgis;               /* map is plmn => _priv_tmgi_t */
    ogs_sockaddr_t *notification_bind_address;
} _context_t;

static _context_t *__self = NULL;

_context_t *_context_new()
{
    __self = (_context_t*)ogs_calloc(1, sizeof(_context_t));
    return __self;
}

int _context_parse_config(const char *local)
{
    /* parse local config for library settings.    */
    /***********************************************/
    /* We are looking for an entry like:           */
    /* <local>:                                    */
    /*    notifications:                           */
    /*       mbsmf:                                */
    /*          address: 127.0.0.1                 */
    /*          port: 12345                        */
    /*                                             */
    /* Where <local> is the NF name passed in the  */
    /* local function parameter.                   */
    /*                                             */
    /* The address can be any IPv4 or IPv6 address */
    /* or a hostname that is resolvable to an IPv4 */
    /* or IPv6 address. If not present in the YAML */
    /* configuration then the address defaults to  */
    /* 0.0.0.0.                                    */
    /*                                             */
    /* The port number can be any number between 0 */
    /* and 65535 inclusive. A value of 0 indicates */
    /* that an ephemeral port number should be     */
    /* selected at runtime. If not present in the  */
    /* YAML then this defaults to 0.               */
    /***********************************************/
    yaml_document_t *document = NULL;
    ogs_yaml_iter_t root_iter;

    document = ogs_app()->document;
    ogs_assert(document);

    if (!__self) _context_new();

    ogs_yaml_iter_init(&root_iter, document);
    while (ogs_yaml_iter_next(&root_iter)) {
        const char *root_key = ogs_yaml_iter_key(&root_iter);
        ogs_assert(root_key);
        if (!strcmp(root_key, local)) {
            ogs_yaml_iter_t local_iter;
            ogs_yaml_iter_recurse(&root_iter, &local_iter);
            while (ogs_yaml_iter_next(&local_iter)) {
                const char *local_key = ogs_yaml_iter_key(&local_iter);
                ogs_assert(local_key);
                if (!strcmp(local_key, "notifications")) {
                    ogs_yaml_iter_t notif_iter;
                    ogs_yaml_iter_recurse(&local_iter, &notif_iter);
                    while (ogs_yaml_iter_next(&notif_iter)) {
                        const char *notif_key = ogs_yaml_iter_key(&notif_iter);
                        ogs_assert(notif_key);
                        if (!strcmp(notif_key, "mbsmf")) {
                            ogs_yaml_iter_t mbsmf_notif_iter;
                            char *addr = NULL;
                            char *port = NULL;
                            ogs_yaml_iter_recurse(&notif_iter, &mbsmf_notif_iter);
                            while (ogs_yaml_iter_next(&mbsmf_notif_iter)) {
                                const char *mbsmf_notif_key = ogs_yaml_iter_key(&mbsmf_notif_iter);
                                ogs_assert(mbsmf_notif_key);
                                if (!strcmp(mbsmf_notif_key, "address")) {
                                    addr = (char*)ogs_yaml_iter_value(&mbsmf_notif_iter);
                                    if (addr) addr = ogs_strdup(addr);
                                }
                                if (!strcmp(mbsmf_notif_key, "port")) {
                                    port = (char*)ogs_yaml_iter_value(&mbsmf_notif_iter);
                                    if (port) port = ogs_strdup(addr);
                                }
                            }
                            if (addr || port) {
                                uint16_t port_num = 0;
                                if (port) {
                                    unsigned long num;
                                    char *end_ptr = NULL;
                                    num = strtoul(port, &end_ptr, 0);
                                    if (!end_ptr || *end_ptr || num >= 65536) {
                                        ogs_error("Notification port must be a number between 0 and 65535 inclusive");
                                        return OGS_ERROR;
                                    }
                                    port_num = (uint16_t)num;
                                }
                                if (addr) {
                                    if (ogs_getaddrinfo(&__self->notification_bind_address, AF_UNSPEC, addr, port_num,
                                                        AI_V4MAPPED | AI_ADDRCONFIG | AI_PASSIVE) != OGS_OK) {
                                        ogs_error("Could not get notification address for %s", addr);
                                        return OGS_ERROR;
                                    }
                                } else {
                                    /* Use IPv4 any address with the given port number */
                                    if (__self->notification_bind_address) ogs_freeaddrinfo(__self->notification_bind_address);
                                    __self->notification_bind_address = (ogs_sockaddr_t*)ogs_calloc(1, sizeof(ogs_sockaddr_t));
                                    __self->notification_bind_address->ogs_sa_family = AF_INET;
                                    __self->notification_bind_address->ogs_sin_port = port_num;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return OGS_OK;
}

static int __hash_free_tmgi(void *rec, const void *key, int klen, const void *value)
{
    _priv_tmgi_t *tmgi = (_priv_tmgi_t*)value;
    _tmgi_free(tmgi);
    return 1;
}

void _context_destroy()
{
    _priv_mbs_session_t *sess, *next;

    if (!__self) return;

    ogs_list_for_each_safe(&__self->mbs_sessions, next, sess) {
        _context_remove_mbs_session(sess);
    }

    if (__self->tmgis) {
        ogs_hash_do(__hash_free_tmgi, NULL, __self->tmgis);
        ogs_hash_destroy(__self->tmgis);
        __self->tmgis = NULL;
    }

    ogs_free(__self);
    __self = NULL;
}

bool _context_add_mbs_session(_priv_mbs_session_t *session)
{
    if (!__self || !session) return false;

    ogs_list_add(&__self->mbs_sessions, session);

    return true;
}

bool _context_remove_mbs_session(_priv_mbs_session_t *session)
{
    if (!__self || !session) return false;

    ogs_list_remove(&__self->mbs_sessions, session);
    _mbs_session_delete(session);

    return true;
}

ogs_list_t *_context_mbs_sessions()
{
    if (!__self) return NULL;
    return &__self->mbs_sessions;
}

bool _context_active_sessions_exists(_priv_mbs_session_t *session)
{
    _priv_mbs_session_t *sess;

    if (!__self) return false;
    if (!session) return false;

    ogs_list_for_each(&__self->mbs_sessions, sess) {
        if (sess == session) return true;
    }

    return false;
}

_priv_mbs_session_t *_context_sbi_object_to_session(ogs_sbi_object_t *sbi_object)
{
    _priv_mbs_session_t *sess;

    if (__self && sbi_object) {
        ogs_list_for_each(&__self->mbs_sessions, sess) {
            if (_ref_count_sbi_object_ptr(sess->sbi_object) == sbi_object) return sess;
        }
    }

    return NULL;
}

const ogs_sockaddr_t *_context_get_notification_address()
{
    if (!__self) return NULL;
    return __self->notification_bind_address;
}

static int __check_notification_server(void *rec, const void *key, int klen, const void *value)
{
    ogs_sbi_server_t *server = rec;
    const _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public_const(value);
    if (subsc->cache && subsc->cache->notif_server == server) return 0;
    return 1;
}

bool _context_is_notification_server(ogs_sbi_server_t *server)
{
    _priv_mbs_session_t *sess;
    ogs_list_for_each(&__self->mbs_sessions, sess) {
        _priv_mbs_status_subscription_t *subsc;
        ogs_list_for_each(&sess->new_subscriptions, subsc) {
            if (subsc->cache && subsc->cache->notif_server == server) return true;
        }
        if (!ogs_hash_do(__check_notification_server, server, sess->session.subscriptions)) return true;
    }
    return false;
}

typedef struct __mbs_status_subsc_filter_s {
    ogs_sbi_server_t *server;
    const char *url_path;
    _priv_mbs_status_subscription_t *subsc;
} __mbs_status_subsc_filter_t;

static bool __url_path_equal(const char *url, const char *path)
{
    /* Both NULL we will say is equal */
    if (!url && !path) return true;
    /* Either NULL then not equal */
    if (!url || !path) return false;
    /* url and path both set */

    /* remove leading / from URL path */
    if (path[0] == '/') path++;

    /* parse URL to extract and compare paths */
    char *temp_url = ogs_strdup(url);
    struct yuarel parsed_url;
    if (yuarel_parse(&parsed_url, temp_url)) {
        /* parse failed, so no match */
        ogs_free(temp_url);
        return false;
    }

    if (!parsed_url.path) {
        /* empty path in URL */
        ogs_free(temp_url);
        /* matches if comparison path is also empty, otherwise no match */
        return path[0] == '\0';
    }

    /* compare paths */
    bool result = (strcmp(parsed_url.path, path) == 0);
    ogs_free(temp_url);

    return result;
}

static int __filter_subscription(void *rec, const void *key, int klen, const void *value)
{
    __mbs_status_subsc_filter_t *filter = (__mbs_status_subsc_filter_t*)rec;
    _priv_mbs_status_subscription_t *subsc = _priv_mbs_status_subscription_from_public((void*)value);

    if (subsc->cache && subsc->cache->notif_server == filter->server &&
        __url_path_equal(subsc->cache->notif_url, filter->url_path)) {
        filter->subsc = subsc;
        return 0;
    }

    return 1;
}

_priv_mbs_status_subscription_t *_context_find_subscription(ogs_sbi_server_t *server, const char *url_path)
{
    _priv_mbs_session_t *sess;
    ogs_list_for_each(&__self->mbs_sessions, sess) {
        _priv_mbs_status_subscription_t *subsc;
        ogs_list_for_each(&sess->new_subscriptions, subsc) {
            if (subsc->cache && subsc->cache->notif_server == server && __url_path_equal(subsc->cache->notif_url, url_path)) {
                return subsc;
            }
        }
        __mbs_status_subsc_filter_t filter = { .server = server, .url_path = url_path };
        if (!ogs_hash_do(__filter_subscription, &filter, sess->session.subscriptions)) {
            return filter.subsc;
        }
    }
    return NULL;
}

bool _context_add_tmgi(_priv_tmgi_t *tmgi)
{
    if (!tmgi) return false;
    if (!__self->tmgis) __self->tmgis = ogs_hash_make();
    ogs_hash_set(__self->tmgis, &tmgi->tmgi.plmn, sizeof(&tmgi->tmgi.plmn), tmgi);
    return true;
}

bool _context_remove_tmgi(_priv_tmgi_t *tmgi)
{
    if (!tmgi) return false;
    if (!__self->tmgis) return false;
    ogs_hash_set(__self->tmgis, &tmgi->tmgi.plmn, sizeof(&tmgi->tmgi.plmn), NULL);
    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
