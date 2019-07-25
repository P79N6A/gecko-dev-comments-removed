










































#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsbit.h"
#include "jsutil.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jsbuiltins.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsemit.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsparse.h"
#include "jspropertytree.h"
#include "jsproxy.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jsexn.h"
#include "jsstaticcheck.h"
#include "jstracer.h"
#include "vm/CallObject.h"
#include "vm/Debugger.h"

#if JS_HAS_GENERATORS
# include "jsiter.h"
#endif

#if JS_HAS_XDR
# include "jsxdrapi.h"
#endif

#ifdef JS_METHODJIT
#include "methodjit/MethodJIT.h"
#endif

#include "jsatominlines.h"
#include "jsfuninlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"
#include "vm/CallObject-inl.h"
#include "vm/ArgumentsObject-inl.h"
#include "vm/Stack-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::types;

inline JSObject *
JSObject::getThrowTypeError() const
{
    return getGlobal()->getThrowTypeError();
}

JSBool
js_GetArgsValue(JSContext *cx, StackFrame *fp, Value *vp)
{
    JSObject *argsobj;
    if (fp->hasOverriddenArgs()) {
        JS_ASSERT(fp->hasCallObj());
        return fp->callObj().getProperty(cx, cx->runtime->atomState.argumentsAtom, vp);
    }
    argsobj = js_GetArgsObject(cx, fp);
    if (!argsobj)
        return JS_FALSE;
    vp->setObject(*argsobj);
    return JS_TRUE;
}

JSBool
js_GetArgsProperty(JSContext *cx, StackFrame *fp, jsid id, Value *vp)
{
    JS_ASSERT(fp->isFunctionFrame());

    if (fp->hasOverriddenArgs()) {
        JS_ASSERT(fp->hasCallObj());

        Value v;
        if (!fp->callObj().getProperty(cx, cx->runtime->atomState.argumentsAtom, &v))
            return false;

        JSObject *obj;
        if (v.isPrimitive()) {
            obj = js_ValueToNonNullObject(cx, v);
            if (!obj)
                return false;
        } else {
            obj = &v.toObject();
        }
        return obj->getGeneric(cx, id, vp);
    }

    vp->setUndefined();
    if (JSID_IS_INT(id)) {
        uint32 arg = uint32(JSID_TO_INT(id));
        ArgumentsObject *argsobj = fp->maybeArgsObj();
        if (arg < fp->numActualArgs()) {
            if (argsobj) {
                const Value &v = argsobj->element(arg);
                if (v.isMagic(JS_ARGS_HOLE))
                    return argsobj->getGeneric(cx, id, vp);
                if (fp->functionScript()->strictModeCode) {
                    *vp = v;
                    return true;
                }
            }
            *vp = fp->canonicalActualArg(arg);
        } else {
            











            if (argsobj)
                return argsobj->getGeneric(cx, id, vp);
        }
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        ArgumentsObject *argsobj = fp->maybeArgsObj();
        if (argsobj && argsobj->hasOverriddenLength())
            return argsobj->getGeneric(cx, id, vp);
        vp->setInt32(fp->numActualArgs());
    }
    return true;
}

js::ArgumentsObject *
ArgumentsObject::create(JSContext *cx, uint32 argc, JSObject &callee)
{
    JS_ASSERT(argc <= StackSpace::ARGS_LENGTH_MAX);

    JSObject *proto;
    if (!js_GetClassPrototype(cx, callee.getGlobal(), JSProto_Object, &proto))
        return NULL;

    TypeObject *type = proto->getNewType(cx);
    if (!type)
        return NULL;

    JS_STATIC_ASSERT(NormalArgumentsObject::RESERVED_SLOTS == 2);
    JS_STATIC_ASSERT(StrictArgumentsObject::RESERVED_SLOTS == 2);
    JSObject *obj = js_NewGCObject(cx, FINALIZE_OBJECT4);
    if (!obj)
        return NULL;

    bool strict = callee.getFunctionPrivate()->inStrictMode();
    EmptyShape *emptyArgumentsShape = EmptyShape::getEmptyArgumentsShape(cx, strict);
    if (!emptyArgumentsShape)
        return NULL;

    ArgumentsData *data = (ArgumentsData *)
        cx->malloc_(offsetof(ArgumentsData, slots) + argc * sizeof(Value));
    if (!data)
        return NULL;
    SetValueRangeToUndefined(data->slots, argc);

    
    obj->init(cx, type, proto->getParent(), false);
    obj->setInitialPropertyInfallible(emptyArgumentsShape);

    ArgumentsObject *argsobj = obj->asArguments();

    JS_ASSERT(UINT32_MAX > (uint64(argc) << PACKED_BITS_COUNT));
    argsobj->setInitialLength(argc);

    argsobj->setCalleeAndData(callee, data);

    JS_ASSERT(argsobj->numFixedSlots() == NormalArgumentsObject::NFIXED_SLOTS);
    JS_ASSERT(argsobj->numFixedSlots() == StrictArgumentsObject::NFIXED_SLOTS);

    return argsobj;
}

struct STATIC_SKIP_INFERENCE PutArg
{
    PutArg(Value *dst) : dst(dst) {}
    Value *dst;
    bool operator()(uintN, Value *src) {
        if (!dst->isMagic(JS_ARGS_HOLE))
            *dst = *src;
        ++dst;
        return true;
    }
};

JSObject *
js_GetArgsObject(JSContext *cx, StackFrame *fp)
{
    



    JS_ASSERT(fp->isFunctionFrame());
    while (fp->isEvalInFunction())
        fp = fp->prev();

    



    if (!fp->script()->createdArgs)
        types::MarkArgumentsCreated(cx, fp->script());

    
    JS_ASSERT_IF(fp->fun()->isHeavyweight(), fp->hasCallObj());
    if (fp->hasArgsObj())
        return &fp->argsObj();

    ArgumentsObject *argsobj =
        ArgumentsObject::create(cx, fp->numActualArgs(), fp->callee());
    if (!argsobj)
        return argsobj;

    








    if (argsobj->isStrictArguments())
        fp->forEachCanonicalActualArg(PutArg(argsobj->data()->slots));
    else
        argsobj->setStackFrame(fp);

    fp->setArgsObj(*argsobj);
    return argsobj;
}

void
js_PutArgsObject(StackFrame *fp)
{
    ArgumentsObject &argsobj = fp->argsObj();
    if (argsobj.isNormalArguments()) {
        JS_ASSERT(argsobj.maybeStackFrame() == fp);
        fp->forEachCanonicalActualArg(PutArg(argsobj.data()->slots));
        argsobj.setStackFrame(NULL);
    } else {
        JS_ASSERT(!argsobj.maybeStackFrame());
    }
}

#ifdef JS_TRACER




JSObject * JS_FASTCALL
js_NewArgumentsOnTrace(JSContext *cx, uint32 argc, JSObject *callee)
{
    ArgumentsObject *argsobj = ArgumentsObject::create(cx, argc, *callee);
    if (!argsobj)
        return NULL;

    if (argsobj->isStrictArguments()) {
        



        JS_ASSERT(!argsobj->maybeStackFrame());
    } else {
        argsobj->setOnTrace();
    }

    return argsobj;
}
JS_DEFINE_CALLINFO_3(extern, OBJECT, js_NewArgumentsOnTrace, CONTEXT, UINT32, OBJECT,
                     0, nanojit::ACCSET_STORE_ANY)


JSBool JS_FASTCALL
js_PutArgumentsOnTrace(JSContext *cx, JSObject *obj, Value *argv)
{
    NormalArgumentsObject *argsobj = obj->asNormalArguments();

    JS_ASSERT(argsobj->onTrace());

    




    Value *srcend = argv + argsobj->initialLength();
    Value *dst = argsobj->data()->slots;
    for (Value *src = argv; src < srcend; ++src, ++dst) {
        if (!dst->isMagic(JS_ARGS_HOLE))
            *dst = *src;
    }

    argsobj->clearOnTrace();
    return true;
}
JS_DEFINE_CALLINFO_3(extern, BOOL, js_PutArgumentsOnTrace, CONTEXT, OBJECT, VALUEPTR, 0,
                     nanojit::ACCSET_STORE_ANY)

#endif 

static JSBool
args_delProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    ArgumentsObject *argsobj = obj->asArguments();
    if (JSID_IS_INT(id)) {
        uintN arg = uintN(JSID_TO_INT(id));
        if (arg < argsobj->initialLength())
            argsobj->setElement(arg, MagicValue(JS_ARGS_HOLE));
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        argsobj->markLengthOverridden();
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom)) {
        argsobj->asNormalArguments()->clearCallee();
    }
    return true;
}

static JSBool
ArgGetter(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    LeaveTrace(cx);

    if (!obj->isNormalArguments())
        return true;

    NormalArgumentsObject *argsobj = obj->asNormalArguments();
    if (JSID_IS_INT(id)) {
        



        uintN arg = uintN(JSID_TO_INT(id));
        if (arg < argsobj->initialLength()) {
            JS_ASSERT(!argsobj->element(arg).isMagic(JS_ARGS_HOLE));
            if (StackFrame *fp = argsobj->maybeStackFrame())
                *vp = fp->canonicalActualArg(arg);
            else
                *vp = argsobj->element(arg);
        }
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        if (!argsobj->hasOverriddenLength())
            vp->setInt32(argsobj->initialLength());
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom));
        const Value &v = argsobj->callee();
        if (!v.isMagic(JS_ARGS_HOLE))
            *vp = v;
    }
    return true;
}

static JSBool
ArgSetter(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp)
{
#ifdef JS_TRACER
    
    
    
    
    LeaveTrace(cx);
#endif


    if (!obj->isNormalArguments())
        return true;

    NormalArgumentsObject *argsobj = obj->asNormalArguments();

    if (JSID_IS_INT(id)) {
        uintN arg = uintN(JSID_TO_INT(id));
        if (arg < argsobj->initialLength()) {
            if (StackFrame *fp = argsobj->maybeStackFrame()) {
                JSScript *script = fp->functionScript();
                if (script->usesArguments) {
                    if (arg < fp->numFormalArgs())
                        TypeScript::SetArgument(cx, script, arg, *vp);
                    fp->canonicalActualArg(arg) = *vp;
                }
                return true;
            }
        }
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom) ||
                  JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom));
    }

    







    AutoValueRooter tvr(cx);
    return js_DeleteProperty(cx, argsobj, id, tvr.addr(), false) &&
           js_DefineProperty(cx, argsobj, id, vp, NULL, NULL, JSPROP_ENUMERATE);
}

static JSBool
args_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
             JSObject **objp)
{
    *objp = NULL;

    NormalArgumentsObject *argsobj = obj->asNormalArguments();

    uintN attrs = JSPROP_SHARED | JSPROP_SHADOWABLE;
    if (JSID_IS_INT(id)) {
        uint32 arg = uint32(JSID_TO_INT(id));
        if (arg >= argsobj->initialLength() || argsobj->element(arg).isMagic(JS_ARGS_HOLE))
            return true;

        attrs |= JSPROP_ENUMERATE;
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        if (argsobj->hasOverriddenLength())
            return true;
    } else {
        if (!JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom))
            return true;

        if (argsobj->callee().isMagic(JS_ARGS_HOLE))
            return true;
    }

    Value undef = UndefinedValue();
    if (!js_DefineProperty(cx, argsobj, id, &undef, ArgGetter, ArgSetter, attrs))
        return JS_FALSE;

    *objp = argsobj;
    return true;
}

static JSBool
args_enumerate(JSContext *cx, JSObject *obj)
{
    NormalArgumentsObject *argsobj = obj->asNormalArguments();

    



    int argc = int(argsobj->initialLength());
    for (int i = -2; i != argc; i++) {
        jsid id = (i == -2)
                  ? ATOM_TO_JSID(cx->runtime->atomState.lengthAtom)
                  : (i == -1)
                  ? ATOM_TO_JSID(cx->runtime->atomState.calleeAtom)
                  : INT_TO_JSID(i);

        JSObject *pobj;
        JSProperty *prop;
        if (!js_LookupProperty(cx, argsobj, id, &pobj, &prop))
            return false;
    }
    return true;
}

static JSBool
StrictArgGetter(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    LeaveTrace(cx);

    if (!obj->isStrictArguments())
        return true;

    StrictArgumentsObject *argsobj = obj->asStrictArguments();

    if (JSID_IS_INT(id)) {
        



        uintN arg = uintN(JSID_TO_INT(id));
        if (arg < argsobj->initialLength()) {
            const Value &v = argsobj->element(arg);
            if (!v.isMagic(JS_ARGS_HOLE))
                *vp = v;
        }
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom));
        if (!argsobj->hasOverriddenLength())
            vp->setInt32(argsobj->initialLength());
    }
    return true;
}

static JSBool
StrictArgSetter(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp)
{
    if (!obj->isStrictArguments())
        return true;

    StrictArgumentsObject *argsobj = obj->asStrictArguments();

    if (JSID_IS_INT(id)) {
        uintN arg = uintN(JSID_TO_INT(id));
        if (arg < argsobj->initialLength()) {
            argsobj->setElement(arg, *vp);
            return true;
        }
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom));
    }

    





    AutoValueRooter tvr(cx);
    return js_DeleteProperty(cx, argsobj, id, tvr.addr(), strict) &&
           js_SetPropertyHelper(cx, argsobj, id, 0, vp, strict);
}

static JSBool
strictargs_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags, JSObject **objp)
{
    *objp = NULL;

    StrictArgumentsObject *argsobj = obj->asStrictArguments();

    uintN attrs = JSPROP_SHARED | JSPROP_SHADOWABLE;
    PropertyOp getter = StrictArgGetter;
    StrictPropertyOp setter = StrictArgSetter;

    if (JSID_IS_INT(id)) {
        uint32 arg = uint32(JSID_TO_INT(id));
        if (arg >= argsobj->initialLength() || argsobj->element(arg).isMagic(JS_ARGS_HOLE))
            return true;

        attrs |= JSPROP_ENUMERATE;
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        if (argsobj->hasOverriddenLength())
            return true;
    } else {
        if (!JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom) &&
            !JSID_IS_ATOM(id, cx->runtime->atomState.callerAtom)) {
            return true;
        }

        attrs = JSPROP_PERMANENT | JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED;
        getter = CastAsPropertyOp(argsobj->getThrowTypeError());
        setter = CastAsStrictPropertyOp(argsobj->getThrowTypeError());
    }

    Value undef = UndefinedValue();
    if (!js_DefineProperty(cx, argsobj, id, &undef, getter, setter, attrs))
        return false;

    *objp = argsobj;
    return true;
}

static JSBool
strictargs_enumerate(JSContext *cx, JSObject *obj)
{
    StrictArgumentsObject *argsobj = obj->asStrictArguments();

    



    JSObject *pobj;
    JSProperty *prop;

    
    if (!js_LookupProperty(cx, argsobj, ATOM_TO_JSID(cx->runtime->atomState.lengthAtom), &pobj, &prop))
        return false;

    
    if (!js_LookupProperty(cx, argsobj, ATOM_TO_JSID(cx->runtime->atomState.calleeAtom), &pobj, &prop))
        return false;

    
    if (!js_LookupProperty(cx, argsobj, ATOM_TO_JSID(cx->runtime->atomState.callerAtom), &pobj, &prop))
        return false;

    for (uint32 i = 0, argc = argsobj->initialLength(); i < argc; i++) {
        if (!js_LookupProperty(cx, argsobj, INT_TO_JSID(i), &pobj, &prop))
            return false;
    }

    return true;
}

static void
args_finalize(JSContext *cx, JSObject *obj)
{
    cx->free_(reinterpret_cast<void *>(obj->asArguments()->data()));
}









static inline void
MaybeMarkGenerator(JSTracer *trc, JSObject *obj)
{
#if JS_HAS_GENERATORS
    StackFrame *fp = (StackFrame *) obj->getPrivate();
    if (fp && fp->isFloatingGenerator()) {
        JSObject *genobj = js_FloatingFrameToGenerator(fp)->obj;
        MarkObject(trc, *genobj, "generator object");
    }
#endif
}

static void
args_trace(JSTracer *trc, JSObject *obj)
{
    ArgumentsObject *argsobj = obj->asArguments();
    if (argsobj->onTrace()) {
        JS_ASSERT(!argsobj->isStrictArguments());
        return;
    }

    ArgumentsData *data = argsobj->data();
    if (data->callee.isObject())
        MarkObject(trc, data->callee.toObject(), js_callee_str);
    MarkValueRange(trc, argsobj->initialLength(), data->slots, js_arguments_str);

    MaybeMarkGenerator(trc, argsobj);
}







Class js::NormalArgumentsObjectClass = {
    "Arguments",
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE |
    JSCLASS_HAS_RESERVED_SLOTS(NormalArgumentsObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    JS_PropertyStub,         
    args_delProperty,
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    args_enumerate,
    reinterpret_cast<JSResolveOp>(args_resolve),
    JS_ConvertStub,
    args_finalize,           
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    args_trace
};






Class js::StrictArgumentsObjectClass = {
    "Arguments",
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE |
    JSCLASS_HAS_RESERVED_SLOTS(StrictArgumentsObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    JS_PropertyStub,         
    args_delProperty,
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    strictargs_enumerate,
    reinterpret_cast<JSResolveOp>(strictargs_resolve),
    JS_ConvertStub,
    args_finalize,           
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    args_trace
};





Class js::DeclEnvClass = {
    js_Object_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

static inline JSObject *
NewDeclEnvObject(JSContext *cx, StackFrame *fp)
{
    JSObject *envobj = js_NewGCObject(cx, FINALIZE_OBJECT2);
    if (!envobj)
        return NULL;

    types::TypeObject *type = cx->compartment->getEmptyType(cx);
    if (!type)
        return NULL;

    EmptyShape *emptyDeclEnvShape = EmptyShape::getEmptyDeclEnvShape(cx);
    if (!emptyDeclEnvShape)
        return NULL;
    envobj->init(cx, type, &fp->scopeChain(), false);
    envobj->setInitialPropertyInfallible(emptyDeclEnvShape);
    envobj->setPrivate(fp);

    return envobj;
}

namespace js {

CallObject *
CreateFunCallObject(JSContext *cx, StackFrame *fp)
{
    JS_ASSERT(fp->isNonEvalFunctionFrame());
    JS_ASSERT(!fp->hasCallObj());

    JSObject *scopeChain = &fp->scopeChain();
    JS_ASSERT_IF(scopeChain->isWith() || scopeChain->isBlock() || scopeChain->isCall(),
                 scopeChain->getPrivate() != fp);

    



    if (JSAtom *lambdaName = CallObjectLambdaName(fp->fun())) {
        scopeChain = NewDeclEnvObject(cx, fp);
        if (!scopeChain)
            return NULL;

        if (!DefineNativeProperty(cx, scopeChain, ATOM_TO_JSID(lambdaName),
                                  ObjectValue(fp->callee()), NULL, NULL,
                                  JSPROP_PERMANENT | JSPROP_READONLY, 0, 0)) {
            return NULL;
        }
    }

    CallObject *callobj = CallObject::create(cx, fp->script(), *scopeChain, &fp->callee());
    if (!callobj)
        return NULL;

    callobj->setStackFrame(fp);
    fp->setScopeChainWithOwnCallObj(*callobj);
    return callobj;
}

CallObject *
CreateEvalCallObject(JSContext *cx, StackFrame *fp)
{
    CallObject *callobj = CallObject::create(cx, fp->script(), fp->scopeChain(), NULL);
    if (!callobj)
        return NULL;

    callobj->setStackFrame(fp);
    fp->setScopeChainWithOwnCallObj(*callobj);
    return callobj;
}

} 

JSObject * JS_FASTCALL
js_CreateCallObjectOnTrace(JSContext *cx, JSFunction *fun, JSObject *callee, JSObject *scopeChain)
{
    JS_ASSERT(!js_IsNamedLambda(fun));
    JS_ASSERT(scopeChain);
    JS_ASSERT(callee);
    return CallObject::create(cx, fun->script(), *scopeChain, callee);
}

JS_DEFINE_CALLINFO_4(extern, OBJECT, js_CreateCallObjectOnTrace, CONTEXT, FUNCTION, OBJECT, OBJECT,
                     0, nanojit::ACCSET_STORE_ANY)

void
js_PutCallObject(StackFrame *fp)
{
    CallObject &callobj = fp->callObj().asCall();
    JS_ASSERT(callobj.maybeStackFrame() == fp);
    JS_ASSERT_IF(fp->isEvalFrame(), fp->isStrictEvalFrame());
    JS_ASSERT(fp->isEvalFrame() == callobj.isForEval());

    
    if (fp->hasArgsObj()) {
        if (!fp->hasOverriddenArgs())
            callobj.setArguments(ObjectValue(fp->argsObj()));
        js_PutArgsObject(fp);
    }

    JSScript *script = fp->script();
    Bindings &bindings = script->bindings;

    if (callobj.isForEval()) {
        JS_ASSERT(script->strictModeCode);
        JS_ASSERT(bindings.countArgs() == 0);

        
        callobj.copyValues(0, NULL, bindings.countVars(), fp->slots());
    } else {
        JSFunction *fun = fp->fun();
        JS_ASSERT(fun == callobj.getCalleeFunction());
        JS_ASSERT(script == fun->script());

        uintN n = bindings.countArgsAndVars();
        if (n > 0) {
            uint32 nvars = bindings.countVars();
            uint32 nargs = bindings.countArgs();
            JS_ASSERT(fun->nargs == nargs);
            JS_ASSERT(nvars + nargs == n);

            JSScript *script = fun->script();
            if (script->usesEval
#ifdef JS_METHODJIT
                || script->debugMode
#endif
                ) {
                callobj.copyValues(nargs, fp->formalArgs(), nvars, fp->slots());
            } else {
                



                uint32 nclosed = script->nClosedArgs;
                for (uint32 i = 0; i < nclosed; i++) {
                    uint32 e = script->getClosedArg(i);
                    callobj.setArg(e, fp->formalArg(e));
                }

                nclosed = script->nClosedVars;
                for (uint32 i = 0; i < nclosed; i++) {
                    uint32 e = script->getClosedVar(i);
                    callobj.setVar(e, fp->slots()[e]);
                }
            }

            



            types::TypeScriptNesting *nesting = script->nesting();
            if (nesting && script->isOuterFunction) {
                nesting->argArray = callobj.argArray();
                nesting->varArray = callobj.varArray();
            }
        }

        
        if (js_IsNamedLambda(fun)) {
            JSObject *env = callobj.getParent();

            JS_ASSERT(env->isDeclEnv());
            JS_ASSERT(env->getPrivate() == fp);
            env->setPrivate(NULL);
        }
    }

    callobj.setStackFrame(NULL);
}

JSBool JS_FASTCALL
js_PutCallObjectOnTrace(JSObject *obj, uint32 nargs, Value *argv,
                        uint32 nvars, Value *slots)
{
    CallObject &callobj = obj->asCall();
    JS_ASSERT(!callobj.maybeStackFrame());

    uintN n = nargs + nvars;
    if (n != 0)
        callobj.copyValues(nargs, argv, nvars, slots);

    return true;
}

JS_DEFINE_CALLINFO_5(extern, BOOL, js_PutCallObjectOnTrace, OBJECT, UINT32, VALUEPTR,
                     UINT32, VALUEPTR, 0, nanojit::ACCSET_STORE_ANY)

namespace js {

static JSBool
GetCallArguments(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    CallObject &callobj = obj->asCall();

    StackFrame *fp = callobj.maybeStackFrame();
    if (fp && !fp->hasOverriddenArgs()) {
        JSObject *argsobj = js_GetArgsObject(cx, fp);
        if (!argsobj)
            return false;
        vp->setObject(*argsobj);
    } else {
        *vp = callobj.getArguments();
    }
    return true;
}

static JSBool
SetCallArguments(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp)
{
    CallObject &callobj = obj->asCall();

    if (StackFrame *fp = callobj.maybeStackFrame())
        fp->setOverriddenArgs();
    callobj.setArguments(*vp);
    return true;
}

JSBool
GetCallArg(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    CallObject &callobj = obj->asCall();
    JS_ASSERT((int16) JSID_TO_INT(id) == JSID_TO_INT(id));
    uintN i = (uint16) JSID_TO_INT(id);

    if (StackFrame *fp = callobj.maybeStackFrame())
        *vp = fp->formalArg(i);
    else
        *vp = callobj.arg(i);
    return true;
}

JSBool
SetCallArg(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp)
{
    CallObject &callobj = obj->asCall();
    JS_ASSERT((int16) JSID_TO_INT(id) == JSID_TO_INT(id));
    uintN i = (uint16) JSID_TO_INT(id);

    if (StackFrame *fp = callobj.maybeStackFrame())
        fp->formalArg(i) = *vp;
    else
        callobj.setArg(i, *vp);

    JSFunction *fun = callobj.getCalleeFunction();
    JSScript *script = fun->script();
    if (!script->ensureHasTypes(cx, fun))
        return false;

    TypeScript::SetArgument(cx, script, i, *vp);

    return true;
}

JSBool
GetCallUpvar(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    CallObject &callobj = obj->asCall();
    JS_ASSERT((int16) JSID_TO_INT(id) == JSID_TO_INT(id));
    uintN i = (uint16) JSID_TO_INT(id);

    *vp = callobj.getCallee()->getFlatClosureUpvar(i);
    return true;
}

JSBool
SetCallUpvar(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp)
{
    CallObject &callobj = obj->asCall();
    JS_ASSERT((int16) JSID_TO_INT(id) == JSID_TO_INT(id));
    uintN i = (uint16) JSID_TO_INT(id);

    callobj.getCallee()->setFlatClosureUpvar(i, *vp);
    return true;
}

JSBool
GetCallVar(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    CallObject &callobj = obj->asCall();
    JS_ASSERT((int16) JSID_TO_INT(id) == JSID_TO_INT(id));
    uintN i = (uint16) JSID_TO_INT(id);

    if (StackFrame *fp = callobj.maybeStackFrame())
        *vp = fp->varSlot(i);
    else
        *vp = callobj.var(i);
    return true;
}

JSBool
SetCallVar(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp)
{
    CallObject &callobj = obj->asCall();

    JS_ASSERT((int16) JSID_TO_INT(id) == JSID_TO_INT(id));
    uintN i = (uint16) JSID_TO_INT(id);

    






#ifdef JS_TRACER
    if (JS_ON_TRACE(cx)) {
        TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);
        if (tm->recorder && tm->tracecx)
            AbortRecording(cx, "upvar write in nested tree");
    }
#endif

    if (StackFrame *fp = callobj.maybeStackFrame())
        fp->varSlot(i) = *vp;
    else
        callobj.setVar(i, *vp);

    JSFunction *fun = callobj.getCalleeFunction();
    JSScript *script = fun->script();
    if (!script->ensureHasTypes(cx, fun))
        return false;

    TypeScript::SetLocal(cx, script, i, *vp);

    return true;
}

} 

#if JS_TRACER
JSBool JS_FASTCALL
js_SetCallArg(JSContext *cx, JSObject *obj, jsid slotid, ValueArgType arg)
{
    Value argcopy = ValueArgToConstRef(arg);
    return SetCallArg(cx, obj, slotid, false , &argcopy);
}
JS_DEFINE_CALLINFO_4(extern, BOOL, js_SetCallArg, CONTEXT, OBJECT, JSID, VALUE, 0,
                     nanojit::ACCSET_STORE_ANY)

JSBool JS_FASTCALL
js_SetCallVar(JSContext *cx, JSObject *obj, jsid slotid, ValueArgType arg)
{
    Value argcopy = ValueArgToConstRef(arg);
    return SetCallVar(cx, obj, slotid, false , &argcopy);
}
JS_DEFINE_CALLINFO_4(extern, BOOL, js_SetCallVar, CONTEXT, OBJECT, JSID, VALUE, 0,
                     nanojit::ACCSET_STORE_ANY)
#endif

static JSBool
call_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags, JSObject **objp)
{
    JS_ASSERT(!obj->getProto());

    if (!JSID_IS_ATOM(id))
        return true;

    JSObject *callee = obj->asCall().getCallee();
#ifdef DEBUG
    if (callee) {
        JSScript *script = callee->getFunctionPrivate()->script();
        JS_ASSERT(!script->bindings.hasBinding(cx, JSID_TO_ATOM(id)));
    }
#endif

    







    if (callee && id == ATOM_TO_JSID(cx->runtime->atomState.argumentsAtom)) {
        if (!DefineNativeProperty(cx, obj, id, UndefinedValue(),
                                  GetCallArguments, SetCallArguments,
                                  JSPROP_PERMANENT | JSPROP_SHARED | JSPROP_ENUMERATE,
                                  0, 0, DNP_DONT_PURGE)) {
            return false;
        }
        *objp = obj;
        return true;
    }

    
    return true;
}

static void
call_trace(JSTracer *trc, JSObject *obj)
{
    JS_ASSERT(obj->isCall());

    MaybeMarkGenerator(trc, obj);
}

JS_PUBLIC_DATA(Class) js::CallClass = {
    "Call",
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_RESERVED_SLOTS(CallObject::RESERVED_SLOTS) |
    JSCLASS_NEW_RESOLVE | JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    (JSResolveOp)call_resolve,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    call_trace
};

bool
StackFrame::getValidCalleeObject(JSContext *cx, Value *vp)
{
    if (!isFunctionFrame()) {
        vp->setNull();
        return true;
    }

    JSFunction *fun = this->fun();
    JSObject &funobj = callee();
    vp->setObject(funobj);

    




    const Value &thisv = functionThis();
    if (thisv.isObject()) {
        JS_ASSERT(funobj.getFunctionPrivate() == fun);

        if (fun->compiledFunObj() == funobj && fun->methodAtom()) {
            JSObject *thisp = &thisv.toObject();
            JSObject *first_barriered_thisp = NULL;

            do {
                




                if (!thisp->isNative())
                    continue;

                const Shape *shape = thisp->nativeLookup(cx, ATOM_TO_JSID(fun->methodAtom()));
                if (shape) {
                    









                    if (shape->isMethod() && thisp->nativeGetMethod(shape) == &funobj) {
                        if (!thisp->methodReadBarrier(cx, *shape, vp))
                            return false;
                        overwriteCallee(vp->toObject());
                        return true;
                    }

                    if (shape->hasSlot()) {
                        Value v = thisp->getSlot(shape->slot());
                        JSObject *clone;

                        if (IsFunctionObject(v, &clone) &&
                            clone->getFunctionPrivate() == fun &&
                            clone->hasMethodObj(*thisp)) {
                            





                            JS_ASSERT_IF(!clone->hasSingletonType(), clone != &funobj);
                            *vp = v;
                            overwriteCallee(*clone);
                            return true;
                        }
                    }
                }

                if (!first_barriered_thisp)
                    first_barriered_thisp = thisp;
            } while ((thisp = thisp->getProto()) != NULL);

            if (!first_barriered_thisp)
                return true;

            










            JSObject *newfunobj = CloneFunctionObject(cx, fun);
            if (!newfunobj)
                return false;
            newfunobj->setMethodObj(*first_barriered_thisp);
            overwriteCallee(*newfunobj);
            vp->setObject(*newfunobj);
            return true;
        }
    }

    return true;
}

static JSBool
fun_getProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    while (!obj->isFunction()) {
        obj = obj->getProto();
        if (!obj)
            return true;
    }
    JSFunction *fun = obj->getFunctionPrivate();

    





    if (fun->isInterpreted()) {
        fun->script()->uninlineable = true;
        MarkTypeObjectFlags(cx, fun, OBJECT_FLAG_UNINLINEABLE);
    }

    
    vp->setNull();

    
    StackFrame *fp = js_GetTopStackFrame(cx, FRAME_EXPAND_NONE);
    if (!fp)
        return true;

    while (!fp->isFunctionFrame() || fp->fun() != fun) {
        fp = fp->prev();
        if (!fp)
            return true;
    }

#ifdef JS_METHODJIT
    if (JSID_IS_ATOM(id, cx->runtime->atomState.callerAtom) && fp && fp->prev()) {
        




        JSInlinedSite *inlined;
        fp->prev()->pcQuadratic(cx->stack, fp, &inlined);
        if (inlined) {
            JSFunction *fun = fp->prev()->jit()->inlineFrames()[inlined->inlineIndex].fun;
            fun->script()->uninlineable = true;
            MarkTypeObjectFlags(cx, fun, OBJECT_FLAG_UNINLINEABLE);
        }
    }
#endif

    if (JSID_IS_ATOM(id, cx->runtime->atomState.argumentsAtom)) {
        
        if (!JS_ReportErrorFlagsAndNumber(cx, JSREPORT_WARNING | JSREPORT_STRICT, js_GetErrorMessage,
                                          NULL, JSMSG_DEPRECATED_USAGE, js_arguments_str)) {
            return false;
        }

        return js_GetArgsValue(cx, fp, vp);
    }

    if (JSID_IS_ATOM(id, cx->runtime->atomState.callerAtom)) {
        if (!fp->prev())
            return true;

        StackFrame *frame = js_GetScriptedCaller(cx, fp->prev());
        if (frame && !frame->getValidCalleeObject(cx, vp))
            return false;

        if (!vp->isObject()) {
            JS_ASSERT(vp->isNull());
            return true;
        }

        
        JSObject &caller = vp->toObject();
        if (caller.compartment() != cx->compartment) {
            vp->setNull();
        } else if (caller.isFunction()) {
            JSFunction *callerFun = caller.getFunctionPrivate();
            if (callerFun->isInterpreted() && callerFun->inStrictMode()) {
                JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, NULL,
                                             JSMSG_CALLER_IS_STRICT);
                return false;
            }
        }

        return true;
    }

    JS_NOT_REACHED("fun_getProperty");
    return false;
}





static const uint16 poisonPillProps[] = {
    ATOM_OFFSET(arguments),
    ATOM_OFFSET(caller),
};

static JSBool
fun_enumerate(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isFunction());

    jsid id;
    bool found;

    if (!obj->isBoundFunction()) {
        id = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
        if (!obj->hasProperty(cx, id, &found, JSRESOLVE_QUALIFIED))
            return false;
    }

    id = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
    if (!obj->hasProperty(cx, id, &found, JSRESOLVE_QUALIFIED))
        return false;
        
    id = ATOM_TO_JSID(cx->runtime->atomState.nameAtom);
    if (!obj->hasProperty(cx, id, &found, JSRESOLVE_QUALIFIED))
        return false;

    for (uintN i = 0; i < JS_ARRAY_LENGTH(poisonPillProps); i++) {
        const uint16 offset = poisonPillProps[i];
        id = ATOM_TO_JSID(OFFSET_TO_ATOM(cx->runtime, offset));
        if (!obj->hasProperty(cx, id, &found, JSRESOLVE_QUALIFIED))
            return false;
    }

    return true;
}

static JSObject *
ResolveInterpretedFunctionPrototype(JSContext *cx, JSObject *obj)
{
#ifdef DEBUG
    JSFunction *fun = obj->getFunctionPrivate();
    JS_ASSERT(fun->isInterpreted());
    JS_ASSERT(!fun->isFunctionPrototype());
#endif

    




    JS_ASSERT(!IsInternalFunctionObject(obj));
    JS_ASSERT(!obj->isBoundFunction());

    



    JSObject *parent = obj->getParent();
    JSObject *objProto;
    if (!js_GetClassPrototype(cx, parent, JSProto_Object, &objProto))
        return NULL;
    JSObject *proto = NewNativeClassInstance(cx, &ObjectClass, objProto, parent);
    if (!proto || !proto->setSingletonType(cx))
        return NULL;

    





    if (!obj->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom),
                             ObjectValue(*proto), JS_PropertyStub, JS_StrictPropertyStub,
                             JSPROP_PERMANENT) ||
        !proto->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.constructorAtom),
                               ObjectValue(*obj), JS_PropertyStub, JS_StrictPropertyStub, 0))
    {
       return NULL;
    }

    return proto;
}

static JSBool
fun_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
            JSObject **objp)
{
    if (!JSID_IS_ATOM(id))
        return true;

    JSFunction *fun = obj->getFunctionPrivate();

    if (JSID_IS_ATOM(id, cx->runtime->atomState.classPrototypeAtom)) {
        











        if (fun->isNative() || fun->isFunctionPrototype())
            return true;

        if (!ResolveInterpretedFunctionPrototype(cx, obj))
            return false;
        *objp = obj;
        return true;
    }

    if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom) ||
        JSID_IS_ATOM(id, cx->runtime->atomState.nameAtom)) {
        JS_ASSERT(!IsInternalFunctionObject(obj));

        Value v;
        if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom))
            v.setInt32(fun->nargs);
        else
            v.setString(fun->atom ? fun->atom : cx->runtime->emptyString);
        
        if (!DefineNativeProperty(cx, obj, id, v, JS_PropertyStub, JS_StrictPropertyStub,
                                  JSPROP_PERMANENT | JSPROP_READONLY, 0, 0)) {
            return false;
        }
        *objp = obj;
        return true;
    }

    for (uintN i = 0; i < JS_ARRAY_LENGTH(poisonPillProps); i++) {
        const uint16 offset = poisonPillProps[i];

        if (JSID_IS_ATOM(id, OFFSET_TO_ATOM(cx->runtime, offset))) {
            JS_ASSERT(!IsInternalFunctionObject(obj));

            PropertyOp getter;
            StrictPropertyOp setter;
            uintN attrs = JSPROP_PERMANENT;
            if (fun->isInterpreted() ? fun->inStrictMode() : obj->isBoundFunction()) {
                JSObject *throwTypeError = obj->getThrowTypeError();

                getter = CastAsPropertyOp(throwTypeError);
                setter = CastAsStrictPropertyOp(throwTypeError);
                attrs |= JSPROP_GETTER | JSPROP_SETTER;
            } else {
                getter = fun_getProperty;
                setter = JS_StrictPropertyStub;
            }

            if (!DefineNativeProperty(cx, obj, id, UndefinedValue(), getter, setter,
                                      attrs, 0, 0)) {
                return false;
            }
            *objp = obj;
            return true;
        }
    }

    return true;
}

#if JS_HAS_XDR


JSBool
js_XDRFunctionObject(JSXDRState *xdr, JSObject **objp)
{
    JSContext *cx;
    JSFunction *fun;
    uint32 firstword;           


    uint32 flagsword;           

    cx = xdr->cx;
    if (xdr->mode == JSXDR_ENCODE) {
        fun = (*objp)->getFunctionPrivate();
        if (!fun->isInterpreted()) {
            JSAutoByteString funNameBytes;
            if (const char *name = GetFunctionNameBytes(cx, fun, &funNameBytes)) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_SCRIPTED_FUNCTION,
                                     name);
            }
            return false;
        }
        firstword = (fun->u.i.skipmin << 2) | !!fun->atom;
        flagsword = (fun->nargs << 16) | fun->flags;
    } else {
        fun = js_NewFunction(cx, NULL, NULL, 0, JSFUN_INTERPRETED, NULL, NULL);
        if (!fun)
            return false;
        fun->clearParent();
        if (!fun->clearType(cx))
            return false;
    }

    AutoObjectRooter tvr(cx, fun);

    if (!JS_XDRUint32(xdr, &firstword))
        return false;
    if ((firstword & 1U) && !js_XDRAtom(xdr, &fun->atom))
        return false;
    if (!JS_XDRUint32(xdr, &flagsword))
        return false;

    if (xdr->mode == JSXDR_DECODE) {
        fun->nargs = flagsword >> 16;
        JS_ASSERT((flagsword & JSFUN_KINDMASK) >= JSFUN_INTERPRETED);
        fun->flags = uint16(flagsword);
        fun->u.i.skipmin = uint16(firstword >> 2);
    }

    



    JSScript *script = fun->u.i.script;
    if (!js_XDRScript(xdr, &script))
        return false;
    fun->u.i.script = script;

    if (xdr->mode == JSXDR_DECODE) {
        *objp = fun;
        fun->u.i.script->setOwnerObject(fun);
        if (!fun->u.i.script->typeSetFunction(cx, fun))
            return false;
        JS_ASSERT(fun->nargs == fun->script()->bindings.countArgs());
        js_CallNewScriptHook(cx, fun->script(), fun);
    }

    return true;
}

#else  

#define js_XDRFunctionObject NULL

#endif 






static JSBool
fun_hasInstance(JSContext *cx, JSObject *obj, const Value *v, JSBool *bp)
{
    while (obj->isFunction()) {
        if (!obj->isBoundFunction())
            break;
        obj = obj->getBoundFunctionTarget();
    }

    Value pval;
    if (!obj->getProperty(cx, cx->runtime->atomState.classPrototypeAtom, &pval))
        return JS_FALSE;

    if (pval.isPrimitive()) {
        



        js_ReportValueError(cx, JSMSG_BAD_PROTOTYPE, -1, ObjectValue(*obj), NULL);
        return JS_FALSE;
    }

    *bp = js_IsDelegate(cx, &pval.toObject(), *v);
    return JS_TRUE;
}

static void
fun_trace(JSTracer *trc, JSObject *obj)
{
    
    JSFunction *fun = (JSFunction *) obj->getPrivate();
    if (!fun)
        return;

    if (fun != obj) {
        
        MarkObject(trc, *fun, "private");

        
        if (fun->isFlatClosure() && fun->script()->bindings.hasUpvars()) {
            MarkValueRange(trc, fun->script()->bindings.countUpvars(),
                           obj->getFlatClosureUpvars(), "upvars");
        }
        return;
    }

    if (fun->atom)
        MarkString(trc, fun->atom, "atom");

    if (fun->isInterpreted() && fun->script()) {
        CheckScriptOwner(fun->script(), obj);
        MarkScript(trc, fun->script(), "script");
    }
}

static void
fun_finalize(JSContext *cx, JSObject *obj)
{
    obj->finalizeUpvarsIfFlatClosure();
}






JS_FRIEND_DATA(Class) js::FunctionClass = {
    js_Function_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE |
    JSCLASS_HAS_RESERVED_SLOTS(JSFunction::CLASS_RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Function),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    fun_enumerate,
    (JSResolveOp)fun_resolve,
    JS_ConvertStub,
    fun_finalize,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,
    fun_hasInstance,
    fun_trace
};

JSString *
fun_toStringHelper(JSContext *cx, JSObject *obj, uintN indent)
{
    if (!obj->isFunction()) {
        if (IsFunctionProxy(obj))
            return Proxy::fun_toString(cx, obj, indent);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_INCOMPATIBLE_PROTO,
                             js_Function_str, js_toString_str,
                             "object");
        return NULL;
    }

    JSFunction *fun = obj->getFunctionPrivate();
    if (!fun)
        return NULL;

    if (!indent && !cx->compartment->toSourceCache.empty()) {
        ToSourceCache::Ptr p = cx->compartment->toSourceCache.ref().lookup(fun);
        if (p)
            return p->value;
    }

    JSString *str = JS_DecompileFunction(cx, fun, indent);
    if (!str)
        return NULL;

    if (!indent) {
        Maybe<ToSourceCache> &lazy = cx->compartment->toSourceCache;

        if (lazy.empty()) {
            lazy.construct();
            if (!lazy.ref().init())
                return NULL;
        }

        if (!lazy.ref().put(fun, str))
            return NULL;
    }

    return str;
}

static JSBool
fun_toString(JSContext *cx, uintN argc, Value *vp)
{
    JS_ASSERT(IsFunctionObject(vp[0]));
    uint32_t indent = 0;

    if (argc != 0 && !ValueToECMAUint32(cx, vp[2], &indent))
        return false;

    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;

    JSString *str = fun_toStringHelper(cx, obj, indent);
    if (!str)
        return false;

    vp->setString(str);
    return true;
}

#if JS_HAS_TOSOURCE
static JSBool
fun_toSource(JSContext *cx, uintN argc, Value *vp)
{
    JS_ASSERT(IsFunctionObject(vp[0]));

    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;

    JSString *str = fun_toStringHelper(cx, obj, JS_DONT_PRETTY_PRINT);
    if (!str)
        return false;

    vp->setString(str);
    return true;
}
#endif

JSBool
js_fun_call(JSContext *cx, uintN argc, Value *vp)
{
    Value fval = vp[1];

    if (!js_IsCallable(fval)) {
        ReportIncompatibleMethod(cx, CallReceiverFromVp(vp), &FunctionClass);
        return false;
    }

    Value *argv = vp + 2;
    Value thisv;
    if (argc == 0) {
        thisv.setUndefined();
    } else {
        thisv = argv[0];

        argc--;
        argv++;
    }

    
    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, argc, &args))
        return JS_FALSE;

    
    args.calleev() = fval;
    args.thisv() = thisv;
    memcpy(args.array(), argv, argc * sizeof *argv);

    bool ok = Invoke(cx, args);
    *vp = args.rval();
    return ok;
}


JSBool
js_fun_apply(JSContext *cx, uintN argc, Value *vp)
{
    
    Value fval = vp[1];
    if (!js_IsCallable(fval)) {
        ReportIncompatibleMethod(cx, CallReceiverFromVp(vp), &FunctionClass);
        return false;
    }

    
    if (argc < 2 || vp[3].isNullOrUndefined())
        return js_fun_call(cx, (argc > 0) ? 1 : 0, vp);

    

    
    if (!vp[3].isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_APPLY_ARGS, js_apply_str);
        return false;
    }

    



    JSObject *aobj = &vp[3].toObject();
    jsuint length;
    if (!js_GetLengthProperty(cx, aobj, &length))
        return false;

    
    if (length > StackSpace::ARGS_LENGTH_MAX) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_TOO_MANY_FUN_APPLY_ARGS);
        return false;
    }

    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, length, &args))
        return false;

    
    args.calleev() = fval;
    args.thisv() = vp[2];

    
    if (!GetElements(cx, aobj, length, args.array()))
        return false;

    
    if (!Invoke(cx, args))
        return false;
    *vp = args.rval();
    return true;
}

namespace js {

JSBool
CallOrConstructBoundFunction(JSContext *cx, uintN argc, Value *vp);

}

inline bool
JSObject::initBoundFunction(JSContext *cx, const Value &thisArg,
                            const Value *args, uintN argslen)
{
    JS_ASSERT(isFunction());

    flags |= JSObject::BOUND_FUNCTION;
    setSlot(JSSLOT_BOUND_FUNCTION_THIS, thisArg);
    setSlot(JSSLOT_BOUND_FUNCTION_ARGS_COUNT, PrivateUint32Value(argslen));
    if (argslen != 0) {
        
        if (!toDictionaryMode(cx))
            return false;
        JS_ASSERT(slotSpan() == JSSLOT_FREE(&FunctionClass));
        if (!setSlotSpan(cx, slotSpan() + argslen))
            return false;

        copySlotRange(FUN_CLASS_RESERVED_SLOTS, args, argslen);
    }
    return true;
}

inline JSObject *
JSObject::getBoundFunctionTarget() const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(isBoundFunction());

    
    return getParent();
}

inline const js::Value &
JSObject::getBoundFunctionThis() const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(isBoundFunction());

    return getSlot(JSSLOT_BOUND_FUNCTION_THIS);
}

inline const js::Value &
JSObject::getBoundFunctionArgument(uintN which) const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(isBoundFunction());
    JS_ASSERT(which < getBoundFunctionArgumentCount());

    return getSlot(FUN_CLASS_RESERVED_SLOTS + which);
}

inline size_t
JSObject::getBoundFunctionArgumentCount() const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(isBoundFunction());

    return getSlot(JSSLOT_BOUND_FUNCTION_ARGS_COUNT).toPrivateUint32();
}

namespace js {


JSBool
CallOrConstructBoundFunction(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj = &vp[0].toObject();
    JS_ASSERT(obj->isFunction());
    JS_ASSERT(obj->isBoundFunction());

    bool constructing = IsConstructing(vp);

    
    uintN argslen = obj->getBoundFunctionArgumentCount();

    if (argc + argslen > StackSpace::ARGS_LENGTH_MAX) {
        js_ReportAllocationOverflow(cx);
        return false;
    }

    
    JSObject *target = obj->getBoundFunctionTarget();

    
    const Value &boundThis = obj->getBoundFunctionThis();

    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, argc + argslen, &args))
        return false;

    
    for (uintN i = 0; i < argslen; i++)
        args[i] = obj->getBoundFunctionArgument(i);
    memcpy(args.array() + argslen, vp + 2, argc * sizeof(Value));

    
    args.calleev().setObject(*target);

    if (!constructing)
        args.thisv() = boundThis;

    if (constructing ? !InvokeConstructor(cx, args) : !Invoke(cx, args))
        return false;

    *vp = args.rval();
    return true;
}

}

#if JS_HAS_GENERATORS
static JSBool
fun_isGenerator(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *funobj;
    if (!IsFunctionObject(vp[1], &funobj)) {
        JS_SET_RVAL(cx, vp, BooleanValue(false));
        return true;
    }

    JSFunction *fun = funobj->getFunctionPrivate();

    bool result = false;
    if (fun->isInterpreted()) {
        JSScript *script = fun->script();
        JS_ASSERT(script->length != 0);
        result = script->code[0] == JSOP_GENERATOR;
    }

    JS_SET_RVAL(cx, vp, BooleanValue(result));
    return true;
}
#endif


static JSBool
fun_bind(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    Value &thisv = args.thisv();

    
    if (!js_IsCallable(thisv)) {
        ReportIncompatibleMethod(cx, args, &FunctionClass);
        return false;
    }

    JSObject *target = &thisv.toObject();

    
    Value *boundArgs = NULL;
    uintN argslen = 0;
    if (args.length() > 1) {
        boundArgs = args.array() + 1;
        argslen = args.length() - 1;
    }

    
    uintN length = 0;
    if (target->isFunction()) {
        uintN nargs = target->getFunctionPrivate()->nargs;
        if (nargs > argslen)
            length = nargs - argslen;
    }

    
    JSAtom *name = target->isFunction() ? target->getFunctionPrivate()->atom : NULL;

    
    JSObject *funobj =
        js_NewFunction(cx, NULL, CallOrConstructBoundFunction, length,
                       JSFUN_CONSTRUCTOR, target, name);
    if (!funobj)
        return false;

    
    Value thisArg = args.length() >= 1 ? args[0] : UndefinedValue();
    if (!funobj->initBoundFunction(cx, thisArg, boundArgs, argslen))
        return false;

    
    

    
    args.rval().setObject(*funobj);
    return true;
}





static bool
OnBadFormal(JSContext *cx, TokenKind tt)
{
    if (tt != TOK_ERROR)
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_FORMAL);
    else
        JS_ASSERT(cx->isExceptionPending());
    return false;
}

namespace js {

JSFunctionSpec function_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,   fun_toSource,   0,0),
#endif
    JS_FN(js_toString_str,   fun_toString,   0,0),
    JS_FN(js_apply_str,      js_fun_apply,   2,0),
    JS_FN(js_call_str,       js_fun_call,    1,0),
    JS_FN("bind",            fun_bind,       1,0),
#if JS_HAS_GENERATORS
    JS_FN("isGenerator",     fun_isGenerator,0,0),
#endif
    JS_FS_END
};

JSBool
Function(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    GlobalObject *global = args.callee().getGlobal();
    if (!global->isRuntimeCodeGenEnabled(cx)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CSP_BLOCKED_FUNCTION);
        return false;
    }

    Bindings bindings(cx);
    uintN lineno;
    const char *filename = CurrentScriptFileAndLine(cx, &lineno);

    uintN n = args.length() ? args.length() - 1 : 0;
    if (n > 0) {
        









        size_t args_length = 0;
        for (uintN i = 0; i < n; i++) {
            
            JSString *arg = js_ValueToString(cx, args[i]);
            if (!arg)
                return false;
            args[i].setString(arg);

            



            size_t old_args_length = args_length;
            args_length = old_args_length + arg->length();
            if (args_length < old_args_length) {
                js_ReportAllocationOverflow(cx);
                return false;
            }
        }

        
        size_t old_args_length = args_length;
        args_length = old_args_length + n - 1;
        if (args_length < old_args_length ||
            args_length >= ~(size_t)0 / sizeof(jschar)) {
            js_ReportAllocationOverflow(cx);
            return false;
        }

        




        LifoAllocScope las(&cx->tempLifoAlloc());
        jschar *cp = cx->tempLifoAlloc().newArray<jschar>(args_length + 1);
        if (!cp) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        jschar *collected_args = cp;

        


        for (uintN i = 0; i < n; i++) {
            JSString *arg = args[i].toString();
            size_t arg_length = arg->length();
            const jschar *arg_chars = arg->getChars(cx);
            if (!arg_chars)
                return false;
            (void) js_strncpy(cp, arg_chars, arg_length);
            cp += arg_length;

            
            *cp++ = (i + 1 < n) ? ',' : 0;
        }

        
        TokenStream ts(cx);
        if (!ts.init(collected_args, args_length, filename, lineno, cx->findVersion()))
            return false;

        
        TokenKind tt = ts.getToken();
        if (tt != TOK_EOF) {
            for (;;) {
                



                if (tt != TOK_NAME)
                    return OnBadFormal(cx, tt);

                




                JSAtom *atom = ts.currentToken().t_atom;

                
                if (bindings.hasBinding(cx, atom)) {
                    JSAutoByteString name;
                    if (!js_AtomToPrintableString(cx, atom, &name))
                        return false;
                    if (!ReportCompileErrorNumber(cx, &ts, NULL,
                                                  JSREPORT_WARNING | JSREPORT_STRICT,
                                                  JSMSG_DUPLICATE_FORMAL, name.ptr())) {
                        return false;
                    }
                }

                uint16 dummy;
                if (!bindings.addArgument(cx, atom, &dummy))
                    return false;

                



                tt = ts.getToken();
                if (tt == TOK_EOF)
                    break;
                if (tt != TOK_COMMA)
                    return OnBadFormal(cx, tt);
                tt = ts.getToken();
            }
        }
    }

    JS::Anchor<JSString *> strAnchor(NULL);
    const jschar *chars;
    size_t length;

    if (args.length()) {
        JSString *str = js_ValueToString(cx, args[args.length() - 1]);
        if (!str)
            return false;
        strAnchor.set(str);
        chars = str->getChars(cx);
        length = str->length();
    } else {
        chars = cx->runtime->emptyString->chars();
        length = 0;
    }

    





    JSFunction *fun = js_NewFunction(cx, NULL, NULL, 0, JSFUN_LAMBDA | JSFUN_INTERPRETED,
                                     global, cx->runtime->atomState.anonymousAtom);
    if (!fun)
        return false;

    JSPrincipals *principals = PrincipalsForCompiledCode(args, cx);
    bool ok = Compiler::compileFunctionBody(cx, fun, principals, &bindings,
                                            chars, length, filename, lineno,
                                            cx->findVersion());
    args.rval().setObject(*fun);
    return ok;
}

bool
IsBuiltinFunctionConstructor(JSFunction *fun)
{
    return fun->maybeNative() == Function;
}

const Shape *
LookupInterpretedFunctionPrototype(JSContext *cx, JSObject *funobj)
{
#ifdef DEBUG
    JSFunction *fun = funobj->getFunctionPrivate();
    JS_ASSERT(fun->isInterpreted());
    JS_ASSERT(!fun->isFunctionPrototype());
    JS_ASSERT(!funobj->isBoundFunction());
#endif

    jsid id = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
    const Shape *shape = funobj->nativeLookup(cx, id);
    if (!shape) {
        if (!ResolveInterpretedFunctionPrototype(cx, funobj))
            return NULL;
        shape = funobj->nativeLookup(cx, id);
    }
    JS_ASSERT(!shape->configurable());
    JS_ASSERT(shape->isDataDescriptor());
    JS_ASSERT(shape->hasSlot());
    JS_ASSERT(!shape->isMethod());
    return shape;
}

} 

JSFunction *
js_NewFunction(JSContext *cx, JSObject *funobj, Native native, uintN nargs,
               uintN flags, JSObject *parent, JSAtom *atom)
{
    JSFunction *fun;

    if (funobj) {
        JS_ASSERT(funobj->isFunction());
        funobj->setParent(parent);
    } else {
        funobj = NewFunction(cx, parent);
        if (!funobj)
            return NULL;
        if (native && !funobj->setSingletonType(cx))
            return NULL;
    }
    JS_ASSERT(!funobj->getPrivate());
    fun = static_cast<JSFunction *>(funobj);

    
    fun->nargs = uint16(nargs);
    fun->flags = flags & (JSFUN_FLAGS_MASK | JSFUN_KINDMASK | JSFUN_TRCINFO);
    if ((flags & JSFUN_KINDMASK) >= JSFUN_INTERPRETED) {
        JS_ASSERT(!native);
        JS_ASSERT(nargs == 0);
        fun->u.i.skipmin = 0;
        fun->u.i.script = NULL;
    } else {
        fun->u.n.clasp = NULL;
        if (flags & JSFUN_TRCINFO) {
#ifdef JS_TRACER
            JSNativeTraceInfo *trcinfo =
                JS_FUNC_TO_DATA_PTR(JSNativeTraceInfo *, native);
            fun->u.n.native = (Native) trcinfo->native;
            fun->u.n.trcinfo = trcinfo;
#else
            fun->u.n.trcinfo = NULL;
#endif
        } else {
            fun->u.n.native = native;
            fun->u.n.trcinfo = NULL;
        }
        JS_ASSERT(fun->u.n.native);
    }
    fun->atom = atom;

    
    fun->setPrivate(fun);
    return fun;
}

JSObject * JS_FASTCALL
js_CloneFunctionObject(JSContext *cx, JSFunction *fun, JSObject *parent,
                       JSObject *proto)
{
    JS_ASSERT(parent);
    JS_ASSERT(proto);

    JSObject *clone;
    if (cx->compartment == fun->compartment()) {
        



        clone = NewNativeClassInstance(cx, &FunctionClass, proto, parent);
        if (!clone)
            return NULL;

        






        if (fun->getProto() == proto && !fun->hasSingletonType())
            clone->setType(fun->type());

        clone->setPrivate(fun);
    } else {
        



        clone = NewFunction(cx, parent);
        if (!clone)
            return NULL;

        JSFunction *cfun = (JSFunction *) clone;
        cfun->nargs = fun->nargs;
        cfun->flags = fun->flags;
        cfun->u = fun->getFunctionPrivate()->u;
        cfun->atom = fun->atom;
        clone->setPrivate(cfun);
        if (cfun->isInterpreted()) {
            JSScript *script = cfun->script();
            JS_ASSERT(script);
            JS_ASSERT(script->compartment() == fun->compartment());
            JS_ASSERT(script->compartment() != cx->compartment);
            JS_OPT_ASSERT(script->ownerObject == fun);

            cfun->u.i.script = NULL;
            JSScript *cscript = js_CloneScript(cx, script);
            if (!cscript)
                return NULL;

            cfun->u.i.script = cscript;
            if (!cfun->u.i.script->typeSetFunction(cx, cfun))
                return NULL;

            cfun->script()->setOwnerObject(cfun);
            js_CallNewScriptHook(cx, cfun->script(), cfun);
            Debugger::onNewScript(cx, cfun->script(), cfun, Debugger::NewHeldScript);
        }
    }
    return clone;
}

#ifdef JS_TRACER
JS_DEFINE_CALLINFO_4(extern, OBJECT, js_CloneFunctionObject, CONTEXT, FUNCTION, OBJECT, OBJECT, 0,
                     nanojit::ACCSET_STORE_ANY)
#endif






JSObject * JS_FASTCALL
js_AllocFlatClosure(JSContext *cx, JSFunction *fun, JSObject *scopeChain)
{
    JS_ASSERT(fun->isFlatClosure());
    JS_ASSERT(JSScript::isValidOffset(fun->script()->upvarsOffset) ==
              fun->script()->bindings.hasUpvars());
    JS_ASSERT_IF(JSScript::isValidOffset(fun->script()->upvarsOffset),
                 fun->script()->upvars()->length == fun->script()->bindings.countUpvars());

    JSObject *closure = CloneFunctionObject(cx, fun, scopeChain, true);
    if (!closure)
        return closure;

    uint32 nslots = fun->script()->bindings.countUpvars();
    if (nslots == 0)
        return closure;

    Value *upvars = (Value *) cx->malloc_(nslots * sizeof(Value));
    if (!upvars)
        return NULL;

    closure->setFlatClosureUpvars(upvars);
    return closure;
}

JS_DEFINE_CALLINFO_3(extern, OBJECT, js_AllocFlatClosure,
                     CONTEXT, FUNCTION, OBJECT, 0, nanojit::ACCSET_STORE_ANY)

JSObject *
js_NewFlatClosure(JSContext *cx, JSFunction *fun, JSOp op, size_t oplen)
{
    








    VOUCH_DOES_NOT_REQUIRE_STACK();
    JSObject *scopeChain = &cx->fp()->scopeChain();

    JSObject *closure = js_AllocFlatClosure(cx, fun, scopeChain);
    if (!closure || !fun->script()->bindings.hasUpvars())
        return closure;

    Value *upvars = closure->getFlatClosureUpvars();
    uintN level = fun->script()->staticLevel;
    JSUpvarArray *uva = fun->script()->upvars();

    for (uint32 i = 0, n = uva->length; i < n; i++)
        upvars[i] = GetUpvar(cx, level, uva->vector[i]);

    return closure;
}

JSFunction *
js_DefineFunction(JSContext *cx, JSObject *obj, jsid id, Native native,
                  uintN nargs, uintN attrs)
{
    PropertyOp gop;
    StrictPropertyOp sop;
    JSFunction *fun;

    if (attrs & JSFUN_STUB_GSOPS) {
        





        attrs &= ~JSFUN_STUB_GSOPS;
        gop = JS_PropertyStub;
        sop = JS_StrictPropertyStub;
    } else {
        gop = NULL;
        sop = NULL;
    }

    







































    bool wasDelegate = obj->isDelegate();

    fun = js_NewFunction(cx, NULL, native, nargs,
                         attrs & (JSFUN_FLAGS_MASK | JSFUN_TRCINFO),
                         obj,
                         JSID_IS_ATOM(id) ? JSID_TO_ATOM(id) : NULL);
    if (!fun)
        return NULL;

    if (!wasDelegate && obj->isDelegate())
        obj->clearDelegate();

    if (!obj->defineProperty(cx, id, ObjectValue(*fun), gop, sop, attrs & ~JSFUN_FLAGS_MASK))
        return NULL;

    return fun;
}

JS_STATIC_ASSERT((JSV2F_CONSTRUCT & JSV2F_SEARCH_STACK) == 0);

JSFunction *
js_ValueToFunction(JSContext *cx, const Value *vp, uintN flags)
{
    JSObject *funobj;
    if (!IsFunctionObject(*vp, &funobj)) {
        js_ReportIsNotFunction(cx, vp, flags);
        return NULL;
    }
    return funobj->getFunctionPrivate();
}

JSObject *
js_ValueToFunctionObject(JSContext *cx, Value *vp, uintN flags)
{
    JSObject *funobj;
    if (!IsFunctionObject(*vp, &funobj)) {
        js_ReportIsNotFunction(cx, vp, flags);
        return NULL;
    }

    return funobj;
}

JSObject *
js_ValueToCallableObject(JSContext *cx, Value *vp, uintN flags)
{
    if (vp->isObject()) {
        JSObject *callable = &vp->toObject();
        if (callable->isCallable())
            return callable;
    }

    js_ReportIsNotFunction(cx, vp, flags);
    return NULL;
}

void
js_ReportIsNotFunction(JSContext *cx, const Value *vp, uintN flags)
{
    const char *name = NULL, *source = NULL;
    AutoValueRooter tvr(cx);
    uintN error = (flags & JSV2F_CONSTRUCT) ? JSMSG_NOT_CONSTRUCTOR : JSMSG_NOT_FUNCTION;
    LeaveTrace(cx);

    











    ptrdiff_t spindex = 0;

    FrameRegsIter i(cx);
    if (!i.done()) {
        uintN depth = js_ReconstructStackDepth(cx, i.fp()->script(), i.pc());
        Value *simsp = i.fp()->base() + depth;
        if (i.fp()->base() <= vp && vp < Min(simsp, i.sp()))
            spindex = vp - simsp;
    }

    if (!spindex)
        spindex = ((flags & JSV2F_SEARCH_STACK) ? JSDVG_SEARCH_STACK : JSDVG_IGNORE_STACK);

    js_ReportValueError3(cx, error, spindex, *vp, NULL, name, source);
}
