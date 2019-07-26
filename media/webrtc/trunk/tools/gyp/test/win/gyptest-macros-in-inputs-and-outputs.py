





"""
Handle macro expansion in inputs and outputs of rules.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'vs-macros'
  test.run_gyp('input-output-macros.gyp', chdir=CHDIR)

  test.build('input-output-macros.gyp', 'test_expansions', chdir=CHDIR)

  test.built_file_must_exist('stuff.blah.something',
      content='Random data file.\nModified.',
      chdir=CHDIR)

  test.pass_test()
