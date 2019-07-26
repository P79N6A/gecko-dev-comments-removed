





"""
Make sure that when ULDI is on, we link .objs that make up .libs rather than
the .libs themselves.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'uldi'
  test.run_gyp('uldi.gyp', chdir=CHDIR)
  
  
  test.build('uldi.gyp', 'final_uldi', chdir=CHDIR, status=1)
  
  
  test.build('uldi.gyp', 'final_no_uldi', chdir=CHDIR)

  test.pass_test()
