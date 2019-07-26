





"""
Make sure RTC setting is extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'compiler-flags'
  test.run_gyp('runtime-checks.gyp', chdir=CHDIR)

  
  test.build('runtime-checks.gyp', 'test_brc_none', chdir=CHDIR, status=1)

  
  test.build('runtime-checks.gyp', 'test_brc_1', chdir=CHDIR)

  
  

  test.pass_test()
