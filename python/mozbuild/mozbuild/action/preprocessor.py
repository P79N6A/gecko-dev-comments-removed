



from __future__ import absolute_import

import sys

from mozbuild.preprocessor import Preprocessor


def main(args):
  pp = Preprocessor()
  pp.handleCommandLine(args, True)


if __name__ == "__main__":
  main(sys.argv[1:])
