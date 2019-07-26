





"""
Make sure runtime C library setting is extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'compiler-flags'
  test.run_gyp('runtime-library.gyp', chdir=CHDIR)
  test.build('runtime-library.gyp', test.ALL, chdir=CHDIR)

  test.pass_test()
