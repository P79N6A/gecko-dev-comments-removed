





"""
Make sure rule names with non-"normal" characters in them don't cause
broken build files. This test was originally causing broken .ninja files.
"""

import TestGyp

test = TestGyp.TestGyp()
test.run_gyp('sanitize-rule-names.gyp')
test.build('sanitize-rule-names.gyp', test.ALL)
test.pass_test()
