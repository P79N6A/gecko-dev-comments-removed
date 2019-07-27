



from __future__ import absolute_import

import sys

from mozwebidlcodegen import BuildSystemWebIDL


def main(argv):
    """Perform WebIDL code generation required by the build system."""
    manager = BuildSystemWebIDL.from_environment().manager
    manager.generate_build_files()


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
