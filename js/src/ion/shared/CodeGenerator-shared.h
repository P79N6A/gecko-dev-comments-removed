








































#ifndef jsion_codegen_shared_h__
#define jsion_codegen_shared_h__

#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "ion/LIR.h"
#include "ion/IonCaches.h"
#include "ion/IonMacroAssembler.h"
#include "ion/IonFrames.h"
#include "ion/IonMacroAssembler.h"
#include "ion/Snapshots.h"
#include "ion/Safepoints.h"

namespace js {
namespace ion {

class OutOfLineCode;
class CodeGenerator;
class MacroAssembler;
class CodeGeneratorShared : public LInstructionVisitor
{
    js::Vector<OutOfLineCode *, 0, SystemAllocPolicy> outOfLineCode_;

  protected:
    MacroAssembler masm;
    MIRGenerator *gen;
    LIRGraph &graph;
    LBlock *current;
    SnapshotWriter snapshots_;
    IonCode *deoptTable_;
#ifdef DEBUG
    uint32 pushedArgs_;
#endif
    uint32 lastOsiPointOffset_;
    SafepointWriter safepoints_;
    Label invalidate_;
    CodeOffsetLabel invalidateEpilogueData_;

    js::Vector<SafepointIndex, 0, SystemAllocPolicy> safepointIndices_;
    js::Vector<OsiIndex, 0, SystemAllocPolicy> osiIndices_;

    
    js::Vector<SnapshotOffset, 0, SystemAllocPolicy> bailouts_;

    
    js::Vector<IonCache, 0, SystemAllocPolicy> cacheList_;

  protected:
    
    
    size_t osrEntryOffset_;

    inline void setOsrEntryOffset(size_t offset) {
        JS_ASSERT(osrEntryOffset_ == 0);
        osrEntryOffset_ = offset;
    }
    inline size_t getOsrEntryOffset() const {
        return osrEntryOffset_;
    }

    typedef js::Vector<SafepointIndex, 8, SystemAllocPolicy> SafepointIndices;

  protected:
    
    
    
    int32 frameDepth_;

    
    FrameSizeClass frameClass_;

    
    inline int32 ArgToStackOffset(int32 slot) const {
        return masm.framePushed() + sizeof(IonJSFrameLayout) + slot;
    }

    
    inline int32 CalleeStackOffset() const {
        return masm.framePushed() + IonJSFrameLayout::offsetOfCalleeToken();
    }

    inline int32 SlotToStackOffset(int32 slot) const {
        JS_ASSERT(slot > 0 && slot <= int32(graph.localSlotCount()));
        int32 offset = masm.framePushed() - (slot * STACK_SLOT_SIZE);
        JS_ASSERT(offset >= 0);
        return offset;
    }

    
    inline int32 StackOffsetOfPassedArg(int32 slot) const {
        
        JS_ASSERT(slot >= 0 && slot <= int32(graph.argumentSlotCount()));
        int32 offset = masm.framePushed() -
                       (graph.localSlotCount() * STACK_SLOT_SIZE) -
                       (slot * sizeof(Value));
        
        
        
        
        

        offset &= ~7;
        JS_ASSERT(offset >= 0);
        return offset;
    }

    inline int32 ToStackOffset(const LAllocation *a) const {
        if (a->isArgument())
            return ArgToStackOffset(a->toArgument()->index());
        return SlotToStackOffset(a->toStackSlot()->slot());
    }

    uint32 frameSize() const {
        return frameClass_ == FrameSizeClass::None() ? frameDepth_ : frameClass_.frameSize();
    }

  protected:

    size_t allocateCache(const IonCache &cache) {
        size_t index = cacheList_.length();
        cacheList_.append(cache);
        return index;
    }

  protected:
    
    
    bool encode(LSnapshot *snapshot);
    bool encodeSlots(LSnapshot *snapshot, MResumePoint *resumePoint, uint32 *startIndex);

    
    
    
    bool assignBailoutId(LSnapshot *snapshot);

    
    void encodeSafepoint(LSafepoint *safepoint);

    
    
    void encodeSafepoints();

    
    
    bool markSafepoint(LInstruction *ins);
    bool markSafepointAt(uint32 offset, LInstruction *ins);

    
    
    
    
    bool markOsiPoint(LOsiPoint *ins, uint32 *returnPointOffset);

    void emitPreBarrier(Register base, const LAllocation *index, MIRType type);

    inline bool isNextBlock(LBlock *block) {
        return (current->mir()->id() + 1 == block->mir()->id());
    }

  public:
    
    
    
    
    inline void saveLive(LInstruction *ins);
    inline void restoreLive(LInstruction *ins);

    template <typename T>
    void pushArg(const T &t) {
        masm.Push(t);
#ifdef DEBUG
        pushedArgs_++;
#endif
    }

    bool callVM(const VMFunction &f, LInstruction *ins);

  protected:
    bool addOutOfLineCode(OutOfLineCode *code);
    bool generateOutOfLineCode();

    void linkAbsoluteLabels() {
    }

  private:
    void generateInvalidateEpilogue();

  public:
    CodeGeneratorShared(MIRGenerator *gen, LIRGraph &graph);
};


struct HeapLabel
  : public TempObject,
    public Label
{
};


class OutOfLineCode : public TempObject
{
    Label entry_;
    Label rejoin_;
    uint32 framePushed_;

  public:
    OutOfLineCode()
      : framePushed_(0)
    { }

    virtual bool generate(CodeGeneratorShared *codegen) = 0;

    Label *entry() {
        return &entry_;
    }
    Label *rejoin() {
        return &rejoin_;
    }
    void setFramePushed(uint32 framePushed) {
        framePushed_ = framePushed;
    }
    uint32 framePushed() const {
        return framePushed_;
    }
};


template <typename T>
class OutOfLineCodeBase : public OutOfLineCode
{
  public:
    virtual bool generate(CodeGeneratorShared *codegen) {
        return accept(static_cast<T *>(codegen));
    }

  public:
    virtual bool accept(T *codegen) = 0;
};

} 
} 

#endif 

