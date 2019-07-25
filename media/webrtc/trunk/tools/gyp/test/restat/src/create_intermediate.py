





import os
import sys

"""
Create argv[1] iff it doesn't already exist.
"""

outfile = sys.argv[1]
if os.path.exists(outfile):
  sys.exit()
open(outfile, "wb").close()
