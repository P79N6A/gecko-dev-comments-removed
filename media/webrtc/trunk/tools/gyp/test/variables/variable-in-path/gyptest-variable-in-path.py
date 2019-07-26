





"""
Make sure <(CONFIGURATION_NAME) variable is correctly expanded.
"""

import TestGyp

import sys

test = TestGyp.TestGyp()
test.set_configuration('C1')

test.run_gyp('variable-in-path.gyp')
test.build('variable-in-path.gyp', 'hello1')
test.build('variable-in-path.gyp', 'hello2')


test.pass_test()
