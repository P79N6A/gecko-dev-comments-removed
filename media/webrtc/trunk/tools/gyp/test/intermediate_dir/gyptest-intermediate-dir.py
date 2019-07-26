





"""
Verifies that targets have independent INTERMEDIATE_DIRs.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('test.gyp', chdir='src')
test.build('test.gyp', 'target1', chdir='src')

intermediate_file1 = test.read('src/outfile.txt')
test.must_contain(intermediate_file1, 'target1')

shared_intermediate_file1 = test.read('src/shared_outfile.txt')
test.must_contain(shared_intermediate_file1, 'shared_target1')

test.run_gyp('test2.gyp', chdir='src')

test.sleep()
test.touch('src/shared_infile.txt')
test.build('test2.gyp', 'target2', chdir='src')


intermediate_file2 = test.read('src/outfile.txt')
test.must_contain(intermediate_file1, 'target1')
test.must_contain(intermediate_file2, 'target2')

shared_intermediate_file2 = test.read('src/shared_outfile.txt')
if shared_intermediate_file1 != shared_intermediate_file2:
  test.fail_test(shared_intermediate_file1 + ' != ' + shared_intermediate_file2)

test.must_contain(shared_intermediate_file1, 'shared_target2')
test.must_contain(shared_intermediate_file2, 'shared_target2')

test.pass_test()
