




"""Windows can't run .sh files, so this is a small python wrapper around
update.sh.
"""

import os
import subprocess
import sys


def main():
  if sys.platform in ['win32', 'cygwin']:
    return 0

  
  
  
  
  
  
  
  
  
  return subprocess.call(
      [os.path.join(os.path.dirname(__file__), 'update.sh')] +  sys.argv[1:],
      stderr=os.fdopen(os.dup(sys.stdin.fileno())))


if __name__ == '__main__':
  sys.exit(main())
