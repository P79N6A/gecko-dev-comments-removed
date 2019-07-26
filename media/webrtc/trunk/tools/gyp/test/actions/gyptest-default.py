





"""
Verifies simple actions when using the default build target.
"""

import TestGyp

test = TestGyp.TestGyp(workdir='workarea_default')

test.run_gyp('actions.gyp', chdir='src')

test.relocate('src', 'relocate/src')






if test.format in ['ninja', 'android']:
  test.build('actions.gyp', test.ALL, chdir='relocate/src')
else:
  
  
  test.build('actions.gyp', chdir='relocate/src')
  test.must_match('relocate/src/subdir1/actions-out/action-counter.txt', '1')
  test.must_match('relocate/src/subdir1/actions-out/action-counter_2.txt', '1')
  test.build('actions.gyp', chdir='relocate/src')
  test.must_match('relocate/src/subdir1/actions-out/action-counter.txt', '2')
  test.must_match('relocate/src/subdir1/actions-out/action-counter_2.txt', '2')

  
  
  
  
  test.build('actions.gyp', test.ALL, chdir='relocate/src')
  test.must_match('relocate/src/subdir1/actions-out/action-counter.txt', '2')
  test.must_match('relocate/src/subdir1/actions-out/action-counter_2.txt', '2')

expect = """\
Hello from program.c
Hello from make-prog1.py
Hello from make-prog2.py
"""

if test.format == 'xcode':
  chdir = 'relocate/src/subdir1'
else:
  chdir = 'relocate/src'
test.run_built_executable('program', chdir=chdir, stdout=expect)


test.must_match('relocate/src/subdir2/file.out', "Hello from make-file.py\n")


expect = "Hello from generate_main.py\n"

if test.format == 'xcode':
  chdir = 'relocate/src/subdir3'
else:
  chdir = 'relocate/src'
test.run_built_executable('null_input', chdir=chdir, stdout=expect)


test.pass_test()
