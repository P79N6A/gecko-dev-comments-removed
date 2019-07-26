




"""google_api's auto-internal gyp integration.

Takes one argument, a path.  Prints 1 if the path exists, 0 if not.
"""


import os
import sys


if __name__ == '__main__':
  if os.path.exists(sys.argv[1]):
    print 1
  else:
    print 0
