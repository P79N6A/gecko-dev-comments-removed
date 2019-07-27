






from __future__ import absolute_import

from mozpack.files import FileFinder
from mozpack.copier import Jarrer
from mozpack.errors import errors

import argparse
import mozpack.path as mozpath
import sys

def main(args):
    parser = argparse.ArgumentParser()
    parser.add_argument("-C", metavar='DIR', default=".",
                        help="Change to given directory before considering "
                        "other paths")
    parser.add_argument("zip", help="Path to zip file to write")
    parser.add_argument("input", nargs="+",
                        help="Path to files to add to zip")
    args = parser.parse_args(args)

    jarrer = Jarrer(optimize=False)

    with errors.accumulate():
        finder = FileFinder(args.C)
        for path in args.input:
            for p, f in finder.find(path):
                jarrer.add(p, f)
        jarrer.copy(mozpath.join(args.C, args.zip))


if __name__ == '__main__':
    main(sys.argv[1:])
