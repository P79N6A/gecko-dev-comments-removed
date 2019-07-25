






from __future__ import with_statement

import sys
import histogram_tools
import itertools

banner = """/* This file is auto-generated, see gen-histogram-data.py.  */
"""



def print_array_entry(histogram):
    cpp_guard = histogram.cpp_guard()
    if cpp_guard:
        print "#if defined(%s)" % cpp_guard
    print "  { \"%s\", %s, %s, %s, %s, \"%s\" }," \
        % (histogram.name(), histogram.low(), histogram.high(),
           histogram.n_buckets(), histogram.nsITelemetry_kind(),
           histogram.description())
    if cpp_guard:
        print "#endif"

def write_histogram_table(histograms):
    print "const TelemetryHistogram gHistograms[] = {"
    for histogram in histograms:
        print_array_entry(histogram)
    print "};"






def static_assert(expression, message):
    print "MOZ_STATIC_ASSERT(%s, \"%s\");" % (expression, message)

def static_asserts_for_boolean(histogram):
    pass

def static_asserts_for_flag(histogram):
    pass

def static_asserts_for_enumerated(histogram):
    n_values = histogram.high()
    static_assert("%s > 2" % n_values,
                  "Not enough values for %s" % histogram.name())

def shared_static_asserts(histogram):
    name = histogram.name()
    low = histogram.low()
    high = histogram.high()
    n_buckets = histogram.n_buckets()
    static_assert("%s < %s" % (low, high), "low >= high for %s" % name)
    static_assert("%s > 2" % n_buckets, "Not enough values for %s" % name)
    static_assert("%s >= 1" % low, "Incorrect low value for %s" % name)
    static_assert("%s > %s" % (high, n_buckets),
                  "high must be > number of buckets for %s; you may want an enumerated histogram" % name)

def static_asserts_for_linear(histogram):
    shared_static_asserts(histogram)

def static_asserts_for_exponential(histogram):
    shared_static_asserts(histogram)

def write_histogram_static_asserts(histograms):
    print """
// Perform the checks at the beginning of HistogramGet at
// compile time, so that incorrect histogram definitions
// give compile-time errors, not runtime errors."""

    table = {
        'boolean' : static_asserts_for_boolean,
        'flag' : static_asserts_for_flag,
        'enumerated' : static_asserts_for_enumerated,
        'linear' : static_asserts_for_linear,
        'exponential' : static_asserts_for_exponential,
        }

    for histogram in histograms:
        histogram_tools.table_dispatch(histogram.kind(), table,
                                       lambda f: f(histogram))

def write_debug_histogram_ranges(histograms):
    ranges_lengths = []

    
    
    print "#ifdef DEBUG"
    print "const int gBucketLowerBounds[] = {"
    for histogram in histograms:
        ranges = []
        try:
            ranges = histogram.ranges()
        except histogram_tools.DefinitionException:
            pass
        ranges_lengths.append(len(ranges))
        
        
        
        
        
        
        
        if len(ranges) > 0:
            print ','.join(map(str, ranges)), ','
        else:
            print '/* Skipping %s */' % histogram.name()
    print "};"

    
    print "struct bounds { int offset; int length; };"
    print "const struct bounds gBucketLowerBoundIndex[] = {"
    offset = 0
    for (histogram, range_length) in itertools.izip(histograms, ranges_lengths):
        cpp_guard = histogram.cpp_guard()
        
        
        if cpp_guard:
            print "#if defined(%s)" % cpp_guard
        print "{ %d, %d }," % (offset, range_length)
        if cpp_guard:
            print "#endif"
        offset += range_length
    print "};"
    print "#endif"

def main(argv):
    filename = argv[0]

    histograms = list(histogram_tools.from_file(filename))

    print banner
    write_histogram_table(histograms)
    write_histogram_static_asserts(histograms)
    write_debug_histogram_ranges(histograms)

main(sys.argv[1:])
