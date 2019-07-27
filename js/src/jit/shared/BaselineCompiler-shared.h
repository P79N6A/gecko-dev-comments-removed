





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
    JSScript *script;
    jsbytecode *pc;
    MacroAssembler masm;
    bool ionCompileable_;
    bool ionOSRCompileable_;
    bool compileDebugInstrumentation_;

    TempAllocator &alloc_;
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
            MOZ_ASSERT(offset.offset() <= UINT32_MAX);
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
    CodeOffsetLabel traceLoggerEnterToggleOffset_;
    CodeOffsetLabel traceLoggerExitToggleOffset_;
    CodeOffsetLabel traceLoggerScriptTextIdOffset_;

    BaselineCompilerShared(JSContext *cx, TempAllocator &alloc, JSScript *script);

    ICEntry *allocateICEntry(ICStub *stub, ICEntry::Kind kind) {
        if (!stub)
            return nullptr;

        
        if (!icEntries_.append(ICEntry(script->pcToOffset(pc), kind)))
            return nullptr;
        ICEntry &vecEntry = icEntries_.back();

        
        vecEntry.setFirstStub(stub);

        
        return &vecEntry;
    }

    bool addICLoadLabel(CodeOffsetLabel label) {
        MOZ_ASSERT(!icEntries_.empty());
        ICLoadLabel loadLabel;
        loadLabel.label = label;
        loadLabel.icEntry = icEntries_.length() - 1;
        return icLoadLabels_.append(loadLabel);
    }

    JSFunction *function() const {
        
        
        return script->functionNonDelazifying();
    }

    PCMappingSlotInfo getStackTopSlotInfo() {
        MOZ_ASSERT(frame.numUnsyncedSlots() <= 2);
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

    enum CallVMPhase {
        POST_INITIALIZE,
        PRE_INITIALIZE,
        CHECK_OVER_RECURSED
    };
    bool callVM(const VMFunction &fun, CallVMPhase phase=POST_INITIALIZE);

  public:
    BytecodeAnalysis &analysis() {
        return analysis_;
    }

    void setCompileDebugInstrumentation() {
        compileDebugInstrumentation_ = true;
    }
};

} 
} 

#endif 
