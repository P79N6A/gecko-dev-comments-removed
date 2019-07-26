





"""
Make sure warning-as-error is extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'compiler-flags'
  test.run_gyp('warning-as-error.gyp', chdir=CHDIR)

  
  
  

  test.build('warning-as-error.gyp', 'test_warn_as_error_false', chdir=CHDIR)
  test.build('warning-as-error.gyp', 'test_warn_as_error_unset', chdir=CHDIR)
  test.build('warning-as-error.gyp', 'test_warn_as_error_true', chdir=CHDIR,
    status=1)

  test.pass_test()
