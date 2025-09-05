# License: 5G-MAG Public License (v1.0)
# Author: David Waring
# Copyright: (C) 2023 British Broadcasting Corporation
#
# For full license terms please see the LICENSE file distributed with this
# program. If this file is missing then the license can be retrieved from
# https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view

project('rt-5gc-service-consumers', 'c',
    version : '2.0.0',
    license : '5G-MAG Public',
    meson_version : '>= 1.4.0',
    default_options : [
        'c_std=gnu89',
    ],
)

open5gs_project=subproject('open5gs',required:true)

build_tests = open5gs_project.get_variable('build_tests')

subdir('lib')
subdir('tools')

if build_tests
    subdir('tests')
endif

subdir('docs')
