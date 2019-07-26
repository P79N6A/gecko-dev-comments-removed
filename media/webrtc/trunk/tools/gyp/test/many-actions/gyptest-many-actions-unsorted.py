





"""
Make sure lots of actions in the same target don't cause exceeding command
line length.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('many-actions-unsorted.gyp')
test.build('many-actions-unsorted.gyp', test.ALL)
for i in range(15):
  test.built_file_must_exist('generated_%d.h' % i)



test.touch('file1')
test.build('many-actions-unsorted.gyp', test.ALL)

test.touch('file0')
test.build('many-actions-unsorted.gyp', test.ALL)

test.touch('file2')
test.touch('file3')
test.touch('file4')
test.build('many-actions-unsorted.gyp', test.ALL)

test.pass_test()
