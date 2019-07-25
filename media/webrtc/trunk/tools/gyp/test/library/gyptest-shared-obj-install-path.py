





"""
Verifies that .so files that are order only dependencies are specified by
their install location rather than by their alias.
"""


from __future__ import with_statement

import os
import TestGyp

test = TestGyp.TestGyp(formats=['make'])

test.run_gyp('shared_dependency.gyp',
             chdir='src')
test.relocate('src', 'relocate/src')

test.build('shared_dependency.gyp', test.ALL, chdir='relocate/src')

with open('relocate/src/Makefile') as makefile:
  make_contents = makefile.read()



make_contents = make_contents.replace('include lib1.target.mk', '')
with open('relocate/src/Makefile', 'w') as makefile:
  makefile.write(make_contents)

test.build('shared_dependency.gyp', test.ALL, chdir='relocate/src')

test.pass_test()
