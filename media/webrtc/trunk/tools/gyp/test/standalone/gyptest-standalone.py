





"""
Verifies that a project hierarchy created with the --generator-output=
option can be built even when it's relocated to a different path.
"""

import TestGyp
import os

test = TestGyp.TestGyp()

test.run_gyp('standalone.gyp', '-Gstandalone')



for root, dirs, files in os.walk("."):
  for file in files:
    
    if os.path.splitext(__file__)[0] in file:
      continue
    file = os.path.join(root, file)
    contents = open(file).read()
    if 'standalone.gyp' in contents:
      print 'gyp file referenced in generated output: %s' % file
      test.fail_test()


test.pass_test()
