





#ifndef vm_Stack_h
#define vm_Stack_h

#include "mozilla/MemoryReporting.h"

#include "jsfun.h"
#include "jsscript.h"

#include "jit/IonFrameIterator.h"
#ifdef CHECK_OSIPOINT_REGISTERS
#include "jit/Registers.h" 
#endif
#include "js/OldDebugAPI.h"

struct JSCompartment;
struct JSGenerator;

namespace js {

class ArgumentsObject;
class AsmJSModule;
class InterpreterRegs;
class ScopeObject;
class ScriptFrameIter;
class SPSProfiler;
class StackFrame;
class StaticBlockObject;

struct ScopeCoordinate;































enum MaybeCheckAliasing { CHECK_ALIASING = true, DONT_CHECK_ALIASING = false };



#ifdef DEBUG
extern void
CheckLocalUnaliased(MaybeCheckAliasing checkAliasing, JSScript *script, uint32_t i);
#endif

namespace jit {
    class BaselineFrame;
}




















class AbstractFramePtr
{
    friend class ScriptFrameIter;

    uintptr_t ptr_;

    enum {
        Tag_ScriptFrameIterData = 0x0,
        Tag_StackFrame = 0x1,
        Tag_BaselineFrame = 0x2,
        TagMask = 0x3
    };

  public:
    AbstractFramePtr()
      : ptr_(0)
    {}

    AbstractFramePtr(StackFrame *fp)
      : ptr_(fp ? uintptr_t(fp) | Tag_StackFrame : 0)
    {
        MOZ_ASSERT(asStackFrame() == fp);
    }

    AbstractFramePtr(jit::BaselineFrame *fp)
      : ptr_(fp ? uintptr_t(fp) | Tag_BaselineFrame : 0)
    {
        MOZ_ASSERT(asBaselineFrame() == fp);
    }

    explicit AbstractFramePtr(JSAbstractFramePtr frame)
        : ptr_(uintptr_t(frame.raw()))
    {
    }

    static AbstractFramePtr FromRaw(void *raw) {
        AbstractFramePtr frame;
        frame.ptr_ = uintptr_t(raw);
        return frame;
    }

    bool isScriptFrameIterData() const {
        return !!ptr_ && (ptr_ & TagMask) == Tag_ScriptFrameIterData;
    }
    bool isStackFrame() const {
        return ptr_ & Tag_StackFrame;
    }
    StackFrame *asStackFrame() const {
        JS_ASSERT(isStackFrame());
        StackFrame *res = (StackFrame *)(ptr_ & ~TagMask);
        JS_ASSERT(res);
        return res;
    }
    bool isBaselineFrame() const {
        return ptr_ & Tag_BaselineFrame;
    }
    jit::BaselineFrame *asBaselineFrame() const {
        JS_ASSERT(isBaselineFrame());
        jit::BaselineFrame *res = (jit::BaselineFrame *)(ptr_ & ~TagMask);
        JS_ASSERT(res);
        return res;
    }

    void *raw() const { return reinterpret_cast<void *>(ptr_); }

    bool operator ==(const AbstractFramePtr &other) const { return ptr_ == other.ptr_; }
    bool operator !=(const AbstractFramePtr &other) const { return ptr_ != other.ptr_; }

    operator bool() const { return !!ptr_; }

    inline JSObject *scopeChain() const;
    inline CallObject &callObj() const;
    inline bool initFunctionScopeObjects(JSContext *cx);
    inline void pushOnScopeChain(ScopeObject &scope);

    inline JSCompartment *compartment() const;

    inline bool hasCallObj() const;
    inline bool isGeneratorFrame() const;
    inline bool isYielding() const;
    inline bool isFunctionFrame() const;
    inline bool isGlobalFrame() const;
    inline bool isEvalFrame() const;
    inline bool isFramePushedByExecute() const;
    inline bool isDebuggerFrame() const;

    inline JSScript *script() const;
    inline JSFunction *fun() const;
    inline JSFunction *maybeFun() const;
    inline JSFunction *callee() const;
    inline Value calleev() const;
    inline Value &thisValue() const;

    inline bool isNonEvalFunctionFrame() const;
    inline bool isNonStrictDirectEvalFrame() const;
    inline bool isStrictEvalFrame() const;

    inline unsigned numActualArgs() const;
    inline unsigned numFormalArgs() const;

    inline Value *argv() const;

    inline bool hasArgs() const;
    inline bool hasArgsObj() const;
    inline ArgumentsObject &argsObj() const;
    inline void initArgsObj(ArgumentsObject &argsobj) const;
    inline bool useNewType() const;

    inline bool copyRawFrameSlots(AutoValueVector *vec) const;

    inline Value &unaliasedVar(uint32_t i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING);
    inline Value &unaliasedLocal(uint32_t i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING);
    inline Value &unaliasedFormal(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING);
    inline Value &unaliasedActual(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING);
    template <class Op> inline void unaliasedForEachActual(JSContext *cx, Op op);

    inline bool prevUpToDate() const;
    inline void setPrevUpToDate() const;

    JSObject *evalPrevScopeChain(JSContext *cx) const;

    inline void *maybeHookData() const;
    inline void setHookData(void *data) const;
    inline HandleValue returnValue() const;
    inline void setReturnValue(const Value &rval) const;

    bool hasPushedSPSFrame() const;

    inline void popBlock(JSContext *cx) const;
    inline void popWith(JSContext *cx) const;
};

class NullFramePtr : public AbstractFramePtr
{
  public:
    NullFramePtr()
      : AbstractFramePtr()
    { }
};




enum InitialFrameFlags {
    INITIAL_NONE           =          0,
    INITIAL_CONSTRUCT      =       0x20, 
};

enum ExecuteType {
    EXECUTE_GLOBAL         =        0x1, 
    EXECUTE_DIRECT_EVAL    =        0x4, 
    EXECUTE_INDIRECT_EVAL  =        0x5, 
    EXECUTE_DEBUG          =        0xc, 
    EXECUTE_DEBUG_GLOBAL   =        0xd  
};



class StackFrame
{
  public:
    enum Flags {
        
        GLOBAL             =        0x1,  
        FUNCTION           =        0x2,  

        
        EVAL               =        0x4,  


        











        DEBUGGER           =        0x8,

        GENERATOR          =       0x10,  
        CONSTRUCTING       =       0x20,  

        








        YIELDING           =       0x40,  
        SUSPENDED          =       0x80,  

        
        HAS_CALL_OBJ       =      0x100,  
        HAS_ARGS_OBJ       =      0x200,  

        
        HAS_HOOK_DATA      =      0x400,  
        HAS_RVAL           =      0x800,  
        HAS_SCOPECHAIN     =     0x1000,  

        
        PREV_UP_TO_DATE    =     0x4000,  

        
        HAS_PUSHED_SPS_FRAME =   0x8000,  

        



        RUNNING_IN_JIT     =    0x10000,

        
        USE_NEW_TYPE       =    0x20000   
    };

  private:
    mutable uint32_t    flags_;         
    union {                             
        JSScript        *script;        
        JSFunction      *fun;           
    } exec;
    union {                             
        unsigned        nactual;        
        JSScript        *evalScript;    
    } u;
    mutable JSObject    *scopeChain_;   
    Value               rval_;          
    ArgumentsObject     *argsObj_;      

    




    StackFrame          *prev_;
    jsbytecode          *prevpc_;
    Value               *prevsp_;

    void                *hookData_;     

    



    AbstractFramePtr    evalInFramePrev_;

    Value               *argv_;         
    LifoAlloc::Mark     mark_;          

    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(StackFrame, rval_) % sizeof(Value) == 0);
        JS_STATIC_ASSERT(sizeof(StackFrame) % sizeof(Value) == 0);
    }

    void writeBarrierPost();

    





    Value *slots() const { return (Value *)(this + 1); }
    Value *base() const { return slots() + script()->nfixed(); }

    friend class InterpreterRegs;
    friend class InterpreterStack;
    friend class ScriptFrameIter;
    friend class jit::BaselineFrame;

    




    
    void initCallFrame(JSContext *cx, StackFrame *prev, jsbytecode *prevpc, Value *prevsp, JSFunction &callee,
                       JSScript *script, Value *argv, uint32_t nactual, StackFrame::Flags flags);

    
    void initExecuteFrame(JSContext *cx, JSScript *script, AbstractFramePtr prev,
                          const Value &thisv, JSObject &scopeChain, ExecuteType type);

  public:
    


















    bool prologue(JSContext *cx);
    void epilogue(JSContext *cx);

    bool initFunctionScopeObjects(JSContext *cx);

    
    void initVarsToUndefined();

    









    bool isFunctionFrame() const {
        return !!(flags_ & FUNCTION);
    }

    bool isGlobalFrame() const {
        return !!(flags_ & GLOBAL);
    }

    











    bool isEvalFrame() const {
        return flags_ & EVAL;
    }

    bool isEvalInFunction() const {
        return (flags_ & (EVAL | FUNCTION)) == (EVAL | FUNCTION);
    }

    bool isNonEvalFunctionFrame() const {
        return (flags_ & (FUNCTION | EVAL)) == FUNCTION;
    }

    inline bool isStrictEvalFrame() const {
        return isEvalFrame() && script()->strict();
    }

    bool isNonStrictEvalFrame() const {
        return isEvalFrame() && !script()->strict();
    }

    bool isDirectEvalFrame() const {
        return isEvalFrame() && script()->staticLevel() > 0;
    }

    bool isNonStrictDirectEvalFrame() const {
        return isNonStrictEvalFrame() && isDirectEvalFrame();
    }

    












    StackFrame *prev() const {
        return prev_;
    }

    AbstractFramePtr evalInFramePrev() const {
        JS_ASSERT(isEvalFrame());
        return evalInFramePrev_;
    }

    

















    inline Value &unaliasedVar(uint32_t i, MaybeCheckAliasing = CHECK_ALIASING);
    inline Value &unaliasedLocal(uint32_t i, MaybeCheckAliasing = CHECK_ALIASING);

    bool hasArgs() const { return isNonEvalFunctionFrame(); }
    inline Value &unaliasedFormal(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    inline Value &unaliasedActual(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    template <class Op> inline void unaliasedForEachActual(Op op);

    bool copyRawFrameSlots(AutoValueVector *v);

    unsigned numFormalArgs() const { JS_ASSERT(hasArgs()); return fun()->nargs(); }
    unsigned numActualArgs() const { JS_ASSERT(hasArgs()); return u.nactual; }

    
    Value *argv() const { return argv_; }

    










    ArgumentsObject &argsObj() const;
    void initArgsObj(ArgumentsObject &argsobj);

    JSObject *createRestParameter(JSContext *cx);

    


















    inline HandleObject scopeChain() const;

    inline ScopeObject &aliasedVarScope(ScopeCoordinate sc) const;
    inline GlobalObject &global() const;
    inline CallObject &callObj() const;
    inline JSObject &varObj();

    inline void pushOnScopeChain(ScopeObject &scope);
    inline void popOffScopeChain();

    




    bool pushBlock(JSContext *cx, StaticBlockObject &block);
    void popBlock(JSContext *cx);

    






    void popWith(JSContext *cx);

    














    JSScript *script() const {
        return isFunctionFrame()
               ? isEvalFrame()
                 ? u.evalScript
                 : fun()->nonLazyScript()
               : exec.script;
    }

    
    jsbytecode *prevpc() {
        JS_ASSERT(prev_);
        return prevpc_;
    }

    
    Value *prevsp() {
        JS_ASSERT(prev_);
        return prevsp_;
    }

    








    JSFunction* fun() const {
        JS_ASSERT(isFunctionFrame());
        return exec.fun;
    }

    JSFunction* maybeFun() const {
        return isFunctionFrame() ? fun() : nullptr;
    }

    












    Value &functionThis() const {
        JS_ASSERT(isFunctionFrame());
        if (isEvalFrame())
            return ((Value *)this)[-1];
        return argv()[-1];
    }

    JSObject &constructorThis() const {
        JS_ASSERT(hasArgs());
        return argv()[-1].toObject();
    }

    Value &thisValue() const {
        if (flags_ & (EVAL | GLOBAL))
            return ((Value *)this)[-1];
        return argv()[-1];
    }

    








    JSFunction &callee() const {
        JS_ASSERT(isFunctionFrame());
        return calleev().toObject().as<JSFunction>();
    }

    const Value &calleev() const {
        JS_ASSERT(isFunctionFrame());
        return mutableCalleev();
    }

    const Value &maybeCalleev() const {
        Value &calleev = flags_ & (EVAL | GLOBAL)
                         ? ((Value *)this)[-2]
                         : argv()[-2];
        JS_ASSERT(calleev.isObjectOrNull());
        return calleev;
    }

    Value &mutableCalleev() const {
        JS_ASSERT(isFunctionFrame());
        if (isEvalFrame())
            return ((Value *)this)[-2];
        return argv()[-2];
    }

    CallReceiver callReceiver() const {
        return CallReceiverFromArgv(argv());
    }

    






    inline JSCompartment *compartment() const;

    

    bool hasHookData() const {
        return !!(flags_ & HAS_HOOK_DATA);
    }

    void* hookData() const {
        JS_ASSERT(hasHookData());
        return hookData_;
    }

    void* maybeHookData() const {
        return hasHookData() ? hookData_ : nullptr;
    }

    void setHookData(void *v) {
        hookData_ = v;
        flags_ |= HAS_HOOK_DATA;
    }

    bool hasPushedSPSFrame() {
        return !!(flags_ & HAS_PUSHED_SPS_FRAME);
    }

    void setPushedSPSFrame() {
        flags_ |= HAS_PUSHED_SPS_FRAME;
    }

    void unsetPushedSPSFrame() {
        flags_ &= ~HAS_PUSHED_SPS_FRAME;
    }

    

    bool hasReturnValue() const {
        return !!(flags_ & HAS_RVAL);
    }

    MutableHandleValue returnValue() {
        if (!(flags_ & HAS_RVAL))
            rval_.setUndefined();
        return MutableHandleValue::fromMarkedLocation(&rval_);
    }

    void markReturnValue() {
        flags_ |= HAS_RVAL;
    }

    void setReturnValue(const Value &v) {
        rval_ = v;
        markReturnValue();
    }

    void clearReturnValue() {
        rval_.setUndefined();
        markReturnValue();
    }

    










    bool isGeneratorFrame() const {
        bool ret = flags_ & GENERATOR;
        JS_ASSERT_IF(ret, isNonEvalFunctionFrame());
        return ret;
    }

    void initGeneratorFrame() const {
        JS_ASSERT(!isGeneratorFrame());
        JS_ASSERT(isNonEvalFunctionFrame());
        flags_ |= GENERATOR;
    }

    Value *generatorArgsSnapshotBegin() const {
        JS_ASSERT(isGeneratorFrame());
        return argv() - 2;
    }

    Value *generatorArgsSnapshotEnd() const {
        JS_ASSERT(isGeneratorFrame());
        return argv() + js::Max(numActualArgs(), numFormalArgs());
    }

    Value *generatorSlotsSnapshotBegin() const {
        JS_ASSERT(isGeneratorFrame());
        return (Value *)(this + 1);
    }

    enum TriggerPostBarriers {
        DoPostBarrier = true,
        NoPostBarrier = false
    };
    template <TriggerPostBarriers doPostBarrier>
    void copyFrameAndValues(JSContext *cx, Value *vp, StackFrame *otherfp,
                            const Value *othervp, Value *othersp);

    






    bool isFramePushedByExecute() const {
        return !!(flags_ & (GLOBAL | EVAL));
    }

    



    InitialFrameFlags initialFlags() const {
        JS_STATIC_ASSERT((int)INITIAL_NONE == 0);
        JS_STATIC_ASSERT((int)INITIAL_CONSTRUCT == (int)CONSTRUCTING);
        uint32_t mask = CONSTRUCTING;
        JS_ASSERT((flags_ & mask) != mask);
        return InitialFrameFlags(flags_ & mask);
    }

    void setConstructing() {
        flags_ |= CONSTRUCTING;
    }

    bool isConstructing() const {
        return !!(flags_ & CONSTRUCTING);
    }

    







    inline bool hasCallObj() const;

    bool hasCallObjUnchecked() const {
        return flags_ & HAS_CALL_OBJ;
    }

    bool hasArgsObj() const {
        JS_ASSERT(script()->needsArgsObj());
        return flags_ & HAS_ARGS_OBJ;
    }

    void setUseNewType() {
        JS_ASSERT(isConstructing());
        flags_ |= USE_NEW_TYPE;
    }
    bool useNewType() const {
        JS_ASSERT(isConstructing());
        return flags_ & USE_NEW_TYPE;
    }

    bool isDebuggerFrame() const {
        return !!(flags_ & DEBUGGER);
    }

    bool prevUpToDate() const {
        return !!(flags_ & PREV_UP_TO_DATE);
    }

    void setPrevUpToDate() {
        flags_ |= PREV_UP_TO_DATE;
    }

    bool isYielding() {
        return !!(flags_ & YIELDING);
    }

    void setYielding() {
        flags_ |= YIELDING;
    }

    void clearYielding() {
        flags_ &= ~YIELDING;
    }

    bool isSuspended() const {
        JS_ASSERT(isGeneratorFrame());
        return flags_ & SUSPENDED;
    }

    void setSuspended() {
        JS_ASSERT(isGeneratorFrame());
        flags_ |= SUSPENDED;
    }

    void clearSuspended() {
        JS_ASSERT(isGeneratorFrame());
        flags_ &= ~SUSPENDED;
    }

  public:
    void mark(JSTracer *trc);
    void markValues(JSTracer *trc, unsigned start, unsigned end);
    void markValues(JSTracer *trc, Value *sp, jsbytecode *pc);

    
    bool runningInJit() const {
        return !!(flags_ & RUNNING_IN_JIT);
    }
    void setRunningInJit() {
        flags_ |= RUNNING_IN_JIT;
    }
    void clearRunningInJit() {
        flags_ &= ~RUNNING_IN_JIT;
    }
};

static const size_t VALUES_PER_STACK_FRAME = sizeof(StackFrame) / sizeof(Value);

static inline StackFrame::Flags
ToFrameFlags(InitialFrameFlags initial)
{
    return StackFrame::Flags(initial);
}

static inline InitialFrameFlags
InitialFrameFlagsFromConstructing(bool b)
{
    return b ? INITIAL_CONSTRUCT : INITIAL_NONE;
}

static inline bool
InitialFrameFlagsAreConstructing(InitialFrameFlags initial)
{
    return !!(initial & INITIAL_CONSTRUCT);
}



class InterpreterRegs
{
  public:
    Value *sp;
    jsbytecode *pc;
  private:
    StackFrame *fp_;
  public:
    StackFrame *fp() const { return fp_; }

    unsigned stackDepth() const {
        JS_ASSERT(sp >= fp_->base());
        return sp - fp_->base();
    }

    Value *spForStackDepth(unsigned depth) const {
        JS_ASSERT(fp_->script()->nfixed() + depth <= fp_->script()->nslots());
        return fp_->base() + depth;
    }

    
    void rebaseFromTo(const InterpreterRegs &from, StackFrame &to) {
        fp_ = &to;
        sp = to.slots() + (from.sp - from.fp_->slots());
        pc = from.pc;
        JS_ASSERT(fp_);
    }

    void popInlineFrame() {
        pc = fp_->prevpc();
        sp = fp_->prevsp() - fp_->numActualArgs() - 1;
        fp_ = fp_->prev();
        JS_ASSERT(fp_);
    }
    void prepareToRun(StackFrame &fp, JSScript *script) {
        pc = script->code();
        sp = fp.slots() + script->nfixed();
        fp_ = &fp;
    }

    void setToEndOfScript();

    MutableHandleValue stackHandleAt(int i) {
        return MutableHandleValue::fromMarkedLocation(&sp[i]);
    }

    HandleValue stackHandleAt(int i) const {
        return HandleValue::fromMarkedLocation(&sp[i]);
    }
};



class InterpreterStack
{
    friend class InterpreterActivation;

    static const size_t DEFAULT_CHUNK_SIZE = 4 * 1024;
    LifoAlloc allocator_;

    
    static const size_t MAX_FRAMES = 50 * 1000;
    static const size_t MAX_FRAMES_TRUSTED = MAX_FRAMES + 1000;
    size_t frameCount_;

    inline uint8_t *allocateFrame(JSContext *cx, size_t size);

    inline StackFrame *
    getCallFrame(JSContext *cx, const CallArgs &args, HandleScript script,
                 StackFrame::Flags *pflags, Value **pargv);

    void releaseFrame(StackFrame *fp) {
        frameCount_--;
        allocator_.release(fp->mark_);
    }

  public:
    InterpreterStack()
      : allocator_(DEFAULT_CHUNK_SIZE),
        frameCount_(0)
    { }

    ~InterpreterStack() {
        JS_ASSERT(frameCount_ == 0);
    }

    
    StackFrame *pushExecuteFrame(JSContext *cx, HandleScript script, const Value &thisv,
                                 HandleObject scopeChain, ExecuteType type,
                                 AbstractFramePtr evalInFrame);

    
    StackFrame *pushInvokeFrame(JSContext *cx, const CallArgs &args, InitialFrameFlags initial);

    
    
    bool pushInlineFrame(JSContext *cx, InterpreterRegs &regs, const CallArgs &args,
                         HandleScript script, InitialFrameFlags initial);

    void popInlineFrame(InterpreterRegs &regs);

    inline void purge(JSRuntime *rt);

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        return allocator_.sizeOfExcludingThis(mallocSizeOf);
    }
};

void MarkInterpreterActivations(JSRuntime *rt, JSTracer *trc);



class InvokeArgs : public JS::CallArgs
{
    AutoValueVector v_;

  public:
    InvokeArgs(JSContext *cx) : v_(cx) {}

    bool init(unsigned argc) {
        if (!v_.resize(2 + argc))
            return false;
        ImplicitCast<CallArgs>(*this) = CallArgsFromVp(argc, v_.begin());
        return true;
    }
};

template <>
struct DefaultHasher<AbstractFramePtr> {
    typedef AbstractFramePtr Lookup;

    static js::HashNumber hash(const Lookup &key) {
        return size_t(key.raw());
    }

    static bool match(const AbstractFramePtr &k, const Lookup &l) {
        return k == l;
    }
};



class InterpreterActivation;
class ForkJoinActivation;
class AsmJSActivation;

namespace jit {
    class JitActivation;
};

class Activation
{
  protected:
    JSContext *cx_;
    JSCompartment *compartment_;
    Activation *prev_;

    
    
    
    
    size_t savedFrameChain_;

    
    
    
    
    
    size_t hideScriptedCallerCount_;

    enum Kind { Interpreter, Jit, ForkJoin, AsmJS };
    Kind kind_;

    inline Activation(JSContext *cx, Kind kind_);
    inline ~Activation();

  public:
    JSContext *cx() const {
        return cx_;
    }
    JSCompartment *compartment() const {
        return compartment_;
    }
    Activation *prev() const {
        return prev_;
    }

    bool isInterpreter() const {
        return kind_ == Interpreter;
    }
    bool isJit() const {
        return kind_ == Jit;
    }
    bool isForkJoin() const {
        return kind_ == ForkJoin;
    }
    bool isAsmJS() const {
        return kind_ == AsmJS;
    }

    InterpreterActivation *asInterpreter() const {
        JS_ASSERT(isInterpreter());
        return (InterpreterActivation *)this;
    }
    jit::JitActivation *asJit() const {
        JS_ASSERT(isJit());
        return (jit::JitActivation *)this;
    }
    ForkJoinActivation *asForkJoin() const {
        JS_ASSERT(isForkJoin());
        return (ForkJoinActivation *)this;
    }
    AsmJSActivation *asAsmJS() const {
        JS_ASSERT(isAsmJS());
        return (AsmJSActivation *)this;
    }

    void saveFrameChain() {
        savedFrameChain_++;
    }
    void restoreFrameChain() {
        JS_ASSERT(savedFrameChain_ > 0);
        savedFrameChain_--;
    }
    bool hasSavedFrameChain() const {
        return savedFrameChain_ > 0;
    }

    void hideScriptedCaller() {
        hideScriptedCallerCount_++;
    }
    void unhideScriptedCaller() {
        JS_ASSERT(hideScriptedCallerCount_ > 0);
        hideScriptedCallerCount_--;
    }
    bool scriptedCallerIsHidden() const {
        return hideScriptedCallerCount_ > 0;
    }

  private:
    Activation(const Activation &other) MOZ_DELETE;
    void operator=(const Activation &other) MOZ_DELETE;
};




static const jsbytecode EnableInterruptsPseudoOpcode = -1;

static_assert(EnableInterruptsPseudoOpcode >= JSOP_LIMIT,
              "EnableInterruptsPseudoOpcode must be greater than any opcode");
static_assert(EnableInterruptsPseudoOpcode == jsbytecode(-1),
              "EnableInterruptsPseudoOpcode must be the maximum jsbytecode value");

class InterpreterFrameIterator;
class RunState;

class InterpreterActivation : public Activation
{
    friend class js::InterpreterFrameIterator;

    RunState &state_;
    InterpreterRegs regs_;
    StackFrame *entryFrame_;
    size_t opMask_; 

#ifdef DEBUG
    size_t oldFrameCount_;
#endif

  public:
    inline InterpreterActivation(RunState &state, JSContext *cx, StackFrame *entryFrame);
    inline ~InterpreterActivation();

    inline bool pushInlineFrame(const CallArgs &args, HandleScript script,
                                InitialFrameFlags initial);
    inline void popInlineFrame(StackFrame *frame);

    StackFrame *current() const {
        return regs_.fp();
    }
    InterpreterRegs &regs() {
        return regs_;
    }
    StackFrame *entryFrame() const {
        return entryFrame_;
    }
    size_t opMask() const {
        return opMask_;
    }

    
    void enableInterruptsIfRunning(JSScript *script) {
        if (regs_.fp()->script() == script)
            enableInterruptsUnconditionally();
    }
    void enableInterruptsUnconditionally() {
        opMask_ = EnableInterruptsPseudoOpcode;
    }
    void clearInterruptsMask() {
        opMask_ = 0;
    }
};


class ActivationIterator
{
    uint8_t *jitTop_;

  protected:
    Activation *activation_;

  private:
    void settle();

  public:
    explicit ActivationIterator(JSRuntime *rt);

    ActivationIterator &operator++();

    Activation *activation() const {
        return activation_;
    }
    uint8_t *jitTop() const {
        JS_ASSERT(activation_->isJit());
        return jitTop_;
    }
    bool done() const {
        return activation_ == nullptr;
    }
};

namespace jit {


class JitActivation : public Activation
{
    uint8_t *prevIonTop_;
    JSContext *prevJitJSContext_;
    bool firstFrameIsConstructing_;
    bool active_;

#ifdef CHECK_OSIPOINT_REGISTERS
  protected:
    
    
    uint32_t checkRegs_;
    RegisterDump regs_;
#endif

  public:
    JitActivation(JSContext *cx, bool firstFrameIsConstructing, bool active = true);
    ~JitActivation();

    bool isActive() const {
        return active_;
    }
    void setActive(JSContext *cx, bool active = true);

    uint8_t *prevIonTop() const {
        return prevIonTop_;
    }
    JSCompartment *compartment() const {
        return compartment_;
    }
    bool firstFrameIsConstructing() const {
        return firstFrameIsConstructing_;
    }

#ifdef CHECK_OSIPOINT_REGISTERS
    void setCheckRegs(bool check) {
        checkRegs_ = check;
    }
    static size_t offsetOfCheckRegs() {
        return offsetof(JitActivation, checkRegs_);
    }
    static size_t offsetOfRegs() {
        return offsetof(JitActivation, regs_);
    }
#endif
};


class JitActivationIterator : public ActivationIterator
{
    void settle() {
        while (!done() && !activation_->isJit())
            ActivationIterator::operator++();
    }

  public:
    explicit JitActivationIterator(JSRuntime *rt)
      : ActivationIterator(rt)
    {
        settle();
    }

    JitActivationIterator &operator++() {
        ActivationIterator::operator++();
        settle();
        return *this;
    }

    
    void jitStackRange(uintptr_t *&min, uintptr_t *&end);
};

} 


class InterpreterFrameIterator
{
    InterpreterActivation *activation_;
    StackFrame *fp_;
    jsbytecode *pc_;
    Value *sp_;

  public:
    explicit InterpreterFrameIterator(InterpreterActivation *activation)
      : activation_(activation),
        fp_(nullptr),
        pc_(nullptr),
        sp_(nullptr)
    {
        if (activation) {
            fp_ = activation->current();
            pc_ = activation->regs().pc;
            sp_ = activation->regs().sp;
        }
    }

    StackFrame *frame() const {
        JS_ASSERT(!done());
        return fp_;
    }
    jsbytecode *pc() const {
        JS_ASSERT(!done());
        return pc_;
    }
    Value *sp() const {
        JS_ASSERT(!done());
        return sp_;
    }

    InterpreterFrameIterator &operator++();

    bool done() const {
        return fp_ == nullptr;
    }
};










class AsmJSActivation : public Activation
{
    AsmJSModule &module_;
    AsmJSActivation *prevAsmJS_;
    void *errorRejoinSP_;
    SPSProfiler *profiler_;
    void *resumePC_;

  public:
    AsmJSActivation(JSContext *cx, AsmJSModule &module);
    ~AsmJSActivation();

    JSContext *cx() { return cx_; }
    AsmJSModule &module() const { return module_; }
    AsmJSActivation *prevAsmJS() const { return prevAsmJS_; }

    
    static unsigned offsetOfContext() { return offsetof(AsmJSActivation, cx_); }
    static unsigned offsetOfResumePC() { return offsetof(AsmJSActivation, resumePC_); }

    
    static unsigned offsetOfErrorRejoinSP() { return offsetof(AsmJSActivation, errorRejoinSP_); }

    
    void setResumePC(void *pc) { resumePC_ = pc; }
};

class ScriptFrameIter
{
  public:
    enum SavedOption { STOP_AT_SAVED, GO_THROUGH_SAVED };
    enum ContextOption { CURRENT_CONTEXT, ALL_CONTEXTS };
    enum State { DONE, INTERP, JIT };

    
    
    struct Data
    {
        PerThreadData * perThread_;
        JSContext *     cx_;
        SavedOption     savedOption_;
        ContextOption   contextOption_;
        JSPrincipals *  principals_;

        State           state_;

        jsbytecode *    pc_;

        InterpreterFrameIterator interpFrames_;
        ActivationIterator activations_;

#ifdef JS_ION
        jit::IonFrameIterator ionFrames_;
#endif

        Data(JSContext *cx, PerThreadData *perThread, SavedOption savedOption,
             ContextOption contextOption, JSPrincipals *principals);
        Data(const Data &other);
    };

    ScriptFrameIter(JSContext *cx, SavedOption = STOP_AT_SAVED);
    ScriptFrameIter(JSContext *cx, ContextOption, SavedOption, JSPrincipals* = nullptr);
    ScriptFrameIter(const ScriptFrameIter &iter);
    ScriptFrameIter(const Data &data);
    ScriptFrameIter(AbstractFramePtr frame);

    bool done() const { return data_.state_ == DONE; }

    
    
    

    ScriptFrameIter &operator++();

    JSCompartment *compartment() const;
    Activation *activation() const { return data_.activations_.activation(); }

    bool isInterp() const { JS_ASSERT(!done()); return data_.state_ == INTERP;  }
    bool isJit() const { JS_ASSERT(!done()); return data_.state_ == JIT; }
    inline bool isIon() const;
    inline bool isBaseline() const;

    bool isFunctionFrame() const;
    bool isGlobalFrame() const;
    bool isEvalFrame() const;
    bool isNonEvalFunctionFrame() const;
    bool isGeneratorFrame() const;
    bool isConstructing() const;
    bool hasArgs() const { return isNonEvalFunctionFrame(); }

    inline JSScript *script() const;

    jsbytecode *pc() const { JS_ASSERT(!done()); return data_.pc_; }
    void        updatePcQuadratic();
    JSFunction *callee() const;
    Value       calleev() const;
    unsigned    numActualArgs() const;
    unsigned    numFormalArgs() const;
    Value       unaliasedActual(unsigned i, MaybeCheckAliasing = CHECK_ALIASING) const;
    template <class Op> inline void unaliasedForEachActual(JSContext *cx, Op op);

    JSObject   *scopeChain() const;
    CallObject &callObj() const;

    bool        hasArgsObj() const;
    ArgumentsObject &argsObj() const;

    
    bool        computeThis(JSContext *cx) const;
    Value       thisv() const;

    Value       returnValue() const;
    void        setReturnValue(const Value &v);

    JSFunction *maybeCallee() const {
        return isFunctionFrame() ? callee() : nullptr;
    }

    
    size_t      numFrameSlots() const;
    Value       frameSlotValue(size_t index) const;

    
    
    

    AbstractFramePtr abstractFramePtr() const;
    AbstractFramePtr copyDataAsAbstractFramePtr() const;
    Data *copyData() const;

    
    inline StackFrame *interpFrame() const;

  private:
    Data data_;
#ifdef JS_ION
    jit::InlineFrameIterator ionInlineFrames_;
#endif

    void popActivation();
    void popInterpreterFrame();
#ifdef JS_ION
    void nextJitFrame();
    void popJitFrame();
#endif
    void settleOnActivation();

    friend class ::JSBrokenFrameIterator;
};

#ifdef DEBUG
bool SelfHostedFramesVisible();
#else
static inline bool
SelfHostedFramesVisible()
{
    return false;
}
#endif


class NonBuiltinScriptFrameIter : public ScriptFrameIter
{
    void settle();

  public:
    NonBuiltinScriptFrameIter(JSContext *cx,
                              ScriptFrameIter::SavedOption opt = ScriptFrameIter::STOP_AT_SAVED)
      : ScriptFrameIter(cx, opt)
    {
        settle();
    }

    NonBuiltinScriptFrameIter(JSContext *cx,
                              ScriptFrameIter::ContextOption contextOption,
                              ScriptFrameIter::SavedOption savedOption,
                              JSPrincipals *principals = nullptr)
      : ScriptFrameIter(cx, contextOption, savedOption, principals)
    {
        settle();
    }

    NonBuiltinScriptFrameIter(const ScriptFrameIter::Data &data)
      : ScriptFrameIter(data)
    {}

    NonBuiltinScriptFrameIter &operator++() {
        ScriptFrameIter::operator++();
        settle();
        return *this;
    }
};





class AllFramesIter : public ScriptFrameIter
{
  public:
    AllFramesIter(JSContext *cx)
      : ScriptFrameIter(cx, ScriptFrameIter::ALL_CONTEXTS, ScriptFrameIter::GO_THROUGH_SAVED)
    {}
};



inline JSScript *
ScriptFrameIter::script() const
{
    JS_ASSERT(!done());
    if (data_.state_ == INTERP)
        return interpFrame()->script();
#ifdef JS_ION
    JS_ASSERT(data_.state_ == JIT);
    if (data_.ionFrames_.isOptimizedJS())
        return ionInlineFrames_.script();
    return data_.ionFrames_.script();
#else
    return nullptr;
#endif
}

inline bool
ScriptFrameIter::isIon() const
{
#ifdef JS_ION
    return isJit() && data_.ionFrames_.isOptimizedJS();
#else
    return false;
#endif
}

inline bool
ScriptFrameIter::isBaseline() const
{
#ifdef JS_ION
    return isJit() && data_.ionFrames_.isBaselineJS();
#else
    return false;
#endif
}

inline StackFrame *
ScriptFrameIter::interpFrame() const
{
    JS_ASSERT(data_.state_ == INTERP);
    return data_.interpFrames_.frame();
}

}  
#endif
