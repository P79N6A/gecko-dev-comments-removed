





"""
Make sure link_settings containing -lblah.lib is remapped to just blah.lib.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['ninja'])

  CHDIR = 'linker-flags'
  test.run_gyp('library-adjust.gyp', chdir=CHDIR)
  test.build('library-adjust.gyp', test.ALL, chdir=CHDIR)
  test.pass_test()
