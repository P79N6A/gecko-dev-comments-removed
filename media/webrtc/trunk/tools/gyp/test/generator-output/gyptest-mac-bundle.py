





"""
Verifies mac bundles work with --generator-output.
"""

import TestGyp

import sys

if sys.platform == 'darwin':
  
  test = TestGyp.TestGyp(formats=['!ninja'])

  MAC_BUNDLE_DIR = 'mac-bundle'
  GYPFILES_DIR = 'gypfiles'
  test.writable(test.workpath(MAC_BUNDLE_DIR), False)
  test.run_gyp('test.gyp',
               '--generator-output=' + test.workpath(GYPFILES_DIR),
               chdir=MAC_BUNDLE_DIR)
  test.writable(test.workpath(MAC_BUNDLE_DIR), True)

  test.build('test.gyp', test.ALL, chdir=GYPFILES_DIR)

  test.pass_test()
