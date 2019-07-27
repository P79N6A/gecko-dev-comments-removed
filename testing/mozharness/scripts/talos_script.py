





"""talos

"""

import os
import sys


sys.path.insert(1, os.path.dirname(sys.path[0]))

from mozharness.mozilla.testing.talos import Talos

if __name__ == '__main__':
    talos = Talos()
    talos.run_and_exit()
