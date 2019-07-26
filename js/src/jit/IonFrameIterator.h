





#ifndef jit_IonFrameIterator_h
#define jit_IonFrameIterator_h

#ifdef JS_ION

#include "jsscript.h"
#include "jstypes.h"

#include "jit/IonCode.h"
#include "jit/SnapshotReader.h"

namespace js {
    class ActivationIterator;
};

namespace js {
namespace jit {

enum FrameType
{
    
    
    IonFrame_OptimizedJS,

    
    IonFrame_BaselineJS,

    
    
    IonFrame_BaselineStub,

    
    
    IonFrame_Entry,

    
    
    IonFrame_Rectifier,

    
    
    
    IonFrame_Unwound_OptimizedJS,

    
    IonFrame_Unwound_BaselineStub,

    
    
    IonFrame_Unwound_Rectifier,

    
    
    
    IonFrame_Exit,

    
    
    
    IonFrame_Osr
};

class IonCommonFrameLayout;
class IonJSFrameLayout;
class IonExitFrameLayout;

class BaselineFrame;

class JitActivation;

class IonFrameIterator
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
    IonFrameIterator(uint8_t *top)
      : current_(top),
        type_(IonFrame_Exit),
        returnAddressToFp_(nullptr),
        frameSize_(0),
        cachedSafepointIndex_(nullptr),
        activation_(nullptr)
    { }

    IonFrameIterator(const ActivationIterator &activations);
    IonFrameIterator(IonJSFrameLayout *fp);

    
    FrameType type() const {
        return type_;
    }
    uint8_t *fp() const {
        return current_;
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
        return type_ == IonFrame_BaselineJS || type_ == IonFrame_OptimizedJS;
    }
    bool isBaselineJS() const {
        return type_ == IonFrame_BaselineJS;
    }
    bool isOptimizedJS() const {
        return type_ == IonFrame_OptimizedJS;
    }
    bool isBaselineStub() const {
        return type_ == IonFrame_BaselineStub;
    }
    bool isNative() const;
    bool isOOLNative() const;
    bool isOOLPropertyOp() const;
    bool isOOLProxy() const;
    bool isDOMExit() const;
    bool isEntry() const {
        return type_ == IonFrame_Entry;
    }
    bool isFunctionFrame() const;
    bool isParallelFunctionFrame() const;

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
        JS_ASSERT(type_ != IonFrame_Exit);
        return frameSize_;
    }

    
    
    inline bool done() const {
        return type_ == IonFrame_Entry;
    }
    IonFrameIterator &operator++();

    
    IonScript *ionScript() const;

    
    
    const SafepointIndex *safepoint() const;

    
    
    const OsiIndex *osiIndex() const;

    uintptr_t *spillBase() const;
    MachineState machineState() const;

    template <class Op>
    void forEachCanonicalActualArg(Op op, unsigned start, unsigned count) const {
        JS_ASSERT(isBaselineJS());

        unsigned nactual = numActualArgs();
        if (count == unsigned(-1))
            count = nactual - start;

        unsigned end = start + count;
        JS_ASSERT(start <= end && end <= nactual);

        Value *argv = actualArgs();
        for (unsigned i = start; i < end; i++)
            op(argv[i]);
    }

    void dump() const;

    inline BaselineFrame *baselineFrame() const;
};

class IonJSFrameLayout;
class IonBailoutIterator;



class SnapshotIterator : public SnapshotReader
{
    IonJSFrameLayout *fp_;
    MachineState machine_;
    IonScript *ionScript_;

  private:
    bool hasLocation(const SnapshotReader::Location &loc);
    uintptr_t fromLocation(const SnapshotReader::Location &loc);
    static Value FromTypedPayload(JSValueType type, uintptr_t payload);

    Value slotValue(const Slot &slot);
    bool slotReadable(const Slot &slot);
    void warnUnreadableSlot();

  public:
    SnapshotIterator(IonScript *ionScript, SnapshotOffset snapshotOffset,
                     IonJSFrameLayout *fp, const MachineState &machine);
    SnapshotIterator(const IonFrameIterator &iter);
    SnapshotIterator(const IonBailoutIterator &iter);
    SnapshotIterator();

    Value read() {
        return slotValue(readSlot());
    }
    Value maybeRead(bool silentFailure = false) {
        Slot s = readSlot();
        if (slotReadable(s))
            return slotValue(s);
        if (!silentFailure)
            warnUnreadableSlot();
        return UndefinedValue();
    }

    template <class Op>
    void readFrameArgs(Op &op, const Value *argv, Value *scopeChain, Value *thisv,
                       unsigned start, unsigned formalEnd, unsigned iterEnd, JSScript *script)
    {
        if (scopeChain)
            *scopeChain = read();
        else
            skip();

        
        if (script->argumentsHasVarBinding())
            skip();

        if (thisv)
            *thisv = read();
        else
            skip();

        unsigned i = 0;
        if (formalEnd < start)
            i = start;

        for (; i < start; i++)
            skip();
        for (; i < formalEnd && i < iterEnd; i++) {
            
            
            
            Value v = maybeRead();
            op(v);
        }
        if (iterEnd >= formalEnd) {
            for (; i < iterEnd; i++)
                op(argv[i]);
        }
    }

    Value maybeReadSlotByIndex(size_t index) {
        while (index--) {
            JS_ASSERT(moreSlots());
            skip();
        }

        Value s = maybeRead(true);

        while (moreSlots())
            skip();

        return s;
    }
};



template <AllowGC allowGC=CanGC>
class InlineFrameIteratorMaybeGC
{
    const IonFrameIterator *frame_;
    SnapshotIterator start_;
    SnapshotIterator si_;
    unsigned framesRead_;
    typename MaybeRooted<JSFunction*, allowGC>::RootType callee_;
    typename MaybeRooted<JSScript*, allowGC>::RootType script_;
    jsbytecode *pc_;
    uint32_t numActualArgs_;

  private:
    void findNextFrame();

  public:
    InlineFrameIteratorMaybeGC(JSContext *cx, const IonFrameIterator *iter)
      : callee_(cx),
        script_(cx)
    {
        resetOn(iter);
    }

    InlineFrameIteratorMaybeGC(JSRuntime *rt, const IonFrameIterator *iter)
      : callee_(rt),
        script_(rt)
    {
        resetOn(iter);
    }

    InlineFrameIteratorMaybeGC(JSContext *cx, const IonBailoutIterator *iter);

    InlineFrameIteratorMaybeGC(JSContext *cx, const InlineFrameIteratorMaybeGC *iter)
      : frame_(iter ? iter->frame_ : nullptr),
        framesRead_(0),
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
        return frame_ && framesRead_ < start_.frameCount();
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

    template <class Op>
    void forEachCanonicalActualArg(JSContext *cx, Op op, unsigned start, unsigned count) const {
        unsigned nactual = numActualArgs();
        if (count == unsigned(-1))
            count = nactual - start;

        unsigned end = start + count;
        unsigned nformal = callee()->nargs;

        JS_ASSERT(start <= end && end <= nactual);

        if (more()) {
            
            
            
            
            

            
            unsigned formal_end = (end < nformal) ? end : nformal;
            SnapshotIterator s(si_);
            s.readFrameArgs(op, nullptr, nullptr, nullptr, start, nformal, formal_end, script());

            
            
            InlineFrameIteratorMaybeGC it(cx, this);
            ++it;
            unsigned argsObjAdj = it.script()->argumentsHasVarBinding() ? 1 : 0;
            SnapshotIterator parent_s(it.snapshotIterator());

            
            
            JS_ASSERT(parent_s.slots() >= nactual + 2 + argsObjAdj);
            unsigned skip = parent_s.slots() - nactual - 2 - argsObjAdj;
            for (unsigned j = 0; j < skip; j++)
                parent_s.skip();

            
            parent_s.readFrameArgs(op, nullptr, nullptr, nullptr, nformal, nactual, end, it.script());
        } else {
            SnapshotIterator s(si_);
            Value *argv = frame_->actualArgs();
            s.readFrameArgs(op, argv, nullptr, nullptr, start, nformal, end, script());
        }
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
        if (v.isObject()) {
            JS_ASSERT_IF(script()->hasAnalysis(), script()->analysis()->usesScopeChain());
            return &v.toObject();
        }

        return callee()->environment();
    }

    JSObject *thisObject() const {
        
        SnapshotIterator s(si_);

        
        s.skip();

        
        if (script()->argumentsHasVarBinding())
            s.skip();

        
        
        Value v = s.read();
        JS_ASSERT(v.isObject());
        return &v.toObject();
    }

    InlineFrameIteratorMaybeGC &operator++() {
        findNextFrame();
        return *this;
    }

    void dump() const;

    void resetOn(const IonFrameIterator *iter);

    const IonFrameIterator &frame() const {
        return *frame_;
    }

    
    size_t frameNo() const {
        return start_.frameCount() - framesRead_;
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
