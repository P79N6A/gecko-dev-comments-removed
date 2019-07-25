



"""Writes True if the argument is a directory."""

import os.path
import sys

def main():
  sys.stdout.write(str(os.path.isdir(sys.argv[1])))
  return 0

if __name__ == '__main__':
  sys.exit(main())
