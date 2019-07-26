





"""
Make sure exception handling settings are extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'compiler-flags'
  test.run_gyp('exception-handling.gyp', chdir=CHDIR)

  
  test.build('exception-handling.gyp', 'test_eh_off', chdir=CHDIR,
      status=1)

  
  test.build('exception-handling.gyp', 'test_eh_s', chdir=CHDIR)
  test.build('exception-handling.gyp', 'test_eh_a', chdir=CHDIR)

  
  test.run_built_executable('test_eh_a', chdir=CHDIR, status=1)
  test.run_built_executable('test_eh_s', chdir=CHDIR, status=2)

  test.pass_test()
