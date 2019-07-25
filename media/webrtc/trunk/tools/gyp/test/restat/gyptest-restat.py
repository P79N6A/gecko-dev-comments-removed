





"""
Verify that dependent rules are executed iff a dependency action modifies its
outputs.
"""

import TestGyp
import os

test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

test.run_gyp('restat.gyp', chdir='src')

chdir = 'relocate/src'
test.relocate('src', chdir)




test.build('restat.gyp', 'dependent', chdir=chdir)
test.built_file_must_exist('side_effect', chdir=chdir)
os.remove(test.built_file_path('side_effect', chdir=chdir))
test.build('restat.gyp', 'dependent', chdir=chdir)
test.built_file_must_not_exist('side_effect', chdir=chdir)

test.pass_test()
