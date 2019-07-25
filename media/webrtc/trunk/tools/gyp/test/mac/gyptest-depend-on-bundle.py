





"""
Verifies that a dependency on a bundle causes the whole bundle to be built.
"""

import TestGyp

import sys

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  test.run_gyp('test.gyp', chdir='depend-on-bundle')

  test.build('test.gyp', 'dependent_on_bundle', chdir='depend-on-bundle')

  
  test.built_file_must_exist('dependent_on_bundle', chdir='depend-on-bundle')

  
  test.built_file_must_exist(
      'my_bundle.framework/Versions/A/my_bundle',
      chdir='depend-on-bundle')
  test.built_file_must_exist(  
      'my_bundle.framework/my_bundle',
      chdir='depend-on-bundle')
  test.built_file_must_exist(  
      'my_bundle.framework/Versions/A/Resources/Info.plist',
      chdir='depend-on-bundle')
  test.built_file_must_exist(
      'my_bundle.framework/Versions/A/Resources/English.lproj/'  
      'InfoPlist.strings',
      chdir='depend-on-bundle')

  test.pass_test()
