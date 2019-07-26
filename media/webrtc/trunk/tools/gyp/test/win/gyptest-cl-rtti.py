





"""
Make sure RTTI setting is extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'compiler-flags'
  test.run_gyp('rtti.gyp', chdir=CHDIR)

  
  test.build('rtti.gyp', 'test_rtti_off', chdir=CHDIR, status=1)

  
  test.build('rtti.gyp', 'test_rtti_on', chdir=CHDIR)

  
  test.build('rtti.gyp', 'test_rtti_unset', chdir=CHDIR)

  test.pass_test()
