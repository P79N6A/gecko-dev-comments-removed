




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
    pipe = subprocess.Popen(compiler, shell=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    gcc_output, gcc_error = pipe.communicate()
    if pipe.returncode:
      raise subprocess.CalledProcessError(pipe.returncode, compiler)

    result = re.match(r"(\d+)\.(\d+)", gcc_output)
    return result.group(1) + result.group(2)
  except Exception, e:
    if gcc_error:
      sys.stderr.write(gcc_error)
    print >> sys.stderr, "compiler_version.py failed to execute:", compiler
    print >> sys.stderr, e
    return ""

def GetVersionFromEnvironment(compiler_env):
  """ Returns the version of compiler

  If the compiler was set by the given environment variable and exists,
  return its version, otherwise None is returned.
  """
  cxx = os.getenv(compiler_env, None)
  if cxx:
    cxx_version = GetVersion(cxx)
    if cxx_version != "":
      return cxx_version
  return None

def main():
  
  
  
  
  
  
  
  cxx_version = GetVersionFromEnvironment("CXX_target")
  if cxx_version:
    print cxx_version
    return 0

  cxx_version = GetVersionFromEnvironment("CXX")
  if cxx_version:
    print cxx_version
    return 0

  
  gccversion = GetVersion("g++")
  if gccversion != "":
    print gccversion
    return 0

  return 1

if __name__ == "__main__":
  sys.exit(main())
