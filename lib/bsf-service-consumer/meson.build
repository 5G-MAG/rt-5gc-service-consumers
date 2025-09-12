# License: 5G-MAG Public License (v1.0)
# Copyright: (C) 2022 British Broadcasting Corporation
#
# For full license terms please see the LICENSE file distributed with this
# program. If this file is missing then the license can be retrieved from
# https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view

libapp_dep = open5gs_project.get_variable('libapp_dep')
libcore_dep = open5gs_project.get_variable('libcore_dep')
libcrypt_dep = open5gs_project.get_variable('libcrypt_dep')
libsbi_dep = open5gs_project.get_variable('libsbi_dep')
libinc = open5gs_project.get_variable('libinc')

libscbsf_version_major = 2
libscbsf_version_minor = 0
libscbsf_version_micro = 0

libscbsf_version = '@0@.@1@.@2@'.format(libscbsf_version_major,libscbsf_version_minor,libscbsf_version_micro)
libscbsf_title = 'BSF service consumer library'
libscbsf_description = 'Library that provides a C language API for communicating, as a client, with a BSF using the Nbsf_Management API.'

libscbsf_sources = files('''
    bsf-service-consumer.c
    bsf-service-consumer.h
    bsf-client-sess.c
    bsf-client-sess.h
    bsf-configuration.c
    bsf-configuration.h
    context.c
    context.h
    local.c
    local.h
    log.c
    log.h
    nbsf-management-build.c
    nbsf-management-build.h
    nbsf-process.c
    nbsf-process.h
    pcf-bind.c
    pcf-bind.h
    pcf-bindings-cache.c
    pcf-bindings-cache.h
    utils.c
    utils.h
'''.split())

libscbsf_public_hdrs = files('''
    bsf-service-consumer.h
'''.split())

libscbsf_inc = include_directories('.')

libscbsf = library('scbsf',
    sources : libscbsf_sources,
    c_args : '-DBUILD_BSF_CLIENT_LIB',
    include_directories : [libscbsf_inc, libinc],
    gnu_symbol_visibility : 'hidden',
    dependencies : [libapp_dep,
                    libcore_dep,
                    libcrypt_dep,
		    libsbi_dep],
    version : libscbsf_version,
    install : true)

libscbsf_dep = declare_dependency(
    link_with : libscbsf,
    include_directories : [libscbsf_inc, libinc],
    dependencies : [libapp_dep,
                    libcore_dep,
                    libcrypt_dep,
		    libsbi_dep]
    )

