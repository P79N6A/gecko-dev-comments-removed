





"""
Test variable expansion of '<!()' syntax commands where they are evaluated
more then once..
"""

import TestGyp

test = TestGyp.TestGyp(format='gypd')

expect = test.read('commands-repeated.gyp.stdout').replace('\r\n', '\n')

test.run_gyp('commands-repeated.gyp',
             '--debug', 'variables',
             stdout=expect, ignore_line_numbers=True)










contents = test.read('commands-repeated.gypd').replace('\r\n', '\n')
expect = test.read('commands-repeated.gypd.golden').replace('\r\n', '\n')
if not test.match(contents, expect):
  print "Unexpected contents of `commands-repeated.gypd'"
  test.diff(expect, contents, 'commands-repeated.gypd ')
  test.fail_test()

test.pass_test()
