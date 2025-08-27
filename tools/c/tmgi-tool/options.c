/*
 * License: 5G-MAG Public License (v1.0)
 * Author: David Waring
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ogs-core.h"

#include "options.h"

#define MBS_SERVICE_TOOL_MAX_ARGS 16

static void _print_help(bool is_error);
static void _split_address_and_port(const char *str, char **address, short int *port);
static void _split_mcc_and_mnc(const char *str, struct options_plmn *plmn);

app_options_t* app_options_parse(int *argc, char ***argv)
{
    int opt;
    app_options_t *app_opts;
    int ret_argc = 0;
    static char *ret_argv[MBS_SERVICE_TOOL_MAX_ARGS];

    static const char shortopts[] = "e:dhl:m:M:n:p:s:S:t:Tu";
    static struct option longopts[] = {
        { "delete",          no_argument,       NULL, 'd' },
        { "help",            no_argument,       NULL, 'h' },
        { "mbs-service-id",  required_argument, NULL, 'M' },
        { "nrf-address",     required_argument, NULL, 'n' },
        { "plmn",            required_argument, NULL, 'p' },
        { "scp-address",     required_argument, NULL, 's' },
        { NULL, no_argument, NULL, 0},
    };

    /* copy command name */
    ret_argv[ret_argc++] = (*argv)[0];

    app_opts = ogs_calloc(1, sizeof(*app_opts));

    /* parse command line */
    for (opt = getopt_long(*argc, *argv, shortopts, longopts, NULL);
         opt != -1;
         opt = getopt_long(*argc, *argv, shortopts, longopts, NULL)) {
        switch (opt) {
            case 'd': // delete
                app_opts->delete_tmgi = true;
                break;
            case 'e':
                /* pass to Open5GS */
                ret_argv[ret_argc++] = "-e";
                ret_argv[ret_argc++] = optarg;
                break;
            case 'h': // help
                _print_help(false);
                exit(0);
            case 'l':
                /* pass to Open5GS */
                ret_argv[ret_argc++] = "-l";
                ret_argv[ret_argc++] = optarg;
                break;
            case 'm':
                /* pass to Open5GS */
                ret_argv[ret_argc++] = "-m";
                ret_argv[ret_argc++] = optarg;
                break;
            case 'M': // mbs-service-id
                app_opts->mbs_service_id = optarg;
                break;
            case 'n': // nrf-address
                _split_address_and_port(optarg, &app_opts->nrf_address, &app_opts->nrf_port);
                break;
            case 'p': // plmn
                _split_mcc_and_mnc(optarg, &app_opts->plmn);
                break;
            case 's': // scp-address
                _split_address_and_port(optarg, &app_opts->scp_address, &app_opts->scp_port);
                break;
            case '?':
            default:
                exit(1);
                break;
        }
    }

    if (optind != *argc) {
        dprintf(2, "Command only takes option arguments\n");
        exit(1);
    }

    if (app_opts->delete_tmgi) {
        if (!app_opts->plmn.mcc || !app_opts->plmn.mnc) {
            dprintf(2, "Must specify a PLMN when requesting deletion.\n");
            exit(1);
        }
    } else {
        if (app_opts->plmn.mcc || app_opts->plmn.mnc || app_opts->mbs_service_id) {
            dprintf(2, "Cannot specify PLMN or MBS Service Id when allocating a TMGI.\n");
            exit(1);
        }
    }

    if (!app_opts->scp_address && !app_opts->nrf_address) {
        dprintf(2, "Must provide at least one of an SCP address or an NRF address\n");
        exit(1);
    }

    ret_argv[ret_argc++] = NULL;

    /* update argc and argv to represent left over command line options */
    *argc = ret_argc;
    *argv = ret_argv;

    return app_opts;
}

void app_options_final(app_options_t *options, int argc, char **argv)
{
    if (options->mbs_service_id) ogs_free(options->mbs_service_id);
    if (options->plmn.mcc) ogs_free(options->plmn.mcc);
    if (options->plmn.mnc) ogs_free(options->plmn.mnc);
    if (options->nrf_address) ogs_free(options->nrf_address);
    if (options->scp_address) ogs_free(options->scp_address);
    ogs_free(options);
}

/***************************/
/**** Private functions ****/
/***************************/

static void _print_help(bool is_error)
{
    int fd = is_error?2:1;

    dprintf(fd,
            "Syntax: tmgi-tool -h\n"
            "        tmgi-tool (-n <nrf-address>:<nrf-port>|-s <scp-address>:<scp-port>)\n"
            "        tmgi-tool -d [-M <mbs-service-id>] -p <mcc>-<mnc> (-n <nrf-address>:<nrf-port>|-s <scp-address>:<scp-port>)\n"
            "\n"
            "Options:\n"
            " -h          --help                 Show this help\n"
            " -d          --delete               Deallocate a TMGI instead of allocating one (PLMN must be given).\n"
            " -n IP:PORT  --nrf-address IP:PORT  The IP address and port number to contact the NRF at.\n"
            " -s IP:PORT  --scp-address IP:PORT  The IP address and port number to contact the SCP at.\n"
            " -M ID       --mbs-service-id ID    The MBS Service Id for the TMGI to delete (only valid with -d).\n"
            " -p MCC-MNC  --plmn MCC-MNC         The PLMN to use for the TMGI (only used if -t has been given).\n"
            "\n"
            "Open5GS Options:\n"
            " -e LEVEL    --log-level LEVEL      The logging level to use.\n"
            " -l FILE     --log-file FILE        The file to save the log to.\n"
            " -m DOMAINS  --domain-mask DOMAINS  The domains to limit the logging to.\n"
            "\n"
            "Attributes:\n"
            "  DOMAINS    Comma separated list of logging domains to limit logging to.\n"
            "  FILE       A path to a file in the local file system.\n"
            "  ID         An MBS Service Identifier as a 6 digit hex string.\n"
            "  IP         An IPv4 or IPv6 address.\n"
            "  LEVEL      The minimum logging level to use: trace, debug, info, warn, error or crit.\n"
            "  PORT       A port number between 1 and 65535.\n"
            "  MCC        A PLMN MCC value given as a 3 digit string.\n"
            "  MNC        A PLMN MNC value given as a 2 or 3 digit string.\n"
            "\n"
            "If help is requested then the help message will be displayed and the command will exit without any further operation.\n"
            "\n"
            );
}

static void _split_address_and_port(const char *str, char **address, short int *port)
{
    const char *ptr;

    ptr = strrchr(str, ':');
    if (!ptr) {
        dprintf(2, "Address and port pair must contain a ':'\n");
        exit(1);
    }

    *address = ogs_malloc(ptr-str+1);
    strncpy(*address, str, ptr-str);
    (*address)[ptr-str] = '\0';
    *port = atoi(ptr+1);
}

static void _split_mcc_and_mnc(const char *str, struct options_plmn *plmn)
{
    size_t i;
    size_t len = strlen(str);

    if (len < 6 || len > 7 || str[3] != '-') {
        dprintf(2, "PLMN value is the MCC (3 digits) followed by a '-' and then the MNC (2 or 3 digits).\n");
        exit(1);
    }

    for (i=0; i<3; i++) {
        if (str[i] < '0' || str[i] > '9') {
            dprintf(2, "PLMN MCC must be given as 3 digits.\n");
            exit(1);
        }
    }

    for (i=4; i<len; i++) {
        if (str[i] < '0' || str[i] > '9') {
            dprintf(2, "PLMN MNC must be given as 2 or 3 digits.\n");
            exit(1);
        }
    }

    plmn->mcc = ogs_calloc(1, 4);
    memcpy(plmn->mcc, str, 3);
    plmn->mnc = ogs_strdup(str+4);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
