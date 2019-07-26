





"""
Make sure macro expansion of $(VCInstallDir) is handled, and specifically
always / terminated for compatibility.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'vs-macros'
  test.run_gyp('vcinstalldir.gyp', chdir=CHDIR)
  
  test.build('vcinstalldir.gyp', 'test_slash_trailing', chdir=CHDIR, status=1)
  test.build('vcinstalldir.gyp', 'test_slash_dir', chdir=CHDIR)
  test.pass_test()
