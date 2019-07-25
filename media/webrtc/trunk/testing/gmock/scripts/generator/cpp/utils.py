
















"""Generic utilities for C++ parsing."""

__author__ = 'nnorwitz@google.com (Neal Norwitz)'


import sys



DEBUG = True


def ReadFile(filename, print_error=True):
    """Returns the contents of a file."""
    try:
        fp = open(filename)
        try:
            return fp.read()
        finally:
            fp.close()
    except IOError:
        if print_error:
            print('Error reading %s: %s' % (filename, sys.exc_info()[1]))
        return None
