





"""
Verifies libraries (in link_settings) are properly found.
"""

import TestGyp

import sys

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  test.run_gyp('subdir/test.gyp', chdir='libraries')

  test.build('subdir/test.gyp', test.ALL, chdir='libraries')

  test.pass_test()
