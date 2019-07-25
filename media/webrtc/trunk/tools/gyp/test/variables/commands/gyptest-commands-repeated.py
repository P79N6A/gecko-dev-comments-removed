





"""
Test variable expansion of '<!()' syntax commands where they are evaluated
more then once..
"""

import os

import TestGyp

test = TestGyp.TestGyp(format='gypd')

expect = test.read('commands-repeated.gyp.stdout').replace('\r', '')

test.run_gyp('commands-repeated.gyp',
             '--debug', 'variables', '--debug', 'general',
             stdout=expect)










contents = test.read('commands-repeated.gypd').replace('\r', '')
expect = test.read('commands-repeated.gypd.golden').replace('\r', '')
if not test.match(contents, expect):
  print "Unexpected contents of `commands-repeated.gypd'"
  test.diff(expect, contents, 'commands-repeated.gypd ')
  test.fail_test()

test.pass_test()
