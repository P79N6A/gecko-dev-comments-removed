






























"""fuse_gmock_files.py v0.1.0
Fuses Google Mock and Google Test source code into two .h files and a .cc file.

SYNOPSIS
       fuse_gmock_files.py [GMOCK_ROOT_DIR] OUTPUT_DIR

       Scans GMOCK_ROOT_DIR for Google Mock and Google Test source
       code, assuming Google Test is in the GMOCK_ROOT_DIR/gtest
       sub-directory, and generates three files:
       OUTPUT_DIR/gtest/gtest.h, OUTPUT_DIR/gmock/gmock.h, and
       OUTPUT_DIR/gmock-gtest-all.cc.  Then you can build your tests
       by adding OUTPUT_DIR to the include search path and linking
       with OUTPUT_DIR/gmock-gtest-all.cc.  These three files contain
       everything you need to use Google Mock.  Hence you can
       "install" Google Mock by copying them to wherever you want.

       GMOCK_ROOT_DIR can be omitted and defaults to the parent
       directory of the directory holding this script.

EXAMPLES
       ./fuse_gmock_files.py fused_gmock
       ./fuse_gmock_files.py path/to/unpacked/gmock fused_gmock

This tool is experimental.  In particular, it assumes that there is no
conditional inclusion of Google Mock or Google Test headers.  Please
report any problems to googlemock@googlegroups.com.  You can read
http://code.google.com/p/googlemock/wiki/CookBook for more
information.
"""

__author__ = 'wan@google.com (Zhanyong Wan)'

import os
import re
import sets
import sys



DEFAULT_GMOCK_ROOT_DIR = os.path.join(os.path.dirname(__file__), '..')


sys.path.append(os.path.join(DEFAULT_GMOCK_ROOT_DIR, 'gtest/scripts'))
import fuse_gtest_files
gtest = fuse_gtest_files


INCLUDE_GMOCK_FILE_REGEX = re.compile(r'^\s*#\s*include\s*"(gmock/.+)"')


GMOCK_H_SEED = 'include/gmock/gmock.h'
GMOCK_ALL_CC_SEED = 'src/gmock-all.cc'


GTEST_H_OUTPUT = 'gtest/gtest.h'
GMOCK_H_OUTPUT = 'gmock/gmock.h'
GMOCK_GTEST_ALL_CC_OUTPUT = 'gmock-gtest-all.cc'


def GetGTestRootDir(gmock_root):
  """Returns the root directory of Google Test."""

  return os.path.join(gmock_root, 'gtest')


def ValidateGMockRootDir(gmock_root):
  """Makes sure gmock_root points to a valid gmock root directory.

  The function aborts the program on failure.
  """

  gtest.ValidateGTestRootDir(GetGTestRootDir(gmock_root))
  gtest.VerifyFileExists(gmock_root, GMOCK_H_SEED)
  gtest.VerifyFileExists(gmock_root, GMOCK_ALL_CC_SEED)


def ValidateOutputDir(output_dir):
  """Makes sure output_dir points to a valid output directory.

  The function aborts the program on failure.
  """

  gtest.VerifyOutputFile(output_dir, gtest.GTEST_H_OUTPUT)
  gtest.VerifyOutputFile(output_dir, GMOCK_H_OUTPUT)
  gtest.VerifyOutputFile(output_dir, GMOCK_GTEST_ALL_CC_OUTPUT)


def FuseGMockH(gmock_root, output_dir):
  """Scans folder gmock_root to generate gmock/gmock.h in output_dir."""

  output_file = file(os.path.join(output_dir, GMOCK_H_OUTPUT), 'w')
  processed_files = sets.Set()  

  def ProcessFile(gmock_header_path):
    """Processes the given gmock header file."""

    
    if gmock_header_path in processed_files:
      return

    processed_files.add(gmock_header_path)

    
    for line in file(os.path.join(gmock_root, gmock_header_path), 'r'):
      m = INCLUDE_GMOCK_FILE_REGEX.match(line)
      if m:
        
        ProcessFile('include/' + m.group(1))
      else:
        m = gtest.INCLUDE_GTEST_FILE_REGEX.match(line)
        if m:
          
          
          

          
          if not gtest.GTEST_H_SEED in processed_files:
            processed_files.add(gtest.GTEST_H_SEED)
            output_file.write('#include "%s"\n' % (gtest.GTEST_H_OUTPUT,))
        else:
          
          output_file.write(line)

  ProcessFile(GMOCK_H_SEED)
  output_file.close()


def FuseGMockAllCcToFile(gmock_root, output_file):
  """Scans folder gmock_root to fuse gmock-all.cc into output_file."""

  processed_files = sets.Set()

  def ProcessFile(gmock_source_file):
    """Processes the given gmock source file."""

    
    if gmock_source_file in processed_files:
      return

    processed_files.add(gmock_source_file)

    
    for line in file(os.path.join(gmock_root, gmock_source_file), 'r'):
      m = INCLUDE_GMOCK_FILE_REGEX.match(line)
      if m:
        
        
        

        
        if not GMOCK_H_SEED in processed_files:
          processed_files.add(GMOCK_H_SEED)
          output_file.write('#include "%s"\n' % (GMOCK_H_OUTPUT,))
      else:
        m = gtest.INCLUDE_GTEST_FILE_REGEX.match(line)
        if m:
          
          
          
          pass
        else:
          m = gtest.INCLUDE_SRC_FILE_REGEX.match(line)
          if m:
            
            ProcessFile(m.group(1))
          else:
            
            output_file.write(line)

  ProcessFile(GMOCK_ALL_CC_SEED)


def FuseGMockGTestAllCc(gmock_root, output_dir):
  """Scans folder gmock_root to generate gmock-gtest-all.cc in output_dir."""

  output_file = file(os.path.join(output_dir, GMOCK_GTEST_ALL_CC_OUTPUT), 'w')
  
  gtest.FuseGTestAllCcToFile(GetGTestRootDir(gmock_root), output_file)
  
  FuseGMockAllCcToFile(gmock_root, output_file)
  output_file.close()


def FuseGMock(gmock_root, output_dir):
  """Fuses gtest.h, gmock.h, and gmock-gtest-all.h."""

  ValidateGMockRootDir(gmock_root)
  ValidateOutputDir(output_dir)

  gtest.FuseGTestH(GetGTestRootDir(gmock_root), output_dir)
  FuseGMockH(gmock_root, output_dir)
  FuseGMockGTestAllCc(gmock_root, output_dir)


def main():
  argc = len(sys.argv)
  if argc == 2:
    
    FuseGMock(DEFAULT_GMOCK_ROOT_DIR, sys.argv[1])
  elif argc == 3:
    
    FuseGMock(sys.argv[1], sys.argv[2])
  else:
    print __doc__
    sys.exit(1)


if __name__ == '__main__':
  main()
