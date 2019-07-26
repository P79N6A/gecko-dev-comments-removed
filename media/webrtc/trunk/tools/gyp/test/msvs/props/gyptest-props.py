





"""
Verifies props files are added by using a
props file to set the name of the built executable.
"""

import TestGyp

test = TestGyp.TestGyp(workdir='workarea_all', formats=['msvs'])

test.run_gyp('hello.gyp')

test.build('hello.gyp')

test.built_file_must_exist('Greet.exe')

test.pass_test()
