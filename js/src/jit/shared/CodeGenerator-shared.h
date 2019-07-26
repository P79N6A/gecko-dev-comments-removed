





#ifndef jit_shared_CodeGenerator_shared_h
#define jit_shared_CodeGenerator_shared_h

#include "mozilla/Alignment.h"

#include "jit/IonFrames.h"
#include "jit/IonMacroAssembler.h"
#include "jit/LIR.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"
#include "jit/Safepoints.h"
#include "jit/SnapshotWriter.h"
#include "jit/VMFunctions.h"
#include "vm/ForkJoin.h"

namespace js {
namespace jit {

class OutOfLineCode;
class CodeGenerator;
class MacroAssembler;
class IonCache;
class OutOfLineAbortPar;
class OutOfLinePropagateAbortPar;

template <class ArgSeq, class StoreOutputTo>
class OutOfLineCallVM;

class OutOfLineTruncateSlow;

struct PatchableBackedgeInfo
{
    CodeOffsetJump backedge;
    Label *loopHeader;
    Label *interruptCheck;

    PatchableBackedgeInfo(CodeOffsetJump backedge, Label *loopHeader, Label *interruptCheck)
      : backedge(backedge), loopHeader(loopHeader), interruptCheck(interruptCheck)
    {}
};

class CodeGeneratorShared : public LInstructionVisitor
{
    js::Vector<OutOfLineCode *, 0, SystemAllocPolicy> outOfLineCode_;
    OutOfLineCode *oolIns;

    MacroAssembler &ensureMasm(MacroAssembler *masm);
    mozilla::Maybe<MacroAssembler> maybeMasm_;

  public:
    MacroAssembler &masm;

  protected:
    MIRGenerator *gen;
    LIRGraph &graph;
    LBlock *current;
    SnapshotWriter snapshots_;
    IonCode *deoptTable_;
#ifdef DEBUG
    uint32_t pushedArgs_;
#endif
    uint32_t lastOsiPointOffset_;
    SafepointWriter safepoints_;
    Label invalidate_;
    CodeOffsetLabel invalidateEpilogueData_;

    js::Vector<SafepointIndex, 0, SystemAllocPolicy> safepointIndices_;
    js::Vector<OsiIndex, 0, SystemAllocPolicy> osiIndices_;

    
    js::Vector<SnapshotOffset, 0, SystemAllocPolicy> bailouts_;

    
    js::Vector<uint8_t, 0, SystemAllocPolicy> runtimeData_;

    
    js::Vector<uint32_t, 0, SystemAllocPolicy> cacheList_;

    
    js::Vector<uint32_t, 0, SystemAllocPolicy> pushedArgumentSlots_;

    
    Vector<PatchableBackedgeInfo, 0, SystemAllocPolicy> patchableBackedges_;

    
    
    
    IonInstrumentation sps_;

  protected:
    
    
    size_t osrEntryOffset_;

    TempAllocator &alloc() const {
        return graph.mir().alloc();
    }

    inline void setOsrEntryOffset(size_t offset) {
        JS_ASSERT(osrEntryOffset_ == 0);
        osrEntryOffset_ = offset;
    }
    inline size_t getOsrEntryOffset() const {
        return osrEntryOffset_;
    }

    
    
    size_t skipArgCheckEntryOffset_;

    inline void setSkipArgCheckEntryOffset(size_t offset) {
        JS_ASSERT(skipArgCheckEntryOffset_ == 0);
        skipArgCheckEntryOffset_ = offset;
    }
    inline size_t getSkipArgCheckEntryOffset() const {
        return skipArgCheckEntryOffset_;
    }

    typedef js::Vector<SafepointIndex, 8, SystemAllocPolicy> SafepointIndices;

    bool markArgumentSlots(LSafepoint *safepoint);
    void dropArguments(unsigned argc);

  protected:
    
    
    
    int32_t frameDepth_;

    
    FrameSizeClass frameClass_;

    
    inline int32_t ArgToStackOffset(int32_t slot) const {
        return masm.framePushed() +
               (gen->compilingAsmJS() ? NativeFrameSize : sizeof(IonJSFrameLayout)) +
               slot;
    }

    
    inline int32_t CalleeStackOffset() const {
        return masm.framePushed() + IonJSFrameLayout::offsetOfCalleeToken();
    }

    inline int32_t SlotToStackOffset(int32_t slot) const {
        JS_ASSERT(slot > 0 && slot <= int32_t(graph.localSlotCount()));
        int32_t offset = masm.framePushed() - (slot * STACK_SLOT_SIZE);
        JS_ASSERT(offset >= 0);
        return offset;
    }
    inline int32_t StackOffsetToSlot(int32_t offset) const {
        
        
        
        
        
        
        
        return (masm.framePushed() - offset) / STACK_SLOT_SIZE;
    }

    
    inline int32_t StackOffsetOfPassedArg(int32_t slot) const {
        
        JS_ASSERT(slot >= 0 && slot <= int32_t(graph.argumentSlotCount()));
        int32_t offset = masm.framePushed() -
                       graph.paddedLocalSlotsSize() -
                       (slot * sizeof(Value));

        
        
        
        
        
        
        JS_ASSERT(offset >= 0);
        JS_ASSERT(offset % sizeof(Value) == 0);
        return offset;
    }

    inline int32_t ToStackOffset(const LAllocation *a) const {
        if (a->isArgument())
            return ArgToStackOffset(a->toArgument()->index());
        return SlotToStackOffset(a->toStackSlot()->slot());
    }

    uint32_t frameSize() const {
        return frameClass_ == FrameSizeClass::None() ? frameDepth_ : frameClass_.frameSize();
    }

  protected:
    
    
    
    size_t allocateCache(const IonCache &, size_t size) {
        size_t dataOffset = allocateData(size);
        masm.propagateOOM(cacheList_.append(dataOffset));
        return dataOffset;
    }

#ifdef CHECK_OSIPOINT_REGISTERS
    void resetOsiPointRegs(LSafepoint *safepoint);
    bool shouldVerifyOsiPointRegs(LSafepoint *safepoint);
    void verifyOsiPointRegs(LSafepoint *safepoint);
#endif

  public:

    
    
    
    friend class DataPtr;
    template <typename T>
    class DataPtr
    {
        CodeGeneratorShared *cg_;
        size_t index_;

        T *lookup() {
            return reinterpret_cast<T *>(&cg_->runtimeData_[index_]);
        }
      public:
        DataPtr(CodeGeneratorShared *cg, size_t index)
          : cg_(cg), index_(index) { }

        T * operator ->() {
            return lookup();
        }
        T * operator *() {
            return lookup();
        }
    };

  protected:

    size_t allocateData(size_t size) {
        JS_ASSERT(size % sizeof(void *) == 0);
        size_t dataOffset = runtimeData_.length();
        masm.propagateOOM(runtimeData_.appendN(0, size));
        return dataOffset;
    }

    template <typename T>
    inline size_t allocateCache(const T &cache) {
        size_t index = allocateCache(cache, sizeof(mozilla::AlignedStorage2<T>));
        
        JS_ASSERT(index == cacheList_.back());
        new (&runtimeData_[index]) T(cache);
        return index;
    }

  protected:
    
    
    bool encode(LSnapshot *snapshot);
    bool encodeSlots(LSnapshot *snapshot, MResumePoint *resumePoint, uint32_t *startIndex);

    
    
    
    bool assignBailoutId(LSnapshot *snapshot);

    
    
    void encodeSafepoints();

    
    
    bool markSafepoint(LInstruction *ins);
    bool markSafepointAt(uint32_t offset, LInstruction *ins);

    
    
    
    
    bool markOsiPoint(LOsiPoint *ins, uint32_t *returnPointOffset);

    
    
    
    
    
    void ensureOsiSpace();

    OutOfLineCode *oolTruncateDouble(const FloatRegister &src, const Register &dest);
    bool emitTruncateDouble(const FloatRegister &src, const Register &dest);
    bool emitTruncateFloat32(const FloatRegister &src, const Register &dest);

    void emitPreBarrier(Register base, const LAllocation *index, MIRType type);
    void emitPreBarrier(Address address, MIRType type);

    inline bool isNextBlock(LBlock *block) {
        return (current->mir()->id() + 1 == block->mir()->id());
    }

  public:
    
    
    
    
    
    
    
    
    
    void saveVolatile(Register output) {
        RegisterSet regs = RegisterSet::Volatile();
        regs.takeUnchecked(output);
        masm.PushRegsInMask(regs);
    }
    void restoreVolatile(Register output) {
        RegisterSet regs = RegisterSet::Volatile();
        regs.takeUnchecked(output);
        masm.PopRegsInMask(regs);
    }
    void saveVolatile(FloatRegister output) {
        RegisterSet regs = RegisterSet::Volatile();
        regs.takeUnchecked(output);
        masm.PushRegsInMask(regs);
    }
    void restoreVolatile(FloatRegister output) {
        RegisterSet regs = RegisterSet::Volatile();
        regs.takeUnchecked(output);
        masm.PopRegsInMask(regs);
    }
    void saveVolatile(RegisterSet temps) {
        masm.PushRegsInMask(RegisterSet::VolatileNot(temps));
    }
    void restoreVolatile(RegisterSet temps) {
        masm.PopRegsInMask(RegisterSet::VolatileNot(temps));
    }
    void saveVolatile() {
        masm.PushRegsInMask(RegisterSet::Volatile());
    }
    void restoreVolatile() {
        masm.PopRegsInMask(RegisterSet::Volatile());
    }

    
    
    
    
    inline void saveLive(LInstruction *ins);
    inline void restoreLive(LInstruction *ins);
    inline void restoreLiveIgnore(LInstruction *ins, RegisterSet reg);

    template <typename T>
    void pushArg(const T &t) {
        masm.Push(t);
#ifdef DEBUG
        pushedArgs_++;
#endif
    }

    void storeResultTo(const Register &reg) {
        masm.storeCallResult(reg);
    }

    void storeFloatResultTo(const FloatRegister &reg) {
        masm.storeCallFloatResult(reg);
    }

    template <typename T>
    void storeResultValueTo(const T &t) {
        masm.storeCallResultValue(t);
    }

    bool callVM(const VMFunction &f, LInstruction *ins, const Register *dynStack = nullptr);

    template <class ArgSeq, class StoreOutputTo>
    inline OutOfLineCode *oolCallVM(const VMFunction &fun, LInstruction *ins, const ArgSeq &args,
                                    const StoreOutputTo &out);

    bool callVM(const VMFunctionsModal &f, LInstruction *ins, const Register *dynStack = nullptr) {
        return callVM(f[gen->info().executionMode()], ins, dynStack);
    }

    template <class ArgSeq, class StoreOutputTo>
    inline OutOfLineCode *oolCallVM(const VMFunctionsModal &f, LInstruction *ins,
                                    const ArgSeq &args, const StoreOutputTo &out)
    {
        return oolCallVM(f[gen->info().executionMode()], ins, args, out);
    }

    bool addCache(LInstruction *lir, size_t cacheIndex);
    size_t addCacheLocations(const CacheLocationList &locs, size_t *numLocs);

  protected:
    bool addOutOfLineCode(OutOfLineCode *code);
    bool hasOutOfLineCode() { return !outOfLineCode_.empty(); }
    bool generateOutOfLineCode();

    Label *labelForBackedgeWithImplicitCheck(MBasicBlock *mir);

    
    
    
    
    void jumpToBlock(MBasicBlock *mir);
    void jumpToBlock(MBasicBlock *mir, Assembler::Condition cond);

  private:
    void generateInvalidateEpilogue();

  public:
    CodeGeneratorShared(MIRGenerator *gen, LIRGraph *graph, MacroAssembler *masm);

  public:
    template <class ArgSeq, class StoreOutputTo>
    bool visitOutOfLineCallVM(OutOfLineCallVM<ArgSeq, StoreOutputTo> *ool);

    bool visitOutOfLineTruncateSlow(OutOfLineTruncateSlow *ool);

  public:
    bool callTraceLIR(uint32_t blockIndex, LInstruction *lir, const char *bailoutName = nullptr);

    
    
    
    
    
    
    
    
    
    
    
    
    OutOfLineAbortPar *oolAbortPar(ParallelBailoutCause cause, MBasicBlock *basicBlock,
                                   jsbytecode *bytecode);
    OutOfLineAbortPar *oolAbortPar(ParallelBailoutCause cause, LInstruction *lir);
    OutOfLinePropagateAbortPar *oolPropagateAbortPar(LInstruction *lir);
    virtual bool visitOutOfLineAbortPar(OutOfLineAbortPar *ool) = 0;
    virtual bool visitOutOfLinePropagateAbortPar(OutOfLinePropagateAbortPar *ool) = 0;
};


class OutOfLineCode : public TempObject
{
    Label entry_;
    Label rejoin_;
    uint32_t framePushed_;
    jsbytecode *pc_;
    JSScript *script_;

  public:
    OutOfLineCode()
      : framePushed_(0),
        pc_(nullptr),
        script_(nullptr)
    { }

    virtual bool generate(CodeGeneratorShared *codegen) = 0;

    Label *entry() {
        return &entry_;
    }
    virtual void bind(MacroAssembler *masm) {
        masm->bind(entry());
    }
    Label *rejoin() {
        return &rejoin_;
    }
    void setFramePushed(uint32_t framePushed) {
        framePushed_ = framePushed;
    }
    uint32_t framePushed() const {
        return framePushed_;
    }
    void setSource(JSScript *script, jsbytecode *pc) {
        script_ = script;
        pc_ = pc;
    }
    jsbytecode *pc() {
        return pc_;
    }
    JSScript *script() {
        return script_;
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

















template <class SeqType, typename LastType>
class ArgSeq : public SeqType
{
  private:
    typedef ArgSeq<SeqType, LastType> ThisType;
    LastType last_;

  public:
    ArgSeq(const SeqType &seq, const LastType &last)
      : SeqType(seq),
        last_(last)
    { }

    template <typename NextType>
    inline ArgSeq<ThisType, NextType>
    operator, (const NextType &last) const {
        return ArgSeq<ThisType, NextType>(*this, last);
    }

    inline void generate(CodeGeneratorShared *codegen) const {
        codegen->pushArg(last_);
        this->SeqType::generate(codegen);
    }
};


template <>
class ArgSeq<void, void>
{
  private:
    typedef ArgSeq<void, void> ThisType;

  public:
    ArgSeq() { }
    ArgSeq(const ThisType &) { }

    template <typename NextType>
    inline ArgSeq<ThisType, NextType>
    operator, (const NextType &last) const {
        return ArgSeq<ThisType, NextType>(*this, last);
    }

    inline void generate(CodeGeneratorShared *codegen) const {
    }
};

inline ArgSeq<void, void>
ArgList()
{
    return ArgSeq<void, void>();
}



struct StoreNothing
{
    inline void generate(CodeGeneratorShared *codegen) const {
    }
    inline RegisterSet clobbered() const {
        return RegisterSet(); 
    }
};

class StoreRegisterTo
{
  private:
    Register out_;

  public:
    StoreRegisterTo(const Register &out)
      : out_(out)
    { }

    inline void generate(CodeGeneratorShared *codegen) const {
        codegen->storeResultTo(out_);
    }
    inline RegisterSet clobbered() const {
        RegisterSet set = RegisterSet();
        set.add(out_);
        return set;
    }
};

class StoreFloatRegisterTo
{
  private:
    FloatRegister out_;

  public:
    StoreFloatRegisterTo(const FloatRegister &out)
      : out_(out)
    { }

    inline void generate(CodeGeneratorShared *codegen) const {
        codegen->storeFloatResultTo(out_);
    }
    inline RegisterSet clobbered() const {
        RegisterSet set = RegisterSet();
        set.add(out_);
        return set;
    }
};

template <typename Output>
class StoreValueTo_
{
  private:
    Output out_;

  public:
    StoreValueTo_(const Output &out)
      : out_(out)
    { }

    inline void generate(CodeGeneratorShared *codegen) const {
        codegen->storeResultValueTo(out_);
    }
    inline RegisterSet clobbered() const {
        RegisterSet set = RegisterSet();
        set.add(out_);
        return set;
    }
};

template <typename Output>
StoreValueTo_<Output> StoreValueTo(const Output &out)
{
    return StoreValueTo_<Output>(out);
}

template <class ArgSeq, class StoreOutputTo>
class OutOfLineCallVM : public OutOfLineCodeBase<CodeGeneratorShared>
{
  private:
    LInstruction *lir_;
    const VMFunction &fun_;
    ArgSeq args_;
    StoreOutputTo out_;

  public:
    OutOfLineCallVM(LInstruction *lir, const VMFunction &fun, const ArgSeq &args,
                    const StoreOutputTo &out)
      : lir_(lir),
        fun_(fun),
        args_(args),
        out_(out)
    { }

    bool accept(CodeGeneratorShared *codegen) {
        return codegen->visitOutOfLineCallVM(this);
    }

    LInstruction *lir() const { return lir_; }
    const VMFunction &function() const { return fun_; }
    const ArgSeq &args() const { return args_; }
    const StoreOutputTo &out() const { return out_; }
};

template <class ArgSeq, class StoreOutputTo>
inline OutOfLineCode *
CodeGeneratorShared::oolCallVM(const VMFunction &fun, LInstruction *lir, const ArgSeq &args,
                               const StoreOutputTo &out)
{
    OutOfLineCode *ool = new(alloc()) OutOfLineCallVM<ArgSeq, StoreOutputTo>(lir, fun, args, out);
    if (!addOutOfLineCode(ool))
        return nullptr;
    return ool;
}

template <class ArgSeq, class StoreOutputTo>
bool
CodeGeneratorShared::visitOutOfLineCallVM(OutOfLineCallVM<ArgSeq, StoreOutputTo> *ool)
{
    LInstruction *lir = ool->lir();

    saveLive(lir);
    ool->args().generate(this);
    if (!callVM(ool->function(), lir))
        return false;
    ool->out().generate(this);
    restoreLiveIgnore(lir, ool->out().clobbered());
    masm.jump(ool->rejoin());
    return true;
}



class OutOfLineAbortPar : public OutOfLineCode
{
  private:
    ParallelBailoutCause cause_;
    MBasicBlock *basicBlock_;
    jsbytecode *bytecode_;

  public:
    OutOfLineAbortPar(ParallelBailoutCause cause, MBasicBlock *basicBlock, jsbytecode *bytecode)
      : cause_(cause),
        basicBlock_(basicBlock),
        bytecode_(bytecode)
    { }

    ParallelBailoutCause cause() {
        return cause_;
    }

    MBasicBlock *basicBlock() {
        return basicBlock_;
    }

    jsbytecode *bytecode() {
        return bytecode_;
    }

    bool generate(CodeGeneratorShared *codegen);
};


class OutOfLinePropagateAbortPar : public OutOfLineCode
{
  private:
    LInstruction *lir_;

  public:
    OutOfLinePropagateAbortPar(LInstruction *lir)
      : lir_(lir)
    { }

    LInstruction *lir() { return lir_; }

    bool generate(CodeGeneratorShared *codegen);
};

extern const VMFunction InterruptCheckInfo;

} 
} 

#endif 
