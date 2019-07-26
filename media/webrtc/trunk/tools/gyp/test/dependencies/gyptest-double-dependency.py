





"""
Verify that pulling in a dependency a second time in a conditional works for
shared_library targets. Regression test for http://crbug.com/122588
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('double_dependency.gyp')


test.pass_test()
