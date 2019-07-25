







































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
namespace detail { struct OOMCheck; }














































































class CallReceiver
{
#ifdef DEBUG
    mutable bool usedRval_;
#endif
  protected:
    Value *argv_;
    CallReceiver() {}
    CallReceiver(Value *argv) : argv_(argv) {
#ifdef DEBUG
        usedRval_ = false;
#endif
    }

  public:
    friend CallReceiver CallReceiverFromVp(Value *);
    friend CallReceiver CallReceiverFromArgv(Value *);
    Value *base() const { return argv_ - 2; }
    JSObject &callee() const { JS_ASSERT(!usedRval_); return argv_[-2].toObject(); }
    Value &calleev() const { JS_ASSERT(!usedRval_); return argv_[-2]; }
    Value &thisv() const { return argv_[-1]; }

    Value &rval() const {
#ifdef DEBUG
        usedRval_ = true;
#endif
        return argv_[-2];
    }

    void calleeHasBeenReset() const {
#ifdef DEBUG
        usedRval_ = false;
#endif
    }
};

JS_ALWAYS_INLINE CallReceiver
CallReceiverFromVp(Value *vp)
{
    return CallReceiver(vp + 2);
}

JS_ALWAYS_INLINE CallReceiver
CallReceiverFromArgv(Value *argv)
{
    return CallReceiver(argv);
}



class CallArgs : public CallReceiver
{
    uintN argc_;
  protected:
    CallArgs() {}
    CallArgs(uintN argc, Value *argv) : CallReceiver(argv), argc_(argc) {}
  public:
    friend CallArgs CallArgsFromVp(uintN, Value *);
    friend CallArgs CallArgsFromArgv(uintN, Value *);
    Value &operator[](unsigned i) const { JS_ASSERT(i < argc_); return argv_[i]; }
    Value *argv() const { return argv_; }
    uintN argc() const { return argc_; }
};

JS_ALWAYS_INLINE CallArgs
CallArgsFromVp(uintN argc, Value *vp)
{
    return CallArgs(argc, vp + 2);
}

JS_ALWAYS_INLINE CallArgs
CallArgsFromArgv(uintN argc, Value *argv)
{
    return CallArgs(argc, argv);
}



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

    
    js::Value           rval_;          
    jsbytecode          *prevpc_;       
    jsbytecode          *imacropc_;     
    void                *hookData_;     
    void                *annotation_;   

    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(StackFrame, rval_) % sizeof(js::Value) == 0);
        JS_STATIC_ASSERT(sizeof(StackFrame) % sizeof(js::Value) == 0);
    }

    inline void initPrev(JSContext *cx);
    jsbytecode *prevpcSlow();

  public:
    







    
    inline void initCallFrame(JSContext *cx, JSObject &callee, JSFunction *fun,
                              uint32 nactual, uint32 flags);

    
    inline void resetInvokeCallFrame();

    
    inline void initCallFrameCallerHalf(JSContext *cx, uint32 flags, void *ncode);
    inline void initCallFrameEarlyPrologue(JSFunction *fun, uint32 nactual);
    inline void initCallFrameLatePrologue();

    
    inline void initEvalFrame(JSContext *cx, JSScript *script, StackFrame *prev,
                              uint32 flags);
    inline void initGlobalFrame(JSScript *script, JSObject &chain, StackFrame *prev,
                                uint32 flags);

    
    inline void stealFrameAndSlots(js::Value *vp, StackFrame *otherfp,
                                   js::Value *othervp, js::Value *othersp);

    
    inline void initDummyFrame(JSContext *cx, JSObject &chain);

    










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

    







    js::Value *slots() const {
        return (js::Value *)(this + 1);
    }

    js::Value *base() const {
        return slots() + script()->nfixed;
    }

    js::Value &varSlot(uintN i) {
        JS_ASSERT(i < script()->nfixed);
        JS_ASSERT_IF(maybeFun(), i < script()->bindings.countVars());
        return slots()[i];
    }

    






    



    jsbytecode *pc(JSContext *cx, StackFrame *next = NULL);

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

    js::Value &formalArg(uintN i) const {
        JS_ASSERT(i < numFormalArgs());
        return formalArgs()[i];
    }

    js::Value *formalArgs() const {
        JS_ASSERT(hasArgs());
        return (js::Value *)this - numFormalArgs();
    }

    js::Value *formalArgsEnd() const {
        JS_ASSERT(hasArgs());
        return (js::Value *)this;
    }

    js::Value *maybeFormalArgs() const {
        return (flags_ & (FUNCTION | EVAL)) == FUNCTION
               ? formalArgs()
               : NULL;
    }

    inline uintN numActualArgs() const;
    inline js::Value *actualArgs() const;
    inline js::Value *actualArgsEnd() const;

    inline js::Value &canonicalActualArg(uintN i) const;
    template <class Op>
    inline bool forEachCanonicalActualArg(Op op, uintN start = 0, uintN count = uintN(-1));
    template <class Op> inline bool forEachFormalArg(Op op);

    inline void clearMissingArgs();

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

    













    js::Value &functionThis() const {
        JS_ASSERT(isFunctionFrame());
        if (isEvalFrame())
            return ((js::Value *)this)[-1];
        return formalArgs()[-1];
    }

    JSObject &constructorThis() const {
        JS_ASSERT(hasArgs());
        return formalArgs()[-1].toObject();
    }

    js::Value &globalThis() const {
        JS_ASSERT(isGlobalFrame());
        return ((js::Value *)this)[-1];
    }

    js::Value &thisValue() const {
        if (flags_ & (EVAL | GLOBAL))
            return ((js::Value *)this)[-1];
        return formalArgs()[-1];
    }

    






    js::Value &calleev() const {
        JS_ASSERT(isFunctionFrame());
        if (isEvalFrame())
            return ((js::Value *)this)[-2];
        return formalArgs()[-2];
    }

    JSObject &callee() const {
        JS_ASSERT(isFunctionFrame());
        return calleev().toObject();
    }

    JSObject *maybeCallee() const {
        return isFunctionFrame() ? &callee() : NULL;
    }

    js::CallReceiver callReceiver() const {
        return js::CallReceiverFromArgv(formalArgs());
    }

    





    bool getValidCalleeObject(JSContext *cx, js::Value *vp);

    























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

    

    const js::Value &returnValue() {
        if (!(flags_ & HAS_RVAL))
            rval_.setUndefined();
        return rval_;
    }

    void markReturnValue() {
        flags_ |= HAS_RVAL;
    }

    void setReturnValue(const js::Value &v) {
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

    



    bool isConstructing() const {
        return !!(flags_ & CONSTRUCTING);
    }

    uint32 isConstructingFlag() const {
        JS_ASSERT(isFunctionFrame());
        JS_ASSERT((flags_ & ~(CONSTRUCTING | FUNCTION)) == 0);
        return flags_;
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
        return -(fun->nargs + 2) * sizeof(js::Value);
    }

    static ptrdiff_t offsetOfThis(JSFunction *fun) {
        return fun == NULL
               ? -1 * ptrdiff_t(sizeof(js::Value))
               : -(fun->nargs + 1) * ptrdiff_t(sizeof(js::Value));
    }

    static ptrdiff_t offsetOfFormalArg(JSFunction *fun, uintN i) {
        JS_ASSERT(i < fun->nargs);
        return (-(int)fun->nargs + i) * sizeof(js::Value);
    }

    static size_t offsetOfFixed(uintN i) {
        return sizeof(StackFrame) + i * sizeof(js::Value);
    }

#ifdef JS_METHODJIT
    js::mjit::JITScript *jit() {
        return script()->getJIT(isConstructing());
    }
#endif

    void methodjitStaticAsserts();
};

static const size_t VALUES_PER_STACK_FRAME = sizeof(StackFrame) / sizeof(Value);

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

    
    void rebaseFromTo(StackFrame *from, StackFrame *to) {
        fp_ = to;
        sp = to->slots() + (sp - from->slots());
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
#ifdef XP_WIN
    mutable Value *commitEnd_;
#endif
    Value *end_;
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
    friend struct detail::OOMCheck;
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

    



    JSObject &varObjForFrame(const StackFrame *fp);

#ifdef JS_TRACER
    





    inline bool ensureEnoughSpaceToEnterTrace();
#endif

    






    static const size_t MAX_INLINE_CALLS = 3000;

    







    static const size_t STACK_QUOTA = MAX_INLINE_CALLS * (VALUES_PER_STACK_FRAME + 18);

    












    inline Value *getStackLimit(JSContext *cx);

    







    bool bumpLimitWithinQuota(JSContext *maybecx, StackFrame *base, Value *from, uintN nvals, Value **limit) const;

    



    bool bumpLimit(JSContext *cx, StackFrame *base, Value *from, uintN nvals, Value **limit) const;

    
    void mark(JSTracer *trc);
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
    bool getSegmentAndFrame(JSContext *cx, uintN vplen, uintN nslots,
                            FrameGuard *frameGuard) const;
    void pushSegmentAndFrame(FrameRegs &regs, FrameGuard *frameGuard);
    void pushSegmentAndFrameImpl(FrameRegs &regs, StackSegment &seg);
    void popSegmentAndFrame();
    void popSegmentAndFrameImpl();

    template <class Check>
    inline StackFrame *getCallFrame(JSContext *cx, Value *sp, uintN nactual,
                                    JSFunction *fun, JSScript *script, uint32 *pflags,
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
    bool running() const         { JS_ASSERT_IF(regs_, regs_->fp()); return !!regs_; }

    
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

    
    js::StackSegment *currentSegment() const {
        assertSegmentsInSync();
        return seg_;
    }

    
    inline JSObject &currentVarObj() const;

    
    inline StackFrame *findFrameAtLevel(uintN targetLevel) const;

#ifdef DEBUG
    
    bool contains(const StackFrame *fp) const;
#endif

    
    void saveActiveSegment();

    
    void restoreSegment();

    












    





    bool pushInvokeArgs(JSContext *cx, uintN argc, InvokeArgsGuard *ag);

    
    inline StackFrame *
    getInvokeFrame(JSContext *cx, const CallArgs &args,
                   JSFunction *fun, JSScript *script, uint32 *flags,
                   InvokeFrameGuard *frameGuard) const;
    void pushInvokeFrame(const CallArgs &args,
                         InvokeFrameGuard *frameGuard);

    
    bool getExecuteFrame(JSContext *cx, JSScript *script,
                         ExecuteFrameGuard *frameGuard) const;
    void pushExecuteFrame(JSObject *initialVarObj,
                          ExecuteFrameGuard *frameGuard);

    
    bool getGeneratorFrame(JSContext *cx, uintN vplen, uintN nslots,
                           GeneratorFrameGuard *frameGuard);
    void pushGeneratorFrame(FrameRegs &regs,
                            GeneratorFrameGuard *frameGuard);

    
    bool pushDummyFrame(JSContext *cx, JSObject &scopeChain,
                        DummyFrameGuard *frameGuard);

    





    inline StackFrame *
    getInlineFrame(JSContext *cx, Value *sp, uintN nactual,
                   JSFunction *fun, JSScript *script, uint32 *flags) const;
    inline StackFrame *
    getInlineFrameWithinLimit(JSContext *cx, Value *sp, uintN nactual,
                              JSFunction *fun, JSScript *script, uint32 *flags,
                              StackFrame *base, Value **limit) const;
    inline void pushInlineFrame(JSScript *script, StackFrame *fp, FrameRegs &regs);
    inline void popInlineFrame();

    
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





struct InvokeArgsAlreadyOnTheStack : CallArgs
{
    InvokeArgsAlreadyOnTheStack(uintN argc, Value *vp) : CallArgs(argc, vp + 2) {}
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
    friend class ContextStack;
    ContextStack *stack_;  
    StackSegment *seg_;
    Value *vp_;
    StackFrame *fp_;
  public:
    FrameGuard() : stack_(NULL), vp_(NULL), fp_(NULL) {}
    ~FrameGuard();
    bool pushed() const { return stack_ != NULL; }
    StackSegment *segment() const { return seg_; }
    Value *vp() const { return vp_; }
    StackFrame *fp() const { return fp_; }
};

class ExecuteFrameGuard : public FrameGuard
{
    friend class ContextStack;
    FrameRegs regs_;
};

class DummyFrameGuard : public FrameGuard
{
    friend class ContextStack;
    FrameRegs regs_;
};

class GeneratorFrameGuard : public FrameGuard
{};













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
    inline FrameRegsIter(JSContext *cx);

    bool done() const { return fp_ == NULL; }
    inline FrameRegsIter &operator++();

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
