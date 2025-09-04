# Copyright: (C) 2025 British Broadcasting Corporation
# Author: David Waring <david.waring2@bbc.co.uk>
# License: 5G-MAG Public License (v1.0)
#
# For full license terms please see the LICENSE file distributed with this
# program. If this file is missing then the license can be retrieved from
# https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view

doxygen_exe = find_program('doxygen', required: false)


if doxygen_exe.found()
  dot_exe = find_program('dot', required: false)
  doxyfile_configs = {}
  doxygen_configurations = {}
  doxygen_deps = []

  foreach lib_entry : [
                 ['bsf-service-consumer', libscbsf_title, libscbsf_description, libscbsf_public_hdrs],
                 ['pcf-service-consumer', libscpcf_title, libscpcf_description, libscpcf_public_hdrs],
                 ['mb-smf-service-consumer', libscmbsmf_title, libscmbsmf_description, libscmbsmf_public_hdrs],
                ]
    lib_dir = lib_entry[0]
    lib_name = lib_entry[1]
    lib_desc = lib_entry[2]
    lib_files = lib_entry[3]

    strip_paths = []
    public_hdrs_split = lib_files[0].full_path().split('/')
    i = public_hdrs_split.length() - 1
    foreach p : public_hdrs_split
      i = i - 1
      if i <= 0
        break
      endif
      strip_paths += p
    endforeach
    strip_paths = '/'.join(strip_paths)
    src_files = ''
    src_files_sep = ''
    foreach f : lib_files
      src_files += src_files_sep + f.full_path()
      src_files_sep = ' '
    endforeach
    doxyfile_config = configuration_data({
      'LIBRARY_NAME' : lib_name,
      'LIBRARY_DESCRIPTION' : lib_desc,
      'SOURCE_FILES' : src_files,
      'STRIP_PATHS' : strip_paths,
      'OUTPUT_DIR' : meson.current_build_dir() / lib_dir,
      'HAVE_DOT' : 'NO',
      'DOT_PATH' : ''
    })
    if dot_exe.found()
      doxyfile_config.set('HAVE_DOT', 'YES')
      doxyfile_config.set('DOT_PATH', dot_exe.full_path())
    endif
    doxyfile_configs += {lib_dir: doxyfile_config}
    doxygen_configuration = configure_file(input: 'Doxyfile.in', output: 'Doxyfile-' + lib_dir, configuration: doxyfile_config)
    doxygen_configurations += {lib_dir: doxygen_configuration}
    doxygen_deps += [run_target('docs-' + lib_dir, command: [doxygen_exe, doxygen_configuration])]
  endforeach

  alias_target('docs', doxygen_deps)
endif
