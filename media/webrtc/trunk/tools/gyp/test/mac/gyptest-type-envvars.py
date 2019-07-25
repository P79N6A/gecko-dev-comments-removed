





"""
Test that MACH_O_TYPE etc are set correctly.
"""

import TestGyp

import sys

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  test.run_gyp('test.gyp', chdir='type_envvars')

  test.build('test.gyp', test.ALL, chdir='type_envvars')

  

  test.pass_test()
