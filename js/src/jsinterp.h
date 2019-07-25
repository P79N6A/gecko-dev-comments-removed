







































#ifndef jsinterp_h___
#define jsinterp_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsfun.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jsvalue.h"

struct JSFrameRegs
{
    STATIC_SKIP_INFERENCE
    js::Value       *sp;                  
    jsbytecode      *pc;                  
    JSStackFrame    *fp;                  
};


enum JSInterpFlags
{
    JSINTERP_RECORD            =     0x1, 
    JSINTERP_SAFEPOINT         =     0x2  
};


enum JSFrameFlags
{
    
    JSFRAME_GLOBAL             =     0x1, 
    JSFRAME_FUNCTION           =     0x2, 
    JSFRAME_DUMMY              =     0x4, 

    
    JSFRAME_EVAL               =     0x8, 
    JSFRAME_DEBUGGER           =    0x10, 
    JSFRAME_GENERATOR          =    0x20, 
    JSFRAME_FLOATING_GENERATOR =    0x40, 
    JSFRAME_CONSTRUCTING       =    0x80, 

    
    JSFRAME_ASSIGNING          =   0x100, 
    JSFRAME_YIELDING           =   0x200, 
    JSFRAME_BAILED_AT_RETURN   =   0x400, 

    
    JSFRAME_OVERRIDE_ARGS      =  0x1000, 
    JSFRAME_OVERFLOW_ARGS      =  0x2000, 
    JSFRAME_UNDERFLOW_ARGS     =  0x4000, 

    
    JSFRAME_HAS_IMACRO_PC      =   0x8000, 
    JSFRAME_HAS_CALL_OBJ       =  0x10000, 
    JSFRAME_HAS_ARGS_OBJ       =  0x20000, 
    JSFRAME_HAS_HOOK_DATA      =  0x40000, 
    JSFRAME_HAS_ANNOTATION     =  0x80000, 
    JSFRAME_HAS_RVAL           = 0x100000, 
    JSFRAME_HAS_SCOPECHAIN     = 0x200000, 
    JSFRAME_HAS_PREVPC         = 0x400000  
};





struct JSStackFrame
{
  private:
    mutable uint32      flags_;         
    union {                             
        JSScript        *script;        
        JSFunction      *fun;           
    } exec;
    union {                             
        uintN           nactual;        
        JSObject        *obj;           
        JSScript        *script;        
    } args;
    mutable JSObject    *scopeChain_;   
    JSStackFrame        *prev_;         
    void                *ncode_;        

    
    js::Value           rval_;          
    jsbytecode          *prevpc_;       
    jsbytecode          *imacropc_;     
    void                *hookData_;     
    void                *annotation_;   

#if JS_BITS_PER_WORD == 32
    void                *padding;
#endif

    friend class js::StackSpace;
    friend class js::FrameRegsIter;
    friend struct JSContext;

    inline void initPrev(JSContext *cx);

  public:
    















    bool isFunctionFrame() const {
        return !!(flags_ & JSFRAME_FUNCTION);
    }

    bool isGlobalFrame() const {
        return !!(flags_ & JSFRAME_GLOBAL);
    }

    bool isDummyFrame() const {
        return !!(flags_ & JSFRAME_DUMMY);
    }

    bool isScriptFrame() const {
        return !!(flags_ & (JSFRAME_FUNCTION | JSFRAME_GLOBAL));
    }

    bool isEvalFrame() const {
        JS_ASSERT_IF(flags_ & JSFRAME_EVAL, isScriptFrame());
        return flags_ & JSFRAME_EVAL;
    }

    









    
    inline void initCallFrame(JSContext *cx, JSObject &callee, JSFunction *fun,
                              uint32 nactual, uint32 flags);

    
    inline void initCallFrameCallerHalf(JSContext *cx, uint32 nactual, uint32 flags);
    inline void initCallFrameEarlyPrologue(JSFunction *fun, void *ncode);
    inline void initCallFrameLatePrologue();

    
    inline void initEvalFrame(JSContext *cx, JSScript *script, JSStackFrame *prev,
                              uint32 flags);
    inline void initGlobalFrame(JSScript *script, JSObject &chain, uint32 flags);

    
    inline void stealFrameAndSlots(js::Value *vp, JSStackFrame *otherfp,
                                   js::Value *othervp, js::Value *othersp);

    
    inline void initDummyFrame(JSContext *cx, JSObject &chain);

    












    JSStackFrame *prev() const {
        return prev_;
    }

    inline void resetGeneratorPrev(JSContext *cx);

    







    js::Value *slots() const {
        return (js::Value *)(this + 1);
    }

    js::Value *base() const {
        return slots() + script()->nfixed;
    }

    






    



    jsbytecode *pc(JSContext *cx, JSStackFrame *next = NULL);

    jsbytecode *prevpc() {
        JS_ASSERT((prev_ != NULL) && (flags_ & JSFRAME_HAS_PREVPC));
        return prevpc_;
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
        return isFunctionFrame() && !isEvalFrame();
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
        return (flags_ & (JSFRAME_FUNCTION | JSFRAME_EVAL)) == JSFRAME_FUNCTION
               ? formalArgs()
               : NULL;
    }

    inline uintN numActualArgs() const;
    inline js::Value *actualArgs() const;
    inline js::Value *actualArgsEnd() const;

    inline js::Value &canonicalActualArg(uintN i) const;
    template <class Op> inline void forEachCanonicalActualArg(Op op);
    template <class Op> inline void forEachFormalArg(Op op);

    
    bool hasArgsObj() const {
        return !!(flags_ & JSFRAME_HAS_ARGS_OBJ);
    }

    JSObject &argsObj() const {
        JS_ASSERT(hasArgsObj());
        JS_ASSERT(!isEvalFrame());
        return *args.obj;
    }

    JSObject *maybeArgsObj() const {
        return hasArgsObj() ? &argsObj() : NULL;
    }

    inline void setArgsObj(JSObject &obj);
    inline void clearArgsObj();

    













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
        if (flags_ & (JSFRAME_EVAL | JSFRAME_GLOBAL))
            return ((js::Value *)this)[-1];
        return formalArgs()[-1];
    }

    inline JSObject *computeThisObject(JSContext *cx);

    






    js::Value &calleeValue() const {
        JS_ASSERT(isFunctionFrame());
        if (isEvalFrame())
            return ((js::Value *)this)[-2];
        return formalArgs()[-2];
    }

    JSObject &callee() const {
        JS_ASSERT(isFunctionFrame());
        return calleeValue().toObject();
    }

    JSObject *maybeCallee() const {
        return isFunctionFrame() ? &callee() : NULL;
    }

    





    bool getValidCalleeObject(JSContext *cx, js::Value *vp);

    

















    JSObject &scopeChain() const {
        JS_ASSERT_IF(!(flags_ & JSFRAME_HAS_SCOPECHAIN), isFunctionFrame());
        if (!(flags_ & JSFRAME_HAS_SCOPECHAIN)) {
            scopeChain_ = callee().getParent();
            flags_ |= JSFRAME_HAS_SCOPECHAIN;
        }
        return *scopeChain_;
    }

    bool hasCallObj() const {
        return !!(flags_ & JSFRAME_HAS_CALL_OBJ);
    }

    inline JSObject &callObj() const;
    inline JSObject *maybeCallObj() const;
    inline void setScopeChainNoCallObj(JSObject &obj);
    inline void setScopeChainAndCallObj(JSObject &obj);
    inline void clearCallObj();

    







    bool hasImacropc() const {
        return flags_ & JSFRAME_HAS_IMACRO_PC;
    }

    jsbytecode *imacropc() const {
        JS_ASSERT(hasImacropc());
        return imacropc_;
    }

    jsbytecode *maybeImacropc() const {
        return hasImacropc() ? imacropc() : NULL;
    }

    void clearImacropc() {
        flags_ &= ~JSFRAME_HAS_IMACRO_PC;
    }

    void setImacropc(jsbytecode *pc) {
        JS_ASSERT(pc);
        JS_ASSERT(!(flags_ & JSFRAME_HAS_IMACRO_PC));
        imacropc_ = pc;
        flags_ |= JSFRAME_HAS_IMACRO_PC;
    }

    

    void* annotation() const {
        return (flags_ & JSFRAME_HAS_ANNOTATION) ? annotation_ : NULL;
    }

    void setAnnotation(void *annot) {
        flags_ |= JSFRAME_HAS_ANNOTATION;
        annotation_ = annot;
    }

    

    bool hasHookData() const {
        return !!(flags_ & JSFRAME_HAS_HOOK_DATA);
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
        flags_ |= JSFRAME_HAS_HOOK_DATA;
    }

    

    const js::Value& returnValue() {
        if (!(flags_ & JSFRAME_HAS_RVAL))
            rval_.setUndefined();
        return rval_;
    }

    void markReturnValue() {
        flags_ |= JSFRAME_HAS_RVAL;
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
        return !!(flags_ & JSFRAME_GENERATOR);
    }

    bool isFloatingGenerator() const {
        JS_ASSERT_IF(flags_ & JSFRAME_FLOATING_GENERATOR, isGeneratorFrame());
        return !!(flags_ & JSFRAME_FLOATING_GENERATOR);
    }

    void initFloatingGenerator() {
        JS_ASSERT(!(flags_ & JSFRAME_GENERATOR));
        flags_ |= (JSFRAME_GENERATOR | JSFRAME_FLOATING_GENERATOR);
    }

    void unsetFloatingGenerator() {
        flags_ &= ~JSFRAME_FLOATING_GENERATOR;
    }

    void setFloatingGenerator() {
        flags_ |= JSFRAME_FLOATING_GENERATOR;
    }

    



    bool isConstructing() const {
        return !!(flags_ & JSFRAME_CONSTRUCTING);
    }

    uint32 isConstructingFlag() const {
        JS_ASSERT(isFunctionFrame());
        JS_ASSERT((flags_ & ~(JSFRAME_CONSTRUCTING | JSFRAME_FUNCTION)) == 0);
        return flags_;
    }

    bool isDebuggerFrame() const {
        return !!(flags_ & JSFRAME_DEBUGGER);
    }

    bool isEvalOrDebuggerFrame() const {
        return !!(flags_ & (JSFRAME_EVAL | JSFRAME_DEBUGGER));
    }

    bool hasOverriddenArgs() const {
        return !!(flags_ & JSFRAME_OVERRIDE_ARGS);
    }

    bool hasOverflowArgs() const {
        return !!(flags_ & JSFRAME_OVERFLOW_ARGS);
    }

    void setOverriddenArgs() {
        flags_ |= JSFRAME_OVERRIDE_ARGS;
    }

    bool isAssigning() const {
        return !!(flags_ & JSFRAME_ASSIGNING);
    }

    void setAssigning() {
        flags_ |= JSFRAME_ASSIGNING;
    }

    void clearAssigning() {
        flags_ &= ~JSFRAME_ASSIGNING;
    }

    bool isYielding() {
        return !!(flags_ & JSFRAME_YIELDING);
    }

    void setYielding() {
        flags_ |= JSFRAME_YIELDING;
    }

    void clearYielding() {
        flags_ &= ~JSFRAME_YIELDING;
    }

    bool isBailedAtReturn() const {
        return flags_ & JSFRAME_BAILED_AT_RETURN;
    }

    void setBailedAtReturn() {
        flags_ |= JSFRAME_BAILED_AT_RETURN;
    }

    











    inline JSObject &varobj(js::StackSegment *seg) const;
    inline JSObject &varobj(JSContext *cx) const;

    

    static size_t offsetOfFlags() {
        return offsetof(JSStackFrame, flags_);
    }

    static size_t offsetOfExec() {
        return offsetof(JSStackFrame, exec);
    }

    void *addressOfArgs() {
        return &args;
    }

    static size_t offsetOfScopeChain() {
        return offsetof(JSStackFrame, scopeChain_);
    }

    JSObject **addressOfScopeChain() {
        JS_ASSERT(flags_ & JSFRAME_HAS_SCOPECHAIN);
        return &scopeChain_;
    }

    static size_t offsetOfPrev() {
        return offsetof(JSStackFrame, prev_);
    }

    static size_t offsetOfReturnValue() {
        return offsetof(JSStackFrame, rval_);
    }

    static ptrdiff_t offsetOfncode() {
        return offsetof(JSStackFrame, ncode_);
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
        return sizeof(JSStackFrame) + i * sizeof(js::Value);
    }

    

    void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(JSStackFrame, rval_) % sizeof(js::Value) == 0);
        JS_STATIC_ASSERT(sizeof(JSStackFrame) % sizeof(js::Value) == 0);
    }

    void methodjitStaticAsserts();

#ifdef DEBUG
    
    static JSObject *const sInvalidScopeChain;
#endif
};

namespace js {

static const size_t VALUES_PER_STACK_FRAME = sizeof(JSStackFrame) / sizeof(Value);

} 


extern JSObject *
js_GetBlockChain(JSContext *cx, JSStackFrame *fp);

extern JSObject *
js_GetBlockChainFast(JSContext *cx, JSStackFrame *fp, JSOp op, size_t oplen);








extern JSObject *
js_GetScopeChain(JSContext *cx, JSStackFrame *fp);

extern JSObject *
js_GetScopeChainFast(JSContext *cx, JSStackFrame *fp, JSOp op, size_t oplen);










extern JSBool
js_GetPrimitiveThis(JSContext *cx, js::Value *vp, js::Class *clasp,
                    const js::Value **vpp);

namespace js {

inline void
PutActivationObjects(JSContext *cx, JSStackFrame *fp);







extern bool
ComputeThisFromArgv(JSContext *cx, js::Value *argv);

JS_ALWAYS_INLINE JSObject *
ComputeThisFromVp(JSContext *cx, js::Value *vp)
{
    extern bool ComputeThisFromArgv(JSContext *, js::Value *);
    return ComputeThisFromArgv(cx, vp + 2) ? &vp[1].toObject() : NULL;
}

JS_ALWAYS_INLINE bool
ComputeThisFromVpInPlace(JSContext *cx, js::Value *vp)
{
    extern bool ComputeThisFromArgv(JSContext *, js::Value *);
    return ComputeThisFromArgv(cx, vp + 2);
}

JS_ALWAYS_INLINE bool
PrimitiveThisTest(JSFunction *fun, const Value &v)
{
    uint16 flags = fun->flags;
    return (v.isString() && !!(flags & JSFUN_THISP_STRING)) ||
           (v.isNumber() && !!(flags & JSFUN_THISP_NUMBER)) ||
           (v.isBoolean() && !!(flags & JSFUN_THISP_BOOLEAN));
}





struct CallArgs
{
    Value *argv_;
    uintN argc_;
  protected:
    CallArgs() {}
    CallArgs(Value *argv, uintN argc) : argv_(argv), argc_(argc) {}
  public:
    Value *base() const { return argv_ - 2; }
    Value &callee() const { return argv_[-2]; }
    Value &thisv() const { return argv_[-1]; }
    Value &operator[](unsigned i) const { JS_ASSERT(i < argc_); return argv_[i]; }
    Value *argv() const { return argv_; }
    uintN argc() const { return argc_; }
    Value &rval() const { return argv_[-2]; }

    bool computeThis(JSContext *cx) const {
        return ComputeThisFromArgv(cx, argv_);
    }
};









extern JS_REQUIRES_STACK bool
Invoke(JSContext *cx, const CallArgs &args, uint32 flags);














#define JSINVOKE_CONSTRUCT      JSFRAME_CONSTRUCTING




#define JSINVOKE_FUNFLAGS       JSINVOKE_CONSTRUCT






extern bool
ExternalInvoke(JSContext *cx, const Value &thisv, const Value &fval,
               uintN argc, Value *argv, Value *rval);

static JS_ALWAYS_INLINE bool
ExternalInvoke(JSContext *cx, JSObject *obj, const Value &fval,
               uintN argc, Value *argv, Value *rval)
{
    return ExternalInvoke(cx, ObjectOrNullValue(obj), fval, argc, argv, rval);
}

extern bool
ExternalGetOrSet(JSContext *cx, JSObject *obj, jsid id, const Value &fval,
                 JSAccessMode mode, uintN argc, Value *argv, Value *rval);









extern JS_REQUIRES_STACK bool
InvokeConstructor(JSContext *cx, const CallArgs &args);

extern JS_REQUIRES_STACK bool
InvokeConstructorWithGivenThis(JSContext *cx, JSObject *thisobj, const Value &fval,
                               uintN argc, Value *argv, Value *rval);





extern JS_FORCES_STACK bool
Execute(JSContext *cx, JSObject *chain, JSScript *script,
        JSStackFrame *prev, uintN flags, Value *result);





extern JS_REQUIRES_STACK JS_NEVER_INLINE bool
Interpret(JSContext *cx, JSStackFrame *stopFp, uintN inlineCallCount = 0, uintN interpFlags = 0);

extern JS_REQUIRES_STACK bool
RunScript(JSContext *cx, JSScript *script, JSStackFrame *fp);

#define JSPROP_INITIALIZER 0x100   /* NB: Not a valid property attribute. */

extern bool
CheckRedeclaration(JSContext *cx, JSObject *obj, jsid id, uintN attrs,
                   JSObject **objp, JSProperty **propp);

extern bool
StrictlyEqual(JSContext *cx, const Value &lval, const Value &rval);


extern bool
SameValue(const Value &v1, const Value &v2, JSContext *cx);

extern JSType
TypeOfValue(JSContext *cx, const Value &v);

inline bool
InstanceOf(JSContext *cx, JSObject *obj, Class *clasp, Value *argv)
{
    if (obj && obj->getClass() == clasp)
        return true;
    extern bool InstanceOfSlow(JSContext *, JSObject *, Class *, Value *);
    return InstanceOfSlow(cx, obj, clasp, argv);
}

extern JSBool
HasInstance(JSContext *cx, JSObject *obj, const js::Value *v, JSBool *bp);

inline void *
GetInstancePrivate(JSContext *cx, JSObject *obj, Class *clasp, Value *argv)
{
    if (!InstanceOf(cx, obj, clasp, argv))
        return NULL;
    return obj->getPrivate();
}

extern bool
ValueToId(JSContext *cx, const Value &v, jsid *idp);








extern const js::Value &
GetUpvar(JSContext *cx, uintN level, js::UpvarCookie cookie);

} 












#ifndef JS_LONE_INTERPRET
# ifdef _MSC_VER
#  define JS_LONE_INTERPRET 0
# else
#  define JS_LONE_INTERPRET 1
# endif
#endif

#define JS_MAX_INLINE_CALL_COUNT 3000

#if !JS_LONE_INTERPRET
# define JS_STATIC_INTERPRET    static
#else
# define JS_STATIC_INTERPRET

extern JS_REQUIRES_STACK JSBool
js_EnterWith(JSContext *cx, jsint stackIndex, JSOp op, size_t oplen);

extern JS_REQUIRES_STACK void
js_LeaveWith(JSContext *cx);







extern JSBool
js_DoIncDec(JSContext *cx, const JSCodeSpec *cs, js::Value *vp, js::Value *vp2);





extern JS_REQUIRES_STACK void
js_TraceOpcode(JSContext *cx);




extern void
js_MeterOpcodePair(JSOp op1, JSOp op2);

extern void
js_MeterSlotOpcode(JSOp op, uint32 slot);

#endif 




extern JS_REQUIRES_STACK JSBool
js_UnwindScope(JSContext *cx, jsint stackDepth, JSBool normalUnwind);

extern JSBool
js_OnUnknownMethod(JSContext *cx, js::Value *vp);

extern JS_REQUIRES_STACK js::Class *
js_IsActiveWithOrBlock(JSContext *cx, JSObject *obj, int stackDepth);

#endif 
