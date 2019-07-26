






#ifndef Stack_h__
#define Stack_h__

#include "jsfun.h"
#include "ion/IonFrameIterator.h"
#include "jsautooplen.h"

struct JSContext;
struct JSCompartment;

extern void js_DumpStackFrame(JSContext *, js::StackFrame *);

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
class DummyFrameGuard;
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

namespace detail {
    struct OOMCheck;
}






































































































class CallReceiver
{
  protected:
#ifdef DEBUG
    mutable bool usedRval_;
    void setUsedRval() const { usedRval_ = true; }
    void clearUsedRval() const { usedRval_ = false; }
#else
    void setUsedRval() const {}
    void clearUsedRval() const {}
#endif
    Value *argv_;
  public:
    friend CallReceiver CallReceiverFromVp(Value *);
    friend CallReceiver CallReceiverFromArgv(Value *);
    Value *base() const { return argv_ - 2; }
    JSObject &callee() const { JS_ASSERT(!usedRval_); return argv_[-2].toObject(); }
    Value &calleev() const { JS_ASSERT(!usedRval_); return argv_[-2]; }
    Value &thisv() const { return argv_[-1]; }

    Value &rval() const {
        setUsedRval();
        return argv_[-2];
    }

    Value *spAfterCall() const {
        setUsedRval();
        return argv_ - 1;
    }

    void setCallee(Value calleev) {
        clearUsedRval();
        this->calleev() = calleev;
    }
};

JS_ALWAYS_INLINE CallReceiver
CallReceiverFromArgv(Value *argv)
{
    CallReceiver receiver;
    receiver.clearUsedRval();
    receiver.argv_ = argv;
    return receiver;
}

JS_ALWAYS_INLINE CallReceiver
CallReceiverFromVp(Value *vp)
{
    return CallReceiverFromArgv(vp + 2);
}



class CallArgs : public CallReceiver
{
  protected:
    unsigned argc_;
  public:
    friend CallArgs CallArgsFromVp(unsigned, Value *);
    friend CallArgs CallArgsFromArgv(unsigned, Value *);
    friend CallArgs CallArgsFromSp(unsigned, Value *);
    Value &operator[](unsigned i) const { JS_ASSERT(i < argc_); return argv_[i]; }
    Value *array() const { return argv_; }
    unsigned length() const { return argc_; }
    Value *end() const { return argv_ + argc_; }
    bool hasDefined(unsigned i) const { return i < argc_ && !argv_[i].isUndefined(); }
};

JS_ALWAYS_INLINE CallArgs
CallArgsFromArgv(unsigned argc, Value *argv)
{
    CallArgs args;
    args.clearUsedRval();
    args.argv_ = argv;
    args.argc_ = argc;
    return args;
}

JS_ALWAYS_INLINE CallArgs
CallArgsFromVp(unsigned argc, Value *vp)
{
    return CallArgsFromArgv(argc, vp + 2);
}

JS_ALWAYS_INLINE CallArgs
CallArgsFromSp(unsigned argc, Value *sp)
{
    return CallArgsFromArgv(argc, sp - argc);
}










class CallArgsList : public CallArgs
{
    friend class StackSegment;
    CallArgsList *prev_;
    bool active_;
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




enum InitialFrameFlags {
    INITIAL_NONE           =          0,
    INITIAL_CONSTRUCT      =       0x40, 
    INITIAL_LOWERED        =   0x200000  
};

enum ExecuteType {
    EXECUTE_GLOBAL         =        0x1, 
    EXECUTE_DIRECT_EVAL    =        0x8, 
    EXECUTE_INDIRECT_EVAL  =        0x9, 
    EXECUTE_DEBUG          =       0x18  
};



class StackFrame
{
  public:
    enum Flags {
        
        GLOBAL             =        0x1,  
        FUNCTION           =        0x2,  
        DUMMY              =        0x4,  

        
        EVAL               =        0x8,  
        DEBUGGER           =       0x10,  
        GENERATOR          =       0x20,  
        CONSTRUCTING       =       0x40,  

        
        YIELDING           =       0x80,  
        FINISHED_IN_INTERP =      0x100,  

        
        OVERFLOW_ARGS      =      0x200,  
        UNDERFLOW_ARGS     =      0x400,  

        
        HAS_CALL_OBJ       =      0x800,  
        HAS_ARGS_OBJ       =     0x1000,  
        HAS_NESTING        =     0x2000,  

        
        HAS_HOOK_DATA      =     0x4000,  
        HAS_ANNOTATION     =     0x8000,  
        HAS_RVAL           =    0x10000,  
        HAS_SCOPECHAIN     =    0x20000,  
        HAS_PREVPC         =    0x40000,  
        HAS_BLOCKCHAIN     =    0x80000,  

        
        DOWN_FRAMES_EXPANDED = 0x100000,  
        LOWERED_CALL_APPLY   = 0x200000,  

        
        PREV_UP_TO_DATE    =   0x400000,   

        
        RUNNING_IN_ION       = 0x800000   
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
    StackFrame          *prev_;         
    void                *ncode_;        
    Value               rval_;          
    StaticBlockObject   *blockChain_;   
    ArgumentsObject     *argsObj_;      
    jsbytecode          *prevpc_;       
    InlinedSite         *prevInline_;   
    void                *hookData_;     
    void                *annotation_;   
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

  private:
    friend class FrameRegs;
    friend class ContextStack;
    friend class StackSpace;
    friend class StackIter;
    friend class CallObject;
    friend class ClonedBlockObject;
    friend class ArgumentsObject;
    friend void ::js_DumpStackFrame(JSContext *, StackFrame *);
    friend void ::js_ReportIsNotFunction(JSContext *, const js::Value *, unsigned);
#ifdef JS_METHODJIT
    friend class mjit::CallCompiler;
    friend class mjit::GetPropCompiler;
    friend struct mjit::ic::GetElementIC;
#endif

    




    
    void initCallFrame(JSContext *cx, JSFunction &callee,
                       JSScript *script, uint32_t nactual, StackFrame::Flags flags);

    
    void initFixupFrame(StackFrame *prev, StackFrame::Flags flags, void *ncode, unsigned nactual);

    
    void initExecuteFrame(JSScript *script, StackFrame *prev, FrameRegs *regs,
                          const Value &thisv, JSObject &scopeChain, ExecuteType type);

    
    void initDummyFrame(JSContext *cx, JSObject &chain);

  public:
    





















    bool prologue(JSContext *cx, bool newType);
    void epilogue(JSContext *cx);

    
    inline bool jitHeavyweightFunctionPrologue(JSContext *cx);
    inline void jitTypeNestingPrologue(JSContext *cx);
    bool jitStrictEvalPrologue(JSContext *cx);

    
    void initFromBailout(JSContext *cx, ion::SnapshotIterator &iter);
    bool initCallObject(JSContext *cx);

    
    void initVarsToUndefined();

    










    bool isFunctionFrame() const {
        return !!(flags_ & FUNCTION);
    }

    bool isGlobalFrame() const {
        return !!(flags_ & GLOBAL);
    }

    bool isDummyFrame() const {
        return !!(flags_ & DUMMY);
    }

    bool isScriptFrame() const {
        bool retval = !!(flags_ & (FUNCTION | GLOBAL));
        JS_ASSERT(retval == !isDummyFrame());
        return retval;
    }

    











    bool isEvalFrame() const {
        JS_ASSERT_IF(flags_ & EVAL, isScriptFrame());
        return flags_ & EVAL;
    }

    bool isEvalInFunction() const {
        return (flags_ & (EVAL | FUNCTION)) == (EVAL | FUNCTION);
    }

    bool isNonEvalFunctionFrame() const {
        return (flags_ & (FUNCTION | EVAL)) == FUNCTION;
    }

    inline bool isStrictEvalFrame() const {
        return isEvalFrame() && script()->strictModeCode;
    }

    bool isNonStrictEvalFrame() const {
        return isEvalFrame() && !script()->strictModeCode;
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
    inline Value &unaliasedActual(unsigned i);
    template <class Op> inline void forEachUnaliasedActual(Op op);

    inline unsigned numFormalArgs() const;
    inline unsigned numActualArgs() const;

    inline Value &canonicalActualArg(unsigned i) const;
    template <class Op>
    inline bool forEachCanonicalActualArg(Op op, unsigned start = 0, unsigned count = unsigned(-1));
    template <class Op> inline bool forEachFormalArg(Op op);



    










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

    














    JSScript *script() const {
        JS_ASSERT(isScriptFrame());
        return isFunctionFrame()
               ? isEvalFrame() ? u.evalScript : fun()->script()
               : exec.script;
    }

    JSScript *maybeScript() const {
        return isScriptFrame() ? script() : NULL;
    }

    
















    jsbytecode *pcQuadratic(const ContextStack &stack, StackFrame *next = NULL,
                            InlinedSite **pinlined = NULL);

    jsbytecode *prevpc(InlinedSite **pinlined) {
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

    JSFunction* maybeScriptFunction() const {
        if (!isFunctionFrame())
            return NULL;
        const StackFrame *fp = this;
        while (fp->isEvalFrame())
            fp = fp->prev();
        return fp->script()->function();
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
        JS_ASSERT(isScriptFrame());
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

    

    void* annotation() const {
        return (flags_ & HAS_ANNOTATION) ? annotation_ : NULL;
    }

    void setAnnotation(void *annot) {
        flags_ |= HAS_ANNOTATION;
        annotation_ = annot;
    }

    

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
    template <class T, class U, TriggerPostBarriers doPostBarrier>
    void copyFrameAndValues(JSContext *cx, T *vp, StackFrame *otherfp, U *othervp, Value *othersp);

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
    void setRunningInIon() {
        flags_ |= RUNNING_IN_ION;
    }
    void clearRunningInIon() {
        flags_ &= ~RUNNING_IN_ION;
    }
};

static const size_t VALUES_PER_STACK_FRAME = sizeof(StackFrame) / sizeof(Value);

static inline unsigned
ToReportFlags(InitialFrameFlags initial)
{
    return unsigned(initial & StackFrame::CONSTRUCTING);
}

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

inline StackFrame *          Valueify(JSStackFrame *fp) { return (StackFrame *)fp; }
static inline JSStackFrame * Jsvalify(StackFrame *fp)   { return (JSStackFrame *)fp; }



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

    
    void prepareToRun(StackFrame &fp, JSScript *script) {
        pc = script->code;
        sp = fp.slots() + script->nfixed;
        fp_ = &fp;
        inlined_ = NULL;
    }

    void setToEndOfScript() {
        JSScript *script = fp()->script();
        sp = fp()->base();
        pc = script->code + script->length - JSOP_STOP_LENGTH;
        JS_ASSERT(*pc == JSOP_STOP);
    }

    
    void initDummyFrame(StackFrame &fp) {
        pc = NULL;
        sp = fp.slots();
        fp_ = &fp;
        inlined_ = NULL;
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
    
    StackSegment *const prevInContext_;

    
    StackSegment *const prevInMemory_;

    
    FrameRegs *regs_;

    
    CallArgsList *calls_;

  public:
    StackSegment(StackSegment *prevInContext,
                 StackSegment *prevInMemory,
                 FrameRegs *regs,
                 CallArgsList *calls)
      : prevInContext_(prevInContext),
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

    StackFrame *computeNextFrame(const StackFrame *fp) const;

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

    







    static const size_t CX_COMPARTMENT = 0xc;

    inline bool ensureSpace(JSContext *cx, MaybeReportError report,
                            Value *from, ptrdiff_t nvals,
                            JSCompartment *dest = (JSCompartment *)CX_COMPARTMENT) const;
    JS_FRIEND_API(bool) ensureSpaceSlow(JSContext *cx, MaybeReportError report,
                                        Value *from, ptrdiff_t nvals,
                                        JSCompartment *dest) const;

    StackSegment &findContainingSegment(const StackFrame *target) const;

    bool containsFast(StackFrame *fp) {
        return (Value *)fp >= base_ && (Value *)fp <= trustedEnd_;
    }

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
    void markFrameValues(JSTracer *trc, StackFrame *fp, Value *slotsEnd, jsbytecode *pc);

    
    void markActiveCompartments();

    
    JS_FRIEND_API(size_t) sizeOfCommitted();

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
                       MaybeExtend extend, bool *pushedSeg,
                       JSCompartment *dest = (JSCompartment *)StackSpace::CX_COMPARTMENT);

    inline StackFrame *
    getCallFrame(JSContext *cx, MaybeReportError report, const CallArgs &args,
                 JSFunction *fun, JSScript *script, StackFrame::Flags *pflags) const;

    
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

    
    bool containsSlow(const StackFrame *target) const;

    

    





    bool pushInvokeArgs(JSContext *cx, unsigned argc, InvokeArgsGuard *ag,
                        MaybeReportError report = REPORT_ERROR);

    
    StackFrame *pushInvokeFrame(JSContext *cx, MaybeReportError report,
                                const CallArgs &args, JSFunction *fun,
                                InitialFrameFlags initial, FrameGuard *fg);

    
    bool pushInvokeFrame(JSContext *cx, const CallArgs &args,
                         InitialFrameFlags initial, InvokeFrameGuard *ifg);

    
    bool pushExecuteFrame(JSContext *cx, JSScript *script, const Value &thisv,
                          JSObject &scopeChain, ExecuteType type,
                          StackFrame *evalInFrame, ExecuteFrameGuard *efg);

    
    bool pushBailoutArgs(JSContext *cx, const ion::IonBailoutIterator &it,
                         InvokeArgsGuard *iag);

    
    StackFrame *pushBailoutFrame(JSContext *cx, const ion::IonBailoutIterator &it,
                                 const CallArgs &args, BailoutFrameGuard *bfg);

    





    bool pushGeneratorFrame(JSContext *cx, JSGenerator *gen, GeneratorFrameGuard *gfg);

    








    bool pushDummyFrame(JSContext *cx, JSCompartment *dest, JSObject &scopeChain, DummyFrameGuard *dfg);

    




    bool pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                         JSFunction &callee, JSScript *script,
                         InitialFrameFlags initial);
    bool pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                         JSFunction &callee, JSScript *script,
                         InitialFrameFlags initial, Value **stackLimit);
    void popInlineFrame(FrameRegs &regs);

    
    void popFrameAfterOverflow();

    
    inline JSScript *currentScript(jsbytecode **pc = NULL) const;
    inline JSScript *currentScriptWithDiagnostics(jsbytecode **pc = NULL) const;

    
    inline HandleObject currentScriptedScopeChain() const;

    




    StackFrame *getFixupFrame(JSContext *cx, MaybeReportError report,
                              const CallArgs &args, JSFunction *fun, JSScript *script,
                              void *ncode, InitialFrameFlags initial, Value **stackLimit);

    bool saveFrameChain();
    void restoreFrameChain();

    



    inline void repointRegs(FrameRegs *regs) { JS_ASSERT(hasfp()); seg_->repointRegs(regs); }

    

    




    void threadReset();

    

    static size_t offsetOfSeg() { return offsetof(ContextStack, seg_); }
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





















class StackIter
{
    friend class ContextStack;
    JSContext    *maybecx_;
  public:
    enum SavedOption { STOP_AT_SAVED, GO_THROUGH_SAVED };
  private:
    SavedOption  savedOption_;

    enum State { DONE, SCRIPTED, NATIVE, IMPLICIT_NATIVE, ION };

    State        state_;

    StackFrame   *fp_;
    CallArgsList *calls_;

    StackSegment *seg_;
    Value        *sp_;
    jsbytecode   *pc_;
    JSScript     *script_;
    CallArgs     args_;

#ifdef JS_ION
    ion::IonActivationIterator ionActivations_;
    ion::IonFrameIterator ionFrames_;
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

    bool done() const { return state_ == DONE; }
    StackIter &operator++();

    bool operator==(const StackIter &rhs) const;
    bool operator!=(const StackIter &rhs) const { return !(*this == rhs); }

    bool isScript() const {
        JS_ASSERT(!done());
        return state_ == SCRIPTED ||
               (state_ == ION && ionFrames_.isScripted());
    }
    bool isIon() const {
        JS_ASSERT(!done());
        return state_ == ION;
    }
    bool isImplicitNativeCall() const {
        JS_ASSERT(!done());
        return state_ == IMPLICIT_NATIVE;
    }
    bool isNativeCall() const {
        JS_ASSERT(!done());
        return state_ == NATIVE || state_ == IMPLICIT_NATIVE ||
               (state_ == ION && ionFrames_.isNative());
    }

    bool isFunctionFrame() const;
    bool isEvalFrame() const;
    bool isNonEvalFunctionFrame() const;
    bool isConstructing() const;

    
    StackFrame *fp() const { JS_ASSERT(isScript()); return fp_; }
    Value      *sp() const { JS_ASSERT(isScript()); return sp_; }
    jsbytecode *pc() const { JS_ASSERT(isScript()); return pc_; }
    JSScript   *script() const { JS_ASSERT(isScript()); return script_; }
    JSFunction *callee() const;
    Value       calleev() const;
    unsigned    numActualArgs() const;
    Value       thisv() const;

    CallArgs nativeArgs() const { JS_ASSERT(isNativeCall()); return args_; }

    template <class Op>
    inline bool forEachCanonicalActualArg(Op op, unsigned start = 0, unsigned count = unsigned(-1));
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

    ScriptFrameIter &operator++() { StackIter::operator++(); settle(); return *this; }
};







class AllFramesIter
{
  public:
    AllFramesIter(StackSpace &space);

    bool done() const { return fp_ == NULL; }
    AllFramesIter& operator++();

    StackFrame *fp() const { return fp_; }

  private:
    void settle();
    StackSegment *seg_;
    StackFrame *fp_;
};

}  
#endif
