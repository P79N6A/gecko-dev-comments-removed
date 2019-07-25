







from __future__ import with_statement

import sys
import histogram_tools

banner = """/* This file is auto-generated, see gen-histogram-enum.py.  */
"""

def main(argv):
    filename = argv[0]

    print banner
    print "enum ID {"
    for (name, definition) in histogram_tools.from_file(filename):
        cpp_guard = definition.get('cpp_guard')
        if cpp_guard:
            print "#if defined(%s)" % cpp_guard
        print "  %s," % (name,)
        if cpp_guard:
            print "#endif"
    print "  HistogramCount"
    print "};"

main(sys.argv[1:])
