





"""
Verifies that Makefiles don't get rebuilt when a source gyp file changes and
the disable_regeneration generator flag is set.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('hello.gyp', '-Gauto_regeneration=0')

test.build('hello.gyp', test.ALL)

test.run_built_executable('hello', stdout="Hello, world!\n")



test.sleep()
test.write('hello.gyp', test.read('hello2.gyp'))

test.build('hello.gyp', test.ALL)


test.run_built_executable('hello', stdout="Hello, world!\n")

test.pass_test()
