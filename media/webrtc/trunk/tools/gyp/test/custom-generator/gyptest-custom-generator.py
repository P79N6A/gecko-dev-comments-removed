




"""Test that custom generators can be passed to --format
"""

import TestGyp

test = TestGyp.TestGypCustom(format='mygenerator.py')
test.run_gyp('test.gyp')



test.must_match('MyBuildFile', 'Testing...\n')

test.pass_test()
