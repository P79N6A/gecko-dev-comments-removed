





#ifndef jit_shared_BaselineCompiler_shared_h
#define jit_shared_BaselineCompiler_shared_h

#include "jit/BaselineFrameInfo.h"
#include "jit/BaselineIC.h"
#include "jit/BytecodeAnalysis.h"
#include "jit/IonMacroAssembler.h"

namespace js {
namespace jit {

class BaselineCompilerShared
{
  protected:
    JSContext *cx;
    RootedScript script;
    jsbytecode *pc;
    MacroAssembler masm;
    bool ionCompileable_;
    bool ionOSRCompileable_;
    bool debugMode_;

    BytecodeAnalysis analysis_;
    FrameInfo frame;

    FallbackICStubSpace stubSpace_;
    js::Vector<ICEntry, 16, SystemAllocPolicy> icEntries_;

    
    struct PCMappingEntry
    {
        uint32_t pcOffset;
        uint32_t nativeOffset;
        PCMappingSlotInfo slotInfo;

        
        
        bool addIndexEntry;

        void fixupNativeOffset(MacroAssembler &masm) {
            CodeOffsetLabel offset(nativeOffset);
            offset.fixup(&masm);
            JS_ASSERT(offset.offset() <= UINT32_MAX);
            nativeOffset = (uint32_t) offset.offset();
        }
    };

    js::Vector<PCMappingEntry, 16, SystemAllocPolicy> pcMappingEntries_;

    
    
    
    
    struct ICLoadLabel {
        size_t icEntry;
        CodeOffsetLabel label;
    };
    js::Vector<ICLoadLabel, 16, SystemAllocPolicy> icLoadLabels_;

    uint32_t pushedBeforeCall_;
    mozilla::DebugOnly<bool> inCall_;

    CodeOffsetLabel spsPushToggleOffset_;

    BaselineCompilerShared(JSContext *cx, HandleScript script);

    ICEntry *allocateICEntry(ICStub *stub, bool isForOp) {
        if (!stub)
            return nullptr;

        
        if (!icEntries_.append(ICEntry((uint32_t) (pc - script->code), isForOp)))
            return nullptr;
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

    PCMappingSlotInfo getStackTopSlotInfo() {
        JS_ASSERT(frame.numUnsyncedSlots() <= 2);
        switch (frame.numUnsyncedSlots()) {
          case 0:
            return PCMappingSlotInfo::MakeSlotInfo();
          case 1:
            return PCMappingSlotInfo::MakeSlotInfo(PCMappingSlotInfo::ToSlotLocation(frame.peek(-1)));
          case 2:
          default:
            return PCMappingSlotInfo::MakeSlotInfo(PCMappingSlotInfo::ToSlotLocation(frame.peek(-1)),
                                                   PCMappingSlotInfo::ToSlotLocation(frame.peek(-2)));
        }
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

    bool callVM(const VMFunction &fun, bool preInitialize=false);

  public:
    BytecodeAnalysis &analysis() {
        return analysis_;
    }
};

} 
} 

#endif 
