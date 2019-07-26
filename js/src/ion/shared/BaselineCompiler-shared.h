






#ifndef jsion_baselinecompiler_shared_h__
#define jsion_baselinecompiler_shared_h__

#include "jscntxt.h"
#include "ion/BaselineFrameInfo.h"
#include "ion/IonSpewer.h"
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
    bool debugMode_;

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

    uint8_t getStackTopSlotInfo() {
        JS_ASSERT(frame.numUnsyncedSlots() <= 2);
        switch (frame.numUnsyncedSlots()) {
          case 0:
            return PCMappingEntry::MakeSlotInfo();
          case 1:
            return PCMappingEntry::MakeSlotInfo(PCMappingEntry::ToSlotLocation(frame.peek(-1)));
          case 2:
          default:
            return PCMappingEntry::MakeSlotInfo(PCMappingEntry::ToSlotLocation(frame.peek(-1)),
                                                PCMappingEntry::ToSlotLocation(frame.peek(-2)));
        }
    }

    bool addPCMappingEntry(uint32_t nativeOffset, uint8_t slotInfo) {

        
        size_t nentries = pcMappingEntries_.length();
        if (nentries > 0 && pcMappingEntries_[nentries - 1].pcOffset == pc - script->code)
            return true;

        masm.flushBuffer();

        PCMappingEntry entry;
        entry.pcOffset = pc - script->code;
        entry.nativeOffset = nativeOffset;
        entry.slotInfo = slotInfo;

        IonSpew(IonSpew_BaselineScripts, "PCMapping (%s:%u): %u => %u (%u:%u:%u)!",
                        script->filename, script->lineno,
                        entry.pcOffset, entry.nativeOffset,
                        (entry.slotInfo & 0x3),
                        ((entry.slotInfo >> 2) & 0x3),
                        ((entry.slotInfo >> 4) & 0x3));

        return pcMappingEntries_.append(entry);
    }

    bool addPCMappingEntry() {
        return addPCMappingEntry(masm.currentOffset(), getStackTopSlotInfo());
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
