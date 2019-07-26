





"""
Make sure we generate a manifest file when linking binaries, including
handling AdditionalManifestFiles.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'linker-flags'
  test.run_gyp('generate-manifest.gyp', chdir=CHDIR)
  test.build('generate-manifest.gyp', test.ALL, chdir=CHDIR)
  test.built_file_must_exist('test_manifest_exe.exe.manifest', chdir=CHDIR)
  test.built_file_must_exist('test_manifest_dll.dll.manifest', chdir=CHDIR)

  
  
  extra1_manifest = test.built_file_path(
      'test_manifest_extra1.exe.manifest', chdir=CHDIR)
  test.must_contain(extra1_manifest, '35138b9a-5d96-4fbd-8e2d-a2440225f93a')
  test.must_not_contain(extra1_manifest, 'e2011457-1546-43c5-a5fe-008deee3d3f0')

  
  extra2_manifest = test.built_file_path(
      'test_manifest_extra2.exe.manifest', chdir=CHDIR)
  test.must_contain(extra2_manifest, '35138b9a-5d96-4fbd-8e2d-a2440225f93a')
  test.must_contain(extra2_manifest, 'e2011457-1546-43c5-a5fe-008deee3d3f0')

  
  extra_list_manifest = test.built_file_path(
      'test_manifest_extra_list.exe.manifest', chdir=CHDIR)
  test.must_contain(extra_list_manifest, '35138b9a-5d96-4fbd-8e2d-a2440225f93a')
  test.must_contain(extra_list_manifest, 'e2011457-1546-43c5-a5fe-008deee3d3f0')

  test.pass_test()
