





"""
Tests the msvs specific msvs_target_platform option.
"""

import TestGyp
import TestCommon


def RunX64(exe, stdout):
  try:
    test.run_built_executable(exe, stdout=stdout)
  except WindowsError, e:
    
    
    
    if e.errno != 193 and '[Error 193]' not in str(e):
      raise


test = TestGyp.TestGyp(formats=['msvs'])

test.run_gyp('configurations.gyp')

test.set_configuration('Debug|x64')
test.build('configurations.gyp', rebuild=True)
RunX64('front_left', stdout=('left\n'))
RunX64('front_right', stdout=('right\n'))

test.set_configuration('Debug|Win32')
test.build('configurations.gyp', rebuild=True)
RunX64('front_left', stdout=('left\n'))
test.run_built_executable('front_right', stdout=('right\n'))

test.pass_test()
