





"""
Verifies that mac_framework_headers works properly.
"""

import TestGyp

import sys

if sys.platform == 'darwin':
  
  test = TestGyp.TestGyp(formats=['xcode'])

  CHDIR = 'framework-headers'
  test.run_gyp('test.gyp', chdir=CHDIR)

  
  test.build('test.gyp', 'test_framework_headers_framework', chdir=CHDIR)

  test.built_file_must_exist(
    'TestFramework.framework/Versions/A/TestFramework', chdir=CHDIR)

  test.built_file_must_exist(
    'TestFramework.framework/Versions/A/Headers/myframework.h', chdir=CHDIR)

  
  test.build('test.gyp', 'test_framework_headers_static', chdir=CHDIR)

  test.built_file_must_exist('libTestLibrary.a', chdir=CHDIR)

  test.built_file_must_exist('include/myframework.h', chdir=CHDIR)

  test.pass_test()
