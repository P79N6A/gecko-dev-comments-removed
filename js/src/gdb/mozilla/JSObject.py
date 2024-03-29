

import gdb
import mozilla.JSString
import mozilla.prettyprinters as prettyprinters
from mozilla.prettyprinters import ptr_pretty_printer, ref_pretty_printer
from mozilla.Root import deref

prettyprinters.clear_module_printers(__name__)

class JSObjectTypeCache(object):
    def __init__(self, value, cache):
        baseshape_flags = gdb.lookup_type('js::BaseShape::Flag')
        self.flag_DELEGATE = prettyprinters.enum_value(baseshape_flags, 'js::BaseShape::DELEGATE')
        self.func_ptr_type = gdb.lookup_type('JSFunction').pointer()
        self.class_NON_NATIVE = gdb.parse_and_eval('js::Class::NON_NATIVE')
        self.NativeObject_ptr_t = gdb.lookup_type('js::NativeObject').pointer()





@ptr_pretty_printer('JSObject')
class JSObjectPtrOrRef(prettyprinters.Pointer):
    def __init__(self, value, cache):
        super(JSObjectPtrOrRef, self).__init__(value, cache)
        if not cache.mod_JSObject:
            cache.mod_JSObject = JSObjectTypeCache(value, cache)
        self.otc = cache.mod_JSObject

    def summary(self):
        group = deref(self.value['group_'])
        classp = group['clasp_']
        class_name = classp['name'].string()
        non_native = classp['flags'] & self.otc.class_NON_NATIVE
        if non_native:
            return '[object {}]'.format(class_name)
        else:
            native = self.value.cast(self.otc.NativeObject_ptr_t)
            shape = deref(native['shape_'])
            baseshape = deref(shape['base_'])
            flags = baseshape['flags']
            is_delegate = bool(flags & self.otc.flag_DELEGATE)
            name = None
            if class_name == 'Function':
                function = self.value
                concrete_type = function.type.strip_typedefs()
                if concrete_type.code == gdb.TYPE_CODE_REF:
                    function = function.address
                function = function.cast(self.otc.func_ptr_type)
                atom = deref(function['atom_'])
                name = str(atom) if atom else '<unnamed>'
            return '[object {}{}]{}'.format(class_name,
                                            ' ' + name if name else '',
                                            ' delegate' if is_delegate else '')

@ref_pretty_printer('JSObject')
def JSObjectRef(value, cache): return JSObjectPtrOrRef(value, cache)
