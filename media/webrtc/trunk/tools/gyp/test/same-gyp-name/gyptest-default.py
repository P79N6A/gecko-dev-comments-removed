





"""
Build a .gyp that depends on 2 gyp files with the same name.
"""

import TestGyp



test = TestGyp.TestGyp(formats=['msvs', 'scons', 'make', 'ninja'])

test.run_gyp('all.gyp', chdir='src')

test.relocate('src', 'relocate/src')

test.build('all.gyp', chdir='relocate/src')

expect1 = """\
Hello from main1.cc
"""

expect2 = """\
Hello from main2.cc
"""

test.run_built_executable('program1', chdir='relocate/src', stdout=expect1)
test.run_built_executable('program2', chdir='relocate/src', stdout=expect2)

test.pass_test()
