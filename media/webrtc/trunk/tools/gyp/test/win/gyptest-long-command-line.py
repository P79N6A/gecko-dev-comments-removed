





"""
Make sure long command lines work.
"""

import TestGyp

import subprocess
import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['ninja', 'msvs'])

  CHDIR = 'long-command-line'
  test.run_gyp('long-command-line.gyp', chdir=CHDIR)
  test.build('long-command-line.gyp', test.ALL, chdir=CHDIR)

  test.pass_test()
