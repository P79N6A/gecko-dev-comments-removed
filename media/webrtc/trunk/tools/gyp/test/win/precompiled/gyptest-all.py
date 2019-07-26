





"""
Verifies that precompiled headers can be specified.
"""

import TestGyp

import sys

if sys.platform == 'win32':
    test = TestGyp.TestGyp(formats=['msvs', 'ninja'], workdir='workarea_all')
    test.run_gyp('hello.gyp')
    test.build('hello.gyp', 'hello')
    test.run_built_executable('hello', stdout="Hello, world!\nHello, two!\n")
    test.up_to_date('hello.gyp', test.ALL)
    test.pass_test()
