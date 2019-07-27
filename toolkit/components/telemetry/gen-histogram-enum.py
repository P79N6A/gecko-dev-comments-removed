













from __future__ import print_function

import histogram_tools
import itertools
import sys

banner = """/* This file is auto-generated, see gen-histogram-enum.py.  */
"""

def main(output, *filenames):
    print(banner, file=output)
    print("enum ID : uint32_t {", file=output)

    groups = itertools.groupby(histogram_tools.from_files(filenames),
                               lambda h: h.name().startswith("USE_COUNTER_"))
    seen_use_counters = False

    
    
    
    for (use_counter_group, histograms) in groups:
        if use_counter_group:
            seen_use_counters = True

        
        
        
        if use_counter_group:
            print("  HistogramFirstUseCounter,", file=output)
            print("  HistogramDUMMY1 = HistogramFirstUseCounter - 1,", file=output)

        for histogram in histograms:
            cpp_guard = histogram.cpp_guard()
            if cpp_guard:
                print("#if defined(%s)" % cpp_guard, file=output)
            print("  %s," % histogram.name(), file=output)
            if cpp_guard:
                print("#endif", file=output)

        if use_counter_group:
            print("  HistogramDUMMY2,", file=output)
            print("  HistogramLastUseCounter = HistogramDUMMY2 - 1,", file=output)

    print("  HistogramCount,", file=output)
    if seen_use_counters:
        print("  HistogramUseCounterCount = HistogramLastUseCounter - HistogramFirstUseCounter + 1", file=output)
    else:
        print("  HistogramUseCounterCount = 0", file=output)
    print("};", file=output)

if __name__ == '__main__':
    main(sys.stdout, *sys.argv[1:])
