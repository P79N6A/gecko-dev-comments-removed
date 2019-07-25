








































#ifndef jsion_codegen_shared_h__
#define jsion_codegen_shared_h__

#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "ion/LIR.h"
#include "ion/IonMacroAssembler.h"
#include "ion/IonFrames.h"
#include "ion/IonMacroAssembler.h"
#include "ion/Snapshots.h"

namespace js {
namespace ion {

class OutOfLineCode;
class CodeGenerator;

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
    uint32 pushedArgs_;

    
    js::Vector<SnapshotOffset, 0, SystemAllocPolicy> bailouts_;

    
    
    js::Vector<IonFrameInfo, 0, SystemAllocPolicy> frameInfoTable_;

    static inline int32 ToInt32(const LAllocation *a) {
        if (a->isConstantValue()) {
            return a->toConstant()->toInt32();
        }
        if (a->isConstantIndex()) {
            return a->toConstantIndex()->index();
        }
        JS_NOT_REACHED("this is not a constant!");
        return -1;
    }

  protected:
    
    
    size_t osrEntryOffset_;

    inline void setOsrEntryOffset(size_t offset) {
        JS_ASSERT(osrEntryOffset_ == 0);
        osrEntryOffset_ = offset;
    }
    inline size_t getOsrEntryOffset() const {
        return osrEntryOffset_;
    }

  protected:
    
    
    
    int32 frameDepth_;

    
    FrameSizeClass frameClass_;

    
    inline int32 ArgToStackOffset(int32 slot) const {
        JS_ASSERT(slot >= 0);
        return masm.framePushed() + sizeof(IonJSFrameLayout) + slot;
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
    
    
    bool encode(LSnapshot *snapshot);
    bool encodeSlots(LSnapshot *snapshot, MResumePoint *resumePoint, uint32 *startIndex);

    
    
    
    bool assignBailoutId(LSnapshot *snapshot);

    
    
    
    bool assignFrameInfo(LSnapshot *snapshot);

    
    
    bool createSafepoint(LInstruction *ins) {
        JS_ASSERT(ins->safepoint());
        return assignFrameInfo(ins->safepoint());
    }

    inline bool isNextBlock(LBlock *block) {
        return (current->mir()->id() + 1 == block->mir()->id());
    }

    template <typename T>
    void pushArg(const T &t) {
        masm.Push(t);
        pushedArgs_++;
    }

  protected:
    bool addOutOfLineCode(OutOfLineCode *code);
    bool generateOutOfLineCode();

    void linkAbsoluteLabels() {
    }

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

