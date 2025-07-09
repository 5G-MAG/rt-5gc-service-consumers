# License: 5G-MAG Public License (v1.0)
# Author: David Waring <david.waring2@bbc.co.uk>
# Copyright: (C) 2025 British Broadcasting Corporation
#
# For full license terms please see the LICENSE file distributed with this
# program. If this file is missing then the license can be retrieved from
# https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view

mbs_service_tool_version = '1.0.0'

mbs_service_tool_sources = files('''
  main.c
  init.h
  init.c
  app-log.h
  app-log.c
  app-sm.h
  app-sm.c
  options.h
  options.c
'''.split())

mbs_service_tool = executable('mbs-service-tool',
  sources : mbs_service_tool_sources,
  c_args : ['-DMBS_SERVICE_TOOL_VERSION="' + mbs_service_tool_version + '"'],
  dependencies : [libscmbsmf_dep,
                  libapp_dep,
                  libsbi_dep,
                  libcore_dep],
  install : true,
  install_dir : get_option('bindir')
  )
