





"""
Make sure we include the default libs.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'linker-flags'
  test.run_gyp('default-libs.gyp', chdir=CHDIR)
  test.build('default-libs.gyp', test.ALL, chdir=CHDIR)

  test.pass_test()
