



import os
import sys

if not os.path.exists(sys.argv[1]):
  raise
open(sys.argv[2], 'w').close()
