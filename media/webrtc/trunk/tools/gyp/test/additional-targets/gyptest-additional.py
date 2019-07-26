





"""
Verifies simple actions when using an explicit build target of 'all'.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('all.gyp', chdir='src')
test.relocate('src', 'relocate/src')


test.build('all.gyp', chdir='relocate/src')

if test.format=='xcode':
  chdir = 'relocate/src/dir1'
else:
  chdir = 'relocate/src'


file_content = 'Hello from emit.py\n'
test.built_file_must_match('out2.txt', file_content, chdir=chdir)

test.built_file_must_not_exist('out.txt', chdir='relocate/src')
test.built_file_must_not_exist('foolib1',
                               type=test.SHARED_LIB,
                               chdir=chdir)


if test.format in ('make', 'ninja', 'android'):
  chdir='relocate/src'
else:
  chdir='relocate/src/dir1'


test.build('actions.gyp', 'action1_target', chdir=chdir)


file_content = 'Hello from emit.py\n'
test.built_file_must_exist('out.txt', chdir=chdir)


test.build('actions.gyp', 'foolib1', chdir=chdir)

test.built_file_must_exist('foolib1',
                           type=test.SHARED_LIB,
                           chdir=chdir,
                           subdir='dir1')

test.pass_test()
