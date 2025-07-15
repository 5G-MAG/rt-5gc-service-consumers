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
static void _split_ssm_address(const char *str, struct options_ssm *ssm);

app_options_t* app_options_parse(int *argc, char ***argv)
{
    int opt;
    app_options_t *app_opts;
    int ret_argc = 0;
    static char *ret_argv[MBS_SERVICE_TOOL_MAX_ARGS];

    static const char shortopts[] = "e:hl:m:Mn:p:s:S:t:Tu";
    static struct option longopts[] = {
        { "help",            no_argument,       NULL, 'h' },
        { "multicast",       no_argument,       NULL, 'M' },
        { "nrf-address",     required_argument, NULL, 'n' },
        { "plmn",            required_argument, NULL, 'p' },
        { "scp-address",     required_argument, NULL, 's' },
        { "ssm",             required_argument, NULL, 'S' },
        { "tmgi",            required_argument, NULL, 't' },
        { "allocate-tmgi",   no_argument,       NULL, 'T' },
        { "allocate-tunnel", no_argument,       NULL, 'u' },
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
            case 'M': // multicast
                app_opts->is_multicast = true;
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
            case 'S': // ssm
                _split_ssm_address(optarg, &app_opts->ssm);
                break;
            case 't': // tmgi
                app_opts->tmgi_id = optarg;
                break;
            case 'T': // allocate-tmgi
                app_opts->req_tmgi = true;
                break;
            case 'u': // allocate-tunnel
                app_opts->req_tunnel = true;
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

    if (!app_opts->req_tmgi && !app_opts->tmgi_id && !app_opts->ssm.source) {
        dprintf(2, "Must either request a TMGI, specify a TMGI or provide an SSM address pair.\n");
        exit(1);
    }

    if (app_opts->req_tmgi && app_opts->tmgi_id) {
        dprintf(2, "Cannot request a TMGI and specify a TMGI at the same time -t and -T are mutually exclusive.\n");
        exit(1);
    }

    if (app_opts->plmn.mcc && !app_opts->tmgi_id) {
        dprintf(2, "Warning: PLMN details are ignored when not accompanied by a TMGI MBS Service ID.\n");
    }

    if (app_opts->is_multicast && !app_opts->ssm.source) {
        dprintf(2, "Must provide an SSM address pair if a multicast MBS Service is requested.\n");
        exit(1);
    }

    if (!app_opts->req_tunnel && !app_opts->ssm.source) {
        dprintf(2, "Must provide an SSM address pair if not requesting a UDP tunnel.\n");
        exit(1);
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
    if (options->ssm.source) ogs_free(options->ssm.source);
    if (options->ssm.dest) ogs_free(options->ssm.dest);
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
            "Syntax: mbs-service-tool -h\n"
            "        mbs-service-tool -S <source-address>:<multicast-address> [-Mu]\n"
            "        mbs-service-tool -T -u [-M] [-S <source-address>:<multicast-address>]\n"
            "        mbs-service-tool -T [-M] -S <source-address>:<multicast-address>\n"
            "        mbs-service-tool -t <mbs-service-id> -u -p <mcc>-<mnc> [-M] [-S <source-address>:<multicast-address>]\n"
            "        mbs-service-tool -t <mbs-service-id> -p <mcc>-<mnc> [-M] -S <source-address>:<multicast-address>\n"
            "\n"
            "Options:\n"
            " -h          --help                 Show this help\n"
            " -n IP:PORT  --nrf-address IP:PORT  The IP address and port number to contact the NRF at.\n"
            " -s IP:PORT  --scp-address IP:PORT  The IP address and port number to contact the SCP at.\n"
            " -S IP:IP    --ssm IP:IP            The Source Specific Multicast address to use in <source>:<dest> form.\n"
            " -t ID       --tmgi ID              The MBS Service Id for the TMGI to request the MBS Service for.\n"
            " -p MCC-MNC  --plmn MCC-MNC         The PLMN to use for the TMGI (only used if -t has been given).\n"
            " -T          --allocate-tmgi        Request that a TMGI be allocated to the MBS Service.\n"
            " -M          --multicast            Request a Multicast MBS Service rather than a Broadcast MBS Service.\n"
            " -u          --allocate-tunnel      Ask for a UDP tunnel to be allocated for the MBS Service.\n"
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

static void _split_ssm_address(const char *str, struct options_ssm *ssm)
{
    const char *ptr;

    ptr = strrchr(str, ':');
    if (!ptr) {
        dprintf(2, "SSM Addresses must be separated by a ':'\n");
        exit(1);
    }

    ssm->source = ogs_calloc(1, ptr-str+1);
    memcpy(ssm->source, str, ptr-str);
    ssm->dest = ogs_strdup(ptr+1);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
