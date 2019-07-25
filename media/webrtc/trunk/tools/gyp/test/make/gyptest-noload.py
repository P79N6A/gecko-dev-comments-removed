





"""
Tests the use of the NO_LOAD flag which makes loading sub .mk files
optional.
"""


from __future__ import with_statement

import os
import TestGyp

test = TestGyp.TestGyp(formats=['make'])

test.run_gyp('all.gyp', chdir='noload')

test.relocate('noload', 'relocate/noload')

test.build('build/all.gyp', test.ALL, chdir='relocate/noload')
test.run_built_executable('exe', chdir='relocate/noload',
                          stdout='Hello from shared.c.\n')


test.build('build/all.gyp', test.ALL, chdir='relocate/noload',
           arguments=['NO_LOAD=lib'])
test.run_built_executable('exe', chdir='relocate/noload',
                          stdout='Hello from shared.c.\n')
test.build('build/all.gyp', test.ALL, chdir='relocate/noload',
           arguments=['NO_LOAD=z'])
test.run_built_executable('exe', chdir='relocate/noload',
                          stdout='Hello from shared.c.\n')


with open('relocate/noload/main.c', 'a') as src_file:
  src_file.write("\n")
test.build('build/all.gyp', test.ALL, chdir='relocate/noload',
           arguments=['NO_LOAD=lib'])
test.run_built_executable('exe', chdir='relocate/noload',
                          stdout='Hello from shared.c.\n')


with open('relocate/noload/lib/shared.c', 'w') as shared_file:
  shared_file.write(
      '#include "shared.h"\n'
      'const char kSharedStr[] = "modified";\n'
  )
test.build('build/all.gyp', test.ALL, chdir='relocate/noload',
           arguments=['NO_LOAD=lib'])
test.run_built_executable('exe', chdir='relocate/noload',
                          stdout='Hello from shared.c.\n')

test.pass_test()
