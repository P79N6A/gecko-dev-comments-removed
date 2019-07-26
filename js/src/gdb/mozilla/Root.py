


import mozilla.prettyprinters
from mozilla.prettyprinters import pretty_printer, template_pretty_printer


mozilla.prettyprinters.clear_module_printers(__name__)




class Common(object):
    
    member = 'ptr'

    
    
    handle = False

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    def __init__(self, value, cache, content_printer=None):
        self.value = value
        self.cache = cache
        self.content_printer = content_printer
    def to_string(self):
        ptr = self.value[self.member]
        if self.handle:
            ptr = ptr.dereference()
        if self.content_printer:
            return self.content_printer(ptr, self.cache).to_string()
        else:
            
            
            
            
            return str(ptr)

@template_pretty_printer("js::Rooted")
class Rooted(Common):
    pass

@template_pretty_printer("JS::Handle")
class Handle(Common):
    handle = True

@template_pretty_printer("JS::MutableHandle")
class MutableHandle(Common):
    handle = True

@template_pretty_printer("js::EncapsulatedPtr")
class EncapsulatedPtr(Common):
    member = 'value'

@pretty_printer("js::EncapsulatedValue")
class EncapsulatedValue(Common):
    member = 'value'


def deref(root):
    tag = root.type.strip_typedefs().tag
    if not tag:
        raise TypeError, "Can't dereference type with no structure tag: %s" % (root.type,)
    elif tag.startswith('js::HeapPtr<'):
        return root['value']
    elif tag.startswith('js::Rooted<'):
        return root['ptr']
    elif tag.startswith('js::Handle<'):
        return root['ptr']
    else:
        raise NotImplementedError
