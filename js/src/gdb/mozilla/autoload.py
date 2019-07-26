

print "Loading JavaScript value pretty-printers; see js/src/gdb/README."
print "If they cause trouble, type: disable pretty-printer .* SpiderMonkey"

import gdb.printing
import mozilla.prettyprinters



import mozilla.jsid
import mozilla.JSObject
import mozilla.JSString
import mozilla.jsval
import mozilla.Root


try:
    import my_mozilla_printers
except ImportError:
    pass


def register(objfile):
    lookup = mozilla.prettyprinters.lookup_for_objfile(objfile)
    if lookup:
        gdb.printing.register_pretty_printer(objfile, lookup, replace=True)
