







































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

namespace detail {

template<typename T> class PrimitiveBehavior { };

template<>
class PrimitiveBehavior<JSString *> {
  public:
    static inline bool isType(const Value &v) { return v.isString(); }
    static inline JSString *extract(const Value &v) { return v.toString(); }
    static inline Class *getClass() { return &StringClass; }
};

template<>
class PrimitiveBehavior<bool> {
  public:
    static inline bool isType(const Value &v) { return v.isBoolean(); }
    static inline bool extract(const Value &v) { return v.toBoolean(); }
    static inline Class *getClass() { return &BooleanClass; }
};

template<>
class PrimitiveBehavior<double> {
  public:
    static inline bool isType(const Value &v) { return v.isNumber(); }
    static inline double extract(const Value &v) { return v.toNumber(); }
    static inline Class *getClass() { return &NumberClass; }
};

} 

template <typename T>
inline bool
GetPrimitiveThis(JSContext *cx, CallReceiver call, T *v)
{
    typedef detail::PrimitiveBehavior<T> Behavior;

    const Value &thisv = call.thisv();
    if (Behavior::isType(thisv)) {
        *v = Behavior::extract(thisv);
        return true;
    }

    if (thisv.isObject() && thisv.toObject().getClass() == Behavior::getClass()) {
        *v = Behavior::extract(thisv.toObject().getPrimitiveThis());
        return true;
    }

    ReportIncompatibleMethod(cx, call, Behavior::getClass());
    return false;
}





























inline bool
ComputeImplicitThis(JSContext *cx, JSObject *obj, const Value &funval, Value *vp)
{
    vp->setUndefined();

    if (!funval.isObject())
        return true;

    if (obj->isGlobal())
        return true;

    if (IsCacheableNonGlobalScope(obj))
        return true;

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
FunctionNeedsPrologue(JSContext *cx, JSFunction *fun)
{
    
    if (fun->isHeavyweight())
        return true;

    
    if (cx->typeInferenceEnabled() && fun->script()->nesting())
        return true;

    return false;
}

inline bool
ScriptPrologue(JSContext *cx, StackFrame *fp, bool newType)
{
    JS_ASSERT_IF(fp->isNonEvalFunctionFrame() && fp->fun()->isHeavyweight(), fp->hasCallObj());

    if (fp->isConstructing()) {
        JSObject *obj = js_CreateThisForFunction(cx, &fp->callee(), newType);
        if (!obj)
            return false;
        fp->functionThis().setObject(*obj);
    }

    Probes::enterJSFun(cx, fp->maybeFun(), fp->script());
    if (cx->compartment->debugMode())
        ScriptDebugPrologue(cx, fp);

    return true;
}

inline bool
ScriptEpilogue(JSContext *cx, StackFrame *fp, bool ok)
{
    Probes::exitJSFun(cx, fp->maybeFun(), fp->script());
    if (cx->compartment->debugMode())
        ok = ScriptDebugEpilogue(cx, fp, ok);

    



    if (fp->isConstructing() && ok) {
        if (fp->returnValue().isPrimitive())
            fp->setReturnValue(ObjectValue(fp->constructorThis()));
    }

    return ok;
}

inline bool
ScriptPrologueOrGeneratorResume(JSContext *cx, StackFrame *fp, bool newType)
{
    if (!fp->isGeneratorFrame())
        return ScriptPrologue(cx, fp, newType);
    if (cx->compartment->debugMode())
        ScriptDebugPrologue(cx, fp);
    return true;
}

inline bool
ScriptEpilogueOrGeneratorYield(JSContext *cx, StackFrame *fp, bool ok)
{
    if (!fp->isYielding())
        return ScriptEpilogue(cx, fp, ok);
    if (cx->compartment->debugMode())
        return ScriptDebugEpilogue(cx, fp, ok);
    return ok;
}

}  

#endif 
