





"""
Test that environment variables are ignored when --ignore-environment is
specified.
"""

import os

import TestGyp

test = TestGyp.TestGyp(format='gypd')

os.environ['GYP_DEFINES'] = 'FOO=BAR'
os.environ['GYP_GENERATORS'] = 'foo'
os.environ['GYP_GENERATOR_FLAGS'] = 'genflag=foo'
os.environ['GYP_GENERATOR_OUTPUT'] = 'somedir'

expect = test.read('commands.gyp.ignore-env.stdout').replace('\r', '')

test.run_gyp('commands.gyp',
             '--debug', 'variables', '--debug', 'general',
             '--ignore-environment',
             stdout=expect)










contents = test.read('commands.gypd').replace('\r', '')
expect = test.read('commands.gypd.golden').replace('\r', '')
if not test.match(contents, expect):
  print "Unexpected contents of `commands.gypd'"
  test.diff(expect, contents, 'commands.gypd ')
  test.fail_test()

test.pass_test()
