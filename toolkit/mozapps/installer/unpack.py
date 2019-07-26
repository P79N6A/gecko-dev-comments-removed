



import sys
import os
from mozpack.packager.unpack import unpack

def main():
    if len(sys.argv) != 2:
        print >>sys.stderr, "Usage: %s directory" % \
                            os.path.basename(sys.argv[0])
        sys.exit(1)

    unpack(sys.argv[1])

if __name__ == "__main__":
    main()
