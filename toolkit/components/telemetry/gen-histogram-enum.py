













import histogram_tools
import itertools
import sys

banner = """/* This file is auto-generated, see gen-histogram-enum.py.  */
"""

def main(argv):
    filenames = argv

    print banner
    print "enum ID : uint32_t {"

    groups = itertools.groupby(histogram_tools.from_files(filenames),
                               lambda h: h.name().startswith("USE_COUNTER_"))
    seen_use_counters = False

    
    
    
    for (use_counter_group, histograms) in groups:
        if use_counter_group:
            seen_use_counters = True

        
        
        
        if use_counter_group:
            print "  HistogramFirstUseCounter,"
            print "  HistogramDUMMY1 = HistogramFirstUseCounter - 1,"

        for histogram in histograms:
            cpp_guard = histogram.cpp_guard()
            if cpp_guard:
                print "#if defined(%s)" % cpp_guard
            print "  %s," % histogram.name()
            if cpp_guard:
                print "#endif"

        if use_counter_group:
            print "  HistogramDUMMY2,"
            print "  HistogramLastUseCounter = HistogramDUMMY2 - 1,"

    print "  HistogramCount,"
    if seen_use_counters:
        print "  HistogramUseCounterCount = HistogramLastUseCounter - HistogramFirstUseCounter + 1"
    else:
        print "  HistogramUseCounterCount = 0"
    print "};"

main(sys.argv[1:])
