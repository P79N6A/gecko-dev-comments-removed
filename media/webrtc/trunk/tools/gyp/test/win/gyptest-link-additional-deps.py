





"""
Make sure additional library dependencies are handled.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'linker-flags'
  test.run_gyp('additional-deps.gyp', chdir=CHDIR)
  test.build('additional-deps.gyp', test.ALL, chdir=CHDIR)

  test.pass_test()
