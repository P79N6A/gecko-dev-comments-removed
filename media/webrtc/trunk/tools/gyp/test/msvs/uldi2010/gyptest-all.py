





"""
Verifies that uldi can be disabled on a per-project-reference basis in vs2010.
"""

import TestGyp

test = TestGyp.TestGyp(formats=['msvs'], workdir='workarea_all')

test.run_gyp('hello.gyp')

if test.uses_msbuild:
  test.must_contain('hello.vcxproj', '<UseLibraryDependencyInputs>false')

test.pass_test()
