





"""
Handle default .idl build rules.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'idl-rules'
  test.run_gyp('basic-idl.gyp', chdir=CHDIR)
  test.build('basic-idl.gyp', test.ALL, chdir=CHDIR)

  test.pass_test()
