# License: 5G-MAG Public License (v1.0)
# Copyright: (C) 2025 British Broadcasting Corporation
# Author(s): David Waring <david.waring2@bbc.co.uk>
#
# For full license terms please see the LICENSE file distributed with this
# program. If this file is missing then the license can be retrieved from
# https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view

libcore = open5gs_project.get_variable('libcore')
libcore_inc = open5gs_project.get_variable('libcore_inc')
libsbi_dep = open5gs_project.get_variable('libsbi_dep')
libproto_dep = open5gs_project.get_variable('libproto_dep')
libsbi_openapi_dep = open5gs_project.get_variable('libsbi_openapi_dep')

pcre2_dep = dependency('libpcre2-8', required: true)

mb_smf_test_harness_src = files('''
    main.c
    unit-test.c
    unit-test.h
'''.split())

mb_smf_test_libs = []

nmbsmf_builders_src = files('''
    nmbsmf-builders.c

    lib-arp.c
    lib-associated-session-id.c
    lib-civic-address.c
    lib-ext-mbs-service-area.c
    lib-flow-description.c
    lib-geographic-area.c
    lib-geographic-coordinate.c
    lib-json-patch.c
    lib-mbs-fsa-id.c
    lib-mbs-media-comp.c
    lib-mbs-media-info.c
    lib-mbs-qos-req.c
    lib-mbs-service-area.c
    lib-mbs-service-info.c
    lib-ncgi.c
    lib-ncgi-tai.c
    lib-nmbsmf-mbs-session-build.c
    lib-nmbsmf-tmgi-build.c
    lib-ssm-addr.c
    lib-tai.c
    lib-tmgi.c
    lib-utils.c
    ogs-openapi-model-status_subscribe_req_data.c
'''.split())
nmbsmf_builders_lib = static_library('mbsmfbuild', nmbsmf_builders_src,
                                     link_with: libcore,
                                     dependencies: [ libsbi_dep, libproto_dep, libsbi_openapi_dep, pcre2_dep ],
                                     include_directories: [ libcore_inc, libscmbsmf_inc, libinc ])
mb_smf_test_libs += [nmbsmf_builders_lib]

mb_smf_test_harness_exe = executable('mb-smf-sc-unit-tests', mb_smf_test_harness_src,
                                     link_whole: mb_smf_test_libs,
                                     link_with: libcore,
                                     dependencies: [ libsbi_dep, libproto_dep, libsbi_openapi_dep, pcre2_dep ],
                                     include_directories: [ libcore_inc, libscmbsmf_inc, libinc ])
test('mb-smf-sc-unit-tests', mb_smf_test_harness_exe, suite: 'unit', args: ['-c'] + files('unit-test.yaml'))
