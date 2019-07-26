





"""
Make sure character set setting is extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'compiler-flags'
  test.run_gyp('character-set.gyp', chdir=CHDIR)
  test.build('character-set.gyp', test.ALL, chdir=CHDIR)

  test.pass_test()
