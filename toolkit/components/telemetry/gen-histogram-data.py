






from __future__ import with_statement

import sys
import histogram_tools
import itertools

banner = """/* This file is auto-generated, see gen-histogram-data.py.  */
"""



class StringTable:
    def __init__(self):
        self.current_index = 0;
        self.table = {}

    def c_strlen(self, string):
        return len(string) + 1

    def stringIndex(self, string):
        if string in self.table:
            return self.table[string]
        else:
            result = self.current_index
            self.table[string] = result
            self.current_index += self.c_strlen(string)
            return result

    def writeDefinition(self, f, name):
        entries = self.table.items()
        entries.sort(key=lambda x:x[1])
        
        
        def explodeToCharArray(string):
            def toCChar(s):
                if s == "'":
                    return "'\\''"
                else:
                    return "'%s'" % s
            return ", ".join(map(toCChar, string))
        f.write("const char %s[] = {\n" % name)
        for (string, offset) in entries[:-1]:
            f.write("  /* %5d */ %s, '\\0',\n"
                    % (offset, explodeToCharArray(string)))
        f.write("  /* %5d */ %s, '\\0' };\n\n"
                % (entries[-1][1], explodeToCharArray(entries[-1][0])))

def print_array_entry(histogram, name_index, desc_index):
    cpp_guard = histogram.cpp_guard()
    if cpp_guard:
        print "#if defined(%s)" % cpp_guard
    print "  { %s, %s, %s, %s, %d, %d, %s }," \
        % (histogram.low(), histogram.high(),
           histogram.n_buckets(), histogram.nsITelemetry_kind(),
           name_index, desc_index,
           "true" if histogram.extended_statistics_ok() else "false")
    if cpp_guard:
        print "#endif"

def write_histogram_table(histograms):
    table = StringTable()

    print "const TelemetryHistogram gHistograms[] = {"
    for histogram in histograms:
        name_index = table.stringIndex(histogram.name())
        desc_index = table.stringIndex(histogram.description())
        print_array_entry(histogram, name_index, desc_index)
    print "};"

    strtab_name = "gHistogramStringTable"
    table.writeDefinition(sys.stdout, strtab_name)
    static_assert("sizeof(%s) <= UINT16_MAX" % strtab_name,
                  "index overflow")






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
