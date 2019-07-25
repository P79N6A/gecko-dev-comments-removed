







































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


enum JSInterpMode
{
    JSINTERP_NORMAL            =     0, 
    JSINTERP_RECORD            =     1, 
    JSINTERP_SAFEPOINT         =     2, 
    JSINTERP_PROFILE           =     3  
};


enum JSFrameFlags
{
    
    JSFRAME_GLOBAL             =      0x1, 
    JSFRAME_FUNCTION           =      0x2, 
    JSFRAME_DUMMY              =      0x4, 

    
    JSFRAME_EVAL               =      0x8, 
    JSFRAME_DEBUGGER           =     0x10, 
    JSFRAME_GENERATOR          =     0x20, 
    JSFRAME_FLOATING_GENERATOR =     0x40, 
    JSFRAME_CONSTRUCTING       =     0x80, 

    
    JSFRAME_YIELDING           =    0x200, 
    JSFRAME_FINISHED_IN_INTERP =    0x400, 

    
    JSFRAME_OVERRIDE_ARGS      =   0x1000, 
    JSFRAME_OVERFLOW_ARGS      =   0x2000, 
    JSFRAME_UNDERFLOW_ARGS     =   0x4000, 

    
    JSFRAME_HAS_IMACRO_PC      =   0x8000, 
    JSFRAME_HAS_CALL_OBJ       =  0x10000, 
    JSFRAME_HAS_ARGS_OBJ       =  0x20000, 
    JSFRAME_HAS_HOOK_DATA      =  0x40000, 
    JSFRAME_HAS_ANNOTATION     =  0x80000, 
    JSFRAME_HAS_RVAL           = 0x100000, 
    JSFRAME_HAS_SCOPECHAIN     = 0x200000, 
    JSFRAME_HAS_PREVPC         = 0x400000  
};

namespace js { namespace mjit { struct JITScript; } }





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
        bool retval = !!(flags_ & (JSFRAME_FUNCTION | JSFRAME_GLOBAL));
        JS_ASSERT(retval == !isDummyFrame());
        return retval;
    }

    











    bool isEvalFrame() const {
        JS_ASSERT_IF(flags_ & JSFRAME_EVAL, isScriptFrame());
        return flags_ & JSFRAME_EVAL;
    }

    bool isNonEvalFunctionFrame() const {
        return (flags_ & (JSFRAME_FUNCTION | JSFRAME_EVAL)) == JSFRAME_FUNCTION;
    }

    bool isStrictEvalFrame() const {
        return isEvalFrame() && script()->strictModeCode;
    }

    bool isNonStrictEvalFrame() const {
        return isEvalFrame() && !script()->strictModeCode;
    }

    









    
    inline void initCallFrame(JSContext *cx, JSObject &callee, JSFunction *fun,
                              uint32 nactual, uint32 flags);

    
    inline void resetInvokeCallFrame();

    
    inline void initCallFrameCallerHalf(JSContext *cx, uint32 flags, void *ncode);
    inline void initCallFrameEarlyPrologue(JSFunction *fun, uint32 nactual);
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

    js::Value &varSlot(uintN i) {
        JS_ASSERT(i < script()->nfixed);
        JS_ASSERT_IF(maybeFun(), i < script()->bindings.countVars());
        return slots()[i];
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
        return (flags_ & (JSFRAME_FUNCTION | JSFRAME_EVAL)) == JSFRAME_FUNCTION
               ? formalArgs()
               : NULL;
    }

    inline uintN numActualArgs() const;
    inline js::Value *actualArgs() const;
    inline js::Value *actualArgsEnd() const;

    inline js::Value &canonicalActualArg(uintN i) const;

    



    template <class Op> inline bool forEachCanonicalActualArg(Op op);
    template <class Op> inline bool forEachFormalArg(Op op);

    inline void clearMissingArgs();

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
        JS_ASSERT_IF(!(flags_ & JSFRAME_HAS_SCOPECHAIN), isFunctionFrame());
        if (!(flags_ & JSFRAME_HAS_SCOPECHAIN)) {
            scopeChain_ = callee().getParent();
            flags_ |= JSFRAME_HAS_SCOPECHAIN;
        }
        return *scopeChain_;
    }

    bool hasCallObj() const {
        bool ret = !!(flags_ & JSFRAME_HAS_CALL_OBJ);
        JS_ASSERT_IF(ret, !isNonStrictEvalFrame());
        return ret;
    }

    inline JSObject &callObj() const;
    inline void setScopeChainNoCallObj(JSObject &obj);
    inline void setScopeChainWithOwnCallObj(JSObject &obj);

    inline void markActivationObjectsAsPut();

    






    JSCompartment *compartment() const {
        JS_ASSERT_IF(isScriptFrame(), scopeChain().compartment() == script()->compartment);
        return scopeChain().compartment();
    }

    inline JSPrincipals *principals(JSContext *cx) const;

    







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

    

    const js::Value &returnValue() {
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

    






    bool isFramePushedByExecute() const {
        return !!(flags_ & (JSFRAME_GLOBAL | JSFRAME_EVAL));
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

    bool isYielding() {
        return !!(flags_ & JSFRAME_YIELDING);
    }

    void setYielding() {
        flags_ |= JSFRAME_YIELDING;
    }

    void clearYielding() {
        flags_ &= ~JSFRAME_YIELDING;
    }

    void setFinishedInInterpreter() {
        flags_ |= JSFRAME_FINISHED_IN_INTERP;
    }

    bool finishedInInterpreter() const {
        return !!(flags_ & JSFRAME_FINISHED_IN_INTERP);
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

#ifdef JS_METHODJIT
    js::mjit::JITScript *jit() {
        return script()->getJIT(isConstructing());
    }
#endif

    void methodjitStaticAsserts();

#ifdef DEBUG
    
    static JSObject *const sInvalidScopeChain;
#endif
};

namespace js {

static const size_t VALUES_PER_STACK_FRAME = sizeof(JSStackFrame) / sizeof(Value);

extern JSObject *
GetBlockChain(JSContext *cx, JSStackFrame *fp);

extern JSObject *
GetBlockChainFast(JSContext *cx, JSStackFrame *fp, JSOp op, size_t oplen);

extern JSObject *
GetScopeChain(JSContext *cx);








extern JSObject *
GetScopeChain(JSContext *cx, JSStackFrame *fp);

extern JSObject *
GetScopeChainFast(JSContext *cx, JSStackFrame *fp, JSOp op, size_t oplen);





void
ReportIncompatibleMethod(JSContext *cx, Value *vp, Class *clasp);









template <typename T>
bool GetPrimitiveThis(JSContext *cx, Value *vp, T *v);

inline void
PutActivationObjects(JSContext *cx, JSStackFrame *fp)
{
    
    if (fp->hasCallObj())
        js_PutCallObject(cx, fp);
    else if (fp->hasArgsObj())
        js_PutArgsObject(cx, fp);
}







inline bool
ScriptPrologue(JSContext *cx, JSStackFrame *fp, JSScript *script);

inline bool
ScriptEpilogue(JSContext *cx, JSStackFrame *fp, bool ok);








inline bool
ScriptPrologueOrGeneratorResume(JSContext *cx, JSStackFrame *fp);

inline bool
ScriptEpilogueOrGeneratorYield(JSContext *cx, JSStackFrame *fp, bool ok);



extern void
ScriptDebugPrologue(JSContext *cx, JSStackFrame *fp);

extern bool
ScriptDebugEpilogue(JSContext *cx, JSStackFrame *fp, bool ok);







extern bool
BoxNonStrictThis(JSContext *cx, const CallReceiver &call);







inline bool
ComputeThis(JSContext *cx, JSStackFrame *fp);









extern JS_REQUIRES_STACK bool
Invoke(JSContext *cx, const CallArgs &args, uint32 flags);

























class InvokeSessionGuard;














#define JSINVOKE_CONSTRUCT      JSFRAME_CONSTRUCTING




#define JSINVOKE_FUNFLAGS       JSINVOKE_CONSTRUCT






extern bool
ExternalInvoke(JSContext *cx, const Value &thisv, const Value &fval,
               uintN argc, Value *argv, Value *rval);

extern bool
ExternalGetOrSet(JSContext *cx, JSObject *obj, jsid id, const Value &fval,
                 JSAccessMode mode, uintN argc, Value *argv, Value *rval);









extern JS_REQUIRES_STACK bool
InvokeConstructor(JSContext *cx, const CallArgs &args);

extern JS_REQUIRES_STACK bool
InvokeConstructorWithGivenThis(JSContext *cx, JSObject *thisobj, const Value &fval,
                               uintN argc, Value *argv, Value *rval);

extern bool
ExternalInvokeConstructor(JSContext *cx, const Value &fval, uintN argc, Value *argv,
                          Value *rval);





extern JS_FORCES_STACK bool
Execute(JSContext *cx, JSObject &chain, JSScript *script,
        JSStackFrame *prev, uintN flags, Value *result);





extern JS_REQUIRES_STACK JS_NEVER_INLINE bool
Interpret(JSContext *cx, JSStackFrame *stopFp, uintN inlineCallCount = 0, JSInterpMode mode = JSINTERP_NORMAL);

extern JS_REQUIRES_STACK bool
RunScript(JSContext *cx, JSScript *script, JSStackFrame *fp);

extern bool
CheckRedeclaration(JSContext *cx, JSObject *obj, jsid id, uintN attrs);

extern bool
StrictlyEqual(JSContext *cx, const Value &lval, const Value &rval, JSBool *equal);

extern bool
LooselyEqual(JSContext *cx, const Value &lval, const Value &rval, JSBool *equal);


extern bool
SameValue(JSContext *cx, const Value &v1, const Value &v2, JSBool *same);

extern JSType
TypeOfValue(JSContext *cx, const Value &v);

extern JSBool
HasInstance(JSContext *cx, JSObject *obj, const js::Value *v, JSBool *bp);

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
js_LogOpcode(JSContext *cx);




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
