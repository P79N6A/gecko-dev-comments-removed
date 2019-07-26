






#ifndef jsion_baselinecompiler_shared_h__
#define jsion_baselinecompiler_shared_h__

#include "jscntxt.h"
#include "ion/BaselineFrameInfo.h"
#include "ion/IonSpewer.h"
#include "ion/BaselineIC.h"
#include "ion/IonInstrumentation.h"
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
    bool debugMode_;

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
            return NULL;

        
        if (!icEntries_.append(ICEntry((uint32_t) (pc - script->code), isForOp)))
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

    bool addPCMappingEntry(uint32_t nativeOffset, PCMappingSlotInfo slotInfo, bool addIndexEntry) {

        
        size_t nentries = pcMappingEntries_.length();
        if (nentries > 0 && pcMappingEntries_[nentries - 1].pcOffset == unsigned(pc - script->code))
            return true;

        PCMappingEntry entry;
        entry.pcOffset = pc - script->code;
        entry.nativeOffset = nativeOffset;
        entry.slotInfo = slotInfo;
        entry.addIndexEntry = addIndexEntry;

        IonSpew(IonSpew_BaselineOp, "PCMapping (%s:%u): %u => %u (%u:%u:%u)!",
                        script->filename(), script->lineno,
                        entry.pcOffset, entry.nativeOffset,
                        (entry.slotInfo.toByte() & 0x3),
                        ((entry.slotInfo.toByte() >> 2) & 0x3),
                        ((entry.slotInfo.toByte() >> 4) & 0x3));

        return pcMappingEntries_.append(entry);
    }

    bool addPCMappingEntry(bool addIndexEntry) {
        return addPCMappingEntry(masm.currentOffset(), getStackTopSlotInfo(), addIndexEntry);
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
