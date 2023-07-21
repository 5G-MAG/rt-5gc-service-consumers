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
   
'''.split())

test5gc_pcf_exe = executable('pcf-service-consumer',
    sources : test5gc_pcf_sources,
    c_args : testunit_core_cc_flags,
    include_directories : srcinc,
    dependencies : [libtest5gc_dep, libscpcf_dep])

test('pcf-service-consumer',
    test5gc_pcf_exe,
    is_parallel : false,
    suite: 'app')


