





"""
Verifies building a target and a subsidiary dependent target from a
.gyp file in a subdirectory, without specifying an explicit output build
directory, and using the generated solution or project file at the top
of the tree as the entry point.
                                 
The configuration sets the Xcode SYMROOT variable and uses --depth=
to make Xcode behave like the other build tools--that is, put all
built targets in a single output build directory at the top of the tree.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('prog1.gyp', '-Dset_symroot=1', '--depth=.', chdir='src')

test.relocate('src', 'relocate/src')


test.build('prog1.gyp', SYMROOT=None, chdir='relocate/src')

test.run_built_executable('prog1',
                          stdout="Hello from prog1.c\n",
                          chdir='relocate/src')

test.run_built_executable('prog2',
                          stdout="Hello from prog2.c\n",
                          chdir='relocate/src')

test.pass_test()
