#ifndef MB_SMF_SC_MACROS_H
#define MB_SMF_SC_MACROS_H
/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#ifdef BUILD_MB_SMF_CLIENT_LIB
    #if defined _WIN32 || defined __CYGWIN__
        #ifdef __GNUC__
            #define MB_SMF_CLIENT_API __attribute__ ((dllexport))
        #else
            #define MB_SMF_CLIENT_API __declspec(dllexport)
        #endif
    #else
        #if __GNUC__ >= 4
            #define MB_SMF_CLIENT_API __attribute__ ((visibility ("default")))
        #else
            #define MB_SMF_CLIENT_API
        #endif
    #endif
#else
    #if defined _WIN32 || defined __CYGWIN__
        #ifdef __GNUC__
            #define MB_SMF_CLIENT_API __attribute__ ((dllimport))
        #else
            #define MB_SMF_CLIENT_API __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
        #endif
    #else
        #define MB_SMF_CLIENT_API
    #endif
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* MB_SMF_SC_MACROS_H */
