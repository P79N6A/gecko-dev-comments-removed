





"""
Verifies that a postbuild copying a dependend framework into an app bundle is
rerun if the resources in the framework change.
"""

import TestGyp

import os.path
import sys

if sys.platform == 'darwin':
  
  test = TestGyp.TestGyp(formats=['ninja', 'xcode'])

  CHDIR = 'postbuild-copy-bundle'
  test.run_gyp('test.gyp', chdir=CHDIR)

  app_bundle_dir = test.built_file_path('Test app.app', chdir=CHDIR)
  bundled_framework_dir = os.path.join(
      app_bundle_dir, 'Contents', 'My Framework.framework', 'Resources')
  final_plist_path = os.path.join(bundled_framework_dir, 'Info.plist')
  final_resource_path = os.path.join(bundled_framework_dir, 'resource_file.sb')

  
  test.build('test.gyp', 'test_app', chdir=CHDIR)
  test.must_exist(final_resource_path)
  test.must_match(final_resource_path,
                  'This is included in the framework bundle.\n')

  test.must_exist(final_plist_path)
  test.must_contain(final_plist_path, '''\
\t<key>RandomKey</key>
\t<string>RandomValue</string>''')

  
  
  test.sleep()
  test.write('postbuild-copy-bundle/resource_file.sb', 'New text\n')
  test.build('test.gyp', 'test_app', chdir=CHDIR)

  test.must_exist(final_resource_path)
  test.must_match(final_resource_path, 'New text\n')

  
  test.sleep()
  contents = test.read('postbuild-copy-bundle/Framework-Info.plist')
  contents = contents.replace('RandomValue', 'NewRandomValue')
  test.write('postbuild-copy-bundle/Framework-Info.plist', contents)
  test.build('test.gyp', 'test_app', chdir=CHDIR)

  test.must_exist(final_plist_path)
  test.must_contain(final_plist_path, '''\
\t<key>RandomKey</key>
\t<string>NewRandomValue</string>''')

  test.pass_test()
