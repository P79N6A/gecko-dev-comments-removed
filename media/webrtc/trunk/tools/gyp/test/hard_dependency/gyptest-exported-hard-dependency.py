





"""
Verify that a hard_dependency that is exported is pulled in as a dependency
for a target if the target is a static library and if the generator will
remove dependencies between static libraries.
"""

import TestGyp

test = TestGyp.TestGyp()

if test.format == 'dump_dependency_json':
  test.skip_test('Skipping test; dependency JSON does not adjust ' \
                 'static libraries.\n')

test.run_gyp('hard_dependency.gyp', chdir='src')

chdir = 'relocate/src'
test.relocate('src', chdir)

test.build('hard_dependency.gyp', 'c', chdir=chdir)





test.built_file_must_exist('a', type=test.STATIC_LIB, chdir=chdir)
test.built_file_must_not_exist('b', type=test.STATIC_LIB, chdir=chdir)
test.built_file_must_exist('c', type=test.STATIC_LIB, chdir=chdir)
test.built_file_must_not_exist('d', type=test.STATIC_LIB, chdir=chdir)

test.pass_test()
