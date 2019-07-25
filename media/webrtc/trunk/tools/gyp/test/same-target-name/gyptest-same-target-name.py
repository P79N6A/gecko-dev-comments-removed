





"""
Check that duplicate targets in a directory gives an error.
"""

import TestGyp

test = TestGyp.TestGyp()


test.run_gyp('all.gyp', chdir='src', status=1, stderr=None)

test.pass_test()
