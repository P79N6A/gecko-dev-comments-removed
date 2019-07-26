





"""
Make sure additional options are handled.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'linker-flags'
  test.run_gyp('additional-options.gyp', chdir=CHDIR)
  test.build('additional-options.gyp', test.ALL, chdir=CHDIR)

  test.pass_test()
