





"""
Verifies that the global xcode_settings processing doesn't throw.
Regression test for http://crbug.com/109163
"""

import TestGyp

import sys

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])
  test.run_gyp('src/dir2/dir2.gyp', chdir='global-settings', depth='src')
  

  
  test.build('dir2/dir2.gyp', 'dir2_target', chdir='global-settings/src',
             SYMROOT='../build')
  test.built_file_must_exist('file.txt', chdir='global-settings/src')

  test.pass_test()
