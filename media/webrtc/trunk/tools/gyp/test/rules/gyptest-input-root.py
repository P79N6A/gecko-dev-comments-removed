





"""
Verifies that RULE_INPUT_ROOT isn't turned into a path in rule actions
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('input-root.gyp', chdir='src')

test.relocate('src', 'relocate/src')

test.build('input-root.gyp', target='test', chdir='relocate/src')

expect = """\
Hello somefile
"""

test.run_built_executable('test', chdir='relocate/src', stdout=expect)
test.pass_test()
