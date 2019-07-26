





"""
Verify that it is possible to build an object that depends on a
PrivateFramework.
"""

import os
import sys
import TestGyp

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  CHDIR = 'framework-dirs'
  test.run_gyp('framework-dirs.gyp', chdir=CHDIR)
  test.build('framework-dirs.gyp', 'calculate', chdir=CHDIR)

  test.pass_test()
