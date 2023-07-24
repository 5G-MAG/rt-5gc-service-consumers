# License: 5G-MAG Public License (v1.0)
# Author: Dev Audsin
# Copyright: (C) 2022 British Broadcasting Corporation
#
# For full license terms please see the LICENSE file distributed with this
# program. If this file is missing then the license can be retrieved from
# https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view

pcf_policyauth_version = '1.0.0'

pcf_policyauth_sources = files('''
  main.c
  init.h
  init.c
  app-log.h
  app-log.c
  app-sm.h
  app-sm.c
  dump-openapi.h
  dump-openapi.c
  options.h
  options.c
'''.split())

pcf_policyauth = executable('pcf-policyauthorization',
  sources : pcf_policyauth_sources,
  c_args : ['-DPCF_POLICYAUTH_VERSION="' + pcf_policyauth_version + '"'],
  include_directories : [ libscbsf_inc, libscpcf_inc ],
  dependencies : [libscbsf_dep,
                  libscpcf_dep,
                  libapp_dep,
                  libsbi_dep,
                  libcore_dep],
  install : true,
  install_dir : get_option('bindir')
  )
