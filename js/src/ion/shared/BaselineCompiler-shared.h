






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

    js::Vector<PCMappingEntry, 16, SystemAllocPolicy> pcMappingEntries_;

    
    
    
    
    struct ICLoadLabel {
        size_t icEntry;
        CodeOffsetLabel label;
    };
    js::Vector<ICLoadLabel, 16, SystemAllocPolicy> icLoadLabels_;

    uint32_t pushedBeforeCall_;
    mozilla::DebugOnly<bool> inCall_;

    BaselineCompilerShared(JSContext *cx, HandleScript script);

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

    bool addICLoadLabel(CodeOffsetLabel label) {
        JS_ASSERT(!icEntries_.empty());
        ICLoadLabel loadLabel;
        loadLabel.label = label;
        loadLabel.icEntry = icEntries_.length() - 1;
        return icLoadLabels_.append(loadLabel);
    }

    JSFunction *function() const {
        return script->function();
    }

    bool addPCMappingEntry() {
        frame.assertSyncedStack();

        masm.flushBuffer();

        PCMappingEntry entry;
        entry.pcOffset = pc - script->code;
        entry.nativeOffset = masm.currentOffset();

        return pcMappingEntries_.append(entry);
    }

    template <typename T>
    void pushArg(const T& t) {
        masm.Push(t);
    }
    void prepareVMCall() {
        pushedBeforeCall_ = masm.framePushed();
        inCall_ = true;

        
        frame.syncStack(0);

        
        masm.Push(BaselineFrameReg);
    }

    bool callVM(const VMFunction &fun);
};

} 
} 

#endif 
