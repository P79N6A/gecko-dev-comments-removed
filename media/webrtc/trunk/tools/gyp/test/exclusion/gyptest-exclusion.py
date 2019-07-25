





"""
Verifies that exclusions (e.g. sources!) are respected.  Excluded sources
that do not exist should not prevent the build from succeeding.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('exclusion.gyp')
test.build('exclusion.gyp')


test.built_file_must_exist('hello' + test._exe, test.EXECUTABLE, bare=True)

test.pass_test()
