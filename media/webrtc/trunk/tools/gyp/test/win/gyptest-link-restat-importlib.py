





"""
Make sure we don't cause unnecessary builds due to import libs appearing
to be out of date.
"""

import TestGyp

import sys
import time

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'importlib'
  test.run_gyp('importlib.gyp', chdir=CHDIR)
  test.build('importlib.gyp', test.ALL, chdir=CHDIR)

  
  
  test.sleep()

  
  
  test.touch('importlib/has-exports.cc')
  test.build('importlib.gyp', 'test_importlib', chdir=CHDIR)

  
  
  
  
  test.up_to_date('importlib.gyp', test.ALL, chdir=CHDIR)

  test.pass_test()
