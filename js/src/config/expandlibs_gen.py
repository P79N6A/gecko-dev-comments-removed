




































'''Given a list of object files and library names, prints a library
descriptor to standard output'''

import sys
import os
import expandlibs_config as conf
from expandlibs import LibDescriptor, isObject

def generate(args):
    desc = LibDescriptor()
    for arg in args:
        if isObject(arg):
            if os.path.exists(arg):
                desc['OBJS'].append(os.path.abspath(arg))
            else:
                raise Exception("File not found: %s" % arg)
        elif os.path.splitext(arg)[1] == conf.LIB_SUFFIX:
            if os.path.exists(arg) or os.path.exists(arg + conf.LIBS_DESC_SUFFIX):
                desc['LIBS'].append(os.path.abspath(arg))
            else:
                raise Exception("File not found: %s" % arg)
    return desc

if __name__ == '__main__':
    print generate(sys.argv[1:])
