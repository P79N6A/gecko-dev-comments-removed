



import sys

import mozbuild.jar


def main(args):
    return mozbuild.jar.main(args)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
