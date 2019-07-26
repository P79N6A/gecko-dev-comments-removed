




"""Copy a file.

This module works much like the cp posix command - it takes 2 arguments:
(src, dst) and copies the file with path |src| to |dst|.
"""

import shutil
import sys


def Main(src, dst):
  
  return shutil.copy(src, dst)


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1], sys.argv[2]))
