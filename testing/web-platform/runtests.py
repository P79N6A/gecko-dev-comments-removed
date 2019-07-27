





import sys

from wptrunner import wptrunner

if __name__ == "__main__":
    success = wptrunner.main()
    if not success:
        sys.exit(1)
