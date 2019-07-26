





"""
Make sure paths are normalized with VS macros properly expanded on Windows.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['ninja'])

  test.run_gyp('normalize-paths.gyp')

  
  
  
  subninja = open(test.built_file_path('obj/some_target.ninja')).read()
  if '$!product_dir' in subninja:
    test.fail_test()
  if 'out\\Default' in subninja:
    test.fail_test()

  second = open(test.built_file_path('obj/second.ninja')).read()
  if ('..\\..\\things\\AnotherName.exe' in second or
      'AnotherName.exe' not in second):
    test.fail_test()

  action = open(test.built_file_path('obj/action.ninja')).read()
  if '..\\..\\out\\Default' in action:
    test.fail_test()
  if '..\\..\\SomethingElse' in action or 'SomethingElse' not in action:
    test.fail_test()
  if '..\\..\\SomeOtherInput' in action or 'SomeOtherInput' not in action:
    test.fail_test()

  test.pass_test()
