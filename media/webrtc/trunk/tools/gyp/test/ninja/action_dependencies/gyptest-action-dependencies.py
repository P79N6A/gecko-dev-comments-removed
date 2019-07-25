





"""
Verify that building an object file correctly depends on running actions in
dependent targets, but not the targets themselves.
"""

import TestGyp








test = TestGyp.TestGyp(formats=['ninja'])

test.run_gyp('action_dependencies.gyp', chdir='src')

chdir = 'relocate/src'
test.relocate('src', chdir)

test.build('action_dependencies.gyp', 'obj/b.b.o', chdir=chdir)



test.built_file_must_not_exist('a', type=test.STATIC_LIB, chdir=chdir)
test.built_file_must_not_exist('b', type=test.STATIC_LIB, chdir=chdir)
test.built_file_must_exist('obj/b.b.o', chdir=chdir)

test.build('action_dependencies.gyp', 'obj/c.c.o', chdir=chdir)



test.built_file_must_exist('a', type=test.STATIC_LIB, chdir=chdir)
test.built_file_must_exist('b', type=test.EXECUTABLE, chdir=chdir)
test.built_file_must_exist('obj/c.c.o', chdir=chdir)


test.pass_test()
