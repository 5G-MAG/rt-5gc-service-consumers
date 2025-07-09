/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2022 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#include "ogs-core.h"

#include "context.h"
#include "pcf-client.h"
#include "pcf-client-sess.h"

static pcf_context_t *self = NULL;

int __pcf_log_domain = 0;

static int pcf_context_prepare(void);
static void pcf_notification_listener_add(const char *notification_listener_addr, const int port);
static void pcf_notification_listener_remove_all(void);
static void pcf_notification_listener_remove(pcf_notification_listener_t *pcf_notification_listen);

void pcf_context_init(void)
{
    ogs_assert(self == NULL);
    /* Initialize PCF library context */
    self = ogs_calloc(1, sizeof(pcf_context_t));
    ogs_assert(self);
    if (!__pcf_log_domain) {
	ogs_log_install_domain(&__pcf_log_domain, "pcf-service-consumer", ogs_core()->log.level);
	ogs_log_config_domain("pcf-service-consumer", ogs_app()->logger.level);
    }
    ogs_list_init(&self->config.pcf_notification_listener_list);
    ogs_list_init(&self->pcf_sessions);
}

void pcf_context_final(void)
{
    ogs_assert(self);
    pcf_notification_listener_remove_all();
    pcf_session_remove_all();
    ogs_free(self);
    self = NULL;
}

pcf_context_t *pcf_self(void)
{
    if (!self) {
	pcf_context_init();
    }
    return self;
}

static int pcf_context_prepare(void)
{
    return OGS_OK;
}

static int pcf_client_context_validation(void)
{
    if(ogs_list_first(&self->config.pcf_notification_listener_list) == 0) {
	    ogs_error("No notification listener address");
		return OGS_ERROR;	  
	}	     
    
    return OGS_OK;
}

bool pcf_parse_config(const char *local)
{
    int rv;
    yaml_document_t *document = NULL;
    ogs_yaml_iter_t root_iter;

    document = ogs_app()->document;
    ogs_assert(document);

    rv = pcf_context_prepare();
    if (rv != OGS_OK) return rv;

    ogs_yaml_iter_init(&root_iter, document);
    while (ogs_yaml_iter_next(&root_iter)) {
        const char *root_key = ogs_yaml_iter_key(&root_iter);
        ogs_assert(root_key);
        if (!strcmp(root_key, local)) {
            ogs_yaml_iter_t pcf_iter;
            ogs_yaml_iter_recurse(&root_iter, &pcf_iter);
            while (ogs_yaml_iter_next(&pcf_iter)) {
                const char *pcf_key = ogs_yaml_iter_key(&pcf_iter);
                ogs_assert(pcf_key);
                if (!strcmp(pcf_key, "notificationListener")) {
                    ogs_list_t list, list6;
                    ogs_socknode_t *node = NULL, *node6 = NULL;

                    ogs_yaml_iter_t sbi_array, sbi_iter;
                    ogs_yaml_iter_recurse(&pcf_iter, &sbi_array);
                    do {
                        int i, family = AF_UNSPEC;
                        int num = 0;
                        const char *hostname[OGS_MAX_NUM_OF_HOSTNAME];
                        int num_of_advertise = 0;
                        const char *advertise[OGS_MAX_NUM_OF_HOSTNAME];
			            uint16_t port = OGS_SBI_HTTP_PORT;                      
                        const char *dev = NULL;
                        ogs_sockaddr_t *addr = NULL;
                        ogs_sockopt_t option;
                        bool is_option = false;
                        if (ogs_yaml_iter_type(&sbi_array) ==
                                YAML_MAPPING_NODE) {
                            memcpy(&sbi_iter, &sbi_array,
                                    sizeof(ogs_yaml_iter_t));
                        } else if (ogs_yaml_iter_type(&sbi_array) ==
                            YAML_SEQUENCE_NODE) {
                            if (!ogs_yaml_iter_next(&sbi_array))
                                break;
                            ogs_yaml_iter_recurse(&sbi_array, &sbi_iter);
                        } else if (ogs_yaml_iter_type(&sbi_array) ==
                            YAML_SCALAR_NODE) {
                            break;
                        } else
                            ogs_assert_if_reached();

                        while (ogs_yaml_iter_next(&sbi_iter)) {
                            const char *sbi_key =
                                ogs_yaml_iter_key(&sbi_iter);
                            ogs_assert(sbi_key);
                            if (!strcmp(sbi_key, "family")) {
                                const char *v = ogs_yaml_iter_value(&sbi_iter);
                                if (v) family = atoi(v);
                                if (family != AF_UNSPEC &&
                                    family != AF_INET && family != AF_INET6) {
                                    ogs_warn("Ignore family(%d) : "
                                        "AF_UNSPEC(%d), "
                                        "AF_INET(%d), AF_INET6(%d) ",
                                        family, AF_UNSPEC, AF_INET, AF_INET6);
                                    family = AF_UNSPEC;
                                }
                            } else if (!strcmp(sbi_key, "addr") ||
                                    !strcmp(sbi_key, "name")) {
                                ogs_yaml_iter_t hostname_iter;
                                ogs_yaml_iter_recurse(&sbi_iter,
                                        &hostname_iter);
                                ogs_assert(ogs_yaml_iter_type(&hostname_iter) !=
                                    YAML_MAPPING_NODE);

                                do {
                                    if (ogs_yaml_iter_type(&hostname_iter) ==
                                            YAML_SEQUENCE_NODE) {
                                        if (!ogs_yaml_iter_next(
                                                    &hostname_iter))
                                            break;
                                    }

                                    ogs_assert(num < OGS_MAX_NUM_OF_HOSTNAME);
                                    hostname[num++] =
                                        ogs_yaml_iter_value(&hostname_iter);
                                } while (
                                    ogs_yaml_iter_type(&hostname_iter) ==
                                        YAML_SEQUENCE_NODE);
                            } else if (!strcmp(sbi_key, "advertise")) {
                                ogs_yaml_iter_t advertise_iter;
                                ogs_yaml_iter_recurse(&sbi_iter,
                                        &advertise_iter);
                                ogs_assert(ogs_yaml_iter_type(
                                    &advertise_iter) != YAML_MAPPING_NODE);

                                do {
                                    if (ogs_yaml_iter_type(&advertise_iter) ==
                                                YAML_SEQUENCE_NODE) {
                                        if (!ogs_yaml_iter_next(
                                                    &advertise_iter))
                                            break;
                                    }

                                    ogs_assert(num_of_advertise <
                                            OGS_MAX_NUM_OF_HOSTNAME);
                                    advertise[num_of_advertise++] =
                                        ogs_yaml_iter_value(&advertise_iter);
                                } while (
                                    ogs_yaml_iter_type(&advertise_iter) ==
                                        YAML_SEQUENCE_NODE);
                            } else if (!strcmp(sbi_key, "port")) {
                                const char *v = ogs_yaml_iter_value(&sbi_iter);
                                if (v)
                                    port = atoi(v);
                            } else if (!strcmp(sbi_key, "dev")) {
                                dev = ogs_yaml_iter_value(&sbi_iter);
                            } else if (!strcmp(sbi_key, "option")) {
                                rv = ogs_app_parse_sockopt_config(&sbi_iter, &option);
                                if (rv != OGS_OK) return rv;
                                is_option = true;
                            } else
                                ogs_warn("unknown key `%s`", sbi_key);
                        }
                        addr = NULL;
                        for (i = 0; i < num; i++) {
                            rv = ogs_addaddrinfo(&addr,
                                    family, hostname[i], port, 0);
                            ogs_assert(rv == OGS_OK);                          
                            pcf_notification_listener_add(hostname[i], port);
                        }

                        ogs_list_init(&list);
                        ogs_list_init(&list6);

                        if (addr) {
                            if (addr->ogs_sa_family == AF_INET)
				ogs_socknode_add(
                                    &list, AF_INET, addr, NULL);
			    if (addr->ogs_sa_family == AF_INET6)
                            	ogs_socknode_add(
                                    &list6, AF_INET6, addr, NULL);
                            ogs_freeaddrinfo(addr);
                        }
                        if (dev) {
                            rv = ogs_socknode_probe(&list, &list6, dev, port, NULL);
                            ogs_assert(rv == OGS_OK);
                        }
                        addr = NULL;
                        for (i = 0; i < num_of_advertise; i++) {
                            rv = ogs_addaddrinfo(&addr,
                                    family, advertise[i], port, 0);
                            ogs_assert(rv == OGS_OK);
                        }
                        node = ogs_list_first(&list);
                        if (node) {
                            ogs_sbi_server_t *server = ogs_sbi_server_add(NULL /*interface*/, OpenAPI_uri_scheme_NULL,
                                    node->addr, is_option ? &option : NULL);
                            ogs_assert(server);

                            if (addr)
                                ogs_sbi_server_set_advertise(
                                        server, AF_INET, addr);
                        }
                        node6 = ogs_list_first(&list6);
                        if (node6) {
                            ogs_sbi_server_t *server = ogs_sbi_server_add(NULL /*interface*/, OpenAPI_uri_scheme_NULL,
                                    node6->addr, is_option ? &option : NULL);
                            ogs_assert(server);

                            if (addr)
                                ogs_sbi_server_set_advertise(
                                        server, AF_INET6, addr);
                        }
                        if (addr)
                            ogs_freeaddrinfo(addr);
                        ogs_socknode_remove_all(&list);
                        ogs_socknode_remove_all(&list6);                       

                    } while (ogs_yaml_iter_type(&sbi_array) ==
                            YAML_SEQUENCE_NODE);
                
                } else if (!strcmp(pcf_key, "service_name")) {
                    /* handle config in sbi library */
                } else if (!strcmp(pcf_key, "discovery")) {
		     
                    /* handle config in sbi library */
                } else
                    ogs_warn("unknown key `%s`", pcf_key);
            }
        }
    }
    rv = pcf_client_context_validation();
    if (rv != OGS_OK) return rv;
    return OGS_OK;
}

pcf_app_session_t *_pcf_client_context_active_sessions_exists(pcf_app_session_t *app_sess) {
    pcf_session_t *session;
    pcf_app_session_t *sess, *next = NULL;

    if(app_sess->pcf_session) {
        session = app_sess->pcf_session;
        ogs_assert(session);
    }

    ogs_list_for_each_safe(&session->pcf_app_sessions, next, sess){
        if(!strcmp(app_sess->pcf_app_session_id, sess->pcf_app_session_id)){
            break;
        }

    }

   if(sess)
   {
       return sess;
   }
    return NULL;

}


static void pcf_notification_listener_add(const char *notification_listener_addr, const int port) {
    pcf_notification_listener_t *pcf_notification_listen;
    pcf_notification_listen = ogs_calloc(1, sizeof(pcf_notification_listener_t));
    ogs_assert(pcf_notification_listen);
    pcf_notification_listen->addr = ogs_strdup(notification_listener_addr);
    pcf_notification_listen->port = port;
    ogs_list_add(&self->config.pcf_notification_listener_list, pcf_notification_listen);
}

static void pcf_notification_listener_remove_all(void)
{
    pcf_notification_listener_t *pcf_notification_listen = NULL, *next = NULL;
    ogs_list_for_each_safe(&self->config.pcf_notification_listener_list, next, pcf_notification_listen)
        pcf_notification_listener_remove(pcf_notification_listen);

}

static void pcf_notification_listener_remove(pcf_notification_listener_t *pcf_notification_listen)
{
    ogs_assert(pcf_notification_listen);
    ogs_list_remove(&self->config.pcf_notification_listener_list, pcf_notification_listen);
    if(pcf_notification_listen->addr)
	    ogs_free(pcf_notification_listen->addr);
    ogs_free(pcf_notification_listen);
}

