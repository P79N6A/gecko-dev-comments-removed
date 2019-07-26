





"""
Handle VS macro expansion containing gyp variables.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'vs-macros'
  test.run_gyp('containing-gyp.gyp', chdir=CHDIR)
  test.build('containing-gyp.gyp', test.ALL, chdir=CHDIR)
  test.pass_test()
