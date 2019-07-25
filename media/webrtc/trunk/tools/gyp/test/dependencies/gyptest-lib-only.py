





"""
Verify that a link time only dependency will get pulled into the set of built
targets, even if no executable uses it.
"""

import TestGyp

import sys

test = TestGyp.TestGyp()

test.run_gyp('lib_only.gyp')

test.build('lib_only.gyp', test.ALL)

test.built_file_must_exist('a', type=test.STATIC_LIB)





if sys.platform == 'darwin':
  if test.format == 'xcode':
    test.built_file_must_not_exist('b', type=test.STATIC_LIB)
  else:
    assert test.format in ('make', 'ninja')
    test.built_file_must_exist('b', type=test.STATIC_LIB)
else:
  
  
  test.built_file_must_exist('b', type=test.STATIC_LIB, subdir='b')

test.pass_test()
