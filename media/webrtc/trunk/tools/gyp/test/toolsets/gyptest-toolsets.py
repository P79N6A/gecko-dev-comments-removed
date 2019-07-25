





"""
Verifies that toolsets are correctly applied
"""

import TestGyp


test = TestGyp.TestGyp(formats=['make'])

test.run_gyp('toolsets.gyp')

test.build('toolsets.gyp', test.ALL)

test.run_built_executable('host-main', stdout="Host\n")
test.run_built_executable('target-main', stdout="Target\n")

test.pass_test()
