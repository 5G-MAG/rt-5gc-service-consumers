# License: 5G-MAG Public License (v1.0)
# Copyright: (C) 2025 British Broadcasting Corporation
# Author(s): David Waring <david.waring2@bbc.co.uk>
#
# For full license terms please see the LICENSE file distributed with this
# program. If this file is missing then the license can be retrieved from
# https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view

libapp_dep = open5gs_project.get_variable('libapp_dep')
libcore_dep = open5gs_project.get_variable('libcore_dep')
libcrypt_dep = open5gs_project.get_variable('libcrypt_dep')
libsbi_dep = open5gs_project.get_variable('libsbi_dep')
libinc = open5gs_project.get_variable('libinc')
libsbi_openapi_inc = open5gs_project.get_variable('libsbi_openapi_inc')

libscmbsmf_version_major = 1
libscmbsmf_version_minor = 0
libscmbsmf_version_micro = 0

libscmbsmf_version = '@0@.@1@.@2@'.format(libscmbsmf_version_major,libscmbsmf_version_minor,libscmbsmf_version_micro)

libscmbsmf_sources = files('''
    mb-smf-service-consumer.c
    mb-smf-service-consumer.h
    context.c
    context.h
    local.c
    local.h
    log.c
    log.h
    macros.h
    mbs-session.c
    mbs-session.h
    mbs-status-notification.c
    mbs-status-notification.h
    mbs-status-notification-result.h
    mbs-status-subscription.c
    mbs-status-subscription.h
    nmbsmf-mbs-session-build.c
    nmbsmf-mbs-session-build.h
    nmbsmf-mbs-session-handle.c
    nmbsmf-mbs-session-handle.h
    nmbsmf-mbs-session-path.c
    nmbsmf-mbs-session-path.h
    nmbsmf-mbs-session-process.c
    nmbsmf-mbs-session-process.h
    nmbsmf-tmgi-build.c
    nmbsmf-tmgi-build.h
    nmbsmf-tmgi-handle.c
    nmbsmf-tmgi-handle.h
    nnrf-disc-handle.c
    nnrf-disc-handle.h
    priv_mbs-session.h
    priv_mbs-status-subscription.h
    priv_tmgi.h
    ref_count_sbi_object.h
    tmgi.c
    tmgi.h
    utils.c
    utils.h

    openapi_status_notify_req_data.c
    openapi_status_subscribe_req_data.c
    openapi_status_subscribe_rsp_data.c
'''.split())

libscmbsmf_public_hdrs = files('''
    macros.h
    mb-smf-service-consumer.h
    mbs-session.h
    mbs-status-notification-result.h
    mbs-status-subscription.h
'''.split())

libscmbsmf_inc = include_directories('.')

libscmbsmf = library('scmbsmf',
    sources : libscmbsmf_sources,
    c_args : '-DBUILD_MB_SMF_CLIENT_LIB',
    include_directories : [libscmbsmf_inc, libinc],
    gnu_symbol_visibility : 'hidden',
    dependencies : [libapp_dep,
                    libcore_dep,
                    libcrypt_dep,
		    libsbi_dep],
    version : libscmbsmf_version,
    soversion : libscmbsmf_version_major.to_string(),
    install : true)

libscmbsmf_dep = declare_dependency(
    link_with : libscmbsmf,
    include_directories : [libscmbsmf_inc, libinc],
    dependencies : [libapp_dep,
                    libcore_dep,
                    libcrypt_dep,
		    libsbi_dep]
    )

