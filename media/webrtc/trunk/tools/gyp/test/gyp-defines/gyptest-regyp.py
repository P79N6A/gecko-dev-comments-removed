





"""
Verifies that when the same value is repeated for a gyp define, duplicates are
stripped from the regeneration rule.
"""

import os
import TestGyp



test = TestGyp.TestGyp(formats=['make', 'android'])

os.environ['GYP_DEFINES'] = 'key=repeated_value key=value1 key=repeated_value'
test.run_gyp('defines.gyp')
test.build('defines.gyp')



test.must_contain('action.txt', 'repeated_value')


test.must_not_contain(
    'Makefile', '"-Dkey=repeated_value" "-Dkey=value1" "-Dkey=repeated_value"')
test.must_contain('Makefile', '"-Dkey=value1" "-Dkey=repeated_value"')



test.sleep()
os.utime("defines.gyp", None)

test.build('defines.gyp')
test.must_contain('action.txt', 'repeated_value')

test.pass_test()
