# License: 5G-MAG Public License (v1.0)
# Copyright: (C) 2022 British Broadcasting Corporation
#
# For full license terms please see the LICENSE file distributed with this
# program. If this file is missing then the license can be retrieved from
# https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view

libtest5gc_dep = open5gs_project.get_variable('libtest5gc_dep')
srcinc = open5gs_project.get_variable('srcinc')
testunit_core_cc_flags = open5gs_project.get_variable('testunit_core_cc_flags')

test5gc_pcf_sources = files('''
    abts-main.c
    pcf-test-media.h
    pcf-test-media.c
    pcf-test-event-subsc.h
    pcf-test-event-subsc.c
      pcf-test-event.h
    pcf-test-event.c
    pcf-test-sm.h
    pcf-test-sm.c
    pcf-test-context.h
    pcf-test-context.c
    pcf-test.h
    pcf-test.c
    pcf-test-session-new.c
    pcf-test-bdt.h
    pcf-test-bdt.c
   
'''.split())

test5gc_pcf_exe = executable('pcf-service-consumer',
    sources : test5gc_pcf_sources,
    c_args : testunit_core_cc_flags,
    include_directories : srcinc,
    dependencies : [libtest5gc_dep, libscpcf_dep])

# the test AF needs an af: section, which the configuration of the vonr test of Open5GS has
test('pcf-service-consumer',
    test5gc_pcf_exe,
    args : ['-c', meson.global_build_root() / 'subprojects' / 'open5gs' / 'configs' / 'vonr.yaml'],
    timeout : 120,
    is_parallel : false,
    suite: 'app')

# the requests and the answers of the library, checked without a core network: the internals are hidden in the shared library, so this
# executable links the objects of the library instead
test5gc_pcf_unit_sources = files('''
    abts-main-unit.c
    pcf-test-unit.h
    pcf-test-unit.c
'''.split())

test5gc_pcf_unit_exe = executable('pcf-service-consumer-unit',
    sources : test5gc_pcf_unit_sources,
    objects : libscpcf.extract_all_objects(recursive : false),
    c_args : testunit_core_cc_flags,
    include_directories : [srcinc, libscpcf_inc, libinc],
    dependencies : [libtest5gc_dep, libapp_dep, libcore_dep, libcrypt_dep, libsbi_dep])

test('pcf-service-consumer-unit',
    test5gc_pcf_unit_exe,
    args : ['-c', meson.global_build_root() / 'subprojects' / 'open5gs' / 'configs' / 'vonr.yaml'],
    timeout : 120,
    is_parallel : false,
    suite: 'app')
