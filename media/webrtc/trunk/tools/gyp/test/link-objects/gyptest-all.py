





"""
Put an object file on the sources list.
Expect the result to link ok.
"""

import TestGyp

import sys

if sys.platform != 'darwin':
  
  test = TestGyp.TestGyp(formats=['make'])

  test.run_gyp('link-objects.gyp')

  test.build('link-objects.gyp', test.ALL)

  test.run_built_executable('link-objects', stdout="PASS\n")

  test.up_to_date('link-objects.gyp', test.ALL)

  test.pass_test()
