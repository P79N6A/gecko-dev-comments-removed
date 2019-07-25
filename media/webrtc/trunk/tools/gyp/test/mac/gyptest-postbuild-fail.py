





"""
Verifies that a failing postbuild step lets the build fail.
"""

import TestGyp

import sys

if sys.platform == 'darwin':
  
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'],
                         match = lambda a, b: True)

  test.run_gyp('test.gyp', chdir='postbuild-fail')

  build_error_code = {
    'xcode': 1,
    'make': 2,
    'ninja': 1,
  }[test.format]


  
  
  


  
  test.build('test.gyp', 'nonbundle', chdir='postbuild-fail',
             status=build_error_code)
  test.built_file_must_exist('static_touch',
                             chdir='postbuild-fail')
  
  
  test.build('test.gyp', 'nonbundle', chdir='postbuild-fail',
             status=build_error_code)


  
  test.build('test.gyp', 'bundle', chdir='postbuild-fail',
             status=build_error_code)
  test.built_file_must_exist('dynamic_touch',
                             chdir='postbuild-fail')
  
  
  test.build('test.gyp', 'bundle', chdir='postbuild-fail',
             status=build_error_code)

  test.pass_test()
