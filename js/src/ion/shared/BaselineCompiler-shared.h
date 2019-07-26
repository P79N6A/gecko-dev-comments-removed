






#ifndef jsion_baselinecompiler_shared_h__
#define jsion_baselinecompiler_shared_h__

#include "jscntxt.h"
#include "ion/BaselineFrameInfo.h"
#include "ion/BaselineIC.h"
#include "ion/IonMacroAssembler.h"

namespace js {
namespace ion {

class BaselineCompilerShared
{
  protected:
    JSContext *cx;
    RootedScript script;
    jsbytecode *pc;
    MacroAssembler masm;
    bool ionCompileable_;

    FrameInfo frame;

    ICStubSpace stubSpace_;
    js::Vector<ICEntry, 16, SystemAllocPolicy> icEntries_;

    
    
    
    
    js::Vector<CodeOffsetLabel, 16, SystemAllocPolicy> icLoadLabels_;

    BaselineCompilerShared(JSContext *cx, JSScript *script);

    ICEntry *allocateICEntry(ICStub *stub) {
        if (!stub)
            return NULL;

        
        ICEntry entry((uint32_t) (pc - script->code));
        if (!icEntries_.append(entry))
            return NULL;
        ICEntry &vecEntry = icEntries_[icEntries_.length() - 1];

        
        vecEntry.setFirstStub(stub);

        
        return &vecEntry;
    }

    bool addICLoadLabel(CodeOffsetLabel offset) {
        return icLoadLabels_.append(offset);
    }

    JSFunction *function() const {
        return script->function();
    }
};

} 
} 

#endif 
