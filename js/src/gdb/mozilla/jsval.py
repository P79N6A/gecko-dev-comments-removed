

import gdb
import gdb.types
import mozilla.prettyprinters
from mozilla.prettyprinters import pretty_printer, ptr_pretty_printer


mozilla.prettyprinters.clear_module_printers(__name__)

























































































class Box(object):
    def __init__(self, asBits, jtc):
        self.asBits = asBits
        self.jtc = jtc
        
        
        if self.asBits < 0:
            self.asBits = self.asBits + (1 << 64)

    
    def tag(self): raise NotImplementedError

    
    def as_uint32(self): raise NotImplementedError
    def as_double(self): raise NotImplementedError
    def as_address(self): raise NotImplementedError




class Punbox(Box):

    FULL_WIDTH     = 64
    TAG_SHIFT      = 47
    PAYLOAD_MASK   = (1 << TAG_SHIFT) - 1
    TAG_MASK       = (1 << (FULL_WIDTH - TAG_SHIFT)) - 1
    TAG_MAX_DOUBLE = 0x1fff0
    TAG_TYPE_MASK  = 0x0000f

    def tag(self):
        tag = self.asBits >> Punbox.TAG_SHIFT
        if tag <= Punbox.TAG_MAX_DOUBLE:
            return self.jtc.DOUBLE
        else:
            return tag & Punbox.TAG_TYPE_MASK

    def as_uint32(self): return int(self.asBits & ((1 << 32) - 1))
    def as_address(self): return gdb.Value(self.asBits & Punbox.PAYLOAD_MASK)

class Nunbox(Box):
    TAG_SHIFT      = 32
    TAG_CLEAR      = 0xffff0000
    PAYLOAD_MASK   = 0xffffffff
    TAG_TYPE_MASK  = 0x0000000f

    def tag(self):
        tag = self.asBits >> Nunbox.TAG_SHIFT
        if tag < Nunbox.TAG_CLEAR:
            return self.jtc.DOUBLE
        return tag & Nunbox.TAG_TYPE_MASK

    def as_uint32(self): return int(self.asBits & Nunbox.PAYLOAD_MASK)
    def as_address(self): return gdb.Value(self.asBits & Nunbox.PAYLOAD_MASK)


class jsvalTypeCache(object):
    def __init__(self, cache):
        
        d = gdb.types.make_enum_dict(gdb.lookup_type('JSValueType'))
        self.DOUBLE    = d['JSVAL_TYPE_DOUBLE']
        self.INT32     = d['JSVAL_TYPE_INT32']
        self.UNDEFINED = d['JSVAL_TYPE_UNDEFINED']
        self.BOOLEAN   = d['JSVAL_TYPE_BOOLEAN']
        self.MAGIC     = d['JSVAL_TYPE_MAGIC']
        self.STRING    = d['JSVAL_TYPE_STRING']
        self.NULL      = d['JSVAL_TYPE_NULL']
        self.OBJECT    = d['JSVAL_TYPE_OBJECT']

        
        
        d = gdb.types.make_enum_dict(gdb.lookup_type('JSWhyMagic'))
        self.magic_names = list(range(max(d.values()) + 1))
        for (k,v) in d.items(): self.magic_names[v] = k

        
        self.boxer = Punbox if cache.void_ptr_t.sizeof == 8 else Nunbox

@pretty_printer('jsval_layout')
class jsval_layout(object):
    def __init__(self, value, cache):
        
        self.cache = cache
        if not cache.mod_jsval:
            cache.mod_jsval = jsvalTypeCache(cache)
        self.jtc = cache.mod_jsval

        self.value = value
        self.box = self.jtc.boxer(value['asBits'], self.jtc)

    def to_string(self):
        tag = self.box.tag()
        if tag == self.jtc.INT32:
            value = self.box.as_uint32()
            signbit = 1 << 31
            value = (value ^ signbit) - signbit
        elif tag == self.jtc.UNDEFINED:
            return 'JSVAL_VOID'
        elif tag == self.jtc.BOOLEAN:
            return 'JSVAL_TRUE' if self.box.as_uint32() else 'JSVAL_FALSE'
        elif tag == self.jtc.MAGIC:
            value = self.box.as_uint32()
            if 0 <= value and value < len(self.jtc.magic_names):
                return '$jsmagic(%s)' % (self.jtc.magic_names[value],)
            else:
                return '$jsmagic(%d)' % (value,)
        elif tag == self.jtc.STRING:
            value = self.box.as_address().cast(self.cache.JSString_ptr_t)
        elif tag == self.jtc.NULL:
            return 'JSVAL_NULL'
        elif tag == self.jtc.OBJECT:
            value = self.box.as_address().cast(self.cache.JSObject_ptr_t)
        elif tag == self.jtc.DOUBLE:
            value = self.value['asDouble']
        else:
            return '$jsval(unrecognized!)'
        return '$jsval(%s)' % (value,)

@pretty_printer('JS::Value')
class JSValue(object):
    def __new__(cls, value, cache):
        return jsval_layout(value['data'], cache)
