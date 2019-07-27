






from mozpack.files import FileFinder
from mozpack.copier import Jarrer
from mozpack.errors import errors

import argparse
import buildconfig
import mozpack.path as mozpath
import os
import sys
import tempfile

def main(args):
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-dir",
                        default=os.path.join(buildconfig.topobjdir,
                                             "dist", "bin"),
                        help="Store paths relative to this directory")
    parser.add_argument("zip", help="Path to zip file to write")
    parser.add_argument("input", nargs="+",
                        help="Path to files to add to zip")
    args = parser.parse_args(args)

    jarrer = Jarrer(optimize=False)

    with errors.accumulate():
        finder = FileFinder(args.base_dir)
        for i in args.input:
            path = mozpath.relpath(i, args.base_dir)
            for p, f in finder.find(path):
                jarrer.add(p, f)
        jarrer.copy(args.zip)


if __name__ == '__main__':
    main(sys.argv[1:])
