





"""
Make sure .s files aren't passed to cl.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'asm-files'
  test.run_gyp('asm-files.gyp', chdir=CHDIR)
  
  
  
  
  test.build('asm-files.gyp', test.ALL, chdir=CHDIR)

  test.pass_test()
