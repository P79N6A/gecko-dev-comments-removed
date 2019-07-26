





#ifndef jit_JitFrameIterator_h
#define jit_JitFrameIterator_h

#ifdef JS_ION

#include "jsfun.h"
#include "jsscript.h"
#include "jstypes.h"

#include "jit/IonCode.h"
#include "jit/Snapshots.h"

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

    
    
    
    JitFrame_Unwound_IonJS,

    
    JitFrame_Unwound_BaselineStub,

    
    
    JitFrame_Unwound_Rectifier,

    
    
    
    JitFrame_Exit
};

enum ReadFrameArgsBehavior {
    
    ReadFrame_Formals,

    
    ReadFrame_Overflown,

    
    ReadFrame_Actuals
};

class IonCommonFrameLayout;
class IonJSFrameLayout;
class IonExitFrameLayout;

class BaselineFrame;

class JitActivation;

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
    ExecutionMode mode_;

    void dumpBaseline() const;

  public:
    explicit JitFrameIterator(uint8_t *top, ExecutionMode mode)
      : current_(top),
        type_(JitFrame_Exit),
        returnAddressToFp_(nullptr),
        frameSize_(0),
        cachedSafepointIndex_(nullptr),
        activation_(nullptr),
        mode_(mode)
    { }

    explicit JitFrameIterator(ThreadSafeContext *cx);
    explicit JitFrameIterator(const ActivationIterator &activations);
    explicit JitFrameIterator(IonJSFrameLayout *fp, ExecutionMode mode);

    
    FrameType type() const {
        return type_;
    }
    uint8_t *fp() const {
        return current_;
    }
    const JitActivation *activation() const {
        return activation_;
    }

    IonCommonFrameLayout *current() const {
        return (IonCommonFrameLayout *)current_;
    }

    inline uint8_t *returnAddress() const;

    IonJSFrameLayout *jsFrame() const {
        JS_ASSERT(isScripted());
        return (IonJSFrameLayout *) fp();
    }

    
    inline bool isFakeExitFrame() const;

    inline IonExitFrameLayout *exitFrame() const;

    
    
    bool checkInvalidation(IonScript **ionScript) const;
    bool checkInvalidation() const;

    bool isScripted() const {
        return type_ == JitFrame_BaselineJS || type_ == JitFrame_IonJS;
    }
    bool isBaselineJS() const {
        return type_ == JitFrame_BaselineJS;
    }
    bool isIonJS() const {
        return type_ == JitFrame_IonJS;
    }
    bool isBaselineStub() const {
        return type_ == JitFrame_BaselineStub;
    }
    bool isNative() const;
    bool isOOLNative() const;
    bool isOOLPropertyOp() const;
    bool isOOLProxy() const;
    bool isDOMExit() const;
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
        JS_ASSERT(type_ != JitFrame_Exit);
        return frameSize_;
    }

    
    
    inline bool done() const {
        return type_ == JitFrame_Entry;
    }
    JitFrameIterator &operator++();

    
    IonScript *ionScript() const;

    
    
    const SafepointIndex *safepoint() const;

    
    
    const OsiIndex *osiIndex() const;

    uintptr_t *spillBase() const;
    MachineState machineState() const;

    template <class Op>
    void unaliasedForEachActual(Op op, ReadFrameArgsBehavior behavior) const {
        JS_ASSERT(isBaselineJS());

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
};

class IonJSFrameLayout;
class IonBailoutIterator;

class RResumePoint;



class SnapshotIterator
{
    SnapshotReader snapshot_;
    RecoverReader recover_;
    IonJSFrameLayout *fp_;
    MachineState machine_;
    IonScript *ionScript_;
    AutoValueVector *instructionResults_;

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
    Value fromInstructionResult(uint32_t index) const;

    Value allocationValue(const RValueAllocation &a);
    bool allocationReadable(const RValueAllocation &a);
    void warnUnreadableAllocation();

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

    
    
    
    
    bool initIntructionResults(AutoValueVector &results);

    void storeInstructionResult(Value v);

  public:
    
    void nextFrame();
    void settleOnFrame();

    inline bool moreFrames() const {
        
        
        return moreInstructions();
    }

  public:
    
    

    SnapshotIterator(IonScript *ionScript, SnapshotOffset snapshotOffset,
                     IonJSFrameLayout *fp, const MachineState &machine);
    SnapshotIterator(const JitFrameIterator &iter);
    SnapshotIterator(const IonBailoutIterator &iter);
    SnapshotIterator();

    Value read() {
        return allocationValue(readAllocation());
    }
    Value maybeRead(bool silentFailure = false) {
        RValueAllocation a = readAllocation();
        if (allocationReadable(a))
            return allocationValue(a);
        if (!silentFailure)
            warnUnreadableAllocation();
        return UndefinedValue();
    }

    void readCommonFrameSlots(Value *scopeChain, Value *rval) {
        if (scopeChain)
            *scopeChain = read();
        else
            skip();

        if (rval)
            *rval = read();
        else
            skip();
    }

    template <class Op>
    void readFunctionFrameArgs(Op &op, ArgumentsObject **argsObj, Value *thisv,
                               unsigned start, unsigned end, JSScript *script)
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
            *thisv = read();
        else
            skip();

        unsigned i = 0;
        if (end < start)
            i = start;

        for (; i < start; i++)
            skip();
        for (; i < end; i++) {
            
            
            
            Value v = maybeRead();
            op(v);
        }
    }

    Value maybeReadAllocByIndex(size_t index) {
        while (index--) {
            JS_ASSERT(moreAllocations());
            skip();
        }

        Value s = maybeRead(true);

        while (moreAllocations())
            skip();

        return s;
    }
};



template <AllowGC allowGC=CanGC>
class InlineFrameIteratorMaybeGC
{
    const JitFrameIterator *frame_;
    SnapshotIterator start_;
    SnapshotIterator si_;
    uint32_t framesRead_;

    
    
    
    
    uint32_t frameCount_;

    typename MaybeRooted<JSFunction*, allowGC>::RootType callee_;
    typename MaybeRooted<JSScript*, allowGC>::RootType script_;
    jsbytecode *pc_;
    uint32_t numActualArgs_;

    struct Nop {
        void operator()(const Value &v) { }
    };

  private:
    void findNextFrame();

    JSObject *computeScopeChain(Value scopeChainValue) const {
        if (scopeChainValue.isObject())
            return &scopeChainValue.toObject();

        if (isFunctionFrame()) {
            
            MOZ_ASSERT(!callee()->isHeavyweight());
            return callee()->environment();
        }

        
        MOZ_ASSERT(!script()->isForEval());
        MOZ_ASSERT(script()->compileAndGo());
        return &script()->global();
    }

  public:
    InlineFrameIteratorMaybeGC(ThreadSafeContext *cx, const JitFrameIterator *iter)
      : callee_(cx),
        script_(cx)
    {
        resetOn(iter);
    }

    InlineFrameIteratorMaybeGC(JSRuntime *rt, const JitFrameIterator *iter)
      : callee_(rt),
        script_(rt)
    {
        resetOn(iter);
    }

    InlineFrameIteratorMaybeGC(ThreadSafeContext *cx, const IonBailoutIterator *iter);

    InlineFrameIteratorMaybeGC(ThreadSafeContext *cx, const InlineFrameIteratorMaybeGC *iter)
      : frame_(iter ? iter->frame_ : nullptr),
        framesRead_(0),
        frameCount_(iter ? iter->frameCount_ : UINT32_MAX),
        callee_(cx),
        script_(cx)
    {
        if (frame_) {
            start_ = SnapshotIterator(*frame_);
            
            
            framesRead_ = iter->framesRead_ - 1;
            findNextFrame();
        }
    }

    bool more() const {
        return frame_ && framesRead_ < frameCount_;
    }
    JSFunction *callee() const {
        JS_ASSERT(callee_);
        return callee_;
    }
    JSFunction *maybeCallee() const {
        return callee_;
    }

    unsigned numActualArgs() const {
        
        
        
        
        
        if (more())
            return numActualArgs_;

        return frame_->numActualArgs();
    }

    template <class ArgOp, class LocalOp>
    void readFrameArgsAndLocals(ThreadSafeContext *cx, ArgOp &argOp, LocalOp &localOp,
                                JSObject **scopeChain, Value *rval,
                                ArgumentsObject **argsObj, Value *thisv,
                                ReadFrameArgsBehavior behavior) const
    {
        SnapshotIterator s(si_);

        
        Value scopeChainValue;
        s.readCommonFrameSlots(&scopeChainValue, rval);

        if (scopeChain)
            *scopeChain = computeScopeChain(scopeChainValue);

        
        if (isFunctionFrame()) {
            unsigned nactual = numActualArgs();
            unsigned nformal = callee()->nargs();

            
            
            
            if (behavior != ReadFrame_Overflown)
                s.readFunctionFrameArgs(argOp, argsObj, thisv, 0, nformal, script());

            if (behavior != ReadFrame_Formals) {
                if (more()) {
                    
                    
                    
                    

                    
                    
                    
                    InlineFrameIteratorMaybeGC it(cx, this);
                    ++it;
                    unsigned argsObjAdj = it.script()->argumentsHasVarBinding() ? 1 : 0;
                    SnapshotIterator parent_s(it.snapshotIterator());

                    
                    
                    
                    JS_ASSERT(parent_s.numAllocations() >= nactual + 3 + argsObjAdj);
                    unsigned skip = parent_s.numAllocations() - nactual - 3 - argsObjAdj;
                    for (unsigned j = 0; j < skip; j++)
                        parent_s.skip();

                    
                    parent_s.readCommonFrameSlots(nullptr, nullptr);
                    parent_s.readFunctionFrameArgs(argOp, nullptr, nullptr,
                                                   nformal, nactual, it.script());
                } else {
                    
                    
                    Value *argv = frame_->actualArgs();
                    for (unsigned i = nformal; i < nactual; i++)
                        argOp(argv[i]);
                }
            }
        }

        
        
        for (unsigned i = 0; i < script()->nfixed(); i++)
            localOp(s.read());
    }

    template <class Op>
    void unaliasedForEachActual(ThreadSafeContext *cx, Op op,
                                ReadFrameArgsBehavior behavior) const
    {
        Nop nop;
        readFrameArgsAndLocals(cx, op, nop, nullptr, nullptr, nullptr, nullptr, behavior);
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

    JSObject *scopeChain() const {
        SnapshotIterator s(si_);

        
        Value v = s.read();
        return computeScopeChain(v);
    }

    JSObject *thisObject() const {
        
        
        Value v = thisValue();
        JS_ASSERT(v.isObject());
        return &v.toObject();
    }

    Value thisValue() const {
        
        SnapshotIterator s(si_);

        
        s.skip();

        
        s.skip();

        
        if (script()->argumentsHasVarBinding())
            s.skip();

        return s.read();
    }

    InlineFrameIteratorMaybeGC &operator++() {
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
    InlineFrameIteratorMaybeGC() MOZ_DELETE;
    InlineFrameIteratorMaybeGC(const InlineFrameIteratorMaybeGC &iter) MOZ_DELETE;
};
typedef InlineFrameIteratorMaybeGC<CanGC> InlineFrameIterator;
typedef InlineFrameIteratorMaybeGC<NoGC> InlineFrameIteratorNoGC;

} 
} 

#endif 

#endif 
