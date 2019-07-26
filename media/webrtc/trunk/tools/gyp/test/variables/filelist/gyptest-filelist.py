





"""
Test variable expansion of '<|(list.txt ...)' syntax commands.
"""

import os
import sys

import TestGyp

test = TestGyp.TestGyp(format='gypd')

expect = test.read('filelist.gyp.stdout')
if sys.platform == 'win32':
  expect = expect.replace('/', r'\\').replace('\r\n', '\n')

test.run_gyp('src/filelist.gyp',
             '--debug', 'variables',
             stdout=expect, ignore_line_numbers=True)










contents = test.read('src/filelist.gypd').replace(
    '\r', '').replace('\\\\', '/')
expect = test.read('filelist.gypd.golden').replace('\r', '')
if not test.match(contents, expect):
  print "Unexpected contents of `src/filelist.gypd'"
  test.diff(expect, contents, 'src/filelist.gypd ')
  test.fail_test()

contents = test.read('src/names.txt')
expect = 'John\nJacob\nJingleheimer\nSchmidt\n'
if not test.match(contents, expect):
  print "Unexpected contents of `src/names.txt'"
  test.diff(expect, contents, 'src/names.txt ')
  test.fail_test()

test.pass_test()
