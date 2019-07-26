





"""
Make sure warning level is extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'compiler-flags'
  test.run_gyp('warning-level.gyp', chdir=CHDIR)

  
  
  
  
  

  test.build('warning-level.gyp', 'test_wl1_fail', chdir=CHDIR, status=1)
  test.build('warning-level.gyp', 'test_wl1_pass', chdir=CHDIR)

  test.build('warning-level.gyp', 'test_wl2_fail', chdir=CHDIR, status=1)
  test.build('warning-level.gyp', 'test_wl2_pass', chdir=CHDIR)

  test.build('warning-level.gyp', 'test_wl3_fail', chdir=CHDIR, status=1)
  test.build('warning-level.gyp', 'test_wl3_pass', chdir=CHDIR)

  test.build('warning-level.gyp', 'test_wl4_fail', chdir=CHDIR, status=1)

  test.build('warning-level.gyp', 'test_def_fail', chdir=CHDIR, status=1)
  test.build('warning-level.gyp', 'test_def_pass', chdir=CHDIR)

  test.pass_test()
