





import os
import sys

here = os.path.split(os.path.abspath(__file__))[0]
sys.path.insert(0, os.path.join(here, "harness"))

from wptrunner import wptrunner

if __name__ == "__main__":
    rv = wptrunner.main()
    sys.exit(rv)
