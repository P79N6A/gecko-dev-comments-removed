





"""
Make sure buffer security check setting is extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'compiler-flags'
  test.run_gyp('buffer-security-check.gyp', chdir=CHDIR)
  test.build('buffer-security-check.gyp', chdir=CHDIR)

  def GetDisassemblyOfMain(exe):
    
    
    
    full_path = test.built_file_path(exe, chdir=CHDIR)
    output = test.run_dumpbin('/disasm', full_path)
    result = []
    in_main = False
    for line in output.splitlines():
      if line == '_main:':
        in_main = True
      elif in_main:
        
        if line.startswith('_'):
          break
        result.append(line)
    return '\n'.join(result)

  
  
  if 'security_cookie' not in GetDisassemblyOfMain('test_bsc_unset.exe'):
    test.fail_test()

  
  if 'security_cookie' not in GetDisassemblyOfMain('test_bsc_on.exe'):
    test.fail_test()

  
  if 'security_cookie' in GetDisassemblyOfMain('test_bsc_off.exe'):
    test.fail_test()

  test.pass_test()
