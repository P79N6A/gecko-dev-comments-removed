





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

class StackFrame;
class FrameRegs;

class InvokeFrameGuard;
class FrameGuard;
class ExecuteFrameGuard;
class GeneratorFrameGuard;

class ScriptFrameIter;
class AllFramesIter;

class ArgumentsObject;
class ScopeObject;
class StaticBlockObject;

struct ScopeCoordinate;































enum MaybeCheckAliasing { CHECK_ALIASING = true, DONT_CHECK_ALIASING = false };



#ifdef DEBUG
extern void
CheckLocalUnaliased(MaybeCheckAliasing checkAliasing, JSScript *script,
                    StaticBlockObject *maybeBlock, unsigned i);
#endif

namespace jit {
    class BaselineFrame;
}


class AbstractFramePtr
{
    uintptr_t ptr_;

  public:
    AbstractFramePtr()
      : ptr_(0)
    {}

    AbstractFramePtr(StackFrame *fp)
        : ptr_(fp ? uintptr_t(fp) | 0x1 : 0)
    {
        JS_ASSERT((uintptr_t(fp) & 1) == 0);
    }

    AbstractFramePtr(jit::BaselineFrame *fp)
      : ptr_(uintptr_t(fp))
    {
        JS_ASSERT((uintptr_t(fp) & 1) == 0);
    }

    explicit AbstractFramePtr(JSAbstractFramePtr frame)
        : ptr_(uintptr_t(frame.raw()))
    {
    }

    bool isStackFrame() const {
        return ptr_ & 0x1;
    }
    StackFrame *asStackFrame() const {
        JS_ASSERT(isStackFrame());
        StackFrame *res = (StackFrame *)(ptr_ & ~0x1);
        JS_ASSERT(res);
        return res;
    }
    bool isBaselineFrame() const {
        return ptr_ && !isStackFrame();
    }
    jit::BaselineFrame *asBaselineFrame() const {
        JS_ASSERT(isBaselineFrame());
        jit::BaselineFrame *res = (jit::BaselineFrame *)ptr_;
        JS_ASSERT(res);
        return res;
    }

    void *raw() const { return reinterpret_cast<void *>(ptr_); }

    bool operator ==(const AbstractFramePtr &other) const { return ptr_ == other.ptr_; }
    bool operator !=(const AbstractFramePtr &other) const { return ptr_ != other.ptr_; }

    operator bool() const { return !!ptr_; }

    inline JSGenerator *maybeSuspendedGenerator(JSRuntime *rt) const;

    inline JSObject *scopeChain() const;
    inline CallObject &callObj() const;
    inline bool initFunctionScopeObjects(JSContext *cx);
    inline void pushOnScopeChain(ScopeObject &scope);

    inline JSCompartment *compartment() const;

    inline StaticBlockObject *maybeBlockChain() const;
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

    inline bool hasArgsObj() const;
    inline ArgumentsObject &argsObj() const;
    inline void initArgsObj(ArgumentsObject &argsobj) const;
    inline bool useNewType() const;

    inline bool copyRawFrameSlots(AutoValueVector *vec) const;

    inline Value &unaliasedVar(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING);
    inline Value &unaliasedLocal(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING);
    inline Value &unaliasedFormal(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING);
    inline Value &unaliasedActual(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING);

    inline bool prevUpToDate() const;
    inline void setPrevUpToDate() const;

    JSObject *evalPrevScopeChain(JSContext *cx) const;

    inline void *maybeHookData() const;
    inline void setHookData(void *data) const;
    inline Value returnValue() const;
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
        HAS_BLOCKCHAIN     =     0x2000,  

        
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
    StaticBlockObject   *blockChain_;   
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

    






  public:
    Value *slots() const { return (Value *)(this + 1); }
    Value *base() const { return slots() + script()->nfixed; }
    Value *argv() const { return argv_; }

  private:
    friend class FrameRegs;
    friend class InterpreterStack;
    friend class ScriptFrameIter;
    friend class CallObject;
    friend class ClonedBlockObject;
    friend class ArgumentsObject;

    




    
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
        return isEvalFrame() && script()->strict;
    }

    bool isNonStrictEvalFrame() const {
        return isEvalFrame() && !script()->strict;
    }

    bool isDirectEvalFrame() const {
        return isEvalFrame() && script()->staticLevel > 0;
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

    

















    inline Value &unaliasedVar(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    inline Value &unaliasedLocal(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);

    bool hasArgs() const { return isNonEvalFunctionFrame(); }
    inline Value &unaliasedFormal(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    inline Value &unaliasedActual(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    template <class Op> inline void forEachUnaliasedActual(Op op);

    bool copyRawFrameSlots(AutoValueVector *v);

    unsigned numFormalArgs() const { JS_ASSERT(hasArgs()); return fun()->nargs; }
    unsigned numActualArgs() const { JS_ASSERT(hasArgs()); return u.nactual; }

    inline Value &canonicalActualArg(unsigned i) const;
    template <class Op>
    inline bool forEachCanonicalActualArg(Op op, unsigned start = 0, unsigned count = unsigned(-1));
    template <class Op> inline bool forEachFormalArg(Op op);

    










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

    










    bool hasBlockChain() const {
        return (flags_ & HAS_BLOCKCHAIN) && blockChain_;
    }

    StaticBlockObject *maybeBlockChain() {
        return (flags_ & HAS_BLOCKCHAIN) ? blockChain_ : NULL;
    }

    StaticBlockObject &blockChain() const {
        JS_ASSERT(hasBlockChain());
        return *blockChain_;
    }

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
        return isFunctionFrame() ? fun() : NULL;
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
        return hasHookData() ? hookData_ : NULL;
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

    Value &returnValue() {
        if (!(flags_ & HAS_RVAL))
            rval_.setUndefined();
        return rval_;
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

    JSGenerator *maybeSuspendedGenerator(JSRuntime *rt);

    






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
    static size_t offsetOfFlags() {
        return offsetof(StackFrame, flags_);
    }

    static size_t offsetOfExec() {
        return offsetof(StackFrame, exec);
    }

    static size_t offsetOfNumActual() {
        return offsetof(StackFrame, u.nactual);
    }

    static size_t offsetOfScopeChain() {
        return offsetof(StackFrame, scopeChain_);
    }

    static size_t offsetOfArgumentsObject() {
        return offsetof(StackFrame, argsObj_);
    }

    static ptrdiff_t offsetOfThis(JSFunction *fun) {
        return fun == NULL
               ? -1 * ptrdiff_t(sizeof(Value))
               : -(fun->nargs + 1) * ptrdiff_t(sizeof(Value));
    }

    static ptrdiff_t offsetOfFormalArg(JSFunction *fun, unsigned i) {
        JS_ASSERT(i < fun->nargs);
        return (-(int)fun->nargs + i) * sizeof(Value);
    }

    static size_t offsetOfFixed(unsigned i) {
        return sizeof(StackFrame) + i * sizeof(Value);
    }

  public:
    void mark(JSTracer *trc);
    void markValues(JSTracer *trc, Value *sp);

    
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

inline AbstractFramePtr Valueify(JSAbstractFramePtr frame) { return AbstractFramePtr(frame); }
static inline JSAbstractFramePtr Jsvalify(AbstractFramePtr frame)   { return JSAbstractFramePtr(frame.raw()); }



class FrameRegs
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
        JS_ASSERT(fp_->script()->nfixed + depth <= fp_->script()->nslots);
        return fp_->base() + depth;
    }

    
    void rebaseFromTo(const FrameRegs &from, StackFrame &to) {
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
        pc = script->code;
        sp = fp.slots() + script->nfixed;
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
    friend class FrameGuard;
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
                                 AbstractFramePtr evalInFrame, FrameGuard *fg);

    
    StackFrame *pushInvokeFrame(JSContext *cx, const CallArgs &args, InitialFrameFlags initial,
                                FrameGuard *fg);

    
    
    bool pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                         HandleScript script, InitialFrameFlags initial);

    void popInlineFrame(FrameRegs &regs);

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

class RunState;

class FrameGuard
{
    friend class InterpreterStack;
    RunState &state_;
    FrameRegs &regs_;
    InterpreterStack *stack_;
    StackFrame *fp_;

    void setPushed(InterpreterStack &stack, StackFrame *fp) {
        stack_ = &stack;
        fp_ = fp;
    }

  public:
    FrameGuard(RunState &state, FrameRegs &regs);
    ~FrameGuard();

    StackFrame *fp() const {
        JS_ASSERT(fp_);
        return fp_;
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

    enum Kind { Interpreter, Jit };
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

    InterpreterActivation *asInterpreter() const {
        JS_ASSERT(isInterpreter());
        return (InterpreterActivation *)this;
    }
    jit::JitActivation *asJit() const {
        JS_ASSERT(isJit());
        return (jit::JitActivation *)this;
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

  private:
    Activation(const Activation &other) MOZ_DELETE;
    void operator=(const Activation &other) MOZ_DELETE;
};




static const jsbytecode EnableInterruptsPseudoOpcode = -1;

class InterpreterFrameIterator;

class InterpreterActivation : public Activation
{
    friend class js::InterpreterFrameIterator;

    StackFrame *const entry_; 
    FrameRegs &regs_;
    jsbytecode *const switchMask_; 

#ifdef DEBUG
    size_t oldFrameCount_;
#endif

  public:
    inline InterpreterActivation(JSContext *cx, StackFrame *entry, FrameRegs &regs,
                                 jsbytecode *const switchMask);
    inline ~InterpreterActivation();

    inline bool pushInlineFrame(const CallArgs &args, HandleScript script,
                                InitialFrameFlags initial);
    inline void popInlineFrame(StackFrame *frame);

    StackFrame *current() const {
        return regs_.fp();
    }
    FrameRegs &regs() const {
        return regs_;
    }

    
    void enableInterruptsIfRunning(JSScript *script) {
        if (regs_.fp()->script() == script)
            enableInterruptsUnconditionally();
    }
    void enableInterruptsUnconditionally() {
        *switchMask_ = EnableInterruptsPseudoOpcode;
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
        return activation_ == NULL;
    }
};

namespace jit {


class JitActivation : public Activation
{
    uint8_t *prevIonTop_;
    JSContext *prevIonJSContext_;
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
        fp_(NULL),
        pc_(NULL),
        sp_(NULL)
    {
        if (activation) {
            fp_ = activation->current();
            pc_ = activation->regs_.pc;
            sp_ = activation->regs_.sp;
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
        return fp_ == NULL;
    }
};



















class ScriptFrameIter
{
  public:
    enum SavedOption { STOP_AT_SAVED, GO_THROUGH_SAVED };
    enum ContextOption { CURRENT_CONTEXT, ALL_CONTEXTS };
    enum State { DONE, SCRIPTED, JIT };

    



    struct Data
    {
        PerThreadData *perThread_;
        JSContext    *cx_;
        SavedOption  savedOption_;
        ContextOption contextOption_;

        State        state_;

        jsbytecode   *pc_;

        InterpreterFrameIterator interpFrames_;
        ActivationIterator activations_;

#ifdef JS_ION
        jit::IonFrameIterator ionFrames_;
#endif

        Data(JSContext *cx, PerThreadData *perThread, SavedOption savedOption,
             ContextOption contextOption);
        Data(const Data &other);
    };

    friend class ::JSBrokenFrameIterator;
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

  public:
    ScriptFrameIter(JSContext *cx, SavedOption = STOP_AT_SAVED);
    ScriptFrameIter(JSContext *cx, ContextOption, SavedOption);
    ScriptFrameIter(const ScriptFrameIter &iter);
    ScriptFrameIter(const Data &data);

    bool done() const { return data_.state_ == DONE; }
    ScriptFrameIter &operator++();

    Data *copyData() const;

    JSCompartment *compartment() const;

    JSScript *script() const {
        JS_ASSERT(!done());
        if (data_.state_ == SCRIPTED)
            return interpFrame()->script();
#ifdef JS_ION
        JS_ASSERT(data_.state_ == JIT);
        if (data_.ionFrames_.isOptimizedJS())
            return ionInlineFrames_.script();
        return data_.ionFrames_.script();
#else
        return NULL;
#endif
    }
    bool isJit() const {
        JS_ASSERT(!done());
        return data_.state_ == JIT;
    }

    bool isIon() const {
#ifdef JS_ION
        return isJit() && data_.ionFrames_.isOptimizedJS();
#else
        return false;
#endif
    }

    bool isBaseline() const {
#ifdef JS_ION
        return isJit() && data_.ionFrames_.isBaselineJS();
#else
        return false;
#endif
    }

    bool isFunctionFrame() const;
    bool isGlobalFrame() const;
    bool isEvalFrame() const;
    bool isNonEvalFunctionFrame() const;
    bool isGeneratorFrame() const;
    bool isConstructing() const;

    bool hasArgs() const { return isNonEvalFunctionFrame(); }

    AbstractFramePtr abstractFramePtr() const;

    





    StackFrame *interpFrame() const {
        JS_ASSERT(data_.state_ == SCRIPTED);
        return data_.interpFrames_.frame();
    }

    Activation *activation() const { return data_.activations_.activation(); }

    jsbytecode *pc() const { JS_ASSERT(!done()); return data_.pc_; }
    void        updatePcQuadratic();
    JSFunction *callee() const;
    Value       calleev() const;
    unsigned    numActualArgs() const;
    unsigned    numFormalArgs() const { return script()->function()->nargs; }
    Value       unaliasedActual(unsigned i, MaybeCheckAliasing = CHECK_ALIASING) const;

    JSObject   *scopeChain() const;
    CallObject &callObj() const;

    bool        hasArgsObj() const;
    ArgumentsObject &argsObj() const;

    
    bool        computeThis(JSContext *cx) const;
    Value       thisv() const;

    Value       returnValue() const;
    void        setReturnValue(const Value &v);

    JSFunction *maybeCallee() const {
        return isFunctionFrame() ? callee() : NULL;
    }

    
    size_t      numFrameSlots() const;
    Value       frameSlotValue(size_t index) const;

    template <class Op>
    inline void ionForEachCanonicalActualArg(JSContext *cx, Op op);
};


class NonBuiltinScriptFrameIter : public ScriptFrameIter
{
#ifdef DEBUG
    static bool includeSelfhostedFrames();
#else
    static bool includeSelfhostedFrames() {
        return false;
    }
#endif

    void settle() {
        if (!includeSelfhostedFrames())
            while (!done() && script()->selfHosted)
                ScriptFrameIter::operator++();
    }

  public:
    NonBuiltinScriptFrameIter(JSContext *cx, ScriptFrameIter::SavedOption opt = ScriptFrameIter::STOP_AT_SAVED)
      : ScriptFrameIter(cx, opt) { settle(); }

    NonBuiltinScriptFrameIter(const ScriptFrameIter::Data &data)
      : ScriptFrameIter(data)
    {}

    NonBuiltinScriptFrameIter &operator++() { ScriptFrameIter::operator++(); settle(); return *this; }
};





class AllFramesIter : public ScriptFrameIter
{
  public:
    AllFramesIter(JSContext *cx)
      : ScriptFrameIter(cx, ScriptFrameIter::ALL_CONTEXTS, ScriptFrameIter::GO_THROUGH_SAVED)
    {}
};

}  
#endif
