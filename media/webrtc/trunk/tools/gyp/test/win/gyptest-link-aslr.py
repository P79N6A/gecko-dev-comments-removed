





"""
Make sure aslr setting is extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'linker-flags'
  test.run_gyp('aslr.gyp', chdir=CHDIR)
  test.build('aslr.gyp', test.ALL, chdir=CHDIR)

  def HasDynamicBase(exe):
    full_path = test.built_file_path(exe, chdir=CHDIR)
    output = test.run_dumpbin('/headers', full_path)
    return '                   Dynamic base' in output

  
  if not HasDynamicBase('test_aslr_default.exe'):
    test.fail_test()
  if HasDynamicBase('test_aslr_no.exe'):
    test.fail_test()
  if not HasDynamicBase('test_aslr_yes.exe'):
    test.fail_test()

  test.pass_test()
