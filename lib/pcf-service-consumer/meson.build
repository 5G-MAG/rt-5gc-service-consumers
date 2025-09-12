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

libscpcf_version_major = 2
libscpcf_version_minor = 0
libscpcf_version_micro = 0

libscpcf_version = '@0@.@1@.@2@'.format(libscpcf_version_major,libscpcf_version_minor,libscpcf_version_micro)
libscpcf_title = 'PCF service consumer library'
libscpcf_description = 'Library that provides a C language API for communicating as a client NF with a PCF via the Npcf_PolicyAuthorization API at reference point N5'

libscpcf_sources = files('''
    init.c
    pcf-service-consumer.h
    pcf-service-consumer.c
    pcf-client.h
    pcf-client.c
    pcf-client-sess.h
    pcf-client-sess.c
    pcf-evsubsc.h
    pcf-evsubsc.c
    pcf-build.h
    pcf-build.c
    pcf-handler.h
    pcf-handler.c
    npcf-process.h
    npcf-process.c
    context.h
    context.c
    utils.h
    utils.c
'''.split())

libscpcf_public_hdrs = files('''
    pcf-service-consumer.h
'''.split())

libscpcf_inc = include_directories('.')

libscpcf = library('scpcf',
    sources : libscpcf_sources,
    c_args : '-DBUILD_PCF_CLIENT_LIB',
    include_directories : [libscpcf_inc, libinc],
    gnu_symbol_visibility : 'hidden',
    dependencies : [libapp_dep,
                    libcore_dep,
                    libcrypt_dep,
		    libsbi_dep],
    version : libscpcf_version,
    install : true)

libscpcf_dep = declare_dependency(
    link_with : libscpcf,
    include_directories : [libscpcf_inc, libinc],
    dependencies : [libapp_dep,
                    libcore_dep,
                    libcrypt_dep,
		    libsbi_dep]
)

