





#ifndef jit_shared_CodeGenerator_shared_h
#define jit_shared_CodeGenerator_shared_h

#include "mozilla/Alignment.h"
#include "mozilla/Move.h"

#include "jit/JitFrames.h"
#include "jit/LIR.h"
#include "jit/MacroAssembler.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"
#include "jit/OptimizationTracking.h"
#include "jit/Safepoints.h"
#include "jit/Snapshots.h"
#include "jit/VMFunctions.h"

namespace js {
namespace jit {

class OutOfLineCode;
class CodeGenerator;
class MacroAssembler;
class IonCache;

template <class ArgSeq, class StoreOutputTo>
class OutOfLineCallVM;

class OutOfLineTruncateSlow;

struct PatchableBackedgeInfo
{
    CodeOffsetJump backedge;
    Label* loopHeader;
    Label* interruptCheck;

    PatchableBackedgeInfo(CodeOffsetJump backedge, Label* loopHeader, Label* interruptCheck)
      : backedge(backedge), loopHeader(loopHeader), interruptCheck(interruptCheck)
    {}
};

struct ReciprocalMulConstants {
    int64_t multiplier;
    int32_t shiftAmount;
};




struct NativeToTrackedOptimizations
{
    
    CodeOffsetLabel startOffset;
    CodeOffsetLabel endOffset;
    const TrackedOptimizations* optimizations;
};

class CodeGeneratorShared : public LElementVisitor
{
    js::Vector<OutOfLineCode*, 0, SystemAllocPolicy> outOfLineCode_;

    MacroAssembler& ensureMasm(MacroAssembler* masm);
    mozilla::Maybe<MacroAssembler> maybeMasm_;

  public:
    MacroAssembler& masm;

  protected:
    MIRGenerator* gen;
    LIRGraph& graph;
    LBlock* current;
    SnapshotWriter snapshots_;
    RecoverWriter recovers_;
    JitCode* deoptTable_;
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

    
    Vector<PatchableBackedgeInfo, 0, SystemAllocPolicy> patchableBackedges_;

#ifdef JS_TRACE_LOGGING
    js::Vector<CodeOffsetLabel, 0, SystemAllocPolicy> patchableTraceLoggers_;
    js::Vector<CodeOffsetLabel, 0, SystemAllocPolicy> patchableTLScripts_;
#endif

  public:
    struct NativeToBytecode {
        CodeOffsetLabel nativeOffset;
        InlineScriptTree* tree;
        jsbytecode* pc;
    };

  protected:
    js::Vector<NativeToBytecode, 0, SystemAllocPolicy> nativeToBytecodeList_;
    uint8_t* nativeToBytecodeMap_;
    uint32_t nativeToBytecodeMapSize_;
    uint32_t nativeToBytecodeTableOffset_;
    uint32_t nativeToBytecodeNumRegions_;

    JSScript** nativeToBytecodeScriptList_;
    uint32_t nativeToBytecodeScriptListLength_;

    bool isProfilerInstrumentationEnabled() {
        return gen->isProfilerInstrumentationEnabled();
    }

    js::Vector<NativeToTrackedOptimizations, 0, SystemAllocPolicy> trackedOptimizations_;
    uint8_t* trackedOptimizationsMap_;
    uint32_t trackedOptimizationsMapSize_;
    uint32_t trackedOptimizationsRegionTableOffset_;
    uint32_t trackedOptimizationsTypesTableOffset_;
    uint32_t trackedOptimizationsAttemptsTableOffset_;

    bool isOptimizationTrackingEnabled() {
        return gen->isOptimizationTrackingEnabled();
    }

  protected:
    
    
    size_t osrEntryOffset_;

    TempAllocator& alloc() const {
        return graph.mir().alloc();
    }

    inline void setOsrEntryOffset(size_t offset) {
        MOZ_ASSERT(osrEntryOffset_ == 0);
        osrEntryOffset_ = offset;
    }
    inline size_t getOsrEntryOffset() const {
        return osrEntryOffset_;
    }

    
    
    size_t skipArgCheckEntryOffset_;

    inline void setSkipArgCheckEntryOffset(size_t offset) {
        MOZ_ASSERT(skipArgCheckEntryOffset_ == 0);
        skipArgCheckEntryOffset_ = offset;
    }
    inline size_t getSkipArgCheckEntryOffset() const {
        return skipArgCheckEntryOffset_;
    }

    typedef js::Vector<SafepointIndex, 8, SystemAllocPolicy> SafepointIndices;

  protected:
#ifdef CHECK_OSIPOINT_REGISTERS
    
    
    
    bool checkOsiPointRegisters;
#endif

    
    
    
    int32_t frameDepth_;

    
    
    
    int32_t frameInitialAdjustment_;

    
    FrameSizeClass frameClass_;

    
    inline int32_t ArgToStackOffset(int32_t slot) const;

    
    inline int32_t CalleeStackOffset() const;

    inline int32_t SlotToStackOffset(int32_t slot) const;
    inline int32_t StackOffsetToSlot(int32_t offset) const;

    
    inline int32_t StackOffsetOfPassedArg(int32_t slot) const;

    inline int32_t ToStackOffset(LAllocation a) const;
    inline int32_t ToStackOffset(const LAllocation* a) const;

    uint32_t frameSize() const {
        return frameClass_ == FrameSizeClass::None() ? frameDepth_ : frameClass_.frameSize();
    }

    inline Operand ToOperand(const LAllocation& a);
    inline Operand ToOperand(const LAllocation* a);
    inline Operand ToOperand(const LDefinition* def);

  protected:
    
    
    
    size_t allocateCache(const IonCache&, size_t size) {
        size_t dataOffset = allocateData(size);
        masm.propagateOOM(cacheList_.append(dataOffset));
        return dataOffset;
    }

#ifdef CHECK_OSIPOINT_REGISTERS
    void resetOsiPointRegs(LSafepoint* safepoint);
    bool shouldVerifyOsiPointRegs(LSafepoint* safepoint);
    void verifyOsiPointRegs(LSafepoint* safepoint);
#endif

    bool addNativeToBytecodeEntry(const BytecodeSite* site);
    void dumpNativeToBytecodeEntries();
    void dumpNativeToBytecodeEntry(uint32_t idx);

    bool addTrackedOptimizationsEntry(const TrackedOptimizations* optimizations);
    void extendTrackedOptimizationsEntry(const TrackedOptimizations* optimizations);

  public:
    MIRGenerator& mirGen() const {
        return *gen;
    }

    
    
    
    friend class DataPtr;
    template <typename T>
    class DataPtr
    {
        CodeGeneratorShared* cg_;
        size_t index_;

        T* lookup() {
            return reinterpret_cast<T*>(&cg_->runtimeData_[index_]);
        }
      public:
        DataPtr(CodeGeneratorShared* cg, size_t index)
          : cg_(cg), index_(index) { }

        T * operator ->() {
            return lookup();
        }
        T * operator*() {
            return lookup();
        }
    };

  protected:

    size_t allocateData(size_t size) {
        MOZ_ASSERT(size % sizeof(void*) == 0);
        size_t dataOffset = runtimeData_.length();
        masm.propagateOOM(runtimeData_.appendN(0, size));
        return dataOffset;
    }

    template <typename T>
    inline size_t allocateCache(const T& cache) {
        size_t index = allocateCache(cache, sizeof(mozilla::AlignedStorage2<T>));
        if (masm.oom())
            return SIZE_MAX;
        
        MOZ_ASSERT(index == cacheList_.back());
        new (&runtimeData_[index]) T(cache);
        return index;
    }

  protected:
    
    void encode(LRecoverInfo* recover);
    void encode(LSnapshot* snapshot);
    void encodeAllocation(LSnapshot* snapshot, MDefinition* def, uint32_t* startIndex);

    
    
    
    bool assignBailoutId(LSnapshot* snapshot);

    
    
    void encodeSafepoints();

    
    bool createNativeToBytecodeScriptList(JSContext* cx);
    bool generateCompactNativeToBytecodeMap(JSContext* cx, JitCode* code);
    void verifyCompactNativeToBytecodeMap(JitCode* code);

    bool generateCompactTrackedOptimizationsMap(JSContext* cx, JitCode* code,
                                                IonTrackedTypeVector* allTypes);
    void verifyCompactTrackedOptimizationsMap(JitCode* code, uint32_t numRegions,
                                              const UniqueTrackedOptimizations& unique,
                                              const IonTrackedTypeVector* allTypes);

    
    
    void markSafepoint(LInstruction* ins);
    void markSafepointAt(uint32_t offset, LInstruction* ins);

    
    
    
    uint32_t markOsiPoint(LOsiPoint* ins);

    
    
    
    
    
    void ensureOsiSpace();

    OutOfLineCode* oolTruncateDouble(FloatRegister src, Register dest, MInstruction* mir);
    void emitTruncateDouble(FloatRegister src, Register dest, MInstruction* mir);
    void emitTruncateFloat32(FloatRegister src, Register dest, MInstruction* mir);

    void emitAsmJSCall(LAsmJSCall* ins);

    void emitPreBarrier(Register base, const LAllocation* index);
    void emitPreBarrier(Address address);

    
    
    
    MBasicBlock* skipTrivialBlocks(MBasicBlock* block) {
        while (block->lir()->isTrivial()) {
            MOZ_ASSERT(block->lir()->rbegin()->numSuccessors() == 1);
            block = block->lir()->rbegin()->getSuccessor(0);
        }
        return block;
    }

    
    
    inline bool isNextBlock(LBlock* block) {
        uint32_t target = skipTrivialBlocks(block->mir())->id();
        uint32_t i = current->mir()->id() + 1;
        if (target < i)
            return false;
        
        for (; i != target; ++i) {
            if (!graph.getBlock(i)->isTrivial())
                return false;
        }
        return true;
    }

  public:
    
    
    
    
    
    
    
    
    
    void saveVolatile(Register output) {
        LiveRegisterSet regs(RegisterSet::Volatile());
        regs.takeUnchecked(output);
        masm.PushRegsInMask(regs);
    }
    void restoreVolatile(Register output) {
        LiveRegisterSet regs(RegisterSet::Volatile());
        regs.takeUnchecked(output);
        masm.PopRegsInMask(regs);
    }
    void saveVolatile(FloatRegister output) {
        LiveRegisterSet regs(RegisterSet::Volatile());
        regs.takeUnchecked(output);
        masm.PushRegsInMask(regs);
    }
    void restoreVolatile(FloatRegister output) {
        LiveRegisterSet regs(RegisterSet::Volatile());
        regs.takeUnchecked(output);
        masm.PopRegsInMask(regs);
    }
    void saveVolatile(LiveRegisterSet temps) {
        masm.PushRegsInMask(LiveRegisterSet(RegisterSet::VolatileNot(temps.set())));
    }
    void restoreVolatile(LiveRegisterSet temps) {
        masm.PopRegsInMask(LiveRegisterSet(RegisterSet::VolatileNot(temps.set())));
    }
    void saveVolatile() {
        masm.PushRegsInMask(LiveRegisterSet(RegisterSet::Volatile()));
    }
    void restoreVolatile() {
        masm.PopRegsInMask(LiveRegisterSet(RegisterSet::Volatile()));
    }

    
    
    
    
    inline void saveLive(LInstruction* ins);
    inline void restoreLive(LInstruction* ins);
    inline void restoreLiveIgnore(LInstruction* ins, LiveRegisterSet reg);

    
    inline void saveLiveVolatile(LInstruction* ins);
    inline void restoreLiveVolatile(LInstruction* ins);

    template <typename T>
    void pushArg(const T& t) {
        masm.Push(t);
#ifdef DEBUG
        pushedArgs_++;
#endif
    }

    void storeResultTo(Register reg) {
        masm.storeCallResult(reg);
    }

    void storeFloatResultTo(FloatRegister reg) {
        masm.storeCallFloatResult(reg);
    }

    template <typename T>
    void storeResultValueTo(const T& t) {
        masm.storeCallResultValue(t);
    }

    void callVM(const VMFunction& f, LInstruction* ins, const Register* dynStack = nullptr);

    template <class ArgSeq, class StoreOutputTo>
    inline OutOfLineCode* oolCallVM(const VMFunction& fun, LInstruction* ins, const ArgSeq& args,
                                    const StoreOutputTo& out);

    void addCache(LInstruction* lir, size_t cacheIndex);
    size_t addCacheLocations(const CacheLocationList& locs, size_t* numLocs);
    ReciprocalMulConstants computeDivisionConstants(uint32_t d, int maxLog);

  protected:
    void addOutOfLineCode(OutOfLineCode* code, const MInstruction* mir);
    void addOutOfLineCode(OutOfLineCode* code, const BytecodeSite* site);
    bool generateOutOfLineCode();

    Label* labelForBackedgeWithImplicitCheck(MBasicBlock* mir);

    
    
    
    
    void jumpToBlock(MBasicBlock* mir);


#ifndef JS_CODEGEN_MIPS
    void jumpToBlock(MBasicBlock* mir, Assembler::Condition cond);
#endif

  private:
    void generateInvalidateEpilogue();

  public:
    CodeGeneratorShared(MIRGenerator* gen, LIRGraph* graph, MacroAssembler* masm);

  public:
    template <class ArgSeq, class StoreOutputTo>
    void visitOutOfLineCallVM(OutOfLineCallVM<ArgSeq, StoreOutputTo>* ool);

    void visitOutOfLineTruncateSlow(OutOfLineTruncateSlow* ool);

    bool omitOverRecursedCheck() const;

#ifdef JS_TRACE_LOGGING
  protected:
    void emitTracelogScript(bool isStart);
    void emitTracelogTree(bool isStart, uint32_t textId);

  public:
    void emitTracelogScriptStart() {
        emitTracelogScript( true);
    }
    void emitTracelogScriptStop() {
        emitTracelogScript( false);
    }
    void emitTracelogStartEvent(uint32_t textId) {
        emitTracelogTree( true, textId);
    }
    void emitTracelogStopEvent(uint32_t textId) {
        emitTracelogTree( false, textId);
    }
#endif
    void emitTracelogIonStart() {
#ifdef JS_TRACE_LOGGING
        emitTracelogScriptStart();
        emitTracelogStartEvent(TraceLogger_IonMonkey);
#endif
    }
    void emitTracelogIonStop() {
#ifdef JS_TRACE_LOGGING
        emitTracelogStopEvent(TraceLogger_IonMonkey);
        emitTracelogScriptStop();
#endif
    }

    inline void verifyHeapAccessDisassembly(uint32_t begin, uint32_t end, bool isLoad,
                                            Scalar::Type type, unsigned numElems,
                                            const Operand& mem, LAllocation alloc);
};


class OutOfLineCode : public TempObject
{
    Label entry_;
    Label rejoin_;
    uint32_t framePushed_;
    const BytecodeSite* site_;

  public:
    OutOfLineCode()
      : framePushed_(0),
        site_()
    { }

    virtual void generate(CodeGeneratorShared* codegen) = 0;

    Label* entry() {
        return &entry_;
    }
    virtual void bind(MacroAssembler* masm) {
        masm->bind(entry());
    }
    Label* rejoin() {
        return &rejoin_;
    }
    void setFramePushed(uint32_t framePushed) {
        framePushed_ = framePushed;
    }
    uint32_t framePushed() const {
        return framePushed_;
    }
    void setBytecodeSite(const BytecodeSite* site) {
        site_ = site;
    }
    const BytecodeSite* bytecodeSite() const {
        return site_;
    }
    jsbytecode* pc() const {
        return site_->pc();
    }
    JSScript* script() const {
        return site_->script();
    }
};


template <typename T>
class OutOfLineCodeBase : public OutOfLineCode
{
  public:
    virtual void generate(CodeGeneratorShared* codegen) {
        accept(static_cast<T*>(codegen));
    }

  public:
    virtual void accept(T* codegen) = 0;
};

















template <typename... ArgTypes>
class ArgSeq;

template <>
class ArgSeq<>
{
  public:
    ArgSeq() { }

    inline void generate(CodeGeneratorShared* codegen) const {
    }
};

template <typename HeadType, typename... TailTypes>
class ArgSeq<HeadType, TailTypes...> : public ArgSeq<TailTypes...>
{
  private:
    HeadType head_;

  public:
    explicit ArgSeq(HeadType&& head, TailTypes&&... tail)
      : ArgSeq<TailTypes...>(mozilla::Move(tail)...),
        head_(mozilla::Move(head))
    { }

    
    
    inline void generate(CodeGeneratorShared* codegen) const {
        this->ArgSeq<TailTypes...>::generate(codegen);
        codegen->pushArg(head_);
    }
};

template <typename... ArgTypes>
inline ArgSeq<ArgTypes...>
ArgList(ArgTypes... args)
{
    return ArgSeq<ArgTypes...>(mozilla::Move(args)...);
}



struct StoreNothing
{
    inline void generate(CodeGeneratorShared* codegen) const {
    }
    inline LiveRegisterSet clobbered() const {
        return LiveRegisterSet(); 
    }
};

class StoreRegisterTo
{
  private:
    Register out_;

  public:
    explicit StoreRegisterTo(Register out)
      : out_(out)
    { }

    inline void generate(CodeGeneratorShared* codegen) const {
        codegen->storeResultTo(out_);
    }
    inline LiveRegisterSet clobbered() const {
        LiveRegisterSet set;
        set.add(out_);
        return set;
    }
};

class StoreFloatRegisterTo
{
  private:
    FloatRegister out_;

  public:
    explicit StoreFloatRegisterTo(FloatRegister out)
      : out_(out)
    { }

    inline void generate(CodeGeneratorShared* codegen) const {
        codegen->storeFloatResultTo(out_);
    }
    inline LiveRegisterSet clobbered() const {
        LiveRegisterSet set;
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
    explicit StoreValueTo_(const Output& out)
      : out_(out)
    { }

    inline void generate(CodeGeneratorShared* codegen) const {
        codegen->storeResultValueTo(out_);
    }
    inline LiveRegisterSet clobbered() const {
        LiveRegisterSet set;
        set.add(out_);
        return set;
    }
};

template <typename Output>
StoreValueTo_<Output> StoreValueTo(const Output& out)
{
    return StoreValueTo_<Output>(out);
}

template <class ArgSeq, class StoreOutputTo>
class OutOfLineCallVM : public OutOfLineCodeBase<CodeGeneratorShared>
{
  private:
    LInstruction* lir_;
    const VMFunction& fun_;
    ArgSeq args_;
    StoreOutputTo out_;

  public:
    OutOfLineCallVM(LInstruction* lir, const VMFunction& fun, const ArgSeq& args,
                    const StoreOutputTo& out)
      : lir_(lir),
        fun_(fun),
        args_(args),
        out_(out)
    { }

    void accept(CodeGeneratorShared* codegen) {
        codegen->visitOutOfLineCallVM(this);
    }

    LInstruction* lir() const { return lir_; }
    const VMFunction& function() const { return fun_; }
    const ArgSeq& args() const { return args_; }
    const StoreOutputTo& out() const { return out_; }
};

template <class ArgSeq, class StoreOutputTo>
inline OutOfLineCode*
CodeGeneratorShared::oolCallVM(const VMFunction& fun, LInstruction* lir, const ArgSeq& args,
                               const StoreOutputTo& out)
{
    MOZ_ASSERT(lir->mirRaw());
    MOZ_ASSERT(lir->mirRaw()->isInstruction());

    OutOfLineCode* ool = new(alloc()) OutOfLineCallVM<ArgSeq, StoreOutputTo>(lir, fun, args, out);
    addOutOfLineCode(ool, lir->mirRaw()->toInstruction());
    return ool;
}

template <class ArgSeq, class StoreOutputTo>
void
CodeGeneratorShared::visitOutOfLineCallVM(OutOfLineCallVM<ArgSeq, StoreOutputTo>* ool)
{
    LInstruction* lir = ool->lir();

    saveLive(lir);
    ool->args().generate(this);
    callVM(ool->function(), lir);
    ool->out().generate(this);
    restoreLiveIgnore(lir, ool->out().clobbered());
    masm.jump(ool->rejoin());
}

} 
} 

#endif 
