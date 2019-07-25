





"""
Verifies that 'copies' with app bundles are handled correctly.
"""

import TestGyp

import os
import sys
import time

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  test.run_gyp('framework.gyp', chdir='framework')

  test.build('framework.gyp', 'copy_target', chdir='framework')

  
  test.built_file_must_exist(
      'Test Framework.framework/foo/Dependency Bundle.framework',
      chdir='framework')
  test.built_file_must_exist(
      'Test Framework.framework/foo/Dependency Bundle.framework/Versions/A',
      chdir='framework')
  test.built_file_must_exist(
      'Test Framework.framework/Versions/A/Libraries/empty.c',
      chdir='framework')


  
  dep_bundle = test.built_file_path('Dependency Bundle.framework',
                                    chdir='framework')
  mtime = os.path.getmtime(dep_bundle)
  atime = os.path.getatime(dep_bundle)
  for i in range(3):
    os.utime(dep_bundle, (atime + i * 1000, mtime + i * 1000))
    test.build('framework.gyp', 'copy_target', chdir='framework')


  
  test.built_file_must_exist('action_file', chdir='framework')

  test.pass_test()
