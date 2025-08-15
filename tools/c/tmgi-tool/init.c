/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ogs-app.h"
#include "ogs-core.h"
#include "ogs-sbi.h"

#include "mb-smf-service-consumer.h"

#include "app-log.h"
#include "app-sm.h"
#include "options.h"

#include "init.h"

static struct {
    app_options_t *options;
    int argc;
    char **argv;
    char *app_config_filename;
} *g_app_context = NULL;

static void _context_init(int argc, char *argv[]);
static void _context_final(void);
static void _write_app_yaml_config();

/**************************/
/**** Public functions ****/
/**************************/

#define APP_NAME "tmgi-tool"

bool tmgi_tool_init(int argc, char *argv[])
{
    ogs_sbi_nf_instance_t *nf_instance;

    _context_init(argc, argv);

    /* Write out an Open5GS app config based on the command line options */
    _write_app_yaml_config();

    /* Add the new config file as an option on the command line parameters */
    g_app_context->argv[g_app_context->argc++] = "-c";
    g_app_context->argv[g_app_context->argc++] = g_app_context->app_config_filename;
    g_app_context->argv[g_app_context->argc] = NULL;

    /* Start-up the Open5GS application */
    ogs_app_initialize(MBS_SERVICE_TOOL_VERSION, g_app_context->app_config_filename, (const char* const*)g_app_context->argv);

    /* initialise logging */
    app_log_init();

    ogs_debug("App initialised, configuring...");

    /* initialise local settings */
    int rv = ogs_app_parse_local_conf(APP_NAME);
    if (rv != OGS_OK) {
        ogs_fatal("Unable to parse local configuration");
    }

    /* Setup Open5GS SBI library */
    ogs_sbi_context_init(OpenAPI_nf_type_AF);
    ogs_sbi_context_parse_config(APP_NAME, g_app_context->options->nrf_address?"nrf":NULL,
                                           g_app_context->options->scp_address?"scp":NULL);
    nf_instance = ogs_sbi_self()->nf_instance;
    ogs_assert(nf_instance);
    ogs_sbi_nf_fsm_init(nf_instance);

    /* Setup Service Consumers */
    if (g_app_context->options->nrf_address || g_app_context->options->scp_address) {
        mb_smf_sc_parse_config(APP_NAME);
    }

    return true;
}

void tmgi_tool_event_termination(void)
{
    mb_smf_sc_terminate();
    ogs_sbi_server_stop_all();
    if (ogs_sbi_self()->nf_instance) ogs_sbi_nf_fsm_fini(ogs_sbi_self()->nf_instance);
    ogs_sbi_context_final();

    /* finally, terminate the event queue */
    ogs_queue_term(ogs_app()->queue);
    ogs_pollset_notify(ogs_app()->pollset);
}

void tmgi_tool_final(void)
{
    app_log_final();
    _context_final();
    ogs_app_terminate();
}

const app_options_t *tmgi_tool_get_app_options()
{
    return g_app_context->options;
}

/***************************/
/**** Private Functions ****/
/***************************/

static void _context_init(int argc, char *argv[])
{
    g_app_context = ogs_calloc(1,sizeof(*g_app_context));
    ogs_assert(g_app_context);

    /* Initialise the context with the command line arguments */
    g_app_context->argc = argc;
    g_app_context->argv = argv;
    g_app_context->options = app_options_parse(&g_app_context->argc, &g_app_context->argv);
}

static void _context_final(void)
{
    if (!g_app_context) return;

    if (g_app_context->app_config_filename) {
        unlink(g_app_context->app_config_filename);
        ogs_free(g_app_context->app_config_filename);
    }
    app_options_final(g_app_context->options, g_app_context->argc, g_app_context->argv);

    ogs_free(g_app_context);
    g_app_context = NULL;
}

static void _write_app_yaml_config()
{
    int fd;
    static const char config_hdr[] =
        "global:\n"
        "  max:\n"
        "    ue: 16\n"
        "\n"
        "sbi:\n"
        "  server:\n"
        "    no_tls: true\n"
        "  client:\n"
        "    no_tls: true\n"
        "\n"
        APP_NAME ":\n"
        "  discovery:\n"
        "    delegated: auto\n"
        "  sbi:\n"
        "    server:\n"
        "      - address: 127.0.0.1\n"
        "        port: 0\n"
        ;
    const char *tmpdir;

    tmpdir = ogs_env_get("TMPDIR");
#ifdef P_tmpdir
    if (!tmpdir) tmpdir = P_tmpdir;
#endif
    if (!tmpdir) tmpdir = OGS_DIR_SEPARATOR_S "tmp";
    g_app_context->app_config_filename = ogs_msprintf("%s%c" APP_NAME "-yaml.XXXXXX", tmpdir, OGS_DIR_SEPARATOR);
    fd = mkostemp(g_app_context->app_config_filename, O_CREAT|O_WRONLY|O_TRUNC|O_CLOEXEC);

    if (fd < 0) {
        ogs_free(g_app_context->app_config_filename);
        g_app_context->app_config_filename = NULL;
        return;
    }

    write(fd, config_hdr, sizeof(config_hdr)-1);

    if (g_app_context->options->nrf_address || g_app_context->options->scp_address) {
        dprintf(fd,
                "    client:\n");
    }

    if (g_app_context->options->nrf_address) {
        dprintf(fd,
                "      nrf:\n"
                "        - uri: http://%s:%i\n",
                g_app_context->options->nrf_address, g_app_context->options->nrf_port);
    }

    if (g_app_context->options->scp_address) {
        dprintf(fd,
                "      scp:\n"
                "        - uri: http://%s:%i\n",
                g_app_context->options->scp_address, g_app_context->options->scp_port);
    }

    close(fd);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
