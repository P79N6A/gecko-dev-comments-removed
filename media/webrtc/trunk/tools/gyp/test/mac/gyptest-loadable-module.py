





"""
Tests that a loadable_module target is built correctly.
"""

import TestGyp

import os
import sys

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  test.run_gyp('test.gyp', chdir='loadable-module')
  test.build('test.gyp', test.ALL, chdir='loadable-module')

  
  test.built_file_must_exist(
      'test_loadable_module.plugin/Contents/MacOS/test_loadable_module',
      chdir='loadable-module')

  
  info_plist = test.built_file_path(
      'test_loadable_module.plugin/Contents/Info.plist',
      chdir='loadable-module')
  test.must_exist(info_plist)
  test.must_contain(info_plist, """
	<key>CFBundleExecutable</key>
	<string>test_loadable_module</string>
""")

  
  test.built_file_must_not_exist(
      'test_loadable_module.plugin/Contents/PkgInfo',
      chdir='loadable-module')
  test.built_file_must_not_exist(
      'test_loadable_module.plugin/Contents/Resources',
      chdir='loadable-module')

  test.pass_test()
