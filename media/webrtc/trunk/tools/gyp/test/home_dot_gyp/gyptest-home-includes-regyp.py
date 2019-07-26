





"""
Verifies inclusion of $HOME/.gyp/include.gypi works properly with relocation
and with regeneration.
"""

import os
import TestGyp



test = TestGyp.TestGyp(formats=['make', 'android'])

os.environ['HOME'] = os.path.abspath('home')

test.run_gyp('all.gyp', chdir='src')



test.relocate('src', 'relocate/src')

test.build('all.gyp', test.ALL, chdir='relocate/src')

test.run_built_executable('printfoo',
                          chdir='relocate/src',
                          stdout='FOO is fromhome\n')


test.sleep()

test.write('home/.gyp/include.gypi', test.read('home2/.gyp/include.gypi'))

test.build('all.gyp', test.ALL, chdir='relocate/src')

test.run_built_executable('printfoo',
                          chdir='relocate/src',
                          stdout='FOO is fromhome2\n')

test.pass_test()
