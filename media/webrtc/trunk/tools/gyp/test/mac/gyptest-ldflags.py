





"""
Verifies that filenames passed to various linker flags are converted into
build-directory relative paths correctly.
"""

import TestGyp

import sys

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  CHDIR = 'ldflags'
  test.run_gyp('subdirectory/test.gyp', chdir=CHDIR)

  test.build('subdirectory/test.gyp', test.ALL, chdir=CHDIR)

  test.pass_test()












































