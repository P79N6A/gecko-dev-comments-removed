





"""
Verifies that app bundles are built correctly.
"""

import TestGyp

import sys

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  test.run_gyp('framework.gyp', chdir='framework')

  test.build('framework.gyp', 'test_framework', chdir='framework')

  
  test.built_file_must_exist(
      'Test Framework.framework/Versions/A/Test Framework',
      chdir='framework')

  
  test.built_file_must_exist(
      'Test Framework.framework/Versions/A/Resources/Info.plist',
      chdir='framework')

  
  test.built_file_must_exist(
      'Test Framework.framework/Versions/A/Resources/English.lproj/'
      'InfoPlist.strings',
      chdir='framework')

  
  test.built_file_must_exist('Test Framework.framework/Versions/Current',
                             chdir='framework')
  test.built_file_must_exist('Test Framework.framework/Resources',
                             chdir='framework')
  test.built_file_must_exist('Test Framework.framework/Test Framework',
                             chdir='framework')
  
  test.built_file_must_not_exist(
      'Test Framework.framework/Versions/A/Resources/PkgInfo',
      chdir='framework')

  test.pass_test()
