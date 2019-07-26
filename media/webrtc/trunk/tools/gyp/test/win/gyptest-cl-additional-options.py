





"""
Make sure additional manual compiler flags are extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'compiler-flags'
  test.run_gyp('additional-options.gyp', chdir=CHDIR)

  
  test.build('additional-options.gyp', 'test_additional_none', chdir=CHDIR,
      status=1)

  
  test.build('additional-options.gyp', 'test_additional_one', chdir=CHDIR)

  test.pass_test()
