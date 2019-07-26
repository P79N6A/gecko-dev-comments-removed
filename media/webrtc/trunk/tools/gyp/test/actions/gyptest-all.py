





"""
Verifies simple actions when using an explicit build target of 'all'.
"""

import glob
import os
import TestGyp

test = TestGyp.TestGyp(workdir='workarea_all')

test.run_gyp('actions.gyp', chdir='src')

test.relocate('src', 'relocate/src')






if test.format in ['ninja', 'android']:
  test.build('actions.gyp', test.ALL, chdir='relocate/src')
else:
  
  
  test.build('actions.gyp', test.ALL, chdir='relocate/src')
  test.must_match('relocate/src/subdir1/actions-out/action-counter.txt', '1')
  test.must_match('relocate/src/subdir1/actions-out/action-counter_2.txt', '1')
  test.build('actions.gyp', test.ALL, chdir='relocate/src')
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



def clean_dep_files():
  for file in (glob.glob('relocate/src/dep_*.txt') +
               glob.glob('relocate/src/deps_all_done_*.txt')):
    if os.path.exists(file):
      os.remove(file)


clean_dep_files()
test.must_not_exist('relocate/src/dep_1.txt')
test.must_not_exist('relocate/src/deps_all_done_first_123.txt')



arguments = []
if test.format == 'make':
  arguments = ['-j']
test.build('actions.gyp', 'action_with_dependencies_123', chdir='relocate/src',
           arguments=arguments)
test.must_exist('relocate/src/deps_all_done_first_123.txt')




clean_dep_files()
test.build('actions.gyp', 'action_with_dependencies_321', chdir='relocate/src',
           arguments=arguments)
test.must_exist('relocate/src/deps_all_done_first_321.txt')
test.must_not_exist('relocate/src/deps_all_done_first_123.txt')


test.pass_test()
