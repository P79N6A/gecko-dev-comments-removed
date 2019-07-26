





"""
Verifies that setting SDKROOT works.
"""

import TestGyp

import sys

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  test.run_gyp('test.gyp', chdir='sdkroot')
  test.build('test.gyp', test.ALL, chdir='sdkroot')
  test.pass_test()
