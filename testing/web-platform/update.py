





import sys
import os

here = os.path.dirname(__file__)
os.path.abspath(sys.path.insert(0, os.path.join(here, "harness")))

from wptrunner import update

if __name__ == "__main__":
    success = update.main()
    sys.exit(0 if success else 1)
