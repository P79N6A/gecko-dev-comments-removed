







































#ifndef jsinterpinlines_h__
#define jsinterpinlines_h__

#include "jsapi.h"
#include "jsbool.h"
#include "jsinterp.h"
#include "jsnum.h"
#include "jsprobes.h"
#include "jsstr.h"
#include "methodjit/MethodJIT.h"

inline void
JSStackFrame::initPrev(JSContext *cx)
{
    JS_ASSERT(flags_ & JSFRAME_HAS_PREVPC);
    if (JSFrameRegs *regs = cx->regs) {
        prev_ = regs->fp;
        prevpc_ = regs->pc;
        JS_ASSERT_IF(!prev_->isDummyFrame() && !prev_->hasImacropc(),
                     uint32(prevpc_ - prev_->script()->code) < prev_->script()->length);
    } else {
        prev_ = NULL;
#ifdef DEBUG
        prevpc_ = (jsbytecode *)0xbadc;
#endif
    }
}

inline void
JSStackFrame::resetGeneratorPrev(JSContext *cx)
{
    initPrev(cx);
}

inline void
JSStackFrame::initCallFrame(JSContext *cx, JSObject &callee, JSFunction *fun,
                            uint32 nactual, uint32 flagsArg)
{
    JS_ASSERT((flagsArg & ~(JSFRAME_CONSTRUCTING |
                            JSFRAME_OVERFLOW_ARGS |
                            JSFRAME_UNDERFLOW_ARGS)) == 0);
    JS_ASSERT(fun == callee.getFunctionPrivate());

    
    flags_ = JSFRAME_FUNCTION | JSFRAME_HAS_PREVPC | JSFRAME_HAS_SCOPECHAIN | flagsArg;
    exec.fun = fun;
    args.nactual = nactual;  
    scopeChain_ = callee.getParent();
    initPrev(cx);
    JS_ASSERT(!hasImacropc());
    JS_ASSERT(!hasHookData());
    JS_ASSERT(annotation() == NULL);

    JS_ASSERT(!hasCallObj());
}

inline void
JSStackFrame::resetInvokeCallFrame()
{
    

    if (hasArgsObj())
        args.nactual = argsObj().getArgsInitialLength();

    JS_ASSERT(!(flags_ & ~(JSFRAME_FUNCTION |
                           JSFRAME_OVERFLOW_ARGS |
                           JSFRAME_UNDERFLOW_ARGS |
                           JSFRAME_HAS_CALL_OBJ |
                           JSFRAME_HAS_ARGS_OBJ |
                           JSFRAME_OVERRIDE_ARGS |
                           JSFRAME_HAS_PREVPC |
                           JSFRAME_HAS_RVAL |
                           JSFRAME_HAS_SCOPECHAIN |
                           JSFRAME_HAS_ANNOTATION |
                           JSFRAME_FINISHED_IN_INTERPRETER)));
    flags_ &= JSFRAME_FUNCTION |
              JSFRAME_OVERFLOW_ARGS |
              JSFRAME_HAS_PREVPC |
              JSFRAME_UNDERFLOW_ARGS;

    JS_ASSERT_IF(!hasCallObj(), scopeChain_ == calleeValue().toObject().getParent());
    JS_ASSERT_IF(hasCallObj(), scopeChain_ == callObj().getParent());
    if (hasCallObj())
        scopeChain_ = callObj().getParent();

    JS_ASSERT(exec.fun == calleeValue().toObject().getFunctionPrivate());
    JS_ASSERT(!hasImacropc());
    JS_ASSERT(!hasHookData());
    JS_ASSERT(annotation() == NULL);
    JS_ASSERT(!hasCallObj());
}

inline void
JSStackFrame::initCallFrameCallerHalf(JSContext *cx, uint32 flagsArg,
                                      void *ncode)
{
    JS_ASSERT((flagsArg & ~(JSFRAME_CONSTRUCTING |
                            JSFRAME_FUNCTION |
                            JSFRAME_OVERFLOW_ARGS |
                            JSFRAME_UNDERFLOW_ARGS)) == 0);

    flags_ = JSFRAME_FUNCTION | flagsArg;
    prev_ = cx->regs->fp;
    ncode_ = ncode;
}





inline void
JSStackFrame::initCallFrameEarlyPrologue(JSFunction *fun, uint32 nactual)
{
    exec.fun = fun;
    if (flags_ & (JSFRAME_OVERFLOW_ARGS | JSFRAME_UNDERFLOW_ARGS))
        args.nactual = nactual;
}





inline void
JSStackFrame::initCallFrameLatePrologue()
{
    SetValueRangeToUndefined(slots(), script()->nfixed);
}

inline void
JSStackFrame::initEvalFrame(JSContext *cx, JSScript *script, JSStackFrame *prev, uint32 flagsArg)
{
    JS_ASSERT(flagsArg & JSFRAME_EVAL);
    JS_ASSERT((flagsArg & ~(JSFRAME_EVAL | JSFRAME_DEBUGGER)) == 0);
    JS_ASSERT(prev->flags_ & (JSFRAME_FUNCTION | JSFRAME_GLOBAL));

    
    js::Value *dstvp = (js::Value *)this - 2;
    js::Value *srcvp = prev->flags_ & (JSFRAME_GLOBAL | JSFRAME_EVAL)
                       ? (js::Value *)prev - 2
                       : prev->formalArgs() - 2;
    dstvp[0] = srcvp[0];
    dstvp[1] = srcvp[1];
    JS_ASSERT_IF(prev->flags_ & JSFRAME_FUNCTION,
                 dstvp[0].toObject().isFunction());

    
    flags_ = flagsArg | JSFRAME_HAS_PREVPC | JSFRAME_HAS_SCOPECHAIN |
             (prev->flags_ & (JSFRAME_FUNCTION |
                              JSFRAME_GLOBAL |
                              JSFRAME_HAS_CALL_OBJ));
    if (isFunctionFrame()) {
        exec = prev->exec;
        args.script = script;
    } else {
        exec.script = script;
    }

    scopeChain_ = &prev->scopeChain();
    JS_ASSERT_IF(isFunctionFrame(), &callObj() == &prev->callObj());

    prev_ = prev;
    prevpc_ = prev->pc(cx);
    JS_ASSERT(!hasImacropc());
    JS_ASSERT(!hasHookData());
    setAnnotation(prev->annotation());
}

inline void
JSStackFrame::initGlobalFrame(JSScript *script, JSObject &chain, uint32 flagsArg)
{
    JS_ASSERT((flagsArg & ~(JSFRAME_EVAL | JSFRAME_DEBUGGER)) == 0);

    
    js::Value *vp = (js::Value *)this - 2;
    vp[0].setUndefined();
    vp[1].setUndefined();  

    
    flags_ = flagsArg | JSFRAME_GLOBAL | JSFRAME_HAS_PREVPC | JSFRAME_HAS_SCOPECHAIN;
    exec.script = script;
    args.script = (JSScript *)0xbad;
    scopeChain_ = &chain;
    prev_ = NULL;
    JS_ASSERT(!hasImacropc());
    JS_ASSERT(!hasHookData());
    JS_ASSERT(annotation() == NULL);
}

inline void
JSStackFrame::initDummyFrame(JSContext *cx, JSObject &chain)
{
    js::PodZero(this);
    flags_ = JSFRAME_DUMMY | JSFRAME_HAS_PREVPC | JSFRAME_HAS_SCOPECHAIN;
    initPrev(cx);
    chain.isGlobal();
    setScopeChainNoCallObj(chain);
}

inline void
JSStackFrame::stealFrameAndSlots(js::Value *vp, JSStackFrame *otherfp,
                                 js::Value *othervp, js::Value *othersp)
{
    JS_ASSERT(vp == (js::Value *)this - (otherfp->formalArgsEnd() - othervp));
    JS_ASSERT(othervp == otherfp->actualArgs() - 2);
    JS_ASSERT(othersp >= otherfp->slots());
    JS_ASSERT(othersp <= otherfp->base() + otherfp->numSlots());

    size_t nbytes = (othersp - othervp) * sizeof(js::Value);
    memcpy(vp, othervp, nbytes);
    JS_ASSERT(vp == actualArgs() - 2);

    





    if (hasCallObj()) {
        callObj().setPrivate(this);
        otherfp->flags_ &= ~JSFRAME_HAS_CALL_OBJ;
        if (js_IsNamedLambda(fun())) {
            JSObject *env = callObj().getParent();
            JS_ASSERT(env->getClass() == &js_DeclEnvClass);
            env->setPrivate(this);
        }
    }
    if (hasArgsObj()) {
        argsObj().setPrivate(this);
        otherfp->flags_ &= ~JSFRAME_HAS_ARGS_OBJ;
    }
}

inline js::Value &
JSStackFrame::canonicalActualArg(uintN i) const
{
    if (i < numFormalArgs())
        return formalArg(i);
    JS_ASSERT(i < numActualArgs());
    return actualArgs()[i];
}

template <class Op>
inline void
JSStackFrame::forEachCanonicalActualArg(Op op)
{
    uintN nformal = fun()->nargs;
    js::Value *formals = formalArgsEnd() - nformal;
    uintN nactual = numActualArgs();
    if (nactual <= nformal) {
        uintN i = 0;
        js::Value *actualsEnd = formals + nactual;
        for (js::Value *p = formals; p != actualsEnd; ++p, ++i)
            op(i, p);
    } else {
        uintN i = 0;
        js::Value *formalsEnd = formalArgsEnd();
        for (js::Value *p = formals; p != formalsEnd; ++p, ++i)
            op(i, p);
        js::Value *actuals = formalsEnd - (nactual + 2);
        js::Value *actualsEnd = formals - 2;
        for (js::Value *p = actuals; p != actualsEnd; ++p, ++i)
            op(i, p);
    }
}

template <class Op>
inline void
JSStackFrame::forEachFormalArg(Op op)
{
    js::Value *formals = formalArgsEnd() - fun()->nargs;
    js::Value *formalsEnd = formalArgsEnd();
    uintN i = 0;
    for (js::Value *p = formals; p != formalsEnd; ++p, ++i)
        op(i, p);
}

JS_ALWAYS_INLINE void
JSStackFrame::clearMissingArgs()
{
    if (flags_ & JSFRAME_UNDERFLOW_ARGS)
        SetValueRangeToUndefined(formalArgs() + numActualArgs(), formalArgsEnd());
}

inline bool
JSStackFrame::computeThis(JSContext *cx)
{
    js::Value &thisv = thisValue();
    if (thisv.isObject())
        return true;
    if (isFunctionFrame()) {
        if (fun()->acceptsPrimitiveThis())
            return true;
        






        JS_ASSERT(!isEvalFrame());
    }
    if (!js::ComputeThisFromArgv(cx, &thisv + 1))
        return NULL;
    JS_ASSERT(IsSaneThisObject(thisv.toObject()));
    return &thisv.toObject();
}

inline JSObject &
JSStackFrame::varobj(js::StackSegment *seg) const
{
    JS_ASSERT(seg->contains(this));
    return isFunctionFrame() ? callObj() : seg->getInitialVarObj();
}

inline JSObject &
JSStackFrame::varobj(JSContext *cx) const
{
    JS_ASSERT(cx->activeSegment()->contains(this));
    return isFunctionFrame() ? callObj() : cx->activeSegment()->getInitialVarObj();
}

inline uintN
JSStackFrame::numActualArgs() const
{
    JS_ASSERT(hasArgs());
    if (JS_UNLIKELY(flags_ & (JSFRAME_OVERFLOW_ARGS | JSFRAME_UNDERFLOW_ARGS)))
        return hasArgsObj() ? argsObj().getArgsInitialLength() : args.nactual;
    return numFormalArgs();
}

inline js::Value *
JSStackFrame::actualArgs() const
{
    JS_ASSERT(hasArgs());
    js::Value *argv = formalArgs();
    if (JS_UNLIKELY(flags_ & JSFRAME_OVERFLOW_ARGS)) {
        uintN nactual = hasArgsObj() ? argsObj().getArgsInitialLength() : args.nactual;
        return argv - (2 + nactual);
    }
    return argv;
}

inline js::Value *
JSStackFrame::actualArgsEnd() const
{
    JS_ASSERT(hasArgs());
    if (JS_UNLIKELY(flags_ & JSFRAME_OVERFLOW_ARGS))
        return formalArgs() - 2;
    return formalArgs() + numActualArgs();
}

inline void
JSStackFrame::setArgsObj(JSObject &obj)
{
    JS_ASSERT_IF(hasArgsObj(), &obj == args.obj);
    JS_ASSERT_IF(!hasArgsObj(), numActualArgs() == obj.getArgsInitialLength());
    args.obj = &obj;
    flags_ |= JSFRAME_HAS_ARGS_OBJ;
}

inline void
JSStackFrame::clearArgsObj()
{
    JS_ASSERT(hasArgsObj());
    args.nactual = args.obj->getArgsInitialLength();
    flags_ ^= JSFRAME_HAS_ARGS_OBJ;
}

inline void
JSStackFrame::setScopeChainNoCallObj(JSObject &obj)
{
#ifdef DEBUG
    JS_ASSERT(&obj != NULL);
    JSObject *callObjBefore = maybeCallObj();
    if (!hasCallObj() && &scopeChain() != sInvalidScopeChain) {
        for (JSObject *pobj = &scopeChain(); pobj; pobj = pobj->getParent())
            JS_ASSERT_IF(pobj->isCall(), pobj->getPrivate() != this);
    }
#endif
    scopeChain_ = &obj;
    flags_ |= JSFRAME_HAS_SCOPECHAIN;
    JS_ASSERT(callObjBefore == maybeCallObj());
}

inline void
JSStackFrame::setScopeChainAndCallObj(JSObject &obj)
{
    JS_ASSERT(&obj != NULL);
    JS_ASSERT(!hasCallObj() && obj.isCall() && obj.getPrivate() == this);
    scopeChain_ = &obj;
    flags_ |= JSFRAME_HAS_SCOPECHAIN | JSFRAME_HAS_CALL_OBJ;
}

inline void
JSStackFrame::clearCallObj()
{
    JS_ASSERT(hasCallObj());
    flags_ ^= JSFRAME_HAS_CALL_OBJ;
}

inline JSObject &
JSStackFrame::callObj() const
{
    JS_ASSERT(hasCallObj());
    JSObject *pobj = &scopeChain();
    while (JS_UNLIKELY(pobj->getClass() != &js_CallClass)) {
        JS_ASSERT(js_IsCacheableNonGlobalScope(pobj) || pobj->isWith());
        pobj = pobj->getParent();
    }
    return *pobj;
}

inline JSObject *
JSStackFrame::maybeCallObj() const
{
    return hasCallObj() ? &callObj() : NULL;
}

namespace js {

class AutoPreserveEnumerators {
    JSContext *cx;
    JSObject *enumerators;

  public:
    AutoPreserveEnumerators(JSContext *cx) : cx(cx), enumerators(cx->enumerators)
    {
    }

    ~AutoPreserveEnumerators()
    {
        cx->enumerators = enumerators;
    }
};

struct AutoInterpPreparer  {
    JSContext *cx;
    JSScript *script;

    AutoInterpPreparer(JSContext *cx, JSScript *script)
      : cx(cx), script(script)
    {
        cx->interpLevel++;
    }

    ~AutoInterpPreparer()
    {
        --cx->interpLevel;
    }
};

inline void
PutActivationObjects(JSContext *cx, JSStackFrame *fp)
{
    JS_ASSERT(fp->isFunctionFrame() && !fp->isEvalFrame());

    
    if (fp->hasCallObj()) {
        js_PutCallObject(cx, fp);
    } else if (fp->hasArgsObj()) {
        js_PutArgsObject(cx, fp);
    }
}

class InvokeSessionGuard
{
    InvokeArgsGuard args_;
    InvokeFrameGuard frame_;
    Value savedCallee_, savedThis_;
    Value *formals_, *actuals_;
    unsigned nformals_;
    JSScript *script_;
    void *code_;
    Value *stackLimit_;
    jsbytecode *stop_;

    bool optimized() const { return frame_.pushed(); }

  public:
    InvokeSessionGuard() : args_(), frame_() {}
    ~InvokeSessionGuard() {}

    bool start(JSContext *cx, const Value &callee, const Value &thisv, uintN argc);
    bool invoke(JSContext *cx) const;

    bool started() const {
        return args_.pushed();
    }

    Value &operator[](unsigned i) const {
        JS_ASSERT(i < argc());
        Value &arg = i < nformals_ ? formals_[i] : actuals_[i];
        JS_ASSERT_IF(optimized(), &arg == &frame_.fp()->canonicalActualArg(i));
        JS_ASSERT_IF(!optimized(), &arg == &args_[i]);
        return arg;
    }

    uintN argc() const {
        return args_.argc();
    }

    const Value &rval() const {
        return optimized() ? frame_.fp()->returnValue() : args_.rval();
    }
};

inline bool
InvokeSessionGuard::invoke(JSContext *cx) const
{
    

    
    formals_[-2] = savedCallee_;
    formals_[-1] = savedThis_;

    if (!optimized())
        return Invoke(cx, args_, 0);

    
    JSStackFrame *fp = frame_.fp();
    fp->clearMissingArgs();
    fp->resetInvokeCallFrame();
    SetValueRangeToUndefined(fp->slots(), script_->nfixed);

    JSBool ok;
    {
        AutoPreserveEnumerators preserve(cx);
        Probes::enterJSFun(cx, fp->fun());
#ifdef JS_METHODJIT
        AutoInterpPreparer prepareInterp(cx, script_);
        ok = mjit::EnterMethodJIT(cx, fp, code_, stackLimit_);
        cx->regs->pc = stop_;
#else
        cx->regs->pc = script_->code;
        ok = Interpret(cx, cx->fp());
#endif
        Probes::exitJSFun(cx, fp->fun());
    }

    PutActivationObjects(cx, fp);

    
    return ok;
}

namespace detail {

template<typename T> class PrimitiveBehavior { };

template<>
class PrimitiveBehavior<JSString *> {
  public:
    static inline bool isType(const Value &v) { return v.isString(); }
    static inline JSString *extract(const Value &v) { return v.toString(); }
    static inline Class *getClass() { return &js_StringClass; }
};

template<>
class PrimitiveBehavior<bool> {
  public:
    static inline bool isType(const Value &v) { return v.isBoolean(); }
    static inline bool extract(const Value &v) { return v.toBoolean(); }
    static inline Class *getClass() { return &js_BooleanClass; }
};

template<>
class PrimitiveBehavior<double> {
  public:
    static inline bool isType(const Value &v) { return v.isNumber(); }
    static inline double extract(const Value &v) { return v.toNumber(); }
    static inline Class *getClass() { return &js_NumberClass; }
};

} 

template <typename T>
bool
GetPrimitiveThis(JSContext *cx, Value *vp, T *v)
{
    typedef detail::PrimitiveBehavior<T> Behavior;

    const Value &thisv = vp[1];
    if (Behavior::isType(thisv)) {
        *v = Behavior::extract(thisv);
        return true;
    }

    if (thisv.isObjectOrNull()) {
        JSObject *obj = thisv.toObjectOrNull();
        if (!obj || obj->getClass() != Behavior::getClass()) {
            obj = ComputeThisFromVp(cx, vp);
            if (!InstanceOf(cx, obj, Behavior::getClass(), vp + 2))
                return false;
        }
        *v = Behavior::extract(thisv.toObject().getPrimitiveThis());
        return true;
    }

    ReportIncompatibleMethod(cx, vp, Behavior::getClass());
    return false;
}












JS_ALWAYS_INLINE JSObject *
ValuePropertyBearer(JSContext *cx, const Value &v, int spindex)
{
    if (v.isObject())
        return &v.toObject();

    JSProtoKey protoKey;
    if (v.isString()) {
        protoKey = JSProto_String;
    } else if (v.isNumber()) {
        protoKey = JSProto_Number;
    } else if (v.isBoolean()) {
        protoKey = JSProto_Boolean;
    } else {
        JS_ASSERT(v.isNull() || v.isUndefined());
        js_ReportIsNullOrUndefined(cx, spindex, v, NULL);
        return NULL;
    }

    JSObject *pobj;
    if (!js_GetClassPrototype(cx, NULL, protoKey, &pobj))
        return NULL;
    return pobj;
}

static inline bool
ScriptEpilogue(JSContext *cx, JSStackFrame *fp, JSBool ok)
{
    Probes::exitJSFun(cx, fp->maybeFun());
    JSInterpreterHook hook = cx->debugHooks->callHook;
    if (hook && fp->hasHookData() && !fp->isExecuteFrame())
        hook(cx, fp, JS_FALSE, &ok, fp->hookData());

    




    if (fp->isFunctionFrame() && !fp->isEvalFrame() && !fp->isYielding())
        PutActivationObjects(cx, fp);

    



    if (fp->isConstructing()) {
        if (fp->returnValue().isPrimitive())
            fp->setReturnValue(ObjectValue(fp->constructorThis()));
        JS_RUNTIME_METER(cx->runtime, constructs);
    }

    return ok;
}

}

#endif 
