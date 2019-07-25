





"""
Verify that a hard_dependency that is not exported is not pulled in as a
dependency for a target if the target does not explicitly specify a dependency
and none of its dependencies export the hard_dependency.
"""

import TestGyp

test = TestGyp.TestGyp()

if test.format == 'dump_dependency_json':
  test.skip_test('Skipping test; dependency JSON does not adjust ' \
                 'static libaries.\n')

test.run_gyp('hard_dependency.gyp', chdir='src')

chdir = 'relocate/src'
test.relocate('src', chdir)

test.build('hard_dependency.gyp', 'd', chdir=chdir)




test.built_file_must_not_exist('a', type=test.STATIC_LIB, chdir=chdir)
test.built_file_must_not_exist('b', type=test.STATIC_LIB, chdir=chdir)
test.built_file_must_not_exist('c', type=test.STATIC_LIB, chdir=chdir)
test.built_file_must_exist('d', type=test.STATIC_LIB, chdir=chdir)

test.pass_test()
