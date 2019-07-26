









"""Get svn revision of working copy

This script tries to get the svn revision as much as it can. It supports
both git-svn and svn. It will fail if not in a git-svn or svn repository;
in this case the script will return "n/a".
This script is a simplified version of lastchange.py which is in Chromium's
src/build/util folder.
"""

import os
import subprocess
import sys

def popen_cmd_and_get_output(cmd, directory):
  """Return (status, output) of executing cmd in a shell."""
  try:
    proc = subprocess.Popen(cmd,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            cwd=directory,
                            shell=(sys.platform=='win32'))
  except OSError:
    
    return None
  if not proc:
    return None

  for line in proc.stdout:
    line = line.strip()
    if not line:
      continue
    words = line.split()
    for index, word in enumerate(words):
      if word == "Revision:":
        return words[index+1]
  
  return None

def main():
  directory = os.path.dirname(sys.argv[0]);
  version = popen_cmd_and_get_output(['git', 'svn', 'info'], directory)
  if version == None:
    version = popen_cmd_and_get_output(['svn', 'info'], directory)
    if version == None:
      print "n/a"
      return 0
  print version
  return 0

if __name__ == '__main__':
  sys.exit(main())
