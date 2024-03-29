

import gdb
import mozilla.prettyprinters as prettyprinters

prettyprinters.clear_module_printers(__name__)

from mozilla.prettyprinters import pretty_printer


class InterpreterTypeCache(object):
    def __init__(self):
        self.tValue = gdb.lookup_type('JS::Value')
        self.tJSOp = gdb.lookup_type('JSOp')
        self.tScriptFrameIterData = gdb.lookup_type('js::ScriptFrameIter::Data')
        self.tInterpreterFrame = gdb.lookup_type('js::InterpreterFrame')
        self.tBaselineFrame = gdb.lookup_type('js::jit::BaselineFrame')
        self.tRematerializedFrame = gdb.lookup_type('js::jit::RematerializedFrame')

@pretty_printer('js::InterpreterRegs')
class InterpreterRegs(object):
    def __init__(self, value, cache):
        self.value = value
        self.cache = cache
        if not cache.mod_Interpreter:
            cache.mod_Interpreter = InterpreterTypeCache()
        self.itc = cache.mod_Interpreter

    
    
    
    def to_string(self):
        fp_ = 'fp_ = {}'.format(self.value['fp_'])
        slots = (self.value['fp_'] + 1).cast(self.itc.tValue.pointer())
        sp = 'sp = fp_.slots() + {}'.format(self.value['sp'] - slots)
        pc = self.value['pc']
        try:
            opcode = pc.dereference().cast(self.itc.tJSOp)
        except:
            opcode = 'bad pc'
        pc = 'pc = {} ({})'.format(pc.cast(self.cache.void_ptr_t), opcode)
        return '{{ {}, {}, {} }}'.format(fp_, sp, pc)

@pretty_printer('js::AbstractFramePtr')
class AbstractFramePtr(object):
    Tag_ScriptFrameIterData = 0x0
    Tag_InterpreterFrame = 0x1
    Tag_BaselineFrame = 0x2
    Tag_RematerializedFrame = 0x3
    TagMask = 0x3

    def __init__(self, value, cache):
        self.value = value
        self.cache = cache
        if not cache.mod_Interpreter:
            cache.mod_Interpreter = InterpreterTypeCache()
        self.itc = cache.mod_Interpreter

    def to_string(self):
        ptr = self.value['ptr_']
        tag = ptr & AbstractFramePtr.TagMask
        ptr = ptr & ~AbstractFramePtr.TagMask
        if tag == AbstractFramePtr.Tag_ScriptFrameIterData:
            label = 'js::ScriptFrameIter::Data'
            ptr = ptr.cast(self.itc.tScriptFrameIterData.pointer())
        if tag == AbstractFramePtr.Tag_InterpreterFrame:
            label = 'js::InterpreterFrame'
            ptr = ptr.cast(self.itc.tInterpreterFrame.pointer())
        if tag == AbstractFramePtr.Tag_BaselineFrame:
            label = 'js::jit::BaselineFrame'
            ptr = ptr.cast(self.itc.tBaselineFrame.pointer())
        if tag == AbstractFramePtr.Tag_RematerializedFrame:
            label = 'js::jit::RematerializedFrame'
            ptr = ptr.cast(self.itc.tRematerializedFrame.pointer())
        return 'AbstractFramePtr (({} *) {})'.format(label, ptr)

    
    
    def children(self):
        yield ('ptr_', self.value['ptr_'])
