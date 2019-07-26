





"""
Verifies that xcode-style GCC_... settings are handled properly.
"""

import TestGyp

import sys

def IgnoreOutput(string, expected_string):
  return True

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  CHDIR = 'xcode-gcc'
  test.run_gyp('test.gyp', chdir=CHDIR)

  
  
  targets = [
    'warn_about_invalid_offsetof_macro',
    'warn_about_missing_newline',
  ]

  for target in targets:
    test.build('test.gyp', target, chdir=CHDIR)
    test.built_file_must_exist(target, chdir=CHDIR)
    fail_target = target + '-fail'
    test.build('test.gyp', fail_target, chdir=CHDIR, status=None,
               stderr=None, match=IgnoreOutput)
    test.built_file_must_not_exist(fail_target, chdir=CHDIR)

  test.pass_test()
