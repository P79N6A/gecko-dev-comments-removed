





"""
Verifies inclusion of $HOME/.gyp/include.gypi works.
"""

import os
import TestGyp

test = TestGyp.TestGyp()

os.environ['HOME'] = os.path.abspath('home')

test.run_gyp('all.gyp', chdir='src')



test.relocate('src', 'relocate/src')

test.build('all.gyp', test.ALL, chdir='relocate/src')

test.run_built_executable('printfoo',
                          chdir='relocate/src',
                          stdout='FOO is fromhome\n')

test.pass_test()
