

import gdb
import re












def check_for_reused_pretty_printer(fn):
    if hasattr(fn, 'enabled'):
        raise RuntimeError, ("pretty-printer function %r registered more than once" % fn)


printers_by_tag = {}



def pretty_printer(type_name):
    def add(fn):
        check_for_reused_pretty_printer(fn)
        add_to_subprinter_list(fn, type_name)
        printers_by_tag[type_name] = fn
        return fn
    return add



ptr_printers_by_tag = {}



def ptr_pretty_printer(type_name):
    def add(fn):
        check_for_reused_pretty_printer(fn)
        add_to_subprinter_list(fn, "ptr-to-" + type_name)
        ptr_printers_by_tag[type_name] = fn
        return fn
    return add



ref_printers_by_tag = {}



def ref_pretty_printer(type_name):
    def add(fn):
        check_for_reused_pretty_printer(fn)
        add_to_subprinter_list(fn, "ref-to-" + type_name)
        ref_printers_by_tag[type_name] = fn
        return fn
    return add



template_printers_by_tag = {}



def template_pretty_printer(template_name):
    def add(fn):
        check_for_reused_pretty_printer(fn)
        add_to_subprinter_list(fn, 'instantiations-of-' + template_name)
        template_printers_by_tag[template_name] = fn
        return fn
    return add





printers_by_regexp = []




def pretty_printer_for_regexp(pattern, name):
    compiled = re.compile(pattern)
    def add(fn):
        check_for_reused_pretty_printer(fn)
        add_to_subprinter_list(fn, name)
        printers_by_regexp.append((compiled, fn))
        return fn
    return add






def clear_module_printers(module_name):
    global printers_by_tag, ptr_printers_by_tag, ref_printers_by_tag
    global template_printers_by_tag, printers_by_regexp

    
    
    def clear_dictionary(d):
        
        
        
        to_delete = []
        for (k, v) in d.iteritems():
            if v.__module__ == module_name:
                to_delete.append(k)
                remove_from_subprinter_list(v)
        for k in to_delete:
            del d[k]

    clear_dictionary(printers_by_tag)
    clear_dictionary(ptr_printers_by_tag)
    clear_dictionary(ref_printers_by_tag)
    clear_dictionary(template_printers_by_tag)

    
    new_list = []
    for p in printers_by_regexp:
        if p.__module__ == module_name:
            remove_from_subprinter_list(p)
        else:
            new_list.append(p)
    printers_by_regexp = new_list




subprinters = []



def add_to_subprinter_list(subprinter, name):
    subprinter.name = name
    subprinter.enabled = True
    subprinters.append(subprinter)


def remove_from_subprinter_list(subprinter):
    subprinters.remove(subprinter)


class NotSpiderMonkeyObjfileError(TypeError):
    pass















class TypeCache(object):
    def __init__(self, objfile):
        self.objfile = objfile

        
        
        
        
        self.void_t = gdb.lookup_type('void')
        self.void_ptr_t = self.void_t.pointer()
        try:
            self.JSString_ptr_t = gdb.lookup_type('JSString').pointer()
            self.JSObject_ptr_t = gdb.lookup_type('JSObject').pointer()
        except gdb.error:
            raise NotSpiderMonkeyObjfileError

        self.mod_JSString = None
        self.mod_JSObject = None
        self.mod_jsval = None


















def implemented_types(t):

    
    def followers(t):
        if t.code == gdb.TYPE_CODE_TYPEDEF:
            yield t.target()
            for t2 in followers(t.target()): yield t2
        elif t.code == gdb.TYPE_CODE_STRUCT:
            base_classes = []
            for f in t.fields():
                if f.is_base_class:
                    yield f.type
                    base_classes.append(f.type)
            for b in base_classes:
                for t2 in followers(b): yield t2

    yield t
    for t2 in followers(t): yield t2

template_regexp = re.compile("([\w_:]+)<")




def lookup_for_objfile(objfile):
    
    try:
        cache = TypeCache(objfile)
    except NotSpiderMonkeyObjfileError:
        if gdb.parameter("verbose"):
            gdb.write("objfile '%s' has no SpiderMonkey code; not registering pretty-printers\n"
                      % (objfile.filename,))
        return None

    
    
    
    def lookup(value):
        
        def check_table(table, tag):
            if tag in table:
                f = table[tag]
                if f.enabled:
                    return f(value, cache)
            return None

        def check_table_by_type_name(table, t):
            if t.code == gdb.TYPE_CODE_TYPEDEF:
                return check_table(table, str(t))
            elif t.code == gdb.TYPE_CODE_STRUCT and t.tag:
                return check_table(table, t.tag)
            else:
                return None

        for t in implemented_types(value.type):
            if t.code == gdb.TYPE_CODE_PTR:
                for t2 in implemented_types(t.target()):
                    p = check_table_by_type_name(ptr_printers_by_tag, t2)
                    if p: return p
            elif t.code == gdb.TYPE_CODE_REF:
                for t2 in implemented_types(t.target()):
                    p = check_table_by_type_name(ref_printers_by_tag, t2)
                    if p: return p
            else:
                p = check_table_by_type_name(printers_by_tag, t)
                if p: return p
                if t.code == gdb.TYPE_CODE_STRUCT and t.tag:
                    m = template_regexp.match(t.tag)
                    if m:
                        p = check_table(template_printers_by_tag, m.group(1))
                        if p: return p

        
        
        
        s = str(value.type)
        for (r, f) in printers_by_regexp:
            if f.enabled:
                m = r.match(s)
                if m:
                    p = f(value, cache)
                    if p: return p

        
        return None

    
    
    lookup.name = "SpiderMonkey"
    lookup.enabled = True
    lookup.subprinters = subprinters

    return lookup




















class Pointer(object):
    def __new__(cls, value, cache):
        
        if value.type.strip_typedefs().code == gdb.TYPE_CODE_PTR and value == 0:
            return None
        return super(Pointer, cls).__new__(cls)

    def __init__(self, value, cache):
        self.value = value
        self.cache = cache

    def to_string(self):
        
        assert not hasattr(self, 'display_hint') or self.display_hint() != 'string'
        concrete_type = self.value.type.strip_typedefs()
        if concrete_type.code == gdb.TYPE_CODE_PTR:
            address = self.value.cast(self.cache.void_ptr_t)
        elif concrete_type.code == gdb.TYPE_CODE_REF:
            address = '@' + str(self.value.address.cast(self.cache.void_ptr_t))
        else:
            assert not "mozilla.prettyprinters.Pointer applied to bad value type"
        try:
            summary = self.summary()
        except gdb.MemoryError as r:
            summary = str(r)
        v = '(%s) %s %s' % (self.value.type, address, summary)
        return v

    def summary(self):
        raise NotImplementedError

field_enum_value = None







def enum_value(t, name):
    global field_enum_value
    f = t[name]
    
    if not field_enum_value:
        if hasattr(f, 'enumval'):
            field_enum_value = lambda f: f.enumval
        else:
            field_enum_value = lambda f: f.bitpos
    return field_enum_value(f)
