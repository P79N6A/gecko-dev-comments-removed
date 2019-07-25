










































#include <string.h>

#include "mozilla/Util.h"

#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jspropertytree.h"
#include "jsproxy.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jsexn.h"

#include "frontend/BytecodeCompiler.h"
#include "frontend/BytecodeEmitter.h"
#include "frontend/TokenStream.h"
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

using namespace mozilla;
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

js::ArgumentsObject *
ArgumentsObject::create(JSContext *cx, uint32 argc, JSObject &callee, StackFrame *fp)
{
    JS_ASSERT(argc <= StackSpace::ARGS_LENGTH_MAX);

    JSObject *proto = callee.getGlobal()->getOrCreateObjectPrototype(cx);
    if (!proto)
        return NULL;

    TypeObject *type = proto->getNewType(cx);
    if (!type)
        return NULL;

    bool strict = callee.toFunction()->inStrictMode();
    Class *clasp = strict ? &StrictArgumentsObjectClass : &NormalArgumentsObjectClass;
    Shape *emptyArgumentsShape =
        EmptyShape::getInitialShape(cx, clasp, proto,
                                    proto->getParent(), FINALIZE_KIND,
                                    BaseShape::INDEXED);
    if (!emptyArgumentsShape)
        return NULL;

    ArgumentsData *data = (ArgumentsData *)
        cx->malloc_(offsetof(ArgumentsData, slots) + argc * sizeof(Value));
    if (!data)
        return NULL;

    data->callee.init(ObjectValue(callee));
    InitValueRange(data->slots, argc, false);

    
    JSObject *obj = JSObject::create(cx, FINALIZE_KIND, emptyArgumentsShape, type, NULL);
    if (!obj)
        return NULL;

    ArgumentsObject *argsobj = obj->asArguments();

    JS_ASSERT(UINT32_MAX > (uint64(argc) << PACKED_BITS_COUNT));
    argsobj->initInitialLength(argc);
    argsobj->initData(data);
    argsobj->setStackFrame(strict ? NULL : fp);

    JS_ASSERT(argsobj->numFixedSlots() >= NormalArgumentsObject::RESERVED_SLOTS);
    JS_ASSERT(argsobj->numFixedSlots() >= StrictArgumentsObject::RESERVED_SLOTS);

    return argsobj;
}

struct STATIC_SKIP_INFERENCE PutArg
{
    PutArg(JSCompartment *comp, HeapValue *dst) : dst(dst), compartment(comp) {}
    HeapValue *dst;
    JSCompartment *compartment;
    bool operator()(uintN, Value *src) {
        JS_ASSERT(dst->isMagic(JS_ARGS_HOLE) || dst->isUndefined());
        if (!dst->isMagic(JS_ARGS_HOLE))
            dst->set(compartment, *src);
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
        ArgumentsObject::create(cx, fp->numActualArgs(), fp->callee(), fp);
    if (!argsobj)
        return argsobj;

    








    if (argsobj->isStrictArguments())
        fp->forEachCanonicalActualArg(PutArg(cx->compartment, argsobj->data()->slots));
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
        JSCompartment *comp = fp->scopeChain().compartment();
        fp->forEachCanonicalActualArg(PutArg(comp, argsobj.data()->slots));
        argsobj.setStackFrame(NULL);
    } else {
        JS_ASSERT(!argsobj.maybeStackFrame());
    }
}

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

static void
args_trace(JSTracer *trc, JSObject *obj)
{
    ArgumentsObject *argsobj = obj->asArguments();
    ArgumentsData *data = argsobj->data();
    MarkValue(trc, data->callee, js_callee_str);
    MarkValueRange(trc, argsobj->initialLength(), data->slots, js_arguments_str);

    








#if JS_HAS_GENERATORS
    StackFrame *fp = argsobj->maybeStackFrame();
    if (fp && fp->isFloatingGenerator())
        MarkObject(trc, js_FloatingFrameToGenerator(fp)->obj, "generator object");
#endif
}







Class js::NormalArgumentsObjectClass = {
    "Arguments",
    JSCLASS_NEW_RESOLVE |
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
    JSCLASS_NEW_RESOLVE |
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
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_RESERVED_SLOTS(CallObject::DECL_ENV_RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
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
    types::TypeObject *type = cx->compartment->getEmptyType(cx);
    if (!type)
        return NULL;

    JSObject *parent = fp->scopeChain().getGlobal();
    Shape *emptyDeclEnvShape =
        EmptyShape::getInitialShape(cx, &DeclEnvClass, NULL,
                                    parent, CallObject::DECL_ENV_FINALIZE_KIND);
    if (!emptyDeclEnvShape)
        return NULL;

    JSObject *envobj = JSObject::create(cx, CallObject::DECL_ENV_FINALIZE_KIND,
                                        emptyDeclEnvShape, type, NULL);
    if (!envobj)
        return NULL;
    envobj->setPrivate(fp);

    if (!envobj->setInternalScopeChain(cx, &fp->scopeChain()))
        return NULL;

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

void
js_PutCallObject(StackFrame *fp)
{
    CallObject &callobj = fp->callObj().asCall();
    JS_ASSERT(callobj.maybeStackFrame() == fp);
    JS_ASSERT_IF(fp->isEvalFrame(), fp->isStrictEvalFrame());
    JS_ASSERT(fp->isEvalFrame() == callobj.isForEval());

    
    if (fp->hasArgsObj()) {
        if (!fp->hasOverriddenArgs())
            callobj.initArguments(ObjectValue(fp->argsObj()));
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
        JS_ASSERT(script == callobj.getCalleeFunction()->script());
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
#ifdef JS_GC_ZEAL
                    callobj.setArg(e, fp->formalArg(e));
#else
                    callobj.initArgUnchecked(e, fp->formalArg(e));
#endif
                }

                nclosed = script->nClosedVars;
                for (uint32 i = 0; i < nclosed; i++) {
                    uint32 e = script->getClosedVar(i);
#ifdef JS_GC_ZEAL
                    callobj.setVar(e, fp->slots()[e]);
#else
                    callobj.initVarUnchecked(e, fp->slots()[e]);
#endif
                }
            }

            



            types::TypeScriptNesting *nesting = script->nesting();
            if (nesting && script->isOuterFunction) {
                nesting->argArray = callobj.argArray();
                nesting->varArray = callobj.varArray();
            }
        }

        
        if (js_IsNamedLambda(fun)) {
            JSObject *env = callobj.internalScopeChain();

            JS_ASSERT(env->isDeclEnv());
            JS_ASSERT(env->getPrivate() == fp);
            env->setPrivate(NULL);
        }
    }

    callobj.setStackFrame(NULL);
}

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
    if (!script->ensureHasTypes(cx))
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

    *vp = callobj.getCallee()->toFunction()->getFlatClosureUpvar(i);
    return true;
}

JSBool
SetCallUpvar(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp)
{
    CallObject &callobj = obj->asCall();
    JS_ASSERT((int16) JSID_TO_INT(id) == JSID_TO_INT(id));
    uintN i = (uint16) JSID_TO_INT(id);

    callobj.getCallee()->toFunction()->setFlatClosureUpvar(i, *vp);
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

    if (StackFrame *fp = callobj.maybeStackFrame())
        fp->varSlot(i) = *vp;
    else
        callobj.setVar(i, *vp);

    JSFunction *fun = callobj.getCalleeFunction();
    JSScript *script = fun->script();
    if (!script->ensureHasTypes(cx))
        return false;

    TypeScript::SetLocal(cx, script, i, *vp);

    return true;
}

} 

static JSBool
call_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags, JSObject **objp)
{
    JS_ASSERT(!obj->getProto());

    if (!JSID_IS_ATOM(id))
        return true;

    JSObject *callee = obj->asCall().getCallee();
#ifdef DEBUG
    if (callee) {
        JSScript *script = callee->toFunction()->script();
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

    
#if JS_HAS_GENERATORS
    StackFrame *fp = (StackFrame *) obj->getPrivate();
    if (fp && fp->isFloatingGenerator())
        MarkObject(trc, js_FloatingFrameToGenerator(fp)->obj, "generator object");
#endif
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

    JSFunction *fun = this->callee().toFunction();
    vp->setObject(*fun);

    




    const Value &thisv = functionThis();
    if (thisv.isObject() && fun->methodAtom() && !fun->isClonedMethod()) {
        JSObject *thisp = &thisv.toObject();
        JSObject *first_barriered_thisp = NULL;

        do {
            




            if (!thisp->isNative())
                continue;

            const Shape *shape = thisp->nativeLookup(cx, ATOM_TO_JSID(fun->methodAtom()));
            if (shape) {
                









                if (shape->isMethod() && thisp->nativeGetMethod(shape) == fun) {
                    if (!thisp->methodReadBarrier(cx, *shape, vp))
                        return false;
                    overwriteCallee(vp->toObject());
                    return true;
                }

                if (shape->hasSlot()) {
                    Value v = thisp->getSlot(shape->slot());
                    JSFunction *clone;

                    if (IsFunctionObject(v, &clone) &&
                        clone->script() == fun->script() &&
                        clone->methodObj() == thisp) {
                        





                        JS_ASSERT_IF(!clone->hasSingletonType(), clone != fun);
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

        










        JSFunction *newfunobj = CloneFunctionObject(cx, fun);
        if (!newfunobj)
            return false;
        newfunobj->setMethodObj(*first_barriered_thisp);
        overwriteCallee(*newfunobj);
        vp->setObject(*newfunobj);
        return true;
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
    JSFunction *fun = obj->toFunction();

    





    if (fun->isInterpreted()) {
        fun->script()->uninlineable = true;
        MarkTypeObjectFlags(cx, fun, OBJECT_FLAG_UNINLINEABLE);
    }

    
    vp->setNull();

    
    StackFrame *fp = js_GetTopStackFrame(cx, FRAME_EXPAND_NONE);
    if (!fp)
        return true;

    while (!fp->isFunctionFrame() || fp->fun() != fun || fp->isEvalFrame()) {
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
            JSFunction *callerFun = caller.toFunction();
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

    for (uintN i = 0; i < ArrayLength(poisonPillProps); i++) {
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
    JSFunction *fun = obj->toFunction();
    JS_ASSERT(fun->isInterpreted());
    JS_ASSERT(!fun->isFunctionPrototype());
#endif

    




    JS_ASSERT(!IsInternalFunctionObject(obj));
    JS_ASSERT(!obj->isBoundFunction());

    



    JSObject *objProto;
    if (!js_GetClassPrototype(cx, obj->getParent(), JSProto_Object, &objProto))
        return NULL;
    JSObject *proto = NewObjectWithGivenProto(cx, &ObjectClass, objProto, NULL);
    if (!proto || !proto->setSingletonType(cx))
        return NULL;

    





    if (!obj->defineProperty(cx, cx->runtime->atomState.classPrototypeAtom,
                             ObjectValue(*proto), JS_PropertyStub, JS_StrictPropertyStub,
                             JSPROP_PERMANENT) ||
        !proto->defineProperty(cx, cx->runtime->atomState.constructorAtom,
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

    JSFunction *fun = obj->toFunction();

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

    for (uintN i = 0; i < ArrayLength(poisonPillProps); i++) {
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
    JSScript *script;
    if (xdr->mode == JSXDR_ENCODE) {
        fun = (*objp)->toFunction();
        if (!fun->isInterpreted()) {
            JSAutoByteString funNameBytes;
            if (const char *name = GetFunctionNameBytes(cx, fun, &funNameBytes)) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_SCRIPTED_FUNCTION,
                                     name);
            }
            return false;
        }
        firstword = !!fun->atom;
        flagsword = (fun->nargs << 16) | fun->flags;
        script = fun->script();
    } else {
        fun = js_NewFunction(cx, NULL, NULL, 0, JSFUN_INTERPRETED, NULL, NULL);
        if (!fun)
            return false;
        if (!fun->clearParent(cx))
            return false;
        if (!fun->clearType(cx))
            return false;
        script = NULL;
    }

    AutoObjectRooter tvr(cx, fun);

    if (!JS_XDRUint32(xdr, &firstword))
        return false;
    if ((firstword & 1U) && !js_XDRAtom(xdr, &fun->atom))
        return false;
    if (!JS_XDRUint32(xdr, &flagsword))
        return false;

    if (!js_XDRScript(xdr, &script))
        return false;

    if (xdr->mode == JSXDR_DECODE) {
        fun->nargs = flagsword >> 16;
        JS_ASSERT((flagsword & JSFUN_KINDMASK) >= JSFUN_INTERPRETED);
        fun->flags = uint16(flagsword);
        fun->setScript(script);
        if (!script->typeSetFunction(cx, fun))
            return false;
        JS_ASSERT(fun->nargs == fun->script()->bindings.countArgs());
        js_CallNewScriptHook(cx, fun->script(), fun);
        *objp = fun;
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
        obj = obj->toFunction()->getBoundFunctionTarget();
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

inline void
JSFunction::trace(JSTracer *trc)
{
    if (isFlatClosure() && hasFlatClosureUpvars()) {
        if (HeapValue *upvars = getFlatClosureUpvars())
            MarkValueRange(trc, script()->bindings.countUpvars(), upvars, "upvars");
    }

    if (isExtended()) {
        MarkValueRange(trc, ArrayLength(toExtended()->extendedSlots),
                       toExtended()->extendedSlots, "nativeReserved");
    }

    if (atom)
        MarkAtom(trc, atom, "atom");

    if (isInterpreted()) {
        if (script())
            MarkScript(trc, script(), "script");
        if (environment())
            MarkObjectUnbarriered(trc, environment(), "fun_callscope");
    }
}

static void
fun_trace(JSTracer *trc, JSObject *obj)
{
    obj->toFunction()->trace(trc);
}

static void
fun_finalize(JSContext *cx, JSObject *obj)
{
    if (obj->toFunction()->isFlatClosure())
        obj->toFunction()->finalizeUpvars();
}






JS_FRIEND_DATA(Class) js::FunctionClass = {
    js_Function_str,
    JSCLASS_NEW_RESOLVE |
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

    JSFunction *fun = obj->toFunction();
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

static const uint32 JSSLOT_BOUND_FUNCTION_THIS       = 0;
static const uint32 JSSLOT_BOUND_FUNCTION_ARGS_COUNT = 1;

static const uint32 BOUND_FUNCTION_RESERVED_SLOTS = 2;

inline bool
JSFunction::initBoundFunction(JSContext *cx, const Value &thisArg,
                              const Value *args, uintN argslen)
{
    JS_ASSERT(isFunction());

    




    if (!toDictionaryMode(cx))
        return false;

    lastProperty()->base()->setObjectFlag(BaseShape::BOUND_FUNCTION);

    if (!setSlotSpan(cx, BOUND_FUNCTION_RESERVED_SLOTS + argslen))
        return false;

    setSlot(JSSLOT_BOUND_FUNCTION_THIS, thisArg);
    setSlot(JSSLOT_BOUND_FUNCTION_ARGS_COUNT, PrivateUint32Value(argslen));

    copySlotRange(BOUND_FUNCTION_RESERVED_SLOTS, args, argslen, false);

    return true;
}

inline JSObject *
JSFunction::getBoundFunctionTarget() const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(isBoundFunction());

    
    return getParent();
}

inline const js::Value &
JSFunction::getBoundFunctionThis() const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(isBoundFunction());

    return getSlot(JSSLOT_BOUND_FUNCTION_THIS);
}

inline const js::Value &
JSFunction::getBoundFunctionArgument(uintN which) const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(isBoundFunction());
    JS_ASSERT(which < getBoundFunctionArgumentCount());

    return getSlot(BOUND_FUNCTION_RESERVED_SLOTS + which);
}

inline size_t
JSFunction::getBoundFunctionArgumentCount() const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(isBoundFunction());

    return getSlot(JSSLOT_BOUND_FUNCTION_ARGS_COUNT).toPrivateUint32();
}

namespace js {


JSBool
CallOrConstructBoundFunction(JSContext *cx, uintN argc, Value *vp)
{
    JSFunction *fun = vp[0].toObject().toFunction();
    JS_ASSERT(fun->isBoundFunction());

    bool constructing = IsConstructing(vp);

    
    uintN argslen = fun->getBoundFunctionArgumentCount();

    if (argc + argslen > StackSpace::ARGS_LENGTH_MAX) {
        js_ReportAllocationOverflow(cx);
        return false;
    }

    
    JSObject *target = fun->getBoundFunctionTarget();

    
    const Value &boundThis = fun->getBoundFunctionThis();

    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, argc + argslen, &args))
        return false;

    
    for (uintN i = 0; i < argslen; i++)
        args[i] = fun->getBoundFunctionArgument(i);
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
    JSFunction *fun;
    if (!IsFunctionObject(vp[1], &fun)) {
        JS_SET_RVAL(cx, vp, BooleanValue(false));
        return true;
    }

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
        uintN nargs = target->toFunction()->nargs;
        if (nargs > argslen)
            length = nargs - argslen;
    }

    
    JSAtom *name = target->isFunction() ? target->toFunction()->atom : NULL;

    JSObject *funobj =
        js_NewFunction(cx, NULL, CallOrConstructBoundFunction, length,
                       JSFUN_CONSTRUCTOR, target, name);
    if (!funobj)
        return false;

    
    if (!funobj->setParent(cx, target))
        return false;

    
    Value thisArg = args.length() >= 1 ? args[0] : UndefinedValue();
    if (!funobj->toFunction()->initBoundFunction(cx, thisArg, boundArgs, argslen))
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

                
                PropertyName *name = ts.currentToken().name();
                if (bindings.hasBinding(cx, name)) {
                    JSAutoByteString bytes;
                    if (!js_AtomToPrintableString(cx, name, &bytes))
                        return false;
                    if (!ReportCompileErrorNumber(cx, &ts, NULL,
                                                  JSREPORT_WARNING | JSREPORT_STRICT,
                                                  JSMSG_DUPLICATE_FORMAL, bytes.ptr()))
                    {
                        return false;
                    }
                }

                uint16 dummy;
                if (!bindings.addArgument(cx, name, &dummy))
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
    bool ok = frontend::CompileFunctionBody(cx, fun, principals, &bindings, chars, length,
                                            filename, lineno, cx->findVersion());
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
    JSFunction *fun = funobj->toFunction();
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
               uintN flags, JSObject *parent, JSAtom *atom, js::gc::AllocKind kind)
{
    JS_ASSERT(kind == JSFunction::FinalizeKind || kind == JSFunction::ExtendedFinalizeKind);
    JS_ASSERT(sizeof(JSFunction) <= gc::Arena::thingSize(JSFunction::FinalizeKind));
    JS_ASSERT(sizeof(FunctionExtended) <= gc::Arena::thingSize(JSFunction::ExtendedFinalizeKind));

    JSFunction *fun;

    if (funobj) {
        JS_ASSERT(funobj->isFunction());
        JS_ASSERT(funobj->getParent() == parent);
    } else {
        funobj = NewObjectWithClassProto(cx, &FunctionClass, NULL, SkipScopeParent(parent), kind);
        if (!funobj)
            return NULL;
    }
    fun = static_cast<JSFunction *>(funobj);

    
    fun->nargs = uint16(nargs);
    fun->flags = flags & (JSFUN_FLAGS_MASK | JSFUN_KINDMASK);
    if ((flags & JSFUN_KINDMASK) >= JSFUN_INTERPRETED) {
        JS_ASSERT(!native);
        fun->script().init(NULL);
        fun->setEnvironment(parent);
    } else {
        fun->u.n.clasp = NULL;
        fun->u.n.native = native;
        JS_ASSERT(fun->u.n.native);
    }
    if (kind == JSFunction::ExtendedFinalizeKind) {
        fun->flags |= JSFUN_EXTENDED;
        fun->initializeExtended();
    }
    fun->atom = atom;

    if (native && !fun->setSingletonType(cx))
        return NULL;

    return fun;
}

JSFunction * JS_FASTCALL
js_CloneFunctionObject(JSContext *cx, JSFunction *fun, JSObject *parent,
                       JSObject *proto, gc::AllocKind kind)
{
    JS_ASSERT(parent);
    JS_ASSERT(proto);

    JSObject *cloneobj = NewObjectWithClassProto(cx, &FunctionClass, NULL, SkipScopeParent(parent), kind);
    if (!cloneobj)
        return NULL;
    JSFunction *clone = static_cast<JSFunction *>(cloneobj);

    clone->nargs = fun->nargs;
    clone->flags = fun->flags & ~JSFUN_EXTENDED;
    clone->u = fun->toFunction()->u;
    clone->atom = fun->atom;

    if (kind == JSFunction::ExtendedFinalizeKind) {
        clone->flags |= JSFUN_EXTENDED;
        clone->initializeExtended();
    }

    if (clone->isInterpreted())
        clone->setEnvironment(parent);

    if (cx->compartment == fun->compartment()) {
        






        if (fun->getProto() == proto && !fun->hasSingletonType())
            clone->setType(fun->type());
    } else {
        



        if (clone->isInterpreted()) {
            JSScript *script = clone->script();
            JS_ASSERT(script);
            JS_ASSERT(script->compartment() == fun->compartment());
            JS_ASSERT(script->compartment() != cx->compartment);

            clone->script().init(NULL);
            JSScript *cscript = js_CloneScript(cx, script);
            if (!cscript)
                return NULL;

            cscript->globalObject = clone->getGlobal();
            clone->setScript(cscript);
            if (!cscript->typeSetFunction(cx, clone))
                return NULL;

            js_CallNewScriptHook(cx, clone->script(), clone);
            Debugger::onNewScript(cx, clone->script(), NULL);
        }
    }
    return clone;
}






JSFunction * JS_FASTCALL
js_AllocFlatClosure(JSContext *cx, JSFunction *fun, JSObject *scopeChain)
{
    JS_ASSERT(fun->isFlatClosure());
    JS_ASSERT(JSScript::isValidOffset(fun->script()->upvarsOffset) ==
              fun->script()->bindings.hasUpvars());
    JS_ASSERT_IF(JSScript::isValidOffset(fun->script()->upvarsOffset),
                 fun->script()->upvars()->length == fun->script()->bindings.countUpvars());

    JSFunction *closure = CloneFunctionObject(cx, fun, scopeChain, JSFunction::ExtendedFinalizeKind);
    if (!closure)
        return closure;

    uint32 nslots = fun->script()->bindings.countUpvars();
    if (nslots == 0)
        return closure;

    HeapValue *data = (HeapValue *) cx->malloc_(nslots * sizeof(HeapValue));
    if (!data)
        return NULL;

    closure->setExtendedSlot(JSFunction::FLAT_CLOSURE_UPVARS_SLOT, PrivateValue(data));
    return closure;
}

JSFunction *
js_NewFlatClosure(JSContext *cx, JSFunction *fun, JSOp op, size_t oplen)
{
    








    VOUCH_DOES_NOT_REQUIRE_STACK();
    JSObject *scopeChain = &cx->fp()->scopeChain();

    JSFunction *closure = js_AllocFlatClosure(cx, fun, scopeChain);
    if (!closure || !fun->script()->bindings.hasUpvars())
        return closure;

    uintN level = fun->script()->staticLevel;
    JSUpvarArray *uva = fun->script()->upvars();

    for (uint32 i = 0, n = uva->length; i < n; i++)
        closure->initFlatClosureUpvar(i, GetUpvar(cx, level, uva->vector[i]));

    return closure;
}

JSFunction *
js_DefineFunction(JSContext *cx, JSObject *obj, jsid id, Native native,
                  uintN nargs, uintN attrs, AllocKind kind)
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

    fun = js_NewFunction(cx, NULL, native, nargs,
                         attrs & (JSFUN_FLAGS_MASK),
                         obj,
                         JSID_IS_ATOM(id) ? JSID_TO_ATOM(id) : NULL,
                         kind);
    if (!fun)
        return NULL;

    if (!obj->defineGeneric(cx, id, ObjectValue(*fun), gop, sop, attrs & ~JSFUN_FLAGS_MASK))
        return NULL;

    return fun;
}

JS_STATIC_ASSERT((JSV2F_CONSTRUCT & JSV2F_SEARCH_STACK) == 0);

JSFunction *
js_ValueToFunction(JSContext *cx, const Value *vp, uintN flags)
{
    JSFunction *fun;
    if (!IsFunctionObject(*vp, &fun)) {
        js_ReportIsNotFunction(cx, vp, flags);
        return NULL;
    }
    return fun;
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
