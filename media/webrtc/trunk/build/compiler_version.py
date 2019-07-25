




"""Compiler version checking tool for gcc

Print gcc version as XY if you are running gcc X.Y.*.
This is used to tweak build flags for gcc 4.4.
"""

import os
import re
import subprocess
import sys

def GetVersion(compiler):
  try:
    
    compiler = compiler + " -dumpversion"
    pipe = subprocess.Popen(compiler, stdout=subprocess.PIPE, shell=True)
    gcc_output = pipe.communicate()[0]
    result = re.match(r"(\d+)\.(\d+)", gcc_output)
    return result.group(1) + result.group(2)
  except Exception, e:
    print >> sys.stderr, "compiler_version.py failed to execute:", compiler
    print >> sys.stderr, e
    return ""

def main():
  
  
  cxx = os.getenv("CXX", None)
  if cxx:
    cxxversion = GetVersion(cxx)
    if cxxversion != "":
      print cxxversion
      return 0
  else:
    
    gccversion = GetVersion("g++")
    if gccversion != "":
      print gccversion
      return 0

  return 1

if __name__ == "__main__":
  sys.exit(main())
