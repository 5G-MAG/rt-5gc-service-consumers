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

#define PCF_POLICYAUTH_MAX_ARGS 16

static void _print_help(bool is_error);
static void _split_address_and_port(const char *str, char **address, short int *port);
static void _split_address_port_and_protocol(const char *str, char **address, short int *port, int *protocol);
static const int _protocol_string_to_enum(const char *pstr);
static const OpenAPI_media_type_e _string_to_media_type(const char *str);

app_options_t* app_options_parse(int *argc, char ***argv)
{
    int opt;
    app_options_t *app_opts;
    int ret_argc = 0;
    static char *ret_argv[PCF_POLICYAUTH_MAX_ARGS];

    static const char shortopts[] = "a:d:D:e:hl:m:n:p:r:t:u:U:";
    static struct option longopts[] = {
        { "ue-address",     required_argument, NULL, 'a' },
        { "min-dl-bw",      required_argument, NULL, 'd' },
        { "max-dl-bw",      required_argument, NULL, 'D' },
        { "log-level",      required_argument, NULL, 'e' },
        { "help",           no_argument,       NULL, 'h' },
        { "log-file",       required_argument, NULL, 'l' },
        { "domain-mask",    required_argument, NULL, 'm' },
        { "nrf-address",    required_argument, NULL, 'n' },
        { "pcf-address",    required_argument, NULL, 'p' },
        { "remote-address", required_argument, NULL, 'r' },
        { "media-type",     required_argument, NULL, 't' },
        { "min-ul-bw",      required_argument, NULL, 'u' },
        { "max-ul-bw",      required_argument, NULL, 'U' },
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
            case 'a':
                _split_address_port_and_protocol(optarg, &app_opts->ue_address, &app_opts->ue_port, &app_opts->protocol);
                break;
            case 'd':
                app_opts->min_dl_bit_rate = atof(optarg);
                break;
            case 'D':
                app_opts->max_dl_bit_rate = atof(optarg);
                break;
            case 'e':
                /* pass to Open5GS */
                ret_argv[ret_argc++] = "-e";
                ret_argv[ret_argc++] = optarg;
                break;
            case 'h':
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
            case 'n':
                _split_address_and_port(optarg, &app_opts->nrf_address, &app_opts->nrf_port);
                break;
            case 'p':
                _split_address_and_port(optarg, &app_opts->pcf_address, &app_opts->pcf_port);
                break;
            case 'r':
                _split_address_port_and_protocol(optarg, &app_opts->remote_address, &app_opts->remote_port, &app_opts->protocol);
                break;
            case 't':
                app_opts->media_type = _string_to_media_type(optarg);
                break;
            case 'u':
                app_opts->min_ul_bit_rate = atof(optarg);
                break;
            case 'U':
                app_opts->max_ul_bit_rate = atof(optarg);
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

    if (!app_opts->ue_address) {
        dprintf(2, "Must provide a UE address\n");
        exit(1);
    }

    if (!app_opts->pcf_address && !app_opts->nrf_address) {
        dprintf(2, "Must provide either the PCF address or an NRF address\n");
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
    if (options->pcf_address) ogs_free(options->pcf_address);
    if (options->ue_address) ogs_free(options->ue_address);
    ogs_free(options);
}

/***************************/
/**** Private functions ****/
/***************************/

static void _print_help(bool is_error)
{
    int fd = is_error?2:1;

    dprintf(fd,
            "Syntax: pcf-policyauthorization -h\n"
            "        pcf-policyauthorization -a <UE-address> [-r <remote-address>] [-t <media-type>] [-d <bitrate>] [-D <bitrate>]\n"
            "                                [-u <bitrate>] [-U <bitrate>] [-e <log-level>] [-l <file>] [-m <log-domains>]\n"
            "                                ( -n <nrf-address> | -p <pcf-address> )\n"
            "\n"
            "Options:\n"
            " -h          --help                 Show this help\n"
            " -a IP[:PORT][/PROTOCOL] --ue-address IP[:PORT][/PROTOCOL]\n"
            "                                    The IP address, port and protocol for the UE, to create an AppSessionContext for.\n"
            " -r [IP][:PORT][/PROTOCOL] --remote-address [IP][:PORT][/PROTOCOL]\n"
            "                                    The remote address, port and protocol, to create an AppSessionContext for.\n"
            " -t MEDIA-TYPE --media-type MEDIA-TYPE\n"
            "                                    The media type to setup the PDU session for.\n"
            " -d BITRATE  --min-dl-bw BITRATE    The minimum downlink bit rate to limit the PDU session to.\n"
            " -D BITRATE  --max-dl-bw BITRATE    The maximum downlink bit rate to limit the PDU session to.\n"
            " -u BITRATE  --min-ul-bw BITRATE    The minimum uplink bit rate to limit the PDU session to.\n"
            " -U BITRATE  --max-ul-bw BITRATE    The maximum uplink bit rate to limit the PDU session to.\n"
            " -n IP:PORT  --nrf-address IP:PORT  The IP address and port number to contact the NRF at (PCF discovery).\n"
            " -p IP:PORT  --pcf-address IP:PORT  The IP address and port number to contact the PCF at (no discovery).\n"
            "\n"
            "Open5GS Options:\n"
            " -e LEVEL    --log-level LEVEL      The logging level to use.\n"
            " -l FILE     --log-file FILE        The file to save the log to.\n"
            " -m DOMAINS  --domain-mask DOMAINS  The domains to limit the logging to.\n"
            "\n"
            "Attributes:\n"
            "  BITRATE    A bit rate in bits per second (can be a decimal fraction).\n"
            "  DOMAINS    Comma separated list of logging domains to limit logging to.\n"
            "  FILE       A path to a file in the local file system.\n"
            "  IP         An IPv4 or IPv6 address.\n"
            "  LEVEL      The minimum logging level to use: trace, debug, info, warn, error or crit.\n"
            "  MEDIA-TYPE The media type for the flow: audio, video, data, application, control, text, message or other.\n"
            "  PORT       A port number between 1 and 65535.\n"
            "  PROTOCOL   An IP protocol to limit the flow selection to: tcp, udp, icmp or sctp.\n"
            "\n"
            "If help is requested then the help message will be displayed and the command will exit without any further operation.\n"
            "Otherwise the UE address must be given and one of either NRF address (discovery mode) and port or PCF address and port\n"
            "(direct mode).\n"
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

/** Split "<address>[:<port>][/<protocol>]"
 *
 * Splits the string into address, port and protocol. Will abort program if:
 * - address already set (can only set once)
 * - protocol already set and different to parsed one.
 */
static void _split_address_port_and_protocol(const char *str, char **address, short int *port, int *protocol)
{
    const char *ptr;

    if (*address) {
        dprintf(2, "Address already set, cannot change to %s\n", str);
        exit(1);
    }

    ptr = strrchr(str, ':');
    if (ptr) {
        const char *ptr2;
        ptr2 = strrchr(str, '/');
        if (ptr2) {
            /* format is <address>:<port>/<protocol> */
            int proto;

            proto = _protocol_string_to_enum(ptr2+1);
            if (*protocol != IPPROTO_IP && *protocol != proto) {
                dprintf(2, "Cannot have two different protocols on the UE<=>Remote connection\n");
                exit(1);
            }

            if (ptr > str) {
                *address = ogs_malloc(ptr-str+1);
                strncpy(*address, str, ptr-str);
                (*address)[ptr-str] = '\0';
            }

            *port = (short int)atoi(ptr+1);

            *protocol = proto;
        } else {
            /* format is <address>:<port> */
            if (ptr > str) {
                *address = ogs_malloc(ptr-str+1);
                strncpy(*address, str, ptr-str);
                (*address)[ptr-str] = '\0';
            }

            *port = (short int)atoi(ptr+1);
        }
    } else {
        ptr = strrchr(str, '/');
        if (ptr) {
            /* format is <address>/<protocol> */
            int proto;

            proto = _protocol_string_to_enum(ptr+1);

            if (*protocol != IPPROTO_IP && *protocol != proto) {
                dprintf(2, "Cannot have two different protocols on the UE<=>Remote connection\n");
                exit(1);
            }

            if (ptr > str) {
                *address = ogs_malloc(ptr-str+1);
                strncpy(*address, str, ptr-str);
                (*address)[ptr-str] = '\0';
            }

            *protocol = proto;
        } else {
            /* format is <address> */
            if (strlen(str) > 0) {
                *address = ogs_strdup(str);
            }
        }
    }
}

static const int _protocol_string_to_enum(const char *pstr)
{
    static const struct {
        const char *str;
        int protocol;
    } protocols[] = {
        {"tcp",  IPPROTO_TCP},
        {"udp",  IPPROTO_UDP},
        {"icmp", IPPROTO_ICMP},
        {"sctp", IPPROTO_SCTP}
    };
    int i;

    for (i=0; i<(sizeof(protocols)/sizeof(protocols[0])); i++) {
        if (!strcasecmp(pstr, protocols[i].str)) {
            return protocols[i].protocol;
        }
    }

    dprintf(2, "Unsupported IP protocol '%s'\n", pstr);
    exit(1);
}

static const OpenAPI_media_type_e _string_to_media_type(const char *str)
{
    static const struct {
        const char *str;
        OpenAPI_media_type_e media_type;
    } media_types[] = {
        {"audio", OpenAPI_media_type_AUDIO},
        {"video", OpenAPI_media_type_VIDEO},
        {"data", OpenAPI_media_type_DATA},
        {"application", OpenAPI_media_type_APPLICATION},
        {"app", OpenAPI_media_type_APPLICATION},
        {"control", OpenAPI_media_type_CONTROL},
        {"text", OpenAPI_media_type_TEXT},
        {"message", OpenAPI_media_type_MESSAGE},
        {"other", OpenAPI_media_type_OTHER},
    };
    int i;

    for (i=0; i<(sizeof(media_types)/sizeof(media_types[0])); i++) {
        if (!strcasecmp(str, media_types[i].str)) {
            return media_types[i].media_type;
        }
    }

    dprintf(2, "Unsupported MediaType '%s'\n", str);
    exit(1);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
