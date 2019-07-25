





"""
Verified things don't explode when there are targets without outputs.
"""

import TestGyp



test = TestGyp.TestGyp(formats=['!ninja'])

test.run_gyp('nooutput.gyp', chdir='src')
test.relocate('src', 'relocate/src')
test.build('nooutput.gyp', chdir='relocate/src')

test.pass_test()
