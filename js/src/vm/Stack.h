







































#ifndef Stack_h__
#define Stack_h__

#include "jsfun.h"

struct JSContext;

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

class ArgumentsObject;

namespace mjit { struct JITScript; }














































































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
    uintN argc_;
  public:
    friend CallArgs CallArgsFromVp(uintN, Value *);
    friend CallArgs CallArgsFromArgv(uintN, Value *);
    friend CallArgs CallArgsFromSp(uintN, Value *);
    Value &operator[](unsigned i) const { JS_ASSERT(i < argc_); return argv_[i]; }
    Value *argv() const { return argv_; }
    uintN argc() const { return argc_; }
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



enum MaybeConstruct {
    NO_CONSTRUCT           =          0, 
    CONSTRUCT              =       0x80  
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
        HAS_PREVPC         =   0x100000   
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
    jsbytecode          *imacropc_;     
    void                *hookData_;     
    void                *annotation_;   

    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(StackFrame, rval_) % sizeof(Value) == 0);
        JS_STATIC_ASSERT(sizeof(StackFrame) % sizeof(Value) == 0);
    }

    inline void initPrev(JSContext *cx);
    jsbytecode *prevpcSlow();

  public:
    







    
    void initCallFrame(JSContext *cx, JSObject &callee, JSFunction *fun,
                       JSScript *script, uint32 nactual, StackFrame::Flags flags);

    
    void resetCallFrame(JSScript *script);

    
    void initJitFrameCallerHalf(JSContext *cx, StackFrame::Flags flags, void *ncode);
    void initJitFrameEarlyPrologue(JSFunction *fun, uint32 nactual);
    void initJitFrameLatePrologue();

    
    void initExecuteFrame(JSScript *script, StackFrame *prev, const Value &thisv,
                          JSObject &scopeChain, ExecuteType type);

    
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

    






    














    jsbytecode *pcQuadratic(JSContext *cx);

    jsbytecode *prevpc() {
        if (flags_ & HAS_PREVPC)
            return prevpc_;
        return prevpcSlow();
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

    inline JSObject &callObj() const;
    inline void setScopeChainNoCallObj(JSObject &obj);
    inline void setScopeChainWithOwnCallObj(JSObject &obj);

    



    inline void putActivationObjects();
    inline void markActivationObjectsAsPut();

    














    JSObject &varObj() {
        JSObject *obj = &scopeChain();
        while (!obj->isVarObj())
            obj = obj->getParent();
        return *obj;
    }

    






    JSCompartment *compartment() const {
        JS_ASSERT_IF(isScriptFrame(), scopeChain().compartment() == script()->compartment);
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

    



    MaybeConstruct isConstructing() const {
        JS_STATIC_ASSERT((int)CONSTRUCT == (int)CONSTRUCTING);
        JS_STATIC_ASSERT((int)NO_CONSTRUCT == 0);
        return MaybeConstruct(flags_ & CONSTRUCTING);
    }

    bool isDebuggerFrame() const {
        return !!(flags_ & DEBUGGER);
    }

    bool isDirectEvalOrDebuggerFrame() const {
        return (flags_ & (EVAL | DEBUGGER)) && !(flags_ & GLOBAL);
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
ToReportFlags(MaybeConstruct construct)
{
    return uintN(construct);
}

static inline StackFrame::Flags
ToFrameFlags(MaybeConstruct construct)
{
    JS_STATIC_ASSERT((int)CONSTRUCT == (int)StackFrame::CONSTRUCTING);
    JS_STATIC_ASSERT((int)NO_CONSTRUCT == 0);
    return StackFrame::Flags(construct);
}

static inline MaybeConstruct
MaybeConstructFromBool(bool b)
{
    return b ? CONSTRUCT : NO_CONSTRUCT;
}

inline StackFrame *          Valueify(JSStackFrame *fp) { return (StackFrame *)fp; }
static inline JSStackFrame * Jsvalify(StackFrame *fp)   { return (JSStackFrame *)fp; }



class FrameRegs
{
  public:
    Value *sp;
    jsbytecode *pc;
  private:
    StackFrame *fp_;
  public:
    StackFrame *fp() const { return fp_; }

    
    static const size_t offsetOfFp = 2 * sizeof(void *);
    static void staticAssert() {
        JS_STATIC_ASSERT(offsetOfFp == offsetof(FrameRegs, fp_));
    }

    
    void rebaseFromTo(const FrameRegs &from, StackFrame *to) {
        fp_ = to;
        sp = to->slots() + (from.sp - from.fp_->slots());
        pc = from.pc;
    }

    
    void popFrame(Value *newsp) {
        pc = fp_->prevpc();
        sp = newsp;
        fp_ = fp_->prev();
    }

    
    void popPartialFrame(Value *newsp) {
        sp = newsp;
        fp_ = fp_->prev();
    }

    
    void prepareToRun(StackFrame *fp, JSScript *script) {
        pc = script->code;
        sp = fp->slots() + script->nfixed;
        fp_ = fp;
    }

    
    void initDummyFrame(StackFrame *fp) {
        pc = NULL;
        sp = fp->slots();
        fp_ = fp;
    }
};



struct StackOverride
{
    Value         *top;
#ifdef DEBUG
    StackSegment  *seg;
    StackFrame    *frame;
#endif
};



class StackSpace
{
    Value         *base_;
    mutable Value *commitEnd_;
    Value         *end_;
    StackSegment  *seg_;
    StackOverride override_;

    static const size_t CAPACITY_VALS  = 512 * 1024;
    static const size_t CAPACITY_BYTES = CAPACITY_VALS * sizeof(Value);
    static const size_t COMMIT_VALS    = 16 * 1024;
    static const size_t COMMIT_BYTES   = COMMIT_VALS * sizeof(Value);

    static void staticAsserts() {
        JS_STATIC_ASSERT(CAPACITY_VALS % COMMIT_VALS == 0);
    }

#ifdef XP_WIN
    JS_FRIEND_API(bool) bumpCommit(JSContext *maybecx, Value *from, ptrdiff_t nvals) const;
#endif

    friend class ContextStack;
    friend class OOMCheck;
    inline bool ensureSpace(JSContext *maybecx, Value *from, ptrdiff_t nvals) const;
    void pushSegment(StackSegment &seg);
    void popSegment();
    inline void pushOverride(Value *top, StackOverride *prev);
    inline void popOverride(const StackOverride &prev);

  public:
    StackSpace();
    bool init();
    ~StackSpace();

    
    StackSegment *currentSegment() const { return seg_; }
    Value *firstUnused() const;

    
    inline Value *activeFirstUnused() const;

    
    StackSegment &containingSegment(const StackFrame *target) const;

#ifdef JS_TRACER
    





    inline bool ensureEnoughSpaceToEnterTrace();
#endif

    













    inline Value *getStackLimit(JSContext *cx);
    bool tryBumpLimit(JSContext *maybecx, Value *from, uintN nvals, Value **limit);

    
    void mark(JSTracer *trc);

    
    JS_FRIEND_API(size_t) committedSize();
};











class NoCheck
{
  public:
    bool operator()(JSContext *, StackSpace &, Value *, uintN) { return true; }
};

class OOMCheck
{
  public:
    bool operator()(JSContext *cx, StackSpace &space, Value *from, uintN nvals);
};

class LimitCheck
{
    Value **limit;
  public:
    LimitCheck(Value **limit) : limit(limit) {}
    bool operator()(JSContext *cx, StackSpace &space, Value *from, uintN nvals);
};



class ContextStack
{
    FrameRegs *regs_;
    StackSegment *seg_;
    StackSpace *space_;
    JSContext *cx_;

    





    void notifyIfNoCodeRunning();

    




    inline bool isCurrentAndActive() const;

#ifdef DEBUG
    void assertSegmentsInSync() const;
    void assertSpaceInSync() const;
#else
    void assertSegmentsInSync() const {}
    void assertSpaceInSync() const {}
#endif

    friend class FrameGuard;
    StackFrame *getSegmentAndFrame(JSContext *cx, uintN vplen, uintN nslots,
                                   FrameGuard *frameGuard) const;
    void pushSegmentAndFrame(FrameGuard *frameGuard);
    void pushSegmentAndFrameImpl(FrameRegs &regs, StackSegment &seg);
    void popSegmentAndFrame();
    void popSegmentAndFrameImpl();

    friend class GeneratorFrameGuard;
    void popGeneratorFrame(GeneratorFrameGuard *gfg);

    template <class Check>
    inline StackFrame *getCallFrame(JSContext *cx, const CallArgs &args,
                                    JSFunction *fun, JSScript *script,
                                    StackFrame::Flags *pflags,
                                    Check check) const;

    friend class InvokeArgsGuard;
    bool pushInvokeArgsSlow(JSContext *cx, uintN argc, InvokeArgsGuard *argsGuard);
    void popInvokeArgsSlow(const InvokeArgsGuard &argsGuard);
    inline void popInvokeArgs(const InvokeArgsGuard &argsGuard);

    friend class InvokeFrameGuard;
    void pushInvokeFrameSlow(InvokeFrameGuard *frameGuard);
    void popInvokeFrameSlow(const InvokeFrameGuard &frameGuard);
    inline void popInvokeFrame(const InvokeFrameGuard &frameGuard);

  public:
    ContextStack(JSContext *cx);
    ~ContextStack();

    



    bool empty() const           { JS_ASSERT_IF(regs_, seg_); return !seg_; }

    





    bool hasfp() const           { JS_ASSERT_IF(regs_, regs_->fp()); return !!regs_; }

    
    FrameRegs &regs() const      { JS_ASSERT(regs_); return *regs_; }

    
    FrameRegs *maybeRegs() const { return regs_; }
    StackFrame *fp() const       { return regs_->fp(); }
    StackFrame *maybefp() const  { return regs_ ? regs_->fp() : NULL; }

    
    StackSpace &space() const    { assertSpaceInSync(); return *space_; }

    




    void threadReset();

    



    void repointRegs(FrameRegs *regs) {
        JS_ASSERT_IF(regs, regs->fp());
        regs_ = regs;
    }

    
    StackSegment *currentSegment() const {
        assertSegmentsInSync();
        return seg_;
    }

    
    inline StackFrame *findFrameAtLevel(uintN targetLevel) const;

#ifdef DEBUG
    
    bool contains(const StackFrame *fp) const;
#endif

    
    void saveActiveSegment();

    
    void restoreSegment();

    





    bool pushInvokeArgs(JSContext *cx, uintN argc, InvokeArgsGuard *ag);

    
    bool pushInvokeFrame(JSContext *cx, const CallArgs &args, MaybeConstruct,
                         JSObject &callee, JSFunction *fun, JSScript *script,
                         InvokeFrameGuard *ifg);

    
    bool pushExecuteFrame(JSContext *cx, JSScript *script, const Value &thisv,
                          JSObject &scopeChain, ExecuteType type,
                          StackFrame *evalInFrame, ExecuteFrameGuard *efg);

    
    bool pushGeneratorFrame(JSContext *cx, JSGenerator *gen, GeneratorFrameGuard *gfg);

    
    bool pushDummyFrame(JSContext *cx, JSObject &scopeChain, DummyFrameGuard *dfg);

    




    template <class Check>
    bool pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                         JSObject &callee, JSFunction *fun, JSScript *script,
                         MaybeConstruct construct, Check check);
    void popInlineFrame();

    







    StackFrame *getFixupFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                              JSFunction *fun, JSScript *script, void *ncode,
                              MaybeConstruct construct, LimitCheck check);

    
    static size_t offsetOfRegs() { return offsetof(ContextStack, regs_); }
};



class InvokeArgsGuard : public CallArgs
{
    friend class ContextStack;
    ContextStack     *stack_;  
    StackSegment     *seg_;    
    StackOverride    prevOverride_;
  public:
    InvokeArgsGuard() : stack_(NULL), seg_(NULL) {}
    ~InvokeArgsGuard();
    bool pushed() const { return stack_ != NULL; }
};

class InvokeFrameGuard

{
    friend class ContextStack;
    ContextStack *stack_;  
    FrameRegs regs_;
    FrameRegs *prevRegs_;
  public:
    InvokeFrameGuard() : stack_(NULL) {}
    ~InvokeFrameGuard();
    bool pushed() const { return stack_ != NULL; }
    void pop();
    StackFrame *fp() const { return regs_.fp(); }
};


class FrameGuard
{
  protected:
    friend class ContextStack;
    ContextStack *stack_;  
    StackSegment *seg_;
    FrameRegs regs_;
  public:
    FrameGuard() : stack_(NULL) {}
    ~FrameGuard();
    bool pushed() const { return stack_ != NULL; }
    StackFrame *fp() const { return regs_.fp(); }
};

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
    ~GeneratorFrameGuard();
};













class FrameRegsIter
{
    JSContext    *cx_;
    StackSegment *seg_;
    StackFrame   *fp_;
    Value        *sp_;
    jsbytecode   *pc_;

    void initSlow();
    void incSlow(StackFrame *oldfp);

  public:
    FrameRegsIter(JSContext *cx);

    bool done() const { return fp_ == NULL; }
    FrameRegsIter &operator++();
    bool operator==(const FrameRegsIter &rhs) const;
    bool operator!=(const FrameRegsIter &rhs) const { return !(*this == rhs); }

    StackFrame *fp() const { return fp_; }
    Value *sp() const { return sp_; }
    jsbytecode *pc() const { return pc_; }
};




class AllFramesIter
{
public:
    AllFramesIter(JSContext *cx);

    bool done() const { return fp_ == NULL; }
    AllFramesIter& operator++();

    StackFrame *fp() const { return fp_; }

private:
    StackSegment *seg_;
    StackFrame *fp_;
};

}  

#endif
