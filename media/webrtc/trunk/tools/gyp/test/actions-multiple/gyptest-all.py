





"""
Verifies two actions can be attached to the same input files.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('actions.gyp', chdir='src')

test.relocate('src', 'relocate/src')


test.build('actions.gyp', test.ALL, chdir='relocate/src')
test.must_contain('relocate/src/output1.txt', 'hello there')
test.must_contain('relocate/src/output2.txt', 'hello there')
test.must_contain('relocate/src/output3.txt', 'hello there')
test.must_contain('relocate/src/output4.txt', 'hello there')



test.run_built_executable(
    'multiple_action_source_filter',
    chdir='relocate/src',
    stdout=(
        '{\n'
        'bar\n'
        'car\n'
        'dar\n'
        'ear\n'
        '}\n'
    ),
)


test.pass_test()
