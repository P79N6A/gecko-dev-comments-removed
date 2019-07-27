







import sys
import histogram_tools

banner = """/* This file is auto-generated, see gen-histogram-enum.py.  */
"""

def main(argv):
    filename = argv[0]

    print banner
    print "enum ID : uint32_t {"
    for histogram in histogram_tools.from_file(filename):
        cpp_guard = histogram.cpp_guard()
        if cpp_guard:
            print "#if defined(%s)" % cpp_guard
        print "  %s," % histogram.name()
        if cpp_guard:
            print "#endif"
    print "  HistogramCount"
    print "};"

main(sys.argv[1:])
