





"""
Make sure pdb is named as expected (shared between .cc files).
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['ninja'])

  CHDIR = 'compiler-flags'
  test.run_gyp('pdbname.gyp', chdir=CHDIR)
  test.build('pdbname.gyp', test.ALL, chdir=CHDIR)

  
  
  test.built_file_must_exist('test_pdbname.pdb', chdir=CHDIR)

  test.pass_test()
