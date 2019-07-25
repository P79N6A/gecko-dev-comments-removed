







































#ifndef Stack_h__
#define Stack_h__

#include "jsfun.h"

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
class DummyFrameGuard;
class GeneratorFrameGuard;

namespace mjit {
    struct JITScript;
    struct CallSite;
    jsbytecode *NativeToPC(JITScript *jit, void *ncode, CallSite **pinline);
}
namespace detail { struct OOMCheck; }

class CallIter;
class FrameRegsIter;
class AllFramesIter;

class ArgumentsObject;

#ifdef JS_METHODJIT
typedef js::mjit::CallSite JSInlinedSite;
#else
struct JSInlinedSite {};
#endif

typedef  size_t JSRejoinState;

































































































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

    void calleeHasBeenReset() const {
        clearUsedRval();
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
    uintN argc_;
  public:
    friend CallArgs CallArgsFromVp(uintN, Value *);
    friend CallArgs CallArgsFromArgv(uintN, Value *);
    friend CallArgs CallArgsFromSp(uintN, Value *);
    Value &operator[](unsigned i) const { JS_ASSERT(i < argc_); return argv_[i]; }
    Value *array() const { return argv_; }
    uintN length() const { return argc_; }
    Value *end() const { return argv_ + argc_; }
};

JS_ALWAYS_INLINE CallArgs
CallArgsFromArgv(uintN argc, Value *argv)
{
    CallArgs args;
    args.clearUsedRval();
    args.argv_ = argv;
    args.argc_ = argc;
    return args;
}

JS_ALWAYS_INLINE CallArgs
CallArgsFromVp(uintN argc, Value *vp)
{
    return CallArgsFromArgv(argc, vp + 2);
}

JS_ALWAYS_INLINE CallArgs
CallArgsFromSp(uintN argc, Value *sp)
{
    return CallArgsFromArgv(argc, sp - argc);
}










class CallArgsList : public CallArgs
{
    friend class StackSegment;
    CallArgsList *prev_;
    bool active_;
  public:
    friend CallArgsList CallArgsListFromVp(uintN, Value *, CallArgsList *);
    friend CallArgsList CallArgsListFromArgv(uintN, Value *, CallArgsList *);
    CallArgsList *prev() const { return prev_; }
    bool active() const { return active_; }
    void setActive() { active_ = true; }
    void setInactive() { active_ = false; }
};

JS_ALWAYS_INLINE CallArgsList
CallArgsListFromArgv(uintN argc, Value *argv, CallArgsList *prev)
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
CallArgsListFromVp(uintN argc, Value *vp, CallArgsList *prev)
{
    return CallArgsListFromArgv(argc, vp + 2, prev);
}




enum InitialFrameFlags {
    INITIAL_NONE           =          0,
    INITIAL_CONSTRUCT      =       0x80, 
    INITIAL_LOWERED        =   0x400000  
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
        FLOATING_GENERATOR =       0x40,  
        CONSTRUCTING       =       0x80,  

        
        YIELDING           =      0x100,  
        FINISHED_IN_INTERP =      0x200,  

        
        OVERRIDE_ARGS      =      0x400,  
        OVERFLOW_ARGS      =      0x800,  
        UNDERFLOW_ARGS     =     0x1000,  

        
        HAS_IMACRO_PC      =     0x2000,  
        HAS_CALL_OBJ       =     0x4000,  
        HAS_ARGS_OBJ       =     0x8000,  
        HAS_HOOK_DATA      =    0x10000,  
        HAS_ANNOTATION     =    0x20000,  
        HAS_RVAL           =    0x40000,  
        HAS_SCOPECHAIN     =    0x80000,  
        HAS_PREVPC         =   0x100000,  

        
        DOWN_FRAMES_EXPANDED = 0x200000,  
        LOWERED_CALL_APPLY   = 0x400000   
    };

  private:
    mutable uint32      flags_;         
    union {                             
        JSScript        *script;        
        JSFunction      *fun;           
    } exec;
    union {                             
        uintN           nactual;        
        ArgumentsObject *obj;           
        JSScript        *script;        
    } args;
    mutable JSObject    *scopeChain_;   
    StackFrame          *prev_;         
    void                *ncode_;        

    
    Value               rval_;          
    jsbytecode          *prevpc_;       
    JSInlinedSite       *prevInline_;   
    jsbytecode          *imacropc_;     
    void                *hookData_;     
    void                *annotation_;   
    JSRejoinState       rejoin_;        


    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(StackFrame, rval_) % sizeof(Value) == 0);
        JS_STATIC_ASSERT(sizeof(StackFrame) % sizeof(Value) == 0);
    }

    inline void initPrev(JSContext *cx);
    jsbytecode *prevpcSlow(JSInlinedSite **pinlined);

  public:
    







    
    void initCallFrame(JSContext *cx, JSObject &callee, JSFunction *fun,
                       JSScript *script, uint32 nactual, StackFrame::Flags flags);

    
    void resetCallFrame(JSScript *script);

    
    void initJitFrameCallerHalf(StackFrame *prev, StackFrame::Flags flags, void *ncode);
    void initJitFrameEarlyPrologue(JSFunction *fun, uint32 nactual);
    bool initJitFrameLatePrologue(JSContext *cx, Value **limit);

    
    void initExecuteFrame(JSScript *script, StackFrame *prev, FrameRegs *regs,
                          const Value &thisv, JSObject &scopeChain, ExecuteType type);

    
    void stealFrameAndSlots(Value *vp, StackFrame *otherfp, Value *othervp, Value *othersp);

    
    void initDummyFrame(JSContext *cx, JSObject &chain);

    










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

    












    StackFrame *prev() const {
        return prev_;
    }

    inline void resetGeneratorPrev(JSContext *cx);
    inline void resetInlinePrev(StackFrame *prevfp, jsbytecode *prevpc);

    inline void initInlineFrame(JSFunction *fun, StackFrame *prevfp, jsbytecode *prevpc);

    







    Value *slots() const {
        return (Value *)(this + 1);
    }

    Value *base() const {
        return slots() + script()->nfixed;
    }

    Value &varSlot(uintN i) {
        JS_ASSERT(i < script()->nfixed);
        JS_ASSERT_IF(maybeFun(), i < script()->bindings.countVars());
        return slots()[i];
    }

    














    



















    jsbytecode *pcQuadratic(const ContextStack &stack, StackFrame *next = NULL,
                            JSInlinedSite **pinlined = NULL);

    jsbytecode *prevpc(JSInlinedSite **pinlined) {
        if (flags_ & HAS_PREVPC) {
            if (pinlined)
                *pinlined = prevInline_;
            return prevpc_;
        }
        return prevpcSlow(pinlined);
    }

    JSInlinedSite *prevInline() {
        JS_ASSERT(flags_ & HAS_PREVPC);
        return prevInline_;
    }

    JSScript *script() const {
        JS_ASSERT(isScriptFrame());
        return isFunctionFrame()
               ? isEvalFrame() ? args.script : fun()->script()
               : exec.script;
    }

    JSScript *functionScript() const {
        JS_ASSERT(isFunctionFrame());
        return isEvalFrame() ? args.script : fun()->script();
    }

    JSScript *globalScript() const {
        JS_ASSERT(isGlobalFrame());
        return exec.script;
    }

    JSScript *maybeScript() const {
        return isScriptFrame() ? script() : NULL;
    }

    size_t numFixed() const {
        return script()->nfixed;
    }

    size_t numSlots() const {
        return script()->nslots;
    }

    size_t numGlobalVars() const {
        JS_ASSERT(isGlobalFrame());
        return exec.script->nfixed;
    }

    





    JSFunction* fun() const {
        JS_ASSERT(isFunctionFrame());
        return exec.fun;
    }

    JSFunction* maybeFun() const {
        return isFunctionFrame() ? fun() : NULL;
    }

    


















    
    bool hasArgs() const {
        return isNonEvalFunctionFrame();
    }

    uintN numFormalArgs() const {
        JS_ASSERT(hasArgs());
        return fun()->nargs;
    }

    Value &formalArg(uintN i) const {
        JS_ASSERT(i < numFormalArgs());
        return formalArgs()[i];
    }

    Value *formalArgs() const {
        JS_ASSERT(hasArgs());
        return (Value *)this - numFormalArgs();
    }

    Value *formalArgsEnd() const {
        JS_ASSERT(hasArgs());
        return (Value *)this;
    }

    Value *maybeFormalArgs() const {
        return (flags_ & (FUNCTION | EVAL)) == FUNCTION
               ? formalArgs()
               : NULL;
    }

    inline uintN numActualArgs() const;
    inline Value *actualArgs() const;
    inline Value *actualArgsEnd() const;

    inline Value &canonicalActualArg(uintN i) const;
    template <class Op>
    inline bool forEachCanonicalActualArg(Op op, uintN start = 0, uintN count = uintN(-1));
    template <class Op> inline bool forEachFormalArg(Op op);

    bool hasArgsObj() const {
        return !!(flags_ & HAS_ARGS_OBJ);
    }

    ArgumentsObject &argsObj() const {
        JS_ASSERT(hasArgsObj());
        JS_ASSERT(!isEvalFrame());
        return *args.obj;
    }

    ArgumentsObject *maybeArgsObj() const {
        return hasArgsObj() ? &argsObj() : NULL;
    }

    inline void setArgsObj(ArgumentsObject &obj);

    













    Value &functionThis() const {
        JS_ASSERT(isFunctionFrame());
        if (isEvalFrame())
            return ((Value *)this)[-1];
        return formalArgs()[-1];
    }

    JSObject &constructorThis() const {
        JS_ASSERT(hasArgs());
        return formalArgs()[-1].toObject();
    }

    Value &globalThis() const {
        JS_ASSERT(isGlobalFrame());
        return ((Value *)this)[-1];
    }

    Value &thisValue() const {
        if (flags_ & (EVAL | GLOBAL))
            return ((Value *)this)[-1];
        return formalArgs()[-1];
    }

    








    JSObject &callee() const {
        JS_ASSERT(isFunctionFrame());
        return calleev().toObject();
    }

    const Value &calleev() const {
        JS_ASSERT(isFunctionFrame());
        return mutableCalleev();
    }

    const Value &maybeCalleev() const {
        JS_ASSERT(isScriptFrame());
        Value &calleev = flags_ & (EVAL | GLOBAL)
                         ? ((Value *)this)[-2]
                         : formalArgs()[-2];
        JS_ASSERT(calleev.isObjectOrNull());
        return calleev;
    }

    




    void overwriteCallee(JSObject &newCallee) {
        JS_ASSERT(callee().getFunctionPrivate() == newCallee.getFunctionPrivate());
        mutableCalleev().setObject(newCallee);
    }

    Value &mutableCalleev() const {
        JS_ASSERT(isFunctionFrame());
        if (isEvalFrame())
            return ((Value *)this)[-2];
        return formalArgs()[-2];
    }

    




    bool getValidCalleeObject(JSContext *cx, Value *vp);

    CallReceiver callReceiver() const {
        return CallReceiverFromArgv(formalArgs());
    }

    





























    JSObject &scopeChain() const {
        JS_ASSERT_IF(!(flags_ & HAS_SCOPECHAIN), isFunctionFrame());
        if (!(flags_ & HAS_SCOPECHAIN)) {
            scopeChain_ = callee().getParent();
            flags_ |= HAS_SCOPECHAIN;
        }
        return *scopeChain_;
    }

    bool hasCallObj() const {
        bool ret = !!(flags_ & HAS_CALL_OBJ);
        JS_ASSERT_IF(ret, !isNonStrictEvalFrame());
        return ret;
    }

    inline CallObject &callObj() const;
    inline void setScopeChainNoCallObj(JSObject &obj);
    inline void setScopeChainWithOwnCallObj(CallObject &obj);

    



    inline bool functionPrologue(JSContext *cx);

    





    inline void functionEpilogue();

    



    inline void markFunctionEpilogueDone();

    inline bool maintainNestingState() const;

    














    JSObject &varObj() {
        JSObject *obj = &scopeChain();
        while (!obj->isVarObj())
            obj = obj->getParent();
        return *obj;
    }

    






    JSCompartment *compartment() const {
        JS_ASSERT_IF(isScriptFrame(), scopeChain().compartment() == script()->compartment());
        return scopeChain().compartment();
    }

    







    bool hasImacropc() const {
        return flags_ & HAS_IMACRO_PC;
    }

    jsbytecode *imacropc() const {
        JS_ASSERT(hasImacropc());
        return imacropc_;
    }

    jsbytecode *maybeImacropc() const {
        return hasImacropc() ? imacropc() : NULL;
    }

    void clearImacropc() {
        flags_ &= ~HAS_IMACRO_PC;
    }

    void setImacropc(jsbytecode *pc) {
        JS_ASSERT(pc);
        JS_ASSERT(!(flags_ & HAS_IMACRO_PC));
        imacropc_ = pc;
        flags_ |= HAS_IMACRO_PC;
    }

    

    void* annotation() const {
        return (flags_ & HAS_ANNOTATION) ? annotation_ : NULL;
    }

    void setAnnotation(void *annot) {
        flags_ |= HAS_ANNOTATION;
        annotation_ = annot;
    }

    

    JSRejoinState rejoin() const {
        return rejoin_;
    }

    void setRejoin(JSRejoinState state) {
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

    

    const Value &returnValue() {
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
        return !!(flags_ & GENERATOR);
    }

    bool isFloatingGenerator() const {
        JS_ASSERT_IF(flags_ & FLOATING_GENERATOR, isGeneratorFrame());
        return !!(flags_ & FLOATING_GENERATOR);
    }

    void initFloatingGenerator() {
        JS_ASSERT(!(flags_ & GENERATOR));
        flags_ |= (GENERATOR | FLOATING_GENERATOR);
    }

    void unsetFloatingGenerator() {
        flags_ &= ~FLOATING_GENERATOR;
    }

    void setFloatingGenerator() {
        flags_ |= FLOATING_GENERATOR;
    }

    






    bool isFramePushedByExecute() const {
        return !!(flags_ & (GLOBAL | EVAL));
    }

    



    InitialFrameFlags initialFlags() const {
        JS_STATIC_ASSERT((int)INITIAL_NONE == 0);
        JS_STATIC_ASSERT((int)INITIAL_CONSTRUCT == (int)CONSTRUCTING);
        JS_STATIC_ASSERT((int)INITIAL_LOWERED == (int)LOWERED_CALL_APPLY);
        uint32 mask = CONSTRUCTING | LOWERED_CALL_APPLY;
        JS_ASSERT((flags_ & mask) != mask);
        return InitialFrameFlags(flags_ & mask);
    }

    bool isConstructing() const {
        return !!(flags_ & CONSTRUCTING);
    }

    





    bool loweredCallOrApply() const {
        return !!(flags_ & LOWERED_CALL_APPLY);
    }

    bool isDebuggerFrame() const {
        return !!(flags_ & DEBUGGER);
    }

    bool hasOverriddenArgs() const {
        return !!(flags_ & OVERRIDE_ARGS);
    }

    bool hasOverflowArgs() const {
        return !!(flags_ & OVERFLOW_ARGS);
    }

    void setOverriddenArgs() {
        flags_ |= OVERRIDE_ARGS;
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

#ifdef DEBUG
    
    static JSObject *const sInvalidScopeChain;
#endif

  public:
    

    static size_t offsetOfFlags() {
        return offsetof(StackFrame, flags_);
    }

    static size_t offsetOfExec() {
        return offsetof(StackFrame, exec);
    }

    static size_t offsetOfArgs() {
        return offsetof(StackFrame, args);
    }    

    void *addressOfArgs() {
        return &args;
    }

    static size_t offsetOfScopeChain() {
        return offsetof(StackFrame, scopeChain_);
    }

    JSObject **addressOfScopeChain() {
        JS_ASSERT(flags_ & HAS_SCOPECHAIN);
        return &scopeChain_;
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

    static ptrdiff_t offsetOfCallee(JSFunction *fun) {
        JS_ASSERT(fun != NULL);
        return -(fun->nargs + 2) * sizeof(Value);
    }

    static ptrdiff_t offsetOfThis(JSFunction *fun) {
        return fun == NULL
               ? -1 * ptrdiff_t(sizeof(Value))
               : -(fun->nargs + 1) * ptrdiff_t(sizeof(Value));
    }

    static ptrdiff_t offsetOfFormalArg(JSFunction *fun, uintN i) {
        JS_ASSERT(i < fun->nargs);
        return (-(int)fun->nargs + i) * sizeof(Value);
    }

    static size_t offsetOfFixed(uintN i) {
        return sizeof(StackFrame) + i * sizeof(Value);
    }

#ifdef JS_METHODJIT
    mjit::JITScript *jit() {
        return script()->getJIT(isConstructing());
    }
#endif

    void methodjitStaticAsserts();
};

static const size_t VALUES_PER_STACK_FRAME = sizeof(StackFrame) / sizeof(Value);

static inline uintN
ToReportFlags(InitialFrameFlags initial)
{
    return uintN(initial & StackFrame::CONSTRUCTING);
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
    JSInlinedSite *inlined_;
    StackFrame *fp_;
  public:
    StackFrame *fp() const { return fp_; }
    JSInlinedSite *inlined() const { return inlined_; }

    
    static const size_t offsetOfFp = 3 * sizeof(void *);
    static const size_t offsetOfInlined = 2 * sizeof(void *);
    static void staticAssert() {
        JS_STATIC_ASSERT(offsetOfFp == offsetof(FrameRegs, fp_));
        JS_STATIC_ASSERT(offsetOfInlined == offsetof(FrameRegs, inlined_));
    }
    void clearInlined() { inlined_ = NULL; }

    
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

    
    void prepareToRun(StackFrame &fp, JSScript *script) {
        pc = script->code;
        sp = fp.slots() + script->nfixed;
        fp_ = &fp;
        inlined_ = NULL;
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
        JS_ASSERT_IF(regs, regs->fp());
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

  public:
    StackSpace();
    bool init();
    ~StackSpace();

    










    static const uintN ARGS_LENGTH_MAX = CAPACITY_VALS - (2 * BUFFER_VALS);

    
    Value *firstUnused() const { return seg_ ? seg_->end() : base_; }

    StackSegment &containingSegment(const StackFrame *target) const;

#ifdef JS_TRACER
    








    inline bool ensureEnoughSpaceToEnterTrace(JSContext *cx);
#endif

    







    static const size_t STACK_JIT_EXTRA = (VALUES_PER_STACK_FRAME + 18) * 10;

    













    inline Value *getStackLimit(JSContext *cx, MaybeReportError report);
    bool tryBumpLimit(JSContext *cx, Value *from, uintN nvals, Value **limit);

    
    void mark(JSTracer *trc);

    
    JS_FRIEND_API(size_t) committedSize();
};



class ContextStack
{
    StackSegment *seg_;
    StackSpace *space_;
    JSContext *cx_;

    







    bool onTop() const;

#ifdef DEBUG
    void assertSpaceInSync() const;
#else
    void assertSpaceInSync() const {}
#endif

    
    StackSegment *pushSegment(JSContext *cx);
    enum MaybeExtend { CAN_EXTEND = true, CANT_EXTEND = false };
    Value *ensureOnTop(JSContext *cx, MaybeReportError report, uintN nvars,
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

    





    bool hasfp() const                { return seg_ && seg_->maybeRegs(); }

    



    FrameRegs *maybeRegs() const      { return seg_ ? seg_->maybeRegs() : NULL; }
    StackFrame *maybefp() const       { return seg_ ? seg_->maybefp() : NULL; }

    
    FrameRegs &regs() const           { JS_ASSERT(hasfp()); return seg_->regs(); }
    StackFrame *fp() const            { JS_ASSERT(hasfp()); return seg_->fp(); }

    
    StackSpace &space() const    { assertSpaceInSync(); return *space_; }

    
    bool containsSlow(const StackFrame *target) const;

    

    





    bool pushInvokeArgs(JSContext *cx, uintN argc, InvokeArgsGuard *ag);

    
    bool pushInvokeFrame(JSContext *cx, const CallArgs &args,
                         InitialFrameFlags initial, InvokeFrameGuard *ifg);

    
    bool pushExecuteFrame(JSContext *cx, JSScript *script, const Value &thisv,
                          JSObject &scopeChain, ExecuteType type,
                          StackFrame *evalInFrame, ExecuteFrameGuard *efg);

    





    bool pushGeneratorFrame(JSContext *cx, JSGenerator *gen, GeneratorFrameGuard *gfg);

    








    bool pushDummyFrame(JSContext *cx, JSCompartment *dest, JSObject &scopeChain, DummyFrameGuard *dfg);

    




    bool pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                         JSObject &callee, JSFunction *fun, JSScript *script,
                         InitialFrameFlags initial);
    bool pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                         JSObject &callee, JSFunction *fun, JSScript *script,
                         InitialFrameFlags initial, Value **stackLimit);
    void popInlineFrame(FrameRegs &regs);

    
    void popFrameAfterOverflow();

    
    inline JSScript *currentScript(jsbytecode **pc = NULL) const;

    
    inline JSObject *currentScriptedScopeChain() const;

    







    StackFrame *getFixupFrame(JSContext *cx, MaybeReportError report,
                              const CallArgs &args, JSFunction *fun, JSScript *script,
                              void *ncode, InitialFrameFlags initial, Value **stackLimit);

    bool saveFrameChain();
    void restoreFrameChain();

    



    void repointRegs(FrameRegs *regs) { JS_ASSERT(hasfp()); seg_->repointRegs(regs); }

    

    




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
    JSContext    *cx_;
  public:
    enum SavedOption { STOP_AT_SAVED, GO_THROUGH_SAVED };
  private:
    SavedOption  savedOption_;

    enum State { DONE, SCRIPTED, NATIVE, IMPLICIT_NATIVE };
    State        state_;

    StackFrame   *fp_;
    CallArgsList *calls_;

    StackSegment *seg_;
    Value        *sp_;
    jsbytecode   *pc_;
    CallArgs     args_;

    void poisonRegs();
    void popFrame();
    void popCall();
    void settleOnNewSegment();
    void settleOnNewState();
    void startOnSegment(StackSegment *seg);

  public:
    StackIter(JSContext *cx, SavedOption = STOP_AT_SAVED);

    bool done() const { return state_ == DONE; }
    StackIter &operator++();

    bool operator==(const StackIter &rhs) const;
    bool operator!=(const StackIter &rhs) const { return !(*this == rhs); }

    bool isScript() const { JS_ASSERT(!done()); return state_ == SCRIPTED; }
    StackFrame *fp() const { JS_ASSERT(!done() && isScript()); return fp_; }
    Value      *sp() const { JS_ASSERT(!done() && isScript()); return sp_; }
    jsbytecode *pc() const { JS_ASSERT(!done() && isScript()); return pc_; }

    bool isNativeCall() const { JS_ASSERT(!done()); return state_ != SCRIPTED; }
    CallArgs nativeArgs() const { JS_ASSERT(!done() && isNativeCall()); return args_; }
};


class FrameRegsIter
{
    StackIter iter_;

    void settle() {
        while (!iter_.done() && !iter_.isScript())
            ++iter_;
    }

  public:
    FrameRegsIter(JSContext *cx, StackIter::SavedOption opt = StackIter::STOP_AT_SAVED)
        : iter_(cx, opt) { settle(); }

    bool done() const { return iter_.done(); }
    FrameRegsIter &operator++() { ++iter_; settle(); return *this; }

    bool operator==(const FrameRegsIter &rhs) const { return iter_ == rhs.iter_; }
    bool operator!=(const FrameRegsIter &rhs) const { return iter_ != rhs.iter_; }

    StackFrame *fp() const { return iter_.fp(); }
    Value      *sp() const { return iter_.sp(); }
    jsbytecode *pc() const { return iter_.pc(); }
};







class AllFramesIter
{
  public:
    AllFramesIter(StackSpace &space);

    bool done() const { return fp_ == NULL; }
    AllFramesIter& operator++();

    StackFrame *fp() const { return fp_; }

  private:
    StackSegment *seg_;
    StackFrame *fp_;
};

}  

#endif
