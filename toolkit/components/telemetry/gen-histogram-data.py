






from __future__ import print_function

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
            e = explodeToCharArray(string)
            if e:
                f.write("  /* %5d */ %s, '\\0',\n"
                        % (offset, explodeToCharArray(string)))
            else:
                f.write("  /* %5d */ '\\0',\n" % offset)
        f.write("  /* %5d */ %s, '\\0' };\n\n"
                % (entries[-1][1], explodeToCharArray(entries[-1][0])))

def print_array_entry(output, histogram, name_index, exp_index):
    cpp_guard = histogram.cpp_guard()
    if cpp_guard:
        print("#if defined(%s)" % cpp_guard, file=output)
    print("  { %s, %s, %s, %s, %d, %d, %s, %s, %s }," \
        % (histogram.low(), histogram.high(),
           histogram.n_buckets(), histogram.nsITelemetry_kind(),
           name_index, exp_index, histogram.dataset(),
           "true" if histogram.extended_statistics_ok() else "false",
           "true" if histogram.keyed() else "false"), file=output)
    if cpp_guard:
        print("#endif", file=output)

def write_histogram_table(output, histograms):
    table = StringTable()

    print("const TelemetryHistogram gHistograms[] = {", file=output)
    for histogram in histograms:
        name_index = table.stringIndex(histogram.name())
        exp_index = table.stringIndex(histogram.expiration())
        print_array_entry(output, histogram, name_index, exp_index)
    print("};", file=output)

    strtab_name = "gHistogramStringTable"
    table.writeDefinition(output, strtab_name)
    static_assert(output, "sizeof(%s) <= UINT32_MAX" % strtab_name,
                  "index overflow")






def static_assert(output, expression, message):
    print("static_assert(%s, \"%s\");" % (expression, message), file=output)

def static_asserts_for_boolean(output, histogram):
    pass

def static_asserts_for_flag(output, histogram):
    pass

def static_asserts_for_count(output, histogram):
    pass

def static_asserts_for_enumerated(output, histogram):
    n_values = histogram.high()
    static_assert(output, "%s > 2" % n_values,
                  "Not enough values for %s" % histogram.name())

def shared_static_asserts(output, histogram):
    name = histogram.name()
    low = histogram.low()
    high = histogram.high()
    n_buckets = histogram.n_buckets()
    static_assert(output, "%s < %s" % (low, high), "low >= high for %s" % name)
    static_assert(output, "%s > 2" % n_buckets, "Not enough values for %s" % name)
    static_assert(output, "%s >= 1" % low, "Incorrect low value for %s" % name)
    static_assert(output, "%s > %s" % (high, n_buckets),
                  "high must be > number of buckets for %s; you may want an enumerated histogram" % name)

def static_asserts_for_linear(output, histogram):
    shared_static_asserts(output, histogram)

def static_asserts_for_exponential(output, histogram):
    shared_static_asserts(output, histogram)

def write_histogram_static_asserts(output, histograms):
    print("""
// Perform the checks at the beginning of HistogramGet at
// compile time, so that incorrect histogram definitions
// give compile-time errors, not runtime errors.""", file=output)

    table = {
        'boolean' : static_asserts_for_boolean,
        'flag' : static_asserts_for_flag,
        'count': static_asserts_for_count,
        'enumerated' : static_asserts_for_enumerated,
        'linear' : static_asserts_for_linear,
        'exponential' : static_asserts_for_exponential,
        }

    for histogram in histograms:
        histogram_tools.table_dispatch(histogram.kind(), table,
                                       lambda f: f(output, histogram))

def write_debug_histogram_ranges(output, histograms):
    ranges_lengths = []

    
    
    print("#ifdef DEBUG", file=output)
    print("const int gBucketLowerBounds[] = {", file=output)
    for histogram in histograms:
        ranges = []
        try:
            ranges = histogram.ranges()
        except histogram_tools.DefinitionException:
            pass
        ranges_lengths.append(len(ranges))
        
        
        
        
        
        
        
        if len(ranges) > 0:
            print(','.join(map(str, ranges)), ',', file=output)
        else:
            print('/* Skipping %s */' % histogram.name(), file=output)
    print("};", file=output)

    
    print("struct bounds { int offset; int length; };", file=output)
    print("const struct bounds gBucketLowerBoundIndex[] = {", file=output)
    offset = 0
    for (histogram, range_length) in itertools.izip(histograms, ranges_lengths):
        cpp_guard = histogram.cpp_guard()
        
        
        if cpp_guard:
            print("#if defined(%s)" % cpp_guard, file=output)
        print("{ %d, %d }," % (offset, range_length), file=output)
        if cpp_guard:
            print("#endif", file=output)
        offset += range_length
    print("};", file=output)
    print("#endif", file=output)

def main(output, *filenames):
    histograms = list(histogram_tools.from_files(filenames))

    print(banner, file=output)
    write_histogram_table(output, histograms)
    write_histogram_static_asserts(output, histograms)
    write_debug_histogram_ranges(output, histograms)

if __name__ == '__main__':
    main(sys.stdout, *sys.argv[1:])
