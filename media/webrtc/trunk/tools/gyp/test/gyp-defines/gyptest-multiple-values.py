





"""
Verifies that when multiple values are supplied for a gyp define, the last one
is used.
"""

import os
import TestGyp

test = TestGyp.TestGyp()

os.environ['GYP_DEFINES'] = 'key=value1 key=value2 key=value3'
test.run_gyp('defines.gyp')
test.build('defines.gyp')
test.must_contain('action.txt', 'value3')



os.environ['GYP_DEFINES'] = 'key=repeated_value key=value1 key=repeated_value'
test.run_gyp('defines.gyp')
if test.format == 'msvs' and not test.uses_msbuild:
  
  
  test.build('defines.gyp', rebuild=True)
else:
  test.build('defines.gyp')
test.must_contain('action.txt', 'repeated_value')

test.pass_test()
