







































import os
import sys

import unittest

path = os.path.abspath(os.path.split(sys.argv[0])[0])
for fname in os.listdir(path):
    if os.path.splitext(fname)[1] != '.py':
        continue
    if fname == "regrtest.py":
        continue
    m = __import__(os.path.splitext(fname)[0])
    try:
        unittest.main(m)
    except SystemExit:
        pass
