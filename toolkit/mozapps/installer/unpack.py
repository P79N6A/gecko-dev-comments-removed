



import sys
import os
from mozpack.packager.unpack import unpack
import buildconfig

def main():
    if len(sys.argv) != 2:
        print >>sys.stderr, "Usage: %s directory" % \
                            os.path.basename(sys.argv[0])
        sys.exit(1)

    buildconfig.substs['USE_ELF_HACK'] = False
    buildconfig.substs['PKG_SKIP_STRIP'] = True
    unpack(sys.argv[1])

if __name__ == "__main__":
    main()
