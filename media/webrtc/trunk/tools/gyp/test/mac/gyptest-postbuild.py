





"""
Verifies that postbuild steps work.
"""

import TestGyp

import sys

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  test.run_gyp('test.gyp', chdir='postbuilds')

  test.build('test.gyp', test.ALL, chdir='postbuilds')

  
  if test.format == 'xcode':
    chdir = 'postbuilds/subdirectory'
  else:
    chdir = 'postbuilds'

  
  test.built_file_must_exist('el.a_touch',
                             type=test.STATIC_LIB,
                             chdir='postbuilds')
  test.built_file_must_exist('el.a_gyp_touch',
                             type=test.STATIC_LIB,
                             chdir='postbuilds')
  test.built_file_must_exist('nest_el.a_touch',
                             type=test.STATIC_LIB,
                             chdir=chdir)
  test.built_file_must_exist(
      'dyna.framework/Versions/A/dyna_touch',
      chdir='postbuilds')
  test.built_file_must_exist(
      'dyna.framework/Versions/A/dyna_gyp_touch',
      chdir='postbuilds')
  test.built_file_must_exist(
      'nest_dyna.framework/Versions/A/nest_dyna_touch',
      chdir=chdir)
  test.built_file_must_exist('dyna_standalone.dylib_gyp_touch',
                             type=test.SHARED_LIB,
                             chdir='postbuilds')
  test.built_file_must_exist('copied_file.txt', chdir='postbuilds')
  test.built_file_must_exist('copied_file_2.txt', chdir=chdir)

  test.pass_test()
