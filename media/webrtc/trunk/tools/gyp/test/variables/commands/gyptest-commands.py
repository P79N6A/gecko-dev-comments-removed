





"""
Test variable expansion of '<!()' syntax commands.
"""

import os

import TestGyp

test = TestGyp.TestGyp(format='gypd')

expect = test.read('commands.gyp.stdout').replace('\r', '')

test.run_gyp('commands.gyp',
             '--debug', 'variables',
             stdout=expect, ignore_line_numbers=True)










contents = test.read('commands.gypd').replace('\r', '')
expect = test.read('commands.gypd.golden').replace('\r', '')
if not test.match(contents, expect):
  print "Unexpected contents of `commands.gypd'"
  test.diff(expect, contents, 'commands.gypd ')
  test.fail_test()

test.pass_test()
