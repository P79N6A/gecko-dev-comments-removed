





"""
Verifies that Makefiles get rebuilt when a source gyp file changes.
"""

import TestGyp



test = TestGyp.TestGyp(formats=['make', 'android'])

test.run_gyp('hello.gyp')

test.build('hello.gyp', test.ALL)

test.run_built_executable('hello', stdout="Hello, world!\n")



test.sleep()
test.write('hello.gyp', test.read('hello2.gyp'))

test.build('hello.gyp', test.ALL)

test.run_built_executable('hello', stdout="Hello, two!\n")

test.pass_test()
