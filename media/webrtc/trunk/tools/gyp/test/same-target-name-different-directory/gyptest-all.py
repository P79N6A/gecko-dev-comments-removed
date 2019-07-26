





"""
Test cases when multiple targets in different directories have the same name.
"""

import TestGyp

test = TestGyp.TestGyp(formats=['android', 'ninja', 'make'])

test.run_gyp('subdirs.gyp', chdir='src')

test.relocate('src', 'relocate/src')


test.build('subdirs.gyp', 'target', chdir='relocate/src')
test.must_exist('relocate/src/subdir1/action1.txt')
test.must_exist('relocate/src/subdir2/action2.txt')



test.build('subdirs.gyp', 'target_same_action_name', chdir='relocate/src')
test.must_exist('relocate/src/subdir1/action.txt')
test.must_exist('relocate/src/subdir2/action.txt')



test.build('subdirs.gyp', 'target_same_rule_name', chdir='relocate/src')
test.must_exist('relocate/src/subdir1/rule.txt')
test.must_exist('relocate/src/subdir2/rule.txt')

test.pass_test()
