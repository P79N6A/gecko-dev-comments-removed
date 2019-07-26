





"""
Verifies build of an executable with C++ define specified by a gyp define, and
the use of the environment during regeneration when the gyp file changes.
"""

import os
import TestGyp

env_stack = []


def PushEnv():
  env_copy = os.environ.copy()
  env_stack.append(env_copy)

def PopEnv():
  os.eniron=env_stack.pop()



test = TestGyp.TestGyp(formats=['make', 'android'])

try:
  PushEnv()
  os.environ['CFLAGS'] = '-O0'
  test.run_gyp('cflags.gyp')
finally:
  
  
  
  PopEnv()

test.build('cflags.gyp')

expect = """\
Using no optimization flag
"""
test.run_built_executable('cflags', stdout=expect)

test.sleep()

try:
  PushEnv()
  os.environ['CFLAGS'] = '-O2'
  test.run_gyp('cflags.gyp')
finally:
  
  
  
  PopEnv()

test.build('cflags.gyp')

expect = """\
Using an optimization flag
"""
test.run_built_executable('cflags', stdout=expect)

test.pass_test()
