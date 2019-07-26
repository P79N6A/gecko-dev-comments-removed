





"""
Runs small tests.
"""

import imp
import os
import sys
import unittest

import TestGyp


test = TestGyp.TestGyp()



sys.path.append(os.path.join(test._cwd, 'pylib'))


files_to_test = [
    'pylib/gyp/MSVSSettings_test.py',
    'pylib/gyp/easy_xml_test.py',
    'pylib/gyp/generator/msvs_test.py',
    'pylib/gyp/generator/ninja_test.py',
    'pylib/gyp/common_test.py',
]


suites = []
for filename in files_to_test:
  
  name = os.path.splitext(os.path.split(filename)[1])[0]
  
  full_filename = os.path.join(test._cwd, filename)
  
  module = imp.load_source(name, full_filename)
  
  suites.append(unittest.defaultTestLoader.loadTestsFromModule(module))

all_tests = unittest.TestSuite(suites)


result = unittest.TextTestRunner(verbosity=2).run(all_tests)
if result.failures or result.errors:
  test.fail_test()

test.pass_test()
