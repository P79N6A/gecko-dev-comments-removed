

import gdb
import mozilla.prettyprinters
from mozilla.prettyprinters import ptr_pretty_printer


mozilla.prettyprinters.clear_module_printers(__name__)


InSymbolRegistry = 0xfffffffe
UniqueSymbol = 0xffffffff

@ptr_pretty_printer("JS::Symbol")
class JSSymbolPtr(mozilla.prettyprinters.Pointer):
    def __init__(self, value, cache):
        super(JSSymbolPtr, self).__init__(value, cache)
        self.value = value

    def to_string(self):
        code = int(self.value['code_'])
        desc = str(self.value['description_'])
        if code == InSymbolRegistry:
            return "Symbol.for({})".format(desc)
        elif code == UniqueSymbol:
            return "Symbol({})".format(desc)
        else:
            
            
            assert desc[0] == '"'
            assert desc[-1] == '"'
            return desc[1:-1]

