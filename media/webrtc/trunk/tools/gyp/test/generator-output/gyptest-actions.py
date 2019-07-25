





"""
Verifies --generator-output= behavior when using actions.
"""

import TestGyp


test = TestGyp.TestGyp(formats=['!ninja'])



test.writable(test.workpath('actions'), False)
test.run_gyp('actions.gyp',
             '--generator-output=' + test.workpath('gypfiles'),
             chdir='actions')

test.writable(test.workpath('actions'), True)

test.relocate('actions', 'relocate/actions')
test.relocate('gypfiles', 'relocate/gypfiles')

test.writable(test.workpath('relocate/actions'), False)






test.writable(test.workpath('relocate/actions/build'), True)
test.writable(test.workpath('relocate/actions/subdir1/build'), True)
test.writable(test.workpath('relocate/actions/subdir1/actions-out'), True)
test.writable(test.workpath('relocate/actions/subdir2/build'), True)
test.writable(test.workpath('relocate/actions/subdir2/actions-out'), True)

test.build('actions.gyp', test.ALL, chdir='relocate/gypfiles')

expect = """\
Hello from program.c
Hello from make-prog1.py
Hello from make-prog2.py
"""

if test.format == 'xcode':
  chdir = 'relocate/actions/subdir1'
else:
  chdir = 'relocate/gypfiles'
test.run_built_executable('program', chdir=chdir, stdout=expect)

test.must_match('relocate/actions/subdir2/actions-out/file.out',
                "Hello from make-file.py\n")

test.pass_test()
