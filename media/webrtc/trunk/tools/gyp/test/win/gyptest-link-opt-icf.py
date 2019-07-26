





"""
Make sure comdat folding optimization setting is extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'linker-flags'
  test.run_gyp('opt-icf.gyp', chdir=CHDIR)
  test.build('opt-icf.gyp', chdir=CHDIR)

  
  
  output = test.run_dumpbin(
      '/disasm', test.built_file_path('test_opticf_default.exe', chdir=CHDIR))
  if output.count('similar_function') != 6: 
    test.fail_test()

  
  output = test.run_dumpbin(
      '/disasm', test.built_file_path('test_opticf_no.exe', chdir=CHDIR))
  if output.count('similar_function') != 6: 
    test.fail_test()

  
  output = test.run_dumpbin(
      '/disasm', test.built_file_path('test_opticf_yes.exe', chdir=CHDIR))
  if output.count('similar_function') != 4: 
    test.fail_test()

  test.pass_test()
