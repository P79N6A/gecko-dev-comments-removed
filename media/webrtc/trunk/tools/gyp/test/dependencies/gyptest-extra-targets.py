





"""
Verify that dependencies don't pull unused targets into the build.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('extra_targets.gyp')



test.build('extra_targets.gyp', test.ALL)

test.pass_test()
