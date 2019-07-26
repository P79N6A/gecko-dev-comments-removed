





"""
Make sure reference optimization setting is extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'linker-flags'
  test.run_gyp('opt-ref.gyp', chdir=CHDIR)
  test.build('opt-ref.gyp', chdir=CHDIR)

  
  output = test.run_dumpbin(
      '/disasm', test.built_file_path('test_optref_default.exe', chdir=CHDIR))
  if 'unused_function' not in output:
    test.fail_test()

  
  output = test.run_dumpbin(
      '/disasm', test.built_file_path('test_optref_no.exe', chdir=CHDIR))
  if 'unused_function' not in output:
    test.fail_test()

  
  output = test.run_dumpbin(
      '/disasm', test.built_file_path('test_optref_yes.exe', chdir=CHDIR))
  if 'unused_function' in output:
    test.fail_test()

  test.pass_test()
