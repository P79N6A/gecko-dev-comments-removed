





"""
Verifies build of an executable with C++ define specified by a gyp define, and
the use of the environment during regeneration when the gyp file changes.
"""

import os
import TestGyp



test = TestGyp.TestGyp(formats=['make', 'android'])

try:
  os.environ['GYP_DEFINES'] = 'value=50'
  test.run_gyp('defines.gyp')
finally:
  
  
  
  os.environ['GYP_DEFINES'] = ''
  del os.environ['GYP_DEFINES']

test.build('defines.gyp')

expect = """\
FOO is defined
VALUE is 1
2*PAREN_VALUE is 12
HASH_VALUE is a#1
"""
test.run_built_executable('defines', stdout=expect)



test.sleep()
test.write('defines.gyp', test.read('defines-env.gyp'))

test.build('defines.gyp', test.ALL)

expect = """\
VALUE is 50
"""
test.run_built_executable('defines', stdout=expect)

test.pass_test()
