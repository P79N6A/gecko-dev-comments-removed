





"""
Verifies that missing 'sources' files are treated as fatal errors when the
the generator flag 'msvs_error_on_missing_sources' is set.
"""

import TestGyp
import os

test = TestGyp.TestGyp(formats=['msvs'], workdir='workarea_all')


test.run_gyp('hello_missing.gyp')


try:
  os.environ['GYP_GENERATOR_FLAGS'] = 'msvs_error_on_missing_sources=0'
  test.run_gyp('hello_missing.gyp')
finally:
  del os.environ['GYP_GENERATOR_FLAGS']


try:
  os.environ['GYP_GENERATOR_FLAGS'] = 'msvs_error_on_missing_sources=1'
  
  
  
  
  
  test.run_gyp('hello_missing.gyp', status=1, stderr=None)
finally:
  del os.environ['GYP_GENERATOR_FLAGS']
test.must_contain_any_line(test.stderr(),
                           ["Missing input files:"])

test.pass_test()