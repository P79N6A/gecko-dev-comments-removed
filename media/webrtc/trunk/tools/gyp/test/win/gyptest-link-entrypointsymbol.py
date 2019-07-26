





"""
Make sure entrypointsymbol setting is extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'linker-flags'
  test.run_gyp('entrypointsymbol.gyp', chdir=CHDIR)

  test.build('entrypointsymbol.gyp', 'test_ok', chdir=CHDIR)
  test.build('entrypointsymbol.gyp', 'test_fail', chdir=CHDIR, status=1)

  test.pass_test()
