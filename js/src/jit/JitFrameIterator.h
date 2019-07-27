





#ifndef jit_JitFrameIterator_h
#define jit_JitFrameIterator_h

#include "jsfun.h"
#include "jsscript.h"
#include "jstypes.h"

#include "jit/IonCode.h"
#include "jit/Snapshots.h"

#include "js/ProfilingFrameIterator.h"

namespace js {
    class ActivationIterator;
};

namespace js {
namespace jit {

enum FrameType
{
    
    
    JitFrame_IonJS,

    
    JitFrame_BaselineJS,

    
    
    JitFrame_BaselineStub,

    
    
    JitFrame_Entry,

    
    
    JitFrame_Rectifier,

    
    JitFrame_IonAccessorIC,

    
    
    
    JitFrame_Unwound_BaselineJS,
    JitFrame_Unwound_IonJS,
    JitFrame_Unwound_BaselineStub,
    JitFrame_Unwound_Rectifier,
    JitFrame_Unwound_IonAccessorIC,

    
    
    
    JitFrame_Exit,

    
    
    
    
    JitFrame_Bailout
};

enum ReadFrameArgsBehavior {
    
    ReadFrame_Formals,

    
    ReadFrame_Overflown,

    
    ReadFrame_Actuals
};

class CommonFrameLayout;
class JitFrameLayout;
class ExitFrameLayout;

class BaselineFrame;

class JitActivation;




void AssertJitStackInvariants(JSContext *cx);

class JitFrameIterator
{
  protected:
    uint8_t *current_;
    FrameType type_;
    uint8_t *returnAddressToFp_;
    size_t frameSize_;

  private:
    mutable const SafepointIndex *cachedSafepointIndex_;
    const JitActivation *activation_;

    void dumpBaseline() const;

  public:
    explicit JitFrameIterator();
    explicit JitFrameIterator(JSContext *cx);
    explicit JitFrameIterator(const ActivationIterator &activations);

    
    FrameType type() const {
        return type_;
    }
    uint8_t *fp() const {
        return current_;
    }
    const JitActivation *activation() const {
        return activation_;
    }

    CommonFrameLayout *current() const {
        return (CommonFrameLayout *)current_;
    }

    inline uint8_t *returnAddress() const;

    
    
    JitFrameLayout *jsFrame() const;

    
    inline bool isFakeExitFrame() const;

    inline ExitFrameLayout *exitFrame() const;

    
    
    bool checkInvalidation(IonScript **ionScript) const;
    bool checkInvalidation() const;

    bool isScripted() const {
        return type_ == JitFrame_BaselineJS || type_ == JitFrame_IonJS || type_ == JitFrame_Bailout;
    }
    bool isBaselineJS() const {
        return type_ == JitFrame_BaselineJS;
    }
    bool isIonScripted() const {
        return type_ == JitFrame_IonJS || type_ == JitFrame_Bailout;
    }
    bool isIonJS() const {
        return type_ == JitFrame_IonJS;
    }
    bool isBailoutJS() const {
        return type_ == JitFrame_Bailout;
    }
    bool isBaselineStub() const {
        return type_ == JitFrame_BaselineStub;
    }
    bool isBaselineStubMaybeUnwound() const {
        return type_ == JitFrame_BaselineStub || type_ == JitFrame_Unwound_BaselineStub;
    }
    bool isRectifierMaybeUnwound() const {
        return type_ == JitFrame_Rectifier || type_ == JitFrame_Unwound_Rectifier;
    }
    bool isBareExit() const;
    template <typename T> bool isExitFrameLayout() const;

    bool isEntry() const {
        return type_ == JitFrame_Entry;
    }
    bool isFunctionFrame() const;

    bool isConstructing() const;

    void *calleeToken() const;
    JSFunction *callee() const;
    JSFunction *maybeCallee() const;
    unsigned numActualArgs() const;
    JSScript *script() const;
    void baselineScriptAndPc(JSScript **scriptRes, jsbytecode **pcRes) const;
    Value *actualArgs() const;

    
    
    uint8_t *returnAddressToFp() const {
        return returnAddressToFp_;
    }

    
    inline size_t prevFrameLocalSize() const;
    inline FrameType prevType() const;
    uint8_t *prevFp() const;

    
    
    size_t frameSize() const {
        MOZ_ASSERT(type_ != JitFrame_Exit);
        return frameSize_;
    }

    
    
    inline bool done() const {
        return type_ == JitFrame_Entry;
    }
    JitFrameIterator &operator++();

    
    IonScript *ionScript() const;

    
    
    IonScript *ionScriptFromCalleeToken() const;

    
    
    const SafepointIndex *safepoint() const;

    
    
    const OsiIndex *osiIndex() const;

    
    
    SnapshotOffset snapshotOffset() const;

    uintptr_t *spillBase() const;
    MachineState machineState() const;

    template <class Op>
    void unaliasedForEachActual(Op op, ReadFrameArgsBehavior behavior) const {
        MOZ_ASSERT(isBaselineJS());

        unsigned nactual = numActualArgs();
        unsigned start, end;
        switch (behavior) {
          case ReadFrame_Formals:
            start = 0;
            end = callee()->nargs();
            break;
          case ReadFrame_Overflown:
            start = callee()->nargs();
            end = nactual;
            break;
          case ReadFrame_Actuals:
            start = 0;
            end = nactual;
        }

        Value *argv = actualArgs();
        for (unsigned i = start; i < end; i++)
            op(argv[i]);
    }

    void dump() const;

    inline BaselineFrame *baselineFrame() const;

#ifdef DEBUG
    bool verifyReturnAddressUsingNativeToBytecodeMap();
#else
    inline bool verifyReturnAddressUsingNativeToBytecodeMap() { return true; }
#endif
};

class JitcodeGlobalTable;

class JitProfilingFrameIterator
{
    uint8_t *fp_;
    FrameType type_;
    void *returnAddressToFp_;

    inline JitFrameLayout *framePtr();
    inline JSScript *frameScript();
    bool tryInitWithPC(void *pc);
    bool tryInitWithTable(JitcodeGlobalTable *table, void *pc, JSRuntime *rt,
                          bool forLastCallSite);

  public:
    JitProfilingFrameIterator(JSRuntime *rt,
                              const JS::ProfilingFrameIterator::RegisterState &state);
    explicit JitProfilingFrameIterator(void *exitFrame);

    void operator++();
    bool done() const { return fp_ == nullptr; }

    void *fp() const { MOZ_ASSERT(!done()); return fp_; }
    void *stackAddress() const { return fp(); }
    FrameType frameType() const { MOZ_ASSERT(!done()); return type_; }
    void *returnAddressToFp() const { MOZ_ASSERT(!done()); return returnAddressToFp_; }
};

class RInstructionResults
{
    
    typedef mozilla::Vector<RelocatableValue, 1, SystemAllocPolicy> Values;
    mozilla::UniquePtr<Values, JS::DeletePolicy<Values> > results_;

    
    
    JitFrameLayout *fp_;

    
    
    
    bool initialized_;

  public:
    explicit RInstructionResults(JitFrameLayout *fp);
    RInstructionResults(RInstructionResults&& src);

    RInstructionResults& operator=(RInstructionResults&& rhs);

    ~RInstructionResults();

    bool init(JSContext *cx, uint32_t numResults);
    bool isInitialized() const;

    JitFrameLayout *frame() const;

    RelocatableValue& operator[](size_t index);

    void trace(JSTracer *trc);
};

struct MaybeReadFallback
{
    enum NoGCValue {
        NoGC_UndefinedValue,
        NoGC_MagicOptimizedOut
    };

    enum FallbackConsequence {
        Fallback_Invalidate,
        Fallback_DoNothing
    };

    JSContext *maybeCx;
    JitActivation *activation;
    const JitFrameIterator *frame;
    const NoGCValue unreadablePlaceholder_;
    const FallbackConsequence consequence;

    explicit MaybeReadFallback(const Value &placeholder = UndefinedValue())
      : maybeCx(nullptr),
        activation(nullptr),
        frame(nullptr),
        unreadablePlaceholder_(noGCPlaceholder(placeholder)),
        consequence(Fallback_Invalidate)
    {
    }

    MaybeReadFallback(JSContext *cx, JitActivation *activation, const JitFrameIterator *frame,
                      FallbackConsequence consequence = Fallback_Invalidate)
      : maybeCx(cx),
        activation(activation),
        frame(frame),
        unreadablePlaceholder_(NoGC_UndefinedValue),
        consequence(consequence)
    {
    }

    bool canRecoverResults() { return maybeCx; }

    Value unreadablePlaceholder() const {
        if (unreadablePlaceholder_ == NoGC_MagicOptimizedOut)
            return MagicValue(JS_OPTIMIZED_OUT);
        return UndefinedValue();
    }

    NoGCValue noGCPlaceholder(Value v) const {
        if (v.isMagic(JS_OPTIMIZED_OUT))
            return NoGC_MagicOptimizedOut;
        return NoGC_UndefinedValue;
    }
};


class RResumePoint;
class RSimdBox;



class SnapshotIterator
{
  protected:
    SnapshotReader snapshot_;
    RecoverReader recover_;
    JitFrameLayout *fp_;
    MachineState machine_;
    IonScript *ionScript_;
    RInstructionResults *instructionResults_;

    enum ReadMethod {
        
        RM_Normal          = 1 << 0,

        
        RM_AlwaysDefault   = 1 << 1,

        
        
        RM_NormalOrDefault = RM_Normal | RM_AlwaysDefault,
    };

  private:
    
    bool hasRegister(Register reg) const {
        return machine_.has(reg);
    }
    uintptr_t fromRegister(Register reg) const {
        return machine_.read(reg);
    }

    bool hasRegister(FloatRegister reg) const {
        return machine_.has(reg);
    }
    double fromRegister(FloatRegister reg) const {
        return machine_.read(reg);
    }

    
    bool hasStack(int32_t offset) const {
        return true;
    }
    uintptr_t fromStack(int32_t offset) const;

    bool hasInstructionResult(uint32_t index) const {
        return instructionResults_;
    }
    bool hasInstructionResults() const {
        return instructionResults_;
    }
    Value fromInstructionResult(uint32_t index) const;

    Value allocationValue(const RValueAllocation &a, ReadMethod rm = RM_Normal);
    bool allocationReadable(const RValueAllocation &a, ReadMethod rm = RM_Normal);
    void writeAllocationValuePayload(const RValueAllocation &a, Value v);
    void warnUnreadableAllocation();

  private:
    friend class RSimdBox;
    const FloatRegisters::RegisterContent *floatAllocationPointer(const RValueAllocation &a) const;

  public:
    
    inline RValueAllocation readAllocation() {
        MOZ_ASSERT(moreAllocations());
        return snapshot_.readAllocation();
    }
    Value skip() {
        snapshot_.skipAllocation();
        return UndefinedValue();
    }

    const RResumePoint *resumePoint() const;
    const RInstruction *instruction() const {
        return recover_.instruction();
    }

    uint32_t numAllocations() const;
    inline bool moreAllocations() const {
        return snapshot_.numAllocationsRead() < numAllocations();
    }

    int32_t readOuterNumActualArgs() const;

    
    
    void storeInstructionResult(Value v);

  public:
    
    uint32_t pcOffset() const;
    inline bool resumeAfter() const {
        
        
        
        if (moreFrames())
            return false;
        return recover_.resumeAfter();
    }
    inline BailoutKind bailoutKind() const {
        return snapshot_.bailoutKind();
    }

  public:
    
    
    inline void nextInstruction() {
        MOZ_ASSERT(snapshot_.numAllocationsRead() == numAllocations());
        recover_.nextInstruction();
        snapshot_.resetNumAllocationsRead();
    }

    
    
    void skipInstruction();

    inline bool moreInstructions() const {
        return recover_.moreInstructions();
    }

  protected:
    
    
    
    
    bool initInstructionResults(MaybeReadFallback &fallback);

    
    
    bool computeInstructionResults(JSContext *cx, RInstructionResults *results) const;

  public:
    
    void nextFrame();
    void settleOnFrame();

    inline bool moreFrames() const {
        
        
        return moreInstructions();
    }

  public:
    
    

    SnapshotIterator(IonScript *ionScript, SnapshotOffset snapshotOffset,
                     JitFrameLayout *fp, const MachineState &machine);
    explicit SnapshotIterator(const JitFrameIterator &iter);
    SnapshotIterator();

    Value read() {
        return allocationValue(readAllocation());
    }

    
    
    
    
    Value readWithDefault(RValueAllocation *alloc) {
        *alloc = RValueAllocation();
        RValueAllocation a = readAllocation();
        if (allocationReadable(a))
            return allocationValue(a);

        *alloc = a;
        return allocationValue(a, RM_AlwaysDefault);
    }

    Value maybeRead(const RValueAllocation &a, MaybeReadFallback &fallback);
    Value maybeRead(MaybeReadFallback &fallback) {
        RValueAllocation a = readAllocation();
        return maybeRead(a, fallback);
    }

    void traceAllocation(JSTracer *trc);

    template <class Op>
    void readFunctionFrameArgs(Op &op, ArgumentsObject **argsObj, Value *thisv,
                               unsigned start, unsigned end, JSScript *script,
                               MaybeReadFallback &fallback)
    {
        
        if (script->argumentsHasVarBinding()) {
            if (argsObj) {
                Value v = read();
                if (v.isObject())
                    *argsObj = &v.toObject().as<ArgumentsObject>();
            } else {
                skip();
            }
        }

        if (thisv)
            *thisv = maybeRead(fallback);
        else
            skip();

        unsigned i = 0;
        if (end < start)
            i = start;

        for (; i < start; i++)
            skip();
        for (; i < end; i++) {
            
            
            
            Value v = maybeRead(fallback);
            op(v);
        }
    }

    
    
    Value maybeReadAllocByIndex(size_t index);

#ifdef TRACK_SNAPSHOTS
    void spewBailingFrom() const {
        snapshot_.spewBailingFrom();
    }
#endif
};



class InlineFrameIterator
{
    const JitFrameIterator *frame_;
    SnapshotIterator start_;
    SnapshotIterator si_;
    uint32_t framesRead_;

    
    
    
    
    uint32_t frameCount_;

    
    
    
    
    
    
    
    RootedFunction calleeTemplate_;
    RValueAllocation calleeRVA_;

    RootedScript script_;
    jsbytecode *pc_;
    uint32_t numActualArgs_;

    struct Nop {
        void operator()(const Value &v) { }
    };

  private:
    void findNextFrame();
    JSObject *computeScopeChain(Value scopeChainValue, MaybeReadFallback &fallback,
                                bool *hasCallObj = nullptr) const;

  public:
    InlineFrameIterator(JSContext *cx, const JitFrameIterator *iter);
    InlineFrameIterator(JSRuntime *rt, const JitFrameIterator *iter);
    InlineFrameIterator(JSContext *cx, const InlineFrameIterator *iter);

    bool more() const {
        return frame_ && framesRead_ < frameCount_;
    }

    
    
    
    
    
    
    
    JSFunction *calleeTemplate() const {
        MOZ_ASSERT(isFunctionFrame());
        return calleeTemplate_;
    }
    JSFunction *maybeCalleeTemplate() const {
        return calleeTemplate_;
    }

    JSFunction *callee(MaybeReadFallback &fallback) const;

    unsigned numActualArgs() const {
        
        
        
        
        
        if (more())
            return numActualArgs_;

        return frame_->numActualArgs();
    }

    template <class ArgOp, class LocalOp>
    void readFrameArgsAndLocals(JSContext *cx, ArgOp &argOp, LocalOp &localOp,
                                JSObject **scopeChain, bool *hasCallObj, Value *rval,
                                ArgumentsObject **argsObj, Value *thisv,
                                ReadFrameArgsBehavior behavior,
                                MaybeReadFallback &fallback) const
    {
        SnapshotIterator s(si_);

        
        if (scopeChain) {
            Value scopeChainValue = s.maybeRead(fallback);
            *scopeChain = computeScopeChain(scopeChainValue, fallback, hasCallObj);
        } else {
            s.skip();
        }

        
        if (rval)
            *rval = s.read();
        else
            s.skip();

        
        if (isFunctionFrame()) {
            unsigned nactual = numActualArgs();
            unsigned nformal = calleeTemplate()->nargs();

            
            
            
            if (behavior != ReadFrame_Overflown)
                s.readFunctionFrameArgs(argOp, argsObj, thisv, 0, nformal, script(), fallback);

            if (behavior != ReadFrame_Formals) {
                if (more()) {
                    
                    
                    
                    

                    
                    
                    
                    InlineFrameIterator it(cx, this);
                    ++it;
                    unsigned argsObjAdj = it.script()->argumentsHasVarBinding() ? 1 : 0;
                    SnapshotIterator parent_s(it.snapshotIterator());

                    
                    
                    
                    MOZ_ASSERT(parent_s.numAllocations() >= nactual + 3 + argsObjAdj);
                    unsigned skip = parent_s.numAllocations() - nactual - 3 - argsObjAdj;
                    for (unsigned j = 0; j < skip; j++)
                        parent_s.skip();

                    
                    MaybeReadFallback unusedFallback;
                    parent_s.skip(); 
                    parent_s.skip(); 
                    parent_s.readFunctionFrameArgs(argOp, nullptr, nullptr,
                                                   nformal, nactual, it.script(),
                                                   fallback);
                } else {
                    
                    
                    Value *argv = frame_->actualArgs();
                    for (unsigned i = nformal; i < nactual; i++)
                        argOp(argv[i]);
                }
            }
        }

        
        
        for (unsigned i = 0; i < script()->nfixed(); i++)
            localOp(s.maybeRead(fallback));
    }

    template <class Op>
    void unaliasedForEachActual(JSContext *cx, Op op,
                                ReadFrameArgsBehavior behavior,
                                MaybeReadFallback &fallback) const
    {
        Nop nop;
        readFrameArgsAndLocals(cx, op, nop, nullptr, nullptr, nullptr,
                               nullptr, nullptr, behavior, fallback);
    }

    JSScript *script() const {
        return script_;
    }
    jsbytecode *pc() const {
        return pc_;
    }
    SnapshotIterator snapshotIterator() const {
        return si_;
    }
    bool isFunctionFrame() const;
    bool isConstructing() const;

    JSObject *scopeChain(MaybeReadFallback &fallback) const {
        SnapshotIterator s(si_);

        
        Value v = s.maybeRead(fallback);
        return computeScopeChain(v, fallback);
    }

    Value thisValue(MaybeReadFallback &fallback) const {
        
        SnapshotIterator s(si_);

        
        s.skip();

        
        s.skip();

        
        if (script()->argumentsHasVarBinding())
            s.skip();

        return s.maybeRead(fallback);
    }

    InlineFrameIterator &operator++() {
        findNextFrame();
        return *this;
    }

    void dump() const;

    void resetOn(const JitFrameIterator *iter);

    const JitFrameIterator &frame() const {
        return *frame_;
    }

    
    size_t frameNo() const {
        return frameCount() - framesRead_;
    }
    size_t frameCount() const {
        MOZ_ASSERT(frameCount_ != UINT32_MAX);
        return frameCount_;
    }

  private:
    InlineFrameIterator() = delete;
    InlineFrameIterator(const InlineFrameIterator &iter) = delete;
};

} 
} 

#endif 
