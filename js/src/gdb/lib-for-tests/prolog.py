import gdb
import os
import re
import sys
import traceback


sys.path[0:0] = [testlibdir]



def run_fragment(fragment, function='breakpoint'):
    
    bp = gdb.Breakpoint(function);
    try:
        gdb.execute("run %s" % (fragment,))
        
        assert bp.hit_count == 1
    finally:
        bp.delete()
    gdb.execute('frame 1')


def assert_eq(actual, expected):
    if actual != expected:
        raise AssertionError, """Unexpected result:
expected: %r
actual:   %r""" % (expected, actual)



def assert_pretty(value, form):
    if isinstance(value, str):
        value = gdb.parse_and_eval(value)
    assert_eq(str(value), form)



def assert_subprinter_registered(printer, subprinter):
    
    

    names = { 'printer': re.escape(printer), 'subprinter': re.escape(subprinter) }
    pat = r'^( +)%(printer)s *\n(\1 +.*\n)*\1 +%(subprinter)s *\n' % names
    output = gdb.execute('info pretty-printer', to_string=True)
    if not re.search(pat, output, re.MULTILINE):
        raise AssertionError, ("assert_subprinter_registered failed to find pretty-printer:\n"
                               "  %s:%s\n"
                               "'info pretty-printer' says:\n"
                               "%s" % (printer, subprinter, output))


gdb.execute('set python print-stack full')


gdb.execute('set confirm off', False)


gdb.execute('set print static-members off')
gdb.execute('set print address off')
gdb.execute('set print pretty off')
gdb.execute('set width 0')

try:
    
    
    execfile(testscript)
except AssertionError as err:
    sys.stderr.write('\nAssertion traceback:\n')
    (t, v, tb) = sys.exc_info()
    traceback.print_tb(tb)
    sys.stderr.write('\nTest assertion failed:\n')
    sys.stderr.write(str(err))
    sys.exit(1)

