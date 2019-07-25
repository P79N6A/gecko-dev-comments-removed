





"""
Verifies that a default gyp define can be overridden.
"""

import os
import TestGyp

test = TestGyp.TestGyp()


test.run_gyp('defines.gyp', '-D', 'OS=fakeos')
test.build('defines.gyp')
test.built_file_must_exist('fakeosprogram', type=test.EXECUTABLE)

os.remove(test.built_file_path('fakeosprogram', type=test.EXECUTABLE))


test.run_gyp('defines.gyp')
test.build('defines.gyp')
test.built_file_must_not_exist('fakeosprogram', type=test.EXECUTABLE)


os.environ['GYP_DEFINES'] = 'OS=fakeos'
test.run_gyp('defines.gyp')
test.build('defines.gyp')
test.built_file_must_exist('fakeosprogram', type=test.EXECUTABLE)

test.pass_test()
