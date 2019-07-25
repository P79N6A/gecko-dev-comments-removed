





"""
Verifies simple build of a "Hello, world!" program with static libraries,
including verifying that libraries are rebuilt correctly when functions
move between libraries.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('library.gyp',
             '-Dlibrary=static_library',
             '-Dmoveable_function=lib1',
             chdir='src')

test.relocate('src', 'relocate/src')

test.build('library.gyp', test.ALL, chdir='relocate/src')

expect = """\
Hello from program.c
Hello from lib1.c
Hello from lib2.c
Hello from lib1_moveable.c
"""
test.run_built_executable('program', chdir='relocate/src', stdout=expect)


test.run_gyp('library.gyp',
             '-Dlibrary=static_library',
             '-Dmoveable_function=lib2',
             chdir='relocate/src')


test.sleep()
contents = test.read('relocate/src/program.c')
contents = contents.replace('Hello', 'Hello again')
test.write('relocate/src/program.c', contents)

test.build('library.gyp', test.ALL, chdir='relocate/src')

expect = """\
Hello again from program.c
Hello from lib1.c
Hello from lib2.c
Hello from lib2_moveable.c
"""
test.run_built_executable('program', chdir='relocate/src', stdout=expect)


test.run_gyp('library.gyp',
             '-Dlibrary=static_library',
             '-Dmoveable_function=lib1',
             chdir='relocate/src')


test.sleep()
contents = test.read('relocate/src/program.c')
contents = contents.replace('again', 'again again')
test.write('relocate/src/program.c', contents)




test.touch('relocate/src/lib2.c')

test.build('library.gyp', test.ALL, chdir='relocate/src')

expect = """\
Hello again again from program.c
Hello from lib1.c
Hello from lib2.c
Hello from lib1_moveable.c
"""
test.run_built_executable('program', chdir='relocate/src', stdout=expect)


test.pass_test()
