





"""
Verifies that app bundles are rebuilt correctly.
"""

import TestGyp

import sys

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  CHDIR = 'rebuild'
  test.run_gyp('test.gyp', chdir=CHDIR)

  test.build('test.gyp', 'test_app', chdir=CHDIR)

  
  test.touch('rebuild/main.c')
  test.build('test.gyp', 'test_app', chdir=CHDIR)

  test.up_to_date('test.gyp', 'test_app', chdir=CHDIR)

  
  
  if test.format != 'xcode':
    
    test.build('test.gyp', 'test_framework_postbuilds', chdir=CHDIR)
    test.up_to_date('test.gyp', 'test_framework_postbuilds', chdir=CHDIR)

    
    
    test.build('test.gyp', 'test_app_postbuilds', chdir=CHDIR)
    test.up_to_date('test.gyp', 'test_app_postbuilds', chdir=CHDIR)

  test.pass_test()
