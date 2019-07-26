





"""
Make sure debug format settings are extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['ninja'])

  CHDIR = 'compiler-flags'
  test.run_gyp('debug-format.gyp', chdir=CHDIR)

  
  
  

  ninja_file = test.built_file_path('obj/test-debug-format-off.ninja',
      chdir=CHDIR)
  test.must_not_contain(ninja_file, '/Z7')
  test.must_not_contain(ninja_file, '/Zi')
  test.must_not_contain(ninja_file, '/ZI')

  ninja_file = test.built_file_path('obj/test-debug-format-oldstyle.ninja',
      chdir=CHDIR)
  test.must_contain(ninja_file, '/Z7')

  ninja_file = test.built_file_path('obj/test-debug-format-pdb.ninja',
      chdir=CHDIR)
  test.must_contain(ninja_file, '/Zi')

  ninja_file = test.built_file_path('obj/test-debug-format-editcontinue.ninja',
      chdir=CHDIR)
  test.must_contain(ninja_file, '/ZI')

  test.pass_test()
