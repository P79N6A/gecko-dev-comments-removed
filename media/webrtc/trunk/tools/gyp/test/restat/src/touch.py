





import os
import sys

"""Cross-platform touch."""

for fname in sys.argv[1:]:
  if os.path.exists(fname):
    os.utime(fname, None)
  else:
    open(fname, 'w').close()
