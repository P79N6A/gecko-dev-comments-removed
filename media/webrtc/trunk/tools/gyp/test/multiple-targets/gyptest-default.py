





"""
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('multiple.gyp', chdir='src')

test.relocate('src', 'relocate/src')



test.build('multiple.gyp', chdir='relocate/src', stderr=None)

expect1 = """\
hello from prog1.c
hello from common.c
"""

expect2 = """\
hello from prog2.c
hello from common.c
"""

test.run_built_executable('prog1', stdout=expect1, chdir='relocate/src')
test.run_built_executable('prog2', stdout=expect2, chdir='relocate/src')

test.pass_test()
