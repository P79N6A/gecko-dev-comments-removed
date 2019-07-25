







































#ifndef jsinterpinlines_h__
#define jsinterpinlines_h__

#include "jsapi.h"
#include "jsbool.h"
#include "jscompartment.h"
#include "jsinterp.h"
#include "jsnum.h"
#include "jsprobes.h"
#include "jsstr.h"
#include "methodjit/MethodJIT.h"

#include "jsfuninlines.h"

#include "vm/Stack-inl.h"

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

class InvokeSessionGuard
{
    InvokeArgsGuard args_;
    InvokeFrameGuard frame_;
    Value savedCallee_, savedThis_;
    Value *formals_, *actuals_;
    unsigned nformals_;
    JSScript *script_;
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

    
    args_.calleeHasBeenReset();

#ifdef JS_METHODJIT
    void *code;
    if (!optimized() || !(code = script_->getJIT(false )->invokeEntry))
#else
    if (!optimized())
#endif
        return Invoke(cx, args_);

    
    StackFrame *fp = frame_.fp();
    fp->clearMissingArgs();
    fp->resetInvokeCallFrame();
    SetValueRangeToUndefined(fp->slots(), script_->nfixed);

    JSBool ok;
    {
        AutoPreserveEnumerators preserve(cx);
        Probes::enterJSFun(cx, fp->fun(), script_);
#ifdef JS_METHODJIT
        ok = mjit::EnterMethodJIT(cx, fp, code, stackLimit_);
        cx->regs().pc = stop_;
#else
        cx->regs().pc = script_->code;
        ok = Interpret(cx, cx->fp());
#endif
        Probes::exitJSFun(cx, fp->fun(), script_);
    }

    
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
inline bool
GetPrimitiveThis(JSContext *cx, Value *vp, T *v)
{
    typedef detail::PrimitiveBehavior<T> Behavior;

    const Value &thisv = vp[1];
    if (Behavior::isType(thisv)) {
        *v = Behavior::extract(thisv);
        return true;
    }

    if (thisv.isObject() && thisv.toObject().getClass() == Behavior::getClass()) {
        *v = Behavior::extract(thisv.toObject().getPrimitiveThis());
        return true;
    }

    ReportIncompatibleMethod(cx, vp, Behavior::getClass());
    return false;
}










































inline bool
ComputeImplicitThis(JSContext *cx, JSObject *obj, const Value &funval, Value *vp)
{
    vp->setUndefined();

    if (!funval.isObject())
        return true;

    if (!obj->isGlobal()) {
        if (IsCacheableNonGlobalScope(obj))
            return true;
    } else {
        JSObject *callee = &funval.toObject();

        if (callee->isProxy()) {
            callee = callee->unwrap();
            if (!callee->isFunction())
                return true; 
        }
        if (callee->isFunction()) {
            JSFunction *fun = callee->getFunctionPrivate();
            if (fun->isInterpreted() && fun->inStrictMode())
                return true;
        }
        if (callee->getGlobal() == cx->fp()->scopeChain().getGlobal())
            return true;;
    }

    obj = obj->thisObject(cx);
    if (!obj)
        return false;

    vp->setObject(*obj);
    return true;
}

inline bool
ComputeThis(JSContext *cx, StackFrame *fp)
{
    Value &thisv = fp->thisValue();
    if (thisv.isObject())
        return true;
    if (fp->isFunctionFrame()) {
        if (fp->fun()->inStrictMode())
            return true;
        






        JS_ASSERT(!fp->isEvalFrame());
    }
    return BoxNonStrictThis(cx, fp->callReceiver());
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

inline bool
ScriptPrologue(JSContext *cx, StackFrame *fp)
{
    JS_ASSERT_IF(fp->isNonEvalFunctionFrame() && fp->fun()->isHeavyweight(), fp->hasCallObj());

    if (fp->isConstructing()) {
        JSObject *obj = js_CreateThisForFunction(cx, &fp->callee());
        if (!obj)
            return false;
        fp->functionThis().setObject(*obj);
    }

    if (cx->compartment->debugMode)
        ScriptDebugPrologue(cx, fp);
    return true;
}

inline bool
ScriptEpilogue(JSContext *cx, StackFrame *fp, bool ok)
{
    if (cx->compartment->debugMode)
        ok = ScriptDebugEpilogue(cx, fp, ok);

    



    if (fp->isConstructing() && ok) {
        if (fp->returnValue().isPrimitive())
            fp->setReturnValue(ObjectValue(fp->constructorThis()));
        JS_RUNTIME_METER(cx->runtime, constructs);
    }

    return ok;
}

inline bool
ScriptPrologueOrGeneratorResume(JSContext *cx, StackFrame *fp)
{
    if (!fp->isGeneratorFrame())
        return ScriptPrologue(cx, fp);
    if (cx->compartment->debugMode)
        ScriptDebugPrologue(cx, fp);
    return true;
}

inline bool
ScriptEpilogueOrGeneratorYield(JSContext *cx, StackFrame *fp, bool ok)
{
    if (!fp->isYielding())
        return ScriptEpilogue(cx, fp, ok);
    if (cx->compartment->debugMode)
        return ScriptDebugEpilogue(cx, fp, ok);
    return ok;
}

}  

#endif
