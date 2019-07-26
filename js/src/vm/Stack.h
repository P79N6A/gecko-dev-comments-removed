






#ifndef Stack_h__
#define Stack_h__

#include "jsfun.h"
#include "jsscript.h"
#ifdef JS_ION
#include "ion/IonFrameIterator.h"
#endif
#include "jsautooplen.h"

struct JSContext;
struct JSCompartment;

namespace js {

class StackFrame;
class FrameRegs;
class StackSegment;
class StackSpace;
class ContextStack;

class InvokeArgsGuard;
class InvokeFrameGuard;
class FrameGuard;
class ExecuteFrameGuard;
class BailoutFrameGuard;
class GeneratorFrameGuard;

class CallIter;
class ScriptFrameIter;
class AllFramesIter;

class ArgumentsObject;
class ScopeObject;
class StaticBlockObject;

struct ScopeCoordinate;

#ifdef JS_METHODJIT
namespace mjit {
    class CallCompiler;
    class GetPropCompiler;
    struct CallSite;
    struct JITScript;
    jsbytecode *NativeToPC(JITScript *jit, void *ncode, CallSite **pinline);
    namespace ic { struct GetElementIC; }
}
typedef mjit::CallSite InlinedSite;
#else
struct InlinedSite {};
#endif
typedef size_t FrameRejoinState;

namespace ion {
    class IonBailoutIterator;
    class SnapshotIterator;
}













































































































class CallArgsList : public JS::CallArgs
{
    friend class StackSegment;
    CallArgsList *prev_;
    bool active_;
  protected:
    CallArgsList() : prev_(NULL), active_(false) {}
  public:
    friend CallArgsList CallArgsListFromVp(unsigned, Value *, CallArgsList *);
    friend CallArgsList CallArgsListFromArgv(unsigned, Value *, CallArgsList *);
    CallArgsList *prev() const { return prev_; }
    bool active() const { return active_; }
    void setActive() { active_ = true; }
    void setInactive() { active_ = false; }
};

JS_ALWAYS_INLINE CallArgsList
CallArgsListFromArgv(unsigned argc, Value *argv, CallArgsList *prev)
{
    CallArgsList args;
#ifdef DEBUG
    args.usedRval_ = false;
#endif
    args.argv_ = argv;
    args.argc_ = argc;
    args.prev_ = prev;
    args.active_ = false;
    return args;
}

JS_ALWAYS_INLINE CallArgsList
CallArgsListFromVp(unsigned argc, Value *vp, CallArgsList *prev)
{
    return CallArgsListFromArgv(argc, vp + 2, prev);
}



enum MaybeCheckAliasing { CHECK_ALIASING = true, DONT_CHECK_ALIASING = false };



class BaselineFrame;


class AbstractFramePtr
{
    uintptr_t ptr_;

  protected:
    AbstractFramePtr()
      : ptr_(0)
    {}

  public:
    AbstractFramePtr(StackFrame *fp)
        : ptr_(fp ? uintptr_t(fp) | 0x1 : 0)
    {
        JS_ASSERT((uintptr_t(fp) & 1) == 0);
    }

    AbstractFramePtr(BaselineFrame *fp)
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

    void *raw() const { return reinterpret_cast<void *>(ptr_); }

    bool operator ==(const AbstractFramePtr &other) const { return ptr_ == other.ptr_; }
    bool operator !=(const AbstractFramePtr &other) const { return ptr_ != other.ptr_; }

    operator bool() const { return !!ptr_; }

    inline JSGenerator *maybeSuspendedGenerator(JSRuntime *rt) const;

    inline UnrootedObject scopeChain() const;
    inline CallObject &callObj() const;
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

    inline UnrootedScript script() const;
    inline JSFunction *fun() const;
    inline JSFunction *maybeFun() const;
    inline JSFunction &callee() const;
    inline Value calleev() const;
    inline Value &thisValue() const;

    inline bool isNonEvalFunctionFrame() const;
    inline bool isNonStrictDirectEvalFrame() const;
    inline bool isStrictEvalFrame() const;

    inline unsigned numActualArgs() const;
    inline unsigned numFormalArgs() const;

    inline Value *formals() const;
    inline Value *actuals() const;

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

    JSObject *evalPrevScopeChain(JSRuntime *rt) const;

    inline void *maybeHookData() const;
    inline void setHookData(void *data) const;
    inline void setReturnValue(const Value &rval) const;

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
    INITIAL_LOWERED        =    0x40000  
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
        FINISHED_IN_INTERP =       0x80,  

        
        OVERFLOW_ARGS      =      0x100,  
        UNDERFLOW_ARGS     =      0x200,  

        
        HAS_CALL_OBJ       =      0x400,  
        HAS_ARGS_OBJ       =      0x800,  

        
        HAS_HOOK_DATA      =     0x1000,  
        HAS_RVAL           =     0x2000,  
        HAS_SCOPECHAIN     =     0x4000,  
        HAS_PREVPC         =     0x8000,  
        HAS_BLOCKCHAIN     =    0x10000,  

        
        DOWN_FRAMES_EXPANDED =  0x20000,  
        LOWERED_CALL_APPLY   =  0x40000,  

        
        PREV_UP_TO_DATE    =    0x80000,  

        
        HAS_PUSHED_SPS_FRAME = 0x100000,  

        
        RUNNING_IN_ION     =   0x200000,  
        CALLING_INTO_ION   =   0x400000,  

        JIT_REVISED_STACK  =   0x800000,  

        
        USE_NEW_TYPE       =  0x1000000   
    };

  private:
    mutable uint32_t    flags_;         
    union {                             
        RawScript       script;         
        JSFunction      *fun;           
    } exec;
    union {                             
        unsigned        nactual;        
        RawScript       evalScript;     
    } u;
    mutable JSObject    *scopeChain_;   
    StackFrame          *prev_;         
    void                *ncode_;        
    Value               rval_;          
    StaticBlockObject   *blockChain_;   
    ArgumentsObject     *argsObj_;      
    jsbytecode          *prevpc_;       
    InlinedSite         *prevInline_;   
    void                *hookData_;     
    FrameRejoinState    rejoin_;        


    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(StackFrame, rval_) % sizeof(Value) == 0);
        JS_STATIC_ASSERT(sizeof(StackFrame) % sizeof(Value) == 0);
    }

    inline void initPrev(JSContext *cx);
    jsbytecode *prevpcSlow(InlinedSite **pinlined);
    void writeBarrierPost();

    






  public:
    Value *slots() const { return (Value *)(this + 1); }
    Value *base() const { return slots() + script()->nfixed; }
    Value *formals() const { return (Value *)this - fun()->nargs; }
    Value *actuals() const { return formals() - (flags_ & OVERFLOW_ARGS ? 2 + u.nactual : 0); }
    unsigned nactual() const { return u.nactual; }

  private:
    friend class FrameRegs;
    friend class ContextStack;
    friend class StackSpace;
    friend class StackIter;
    friend class CallObject;
    friend class ClonedBlockObject;
    friend class ArgumentsObject;
#ifdef JS_METHODJIT
    friend class mjit::CallCompiler;
    friend class mjit::GetPropCompiler;
    friend struct mjit::ic::GetElementIC;
#endif

    




    
    void initCallFrame(JSContext *cx, JSFunction &callee,
                       UnrootedScript script, uint32_t nactual, StackFrame::Flags flags);

    
    void initFixupFrame(StackFrame *prev, StackFrame::Flags flags, void *ncode, unsigned nactual);

    
    void initExecuteFrame(UnrootedScript script, StackFrame *prevLink, AbstractFramePtr prev,
                          FrameRegs *regs, const Value &thisv, JSObject &scopeChain,
                          ExecuteType type);

  public:
    


















    bool prologue(JSContext *cx);
    void epilogue(JSContext *cx);

    
    inline bool jitHeavyweightFunctionPrologue(JSContext *cx);
    bool jitStrictEvalPrologue(JSContext *cx);

    
    void initFromBailout(JSContext *cx, ion::SnapshotIterator &iter);
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

    inline void resetGeneratorPrev(JSContext *cx);

    
























    inline Value &unaliasedVar(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    inline Value &unaliasedLocal(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);

    bool hasArgs() const { return isNonEvalFunctionFrame(); }
    inline Value &unaliasedFormal(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    inline Value &unaliasedActual(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    template <class Op> inline void forEachUnaliasedActual(Op op);

    bool copyRawFrameSlots(AutoValueVector *v);

    inline unsigned numFormalArgs() const;
    inline unsigned numActualArgs() const;

    inline Value &canonicalActualArg(unsigned i) const;
    template <class Op>
    inline bool forEachCanonicalActualArg(Op op, unsigned start = 0, unsigned count = unsigned(-1));
    template <class Op> inline bool forEachFormalArg(Op op);

    void cleanupTornValues();

    










    ArgumentsObject &argsObj() const;
    void initArgsObj(ArgumentsObject &argsobj);

    inline JSObject *createRestParameter(JSContext *cx);

    


















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

    














    UnrootedScript script() const {
        return isFunctionFrame()
               ? isEvalFrame()
                 ? u.evalScript
                 : (RawScript)fun()->nonLazyScript()
               : exec.script;
    }

    














    jsbytecode *pcQuadratic(const ContextStack &stack, size_t maxDepth = SIZE_MAX);

    
    jsbytecode *prevpc(InlinedSite **pinlined = NULL) {
        if (flags_ & HAS_PREVPC) {
            if (pinlined)
                *pinlined = prevInline_;
            return prevpc_;
        }
        return prevpcSlow(pinlined);
    }

    InlinedSite *prevInline() {
        JS_ASSERT(flags_ & HAS_PREVPC);
        return prevInline_;
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
        return formals()[-1];
    }

    JSObject &constructorThis() const {
        JS_ASSERT(hasArgs());
        return formals()[-1].toObject();
    }

    Value &thisValue() const {
        if (flags_ & (EVAL | GLOBAL))
            return ((Value *)this)[-1];
        return formals()[-1];
    }

    








    JSFunction &callee() const {
        JS_ASSERT(isFunctionFrame());
        return *calleev().toObject().toFunction();
    }

    const Value &calleev() const {
        JS_ASSERT(isFunctionFrame());
        return mutableCalleev();
    }

    const Value &maybeCalleev() const {
        Value &calleev = flags_ & (EVAL | GLOBAL)
                         ? ((Value *)this)[-2]
                         : formals()[-2];
        JS_ASSERT(calleev.isObjectOrNull());
        return calleev;
    }

    Value &mutableCalleev() const {
        JS_ASSERT(isFunctionFrame());
        if (isEvalFrame())
            return ((Value *)this)[-2];
        return formals()[-2];
    }

    CallReceiver callReceiver() const {
        return CallReceiverFromArgv(formals());
    }

    






    inline JSCompartment *compartment() const;

    

    FrameRejoinState rejoin() const {
        return rejoin_;
    }

    void setRejoin(FrameRejoinState state) {
        rejoin_ = state;
    }

    

    void setDownFramesExpanded() {
        flags_ |= DOWN_FRAMES_EXPANDED;
    }

    bool downFramesExpanded() {
        return !!(flags_ & DOWN_FRAMES_EXPANDED);
    }

    

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

    

    void *nativeReturnAddress() const {
        return ncode_;
    }

    void setNativeReturnAddress(void *addr) {
        ncode_ = addr;
    }

    void **addressOfNativeReturnAddress() {
        return &ncode_;
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
        return actuals() - 2;
    }

    Value *generatorArgsSnapshotEnd() const {
        JS_ASSERT(isGeneratorFrame());
        return (Value *)this;
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
        JS_STATIC_ASSERT((int)INITIAL_LOWERED == (int)LOWERED_CALL_APPLY);
        uint32_t mask = CONSTRUCTING | LOWERED_CALL_APPLY;
        JS_ASSERT((flags_ & mask) != mask);
        return InitialFrameFlags(flags_ & mask);
    }

    void setConstructing() {
        flags_ |= CONSTRUCTING;
    }

    bool isConstructing() const {
        return !!(flags_ & CONSTRUCTING);
    }

    







    bool hasCallObj() const {
        JS_ASSERT(isStrictEvalFrame() || fun()->isHeavyweight());
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

    





    bool loweredCallOrApply() const {
        return !!(flags_ & LOWERED_CALL_APPLY);
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

    bool hasOverflowArgs() const {
        return !!(flags_ & OVERFLOW_ARGS);
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

    void setFinishedInInterpreter() {
        flags_ |= FINISHED_IN_INTERP;
    }

    bool finishedInInterpreter() const {
        return !!(flags_ & FINISHED_IN_INTERP);
    }

  public:
    

    inline void resetInlinePrev(StackFrame *prevfp, jsbytecode *prevpc);
    inline void initInlineFrame(JSFunction *fun, StackFrame *prevfp, jsbytecode *prevpc);

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

    static size_t offsetOfPrev() {
        return offsetof(StackFrame, prev_);
    }

    static size_t offsetOfReturnValue() {
        return offsetof(StackFrame, rval_);
    }

    static ptrdiff_t offsetOfNcode() {
        return offsetof(StackFrame, ncode_);
    }

    static ptrdiff_t offsetOfArgsObj() {
        return offsetof(StackFrame, argsObj_);
    }

    static ptrdiff_t offsetOfCallee(JSFunction *fun) {
        JS_ASSERT(fun != NULL);
        return -(fun->nargs + 2) * sizeof(Value);
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

#ifdef JS_METHODJIT
    inline mjit::JITScript *jit();
#endif

    void methodjitStaticAsserts();

  public:
    void mark(JSTracer *trc);

    
    bool runningInIon() const {
        return !!(flags_ & RUNNING_IN_ION);
    }
    
    bool callingIntoIon() const {
        return !!(flags_ & CALLING_INTO_ION);
    }
    
    bool beginsIonActivation() const {
        return !!(flags_ & (RUNNING_IN_ION | CALLING_INTO_ION));
    }
    void setRunningInIon() {
        flags_ |= RUNNING_IN_ION;
    }
    void setCallingIntoIon() {
        flags_ |= CALLING_INTO_ION;
    }
    void clearRunningInIon() {
        flags_ &= ~RUNNING_IN_ION;
    }
    void clearCallingIntoIon() {
        flags_ &= ~CALLING_INTO_ION;
    }

    bool jitRevisedStack() const {
        return !!(flags_ & JIT_REVISED_STACK);
    }
    void setJitRevisedStack() const {
        flags_ |= JIT_REVISED_STACK;
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

static inline bool
InitialFrameFlagsAreLowered(InitialFrameFlags initial)
{
    return !!(initial & INITIAL_LOWERED);
}

inline AbstractFramePtr Valueify(JSAbstractFramePtr frame) { return AbstractFramePtr(frame); }
static inline JSAbstractFramePtr Jsvalify(AbstractFramePtr frame)   { return JSAbstractFramePtr(frame.raw()); }



class FrameRegs
{
  public:
    Value *sp;
    jsbytecode *pc;
  private:
    InlinedSite *inlined_;
    StackFrame *fp_;
  public:
    StackFrame *fp() const { return fp_; }
    InlinedSite *inlined() const { return inlined_; }

    
    static const size_t offsetOfFp = 3 * sizeof(void *);
    static const size_t offsetOfInlined = 2 * sizeof(void *);
    static void staticAssert() {
        JS_STATIC_ASSERT(offsetOfFp == offsetof(FrameRegs, fp_));
        JS_STATIC_ASSERT(offsetOfInlined == offsetof(FrameRegs, inlined_));
    }
    void clearInlined() { inlined_ = NULL; }

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
        inlined_ = from.inlined_;
        JS_ASSERT(fp_);
    }

    
    void popFrame(Value *newsp) {
        pc = fp_->prevpc(&inlined_);
        sp = newsp;
        fp_ = fp_->prev();
        JS_ASSERT(fp_);
    }

    
    void popPartialFrame(Value *newsp) {
        sp = newsp;
        fp_ = fp_->prev();
        JS_ASSERT(fp_);
    }

    
    void restorePartialFrame(Value *newfp) {
        fp_ = (StackFrame *) newfp;
    }

    
    void refreshFramePointer(StackFrame *fp) {
        fp_ = fp;
    }

    
    void prepareToRun(StackFrame &fp, UnrootedScript script) {
        pc = script->code;
        sp = fp.slots() + script->nfixed;
        fp_ = &fp;
        inlined_ = NULL;
    }

    void setToEndOfScript() {
        UnrootedScript script = fp()->script();
        sp = fp()->base();
        pc = script->code + script->length - JSOP_STOP_LENGTH;
        JS_ASSERT(*pc == JSOP_STOP);
    }

    
    void expandInline(StackFrame *innerfp, jsbytecode *innerpc) {
        pc = innerpc;
        fp_ = innerfp;
        inlined_ = NULL;
    }

#ifdef JS_METHODJIT
    
    void updateForNcode(mjit::JITScript *jit, void *ncode) {
        pc = mjit::NativeToPC(jit, ncode, &inlined_);
    }
#endif
};



class StackSegment
{
    JSContext *cx_;

    
    StackSegment *const prevInContext_;

    
    StackSegment *const prevInMemory_;

    
    FrameRegs *regs_;

    
    CallArgsList *calls_;

#if JS_BITS_PER_WORD == 32
    



  protected:
    uint32_t padding_;
#endif

  public:
    StackSegment(JSContext *cx,
                 StackSegment *prevInContext,
                 StackSegment *prevInMemory,
                 FrameRegs *regs,
                 CallArgsList *calls)
      : cx_(cx),
        prevInContext_(prevInContext),
        prevInMemory_(prevInMemory),
        regs_(regs),
        calls_(calls)
    {}

    

    Value *slotsBegin() const {
        return (Value *)(this + 1);
    }

    

    FrameRegs &regs() const {
        JS_ASSERT(regs_);
        return *regs_;
    }

    FrameRegs *maybeRegs() const {
        return regs_;
    }

    StackFrame *fp() const {
        return regs_->fp();
    }

    StackFrame *maybefp() const {
        return regs_ ? regs_->fp() : NULL;
    }

    jsbytecode *maybepc() const {
        return regs_ ? regs_->pc : NULL;
    }

    CallArgsList &calls() const {
        JS_ASSERT(calls_);
        return *calls_;
    }

    CallArgsList *maybeCalls() const {
        return calls_;
    }

    Value *callArgv() const {
        return calls_->array();
    }

    Value *maybeCallArgv() const {
        return calls_ ? calls_->array() : NULL;
    }

    JSContext *cx() const {
        return cx_;
    }

    StackSegment *prevInContext() const {
        return prevInContext_;
    }

    StackSegment *prevInMemory() const {
        return prevInMemory_;
    }

    void repointRegs(FrameRegs *regs) {
        regs_ = regs;
    }

    bool isEmpty() const {
        return !calls_ && !regs_;
    }

    bool contains(const StackFrame *fp) const;
    bool contains(const FrameRegs *regs) const;
    bool contains(const CallArgsList *call) const;

    StackFrame *computeNextFrame(const StackFrame *fp, size_t maxDepth) const;

    Value *end() const;

    FrameRegs *pushRegs(FrameRegs &regs);
    void popRegs(FrameRegs *regs);
    void pushCall(CallArgsList &callList);
    void pointAtCall(CallArgsList &callList);
    void popCall();

    

    static const size_t offsetOfRegs() { return offsetof(StackSegment, regs_); }
};

static const size_t VALUES_PER_STACK_SEGMENT = sizeof(StackSegment) / sizeof(Value);
JS_STATIC_ASSERT(sizeof(StackSegment) % sizeof(Value) == 0);



class StackSpace
{
    StackSegment  *seg_;
    Value         *base_;
    mutable Value *conservativeEnd_;
#ifdef XP_WIN
    mutable Value *commitEnd_;
#endif
    Value         *defaultEnd_;
    Value         *trustedEnd_;

    void assertInvariants() const {
        JS_ASSERT(base_ <= conservativeEnd_);
#ifdef XP_WIN
        JS_ASSERT(conservativeEnd_ <= commitEnd_);
        JS_ASSERT(commitEnd_ <= trustedEnd_);
#endif
        JS_ASSERT(conservativeEnd_ <= defaultEnd_);
        JS_ASSERT(defaultEnd_ <= trustedEnd_);
    }

    
    static const size_t CAPACITY_VALS  = 512 * 1024;
    static const size_t CAPACITY_BYTES = CAPACITY_VALS * sizeof(Value);

    
    static const size_t COMMIT_VALS    = 16 * 1024;
    static const size_t COMMIT_BYTES   = COMMIT_VALS * sizeof(Value);

    
    static const size_t BUFFER_VALS    = 16 * 1024;
    static const size_t BUFFER_BYTES   = BUFFER_VALS * sizeof(Value);

    static void staticAsserts() {
        JS_STATIC_ASSERT(CAPACITY_VALS % COMMIT_VALS == 0);
    }

    friend class AllFramesIter;
    friend class ContextStack;
    friend class StackFrame;

    inline bool ensureSpace(JSContext *cx, MaybeReportError report,
                            Value *from, ptrdiff_t nvals) const;
    JS_FRIEND_API(bool) ensureSpaceSlow(JSContext *cx, MaybeReportError report,
                                        Value *from, ptrdiff_t nvals) const;

    StackSegment &findContainingSegment(const StackFrame *target) const;

    bool containsFast(StackFrame *fp) {
        return (Value *)fp >= base_ && (Value *)fp <= trustedEnd_;
    }

    void markFrame(JSTracer *trc, StackFrame *fp, Value *slotsEnd);

  public:
    StackSpace();
    bool init();
    ~StackSpace();

    










    static const unsigned ARGS_LENGTH_MAX = CAPACITY_VALS - (2 * BUFFER_VALS);

    
    inline Value *firstUnused() const { return seg_ ? seg_->end() : base_; }

    StackSegment &containingSegment(const StackFrame *target) const;

    







    static const size_t STACK_JIT_EXTRA = ( 8 + 18) * 10;

    













    inline Value *getStackLimit(JSContext *cx, MaybeReportError report);
    bool tryBumpLimit(JSContext *cx, Value *from, unsigned nvals, Value **limit);

    
    void mark(JSTracer *trc);

    
    void markActiveCompartments();

    




    JS_FRIEND_API(size_t) sizeOf();

#ifdef DEBUG
    
    bool containsSlow(StackFrame *fp);
#endif
};



class ContextStack
{
    StackSegment *seg_;
    StackSpace *const space_;
    JSContext *cx_;

    







    bool onTop() const;

#ifdef DEBUG
    void assertSpaceInSync() const;
#else
    void assertSpaceInSync() const {}
#endif

    
    StackSegment *pushSegment(JSContext *cx);
    enum MaybeExtend { CAN_EXTEND = true, CANT_EXTEND = false };
    Value *ensureOnTop(JSContext *cx, MaybeReportError report, unsigned nvars,
                       MaybeExtend extend, bool *pushedSeg);

    inline StackFrame *
    getCallFrame(JSContext *cx, MaybeReportError report, const CallArgs &args,
                 JSFunction *fun, HandleScript script, StackFrame::Flags *pflags) const;

    
    void popSegment();
    friend class InvokeArgsGuard;
    void popInvokeArgs(const InvokeArgsGuard &iag);
    friend class FrameGuard;
    void popFrame(const FrameGuard &fg);
    friend class GeneratorFrameGuard;
    void popGeneratorFrame(const GeneratorFrameGuard &gfg);

    friend class StackIter;

  public:
    ContextStack(JSContext *cx);
    ~ContextStack();

    

    



    bool empty() const                { return !seg_; }

    




    inline bool hasfp() const { return seg_ && seg_->maybeRegs(); }

    



    inline FrameRegs *maybeRegs() const { return seg_ ? seg_->maybeRegs() : NULL; }
    inline StackFrame *maybefp() const { return seg_ ? seg_->maybefp() : NULL; }

    
    inline FrameRegs &regs() const { JS_ASSERT(hasfp()); return seg_->regs(); }
    inline StackFrame *fp() const { JS_ASSERT(hasfp()); return seg_->fp(); }

    
    StackSpace &space() const { return *space_; }

    

    





    bool pushInvokeArgs(JSContext *cx, unsigned argc, InvokeArgsGuard *ag,
                        MaybeReportError report = REPORT_ERROR);

    
    StackFrame *pushInvokeFrame(JSContext *cx, MaybeReportError report,
                                const CallArgs &args, JSFunction *fun,
                                InitialFrameFlags initial, FrameGuard *fg);

    
    bool pushInvokeFrame(JSContext *cx, const CallArgs &args,
                         InitialFrameFlags initial, InvokeFrameGuard *ifg);

    
    bool pushExecuteFrame(JSContext *cx, HandleScript script, const Value &thisv,
                          HandleObject scopeChain, ExecuteType type,
                          AbstractFramePtr evalInFrame, ExecuteFrameGuard *efg);

    
    bool pushBailoutArgs(JSContext *cx, const ion::IonBailoutIterator &it,
                         InvokeArgsGuard *iag);

    
    StackFrame *pushBailoutFrame(JSContext *cx, const ion::IonBailoutIterator &it,
                                 const CallArgs &args, BailoutFrameGuard *bfg);

    





    bool pushGeneratorFrame(JSContext *cx, JSGenerator *gen, GeneratorFrameGuard *gfg);

    




    bool pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                         HandleFunction callee, HandleScript script,
                         InitialFrameFlags initial,
                         MaybeReportError report = REPORT_ERROR);
    bool pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                         HandleFunction callee, HandleScript script,
                         InitialFrameFlags initial, Value **stackLimit);
    void popInlineFrame(FrameRegs &regs);

    
    void popFrameAfterOverflow();

    





    enum MaybeAllowCrossCompartment {
        DONT_ALLOW_CROSS_COMPARTMENT = false,
        ALLOW_CROSS_COMPARTMENT = true
    };
    inline UnrootedScript currentScript(jsbytecode **pc = NULL,
                                        MaybeAllowCrossCompartment = DONT_ALLOW_CROSS_COMPARTMENT) const;

    
    inline HandleObject currentScriptedScopeChain() const;

    




    StackFrame *getFixupFrame(JSContext *cx, MaybeReportError report,
                              const CallArgs &args, JSFunction *fun, HandleScript script,
                              void *ncode, InitialFrameFlags initial, Value **stackLimit);

    bool saveFrameChain();
    void restoreFrameChain();

    



    inline void repointRegs(FrameRegs *regs) { JS_ASSERT(hasfp()); seg_->repointRegs(regs); }
};



class InvokeArgsGuard : public CallArgsList
{
    friend class ContextStack;
    ContextStack *stack_;
    bool pushedSeg_;
    void setPushed(ContextStack &stack) { JS_ASSERT(!pushed()); stack_ = &stack; }
  public:
    InvokeArgsGuard() : CallArgsList(), stack_(NULL), pushedSeg_(false) {}
    ~InvokeArgsGuard() { if (pushed()) stack_->popInvokeArgs(*this); }
    bool pushed() const { return !!stack_; }
    void pop() { stack_->popInvokeArgs(*this); stack_ = NULL; }
};

class FrameGuard
{
  protected:
    friend class ContextStack;
    ContextStack *stack_;
    bool pushedSeg_;
    FrameRegs regs_;
    FrameRegs *prevRegs_;
    void setPushed(ContextStack &stack) { stack_ = &stack; }
  public:
    FrameGuard() : stack_(NULL), pushedSeg_(false) {}
    ~FrameGuard() { if (pushed()) stack_->popFrame(*this); }
    bool pushed() const { return !!stack_; }
    void pop() { stack_->popFrame(*this); stack_ = NULL; }

    StackFrame *fp() const { return regs_.fp(); }
};

class InvokeFrameGuard : public FrameGuard
{};

class ExecuteFrameGuard : public FrameGuard
{};

class BailoutFrameGuard : public FrameGuard
{};

class DummyFrameGuard : public FrameGuard
{};

class GeneratorFrameGuard : public FrameGuard
{
    friend class ContextStack;
    JSGenerator *gen_;
    Value *stackvp_;
  public:
    ~GeneratorFrameGuard() { if (pushed()) stack_->popGeneratorFrame(*this); }
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





















class StackIter
{
  public:
    enum SavedOption { STOP_AT_SAVED, GO_THROUGH_SAVED };
    enum State { DONE, SCRIPTED, NATIVE, ION };

    



    struct Data
    {
        PerThreadData *perThread_;
        JSContext    *cx_;
        SavedOption  savedOption_;

        State        state_;

        StackFrame   *fp_;
        CallArgsList *calls_;

        StackSegment *seg_;
        jsbytecode   *pc_;
        CallArgs      args_;

        bool          poppedCallDuringSettle_;

#ifdef JS_ION
        ion::IonActivationIterator ionActivations_;
        ion::IonFrameIterator ionFrames_;
#endif

        Data(JSContext *cx, PerThreadData *perThread, SavedOption savedOption);
        Data(JSContext *cx, JSRuntime *rt, StackSegment *seg);
        Data(const Data &other);
    };

    friend class ContextStack;
    friend class ::JSBrokenFrameIterator;
  private:
    Data data_;
#ifdef JS_ION
    ion::InlineFrameIterator ionInlineFrames_;
#endif

    void poisonRegs();
    void popFrame();
    void popCall();
#ifdef JS_ION
    void popIonFrame();
#endif
    void settleOnNewSegment();
    void settleOnNewState();
    void startOnSegment(StackSegment *seg);

  public:
    StackIter(JSContext *cx, SavedOption = STOP_AT_SAVED);
    StackIter(JSRuntime *rt, StackSegment &seg);
    StackIter(const StackIter &iter);
    StackIter(const Data &data);

    bool done() const { return data_.state_ == DONE; }
    StackIter &operator++();

    Data *copyData() const;

    bool operator==(const StackIter &rhs) const;
    bool operator!=(const StackIter &rhs) const { return !(*this == rhs); }

    JSCompartment *compartment() const;

    bool poppedCallDuringSettle() const { return data_.poppedCallDuringSettle_; }

    bool isScript() const {
        JS_ASSERT(!done());
#ifdef JS_ION
        if (data_.state_ == ION)
            return data_.ionFrames_.isScripted();
#endif
        return data_.state_ == SCRIPTED;
    }
    UnrootedScript script() const {
        JS_ASSERT(isScript());
        if (data_.state_ == SCRIPTED)
            return interpFrame()->script();
#ifdef JS_ION
        JS_ASSERT(data_.state_ == ION);
        return ionInlineFrames_.script();
#else
        return NULL;
#endif
    }
    bool isIon() const {
        JS_ASSERT(!done());
        return data_.state_ == ION;
    }
    bool isNativeCall() const {
        JS_ASSERT(!done());
#ifdef JS_ION
        if (data_.state_ == ION)
            return data_.ionFrames_.isNative();
#endif
        return data_.state_ == NATIVE;
    }

    bool isFunctionFrame() const;
    bool isGlobalFrame() const;
    bool isEvalFrame() const;
    bool isNonEvalFunctionFrame() const;
    bool isGeneratorFrame() const;
    bool isConstructing() const;

    bool hasArgs() const { return isNonEvalFunctionFrame(); }

    AbstractFramePtr abstractFramePtr() const;

    





    StackFrame *interpFrame() const { JS_ASSERT(isScript() && !isIon()); return data_.fp_; }

    jsbytecode *pc() const { JS_ASSERT(isScript()); return data_.pc_; }
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

    
    bool        computeThis() const;
    Value       thisv() const;

    Value       returnValue() const;
    void        setReturnValue(const Value &v);

    JSFunction *maybeCallee() const {
        return isFunctionFrame() ? callee() : NULL;
    }

    
    size_t      numFrameSlots() const;
    Value       frameSlotValue(size_t index) const;

    CallArgs nativeArgs() const { JS_ASSERT(isNativeCall()); return data_.args_; }

    template <class Op>
    inline void ionForEachCanonicalActualArg(JSContext *cx, Op op);
};


class ScriptFrameIter : public StackIter
{
    void settle() {
        while (!done() && !isScript())
            StackIter::operator++();
    }

  public:
    ScriptFrameIter(JSContext *cx, StackIter::SavedOption opt = StackIter::STOP_AT_SAVED)
      : StackIter(cx, opt) { settle(); }

    ScriptFrameIter(const StackIter::Data &data)
      : StackIter(data)
    {}

    ScriptFrameIter &operator++() { StackIter::operator++(); settle(); return *this; }
};


class NonBuiltinScriptFrameIter : public StackIter
{
    void settle() {
        while (!done() && (!isScript() || (isFunctionFrame() && callee()->isSelfHostedBuiltin())))
            StackIter::operator++();
    }

  public:
    NonBuiltinScriptFrameIter(JSContext *cx, StackIter::SavedOption opt = StackIter::STOP_AT_SAVED)
        : StackIter(cx, opt) { settle(); }

    NonBuiltinScriptFrameIter &operator++() { StackIter::operator++(); settle(); return *this; }
};








class AllFramesIter
{
  public:
    AllFramesIter(JSRuntime *rt);

    bool done() const { return state_ == DONE; }
    AllFramesIter& operator++();

    bool isIon() const { return state_ == ION; }
    StackFrame *interpFrame() const { JS_ASSERT(state_ == SCRIPTED); return fp_; }
    StackSegment *seg() const { return seg_; }

    AbstractFramePtr abstractFramePtr() const;

  private:
    enum State { DONE, SCRIPTED, ION };

#ifdef JS_ION
    void popIonFrame();
#endif
    void settleOnNewState();

    StackSegment *seg_;
    StackFrame *fp_;
    State state_;

#ifdef JS_ION
    ion::IonActivationIterator ionActivations_;
    ion::IonFrameIterator ionFrames_;
#endif
};

}  
#endif
