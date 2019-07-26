





"""
Make sure libpath is extracted properly.
"""

import TestGyp

import sys

if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  CHDIR = 'linker-flags'

  
  test.run_gyp('subdir/library.gyp', chdir=CHDIR)
  test.build('subdir/library.gyp', test.ALL, chdir=CHDIR)

  
  
  test.run_gyp('library-directories.gyp', chdir=CHDIR)

  
  test.build('library-directories.gyp', 'test_libdirs_none', chdir=CHDIR,
      status=1)

  
  test.build('library-directories.gyp', 'test_libdirs_with', chdir=CHDIR)

  test.pass_test()
