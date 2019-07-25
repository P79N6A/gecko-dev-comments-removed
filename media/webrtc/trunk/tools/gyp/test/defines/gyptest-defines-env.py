





"""
Verifies build of an executable with C++ define specified by a gyp define.
"""

import os
import TestGyp

test = TestGyp.TestGyp()


try:
  os.environ['GYP_DEFINES'] = 'value=10'
  test.run_gyp('defines-env.gyp')
finally:
  del os.environ['GYP_DEFINES']

test.build('defines-env.gyp')

expect = """\
VALUE is 10
"""
test.run_built_executable('defines', stdout=expect)




try:
  os.environ['GYP_DEFINES'] = 'value=20'
  test.run_gyp('defines-env.gyp', '-Dvalue=25')
finally:
  del os.environ['GYP_DEFINES']

test.sleep()
test.touch('defines.c')
test.build('defines-env.gyp')

expect = """\
VALUE is 25
"""
test.run_built_executable('defines', stdout=expect)




try:
  os.environ['GYP_DEFINES'] = 'value=30'
  test.run_gyp('defines-env.gyp', '--ignore-environment')
finally:
  del os.environ['GYP_DEFINES']

test.sleep()
test.touch('defines.c')
test.build('defines-env.gyp')

expect = """\
VALUE is 5
"""
test.run_built_executable('defines', stdout=expect)




try:
  os.environ['GYP_DEFINES'] = 'value=40'
  test.run_gyp('defines-env.gyp', '--ignore-environment', '-Dvalue=45')
finally:
  del os.environ['GYP_DEFINES']

test.sleep()
test.touch('defines.c')
test.build('defines-env.gyp')

expect = """\
VALUE is 45
"""
test.run_built_executable('defines', stdout=expect)


test.pass_test()
