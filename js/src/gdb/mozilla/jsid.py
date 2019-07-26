

import gdb
import mozilla.prettyprinters

from mozilla.prettyprinters import pretty_printer


mozilla.prettyprinters.clear_module_printers(__name__)

@pretty_printer('jsid')
class jsid(object):
    
    
    
    
    TYPE_STRING                 = 0x0
    TYPE_INT                    = 0x1
    TYPE_VOID                   = 0x2
    TYPE_OBJECT                 = 0x4
    TYPE_MASK                   = 0x7

    def __init__(self, value, cache):
        self.value = value
        self.cache = cache
        self.concrete_type = self.value.type.strip_typedefs()

    
    
    
    def as_bits(self):
        if self.concrete_type.code == gdb.TYPE_CODE_STRUCT:
            return self.value['asBits']
        elif self.concrete_type.code == gdb.TYPE_CODE_INT:
            return self.value
        else:
            raise RuntimeError, ("definition of SpiderMonkey 'jsid' type"
                                 "neither struct nor integral type")

    def to_string(self):
        bits = self.as_bits()
        tag = bits & jsid.TYPE_MASK
        if tag == jsid.TYPE_STRING:
            body = bits.cast(self.cache.JSString_ptr_t)
        elif tag & jsid.TYPE_INT:
            body = bits >> 1
        elif tag == jsid.TYPE_VOID:
            return "JSID_VOID"
        elif tag == jsid.TYPE_OBJECT:
            body = ((bits & ~jsid.TYPE_MASK)
                    .cast(self.cache.JSObject_ptr_t))
        else:
            body = "<unrecognized>"
        return '$jsid(%s)' % (body,)
