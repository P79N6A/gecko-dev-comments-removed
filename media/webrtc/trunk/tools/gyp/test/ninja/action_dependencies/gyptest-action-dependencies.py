





"""
Verify that building an object file correctly depends on running actions in
dependent targets, but not the targets themselves.
"""

import os
import sys
import TestGyp








test = TestGyp.TestGyp(formats=['ninja'])

test.run_gyp('action_dependencies.gyp', chdir='src')

chdir = 'relocate/src'
test.relocate('src', chdir)

objext = '.obj' if sys.platform == 'win32' else '.o'

test.build('action_dependencies.gyp',
           os.path.join('obj', 'b.b' + objext),
           chdir=chdir)



test.built_file_must_not_exist('a', type=test.STATIC_LIB, chdir=chdir)
test.built_file_must_not_exist('b', type=test.STATIC_LIB, chdir=chdir)
test.built_file_must_exist(os.path.join('obj', 'b.b' + objext), chdir=chdir)

test.build('action_dependencies.gyp',
           os.path.join('obj', 'c.c' + objext),
           chdir=chdir)



test.built_file_must_exist('a', type=test.STATIC_LIB, chdir=chdir)
test.built_file_must_exist('b', type=test.EXECUTABLE, chdir=chdir)
test.built_file_must_exist(os.path.join('obj', 'c.c' + objext), chdir=chdir)


test.pass_test()
