







































#include "GlobalObject.h"

#include "jscntxt.h"
#include "jsexn.h"
#include "jsmath.h"
#include "json.h"

#include "builtin/RegExp.h"
#include "frontend/CodeGenerator.h"

#include "jsobjinlines.h"
#include "vm/RegExpObject-inl.h"

using namespace js;

JSObject *
js_InitObjectClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    GlobalObject *global = obj->asGlobal();
    if (!global->functionObjectClassesInitialized()) {
        if (!global->initFunctionAndObjectClasses(cx))
            return NULL;
    }

    return global->getObjectPrototype();
}

JSObject *
js_InitFunctionClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    GlobalObject *global = obj->asGlobal();
    return global->functionObjectClassesInitialized()
           ? global->getFunctionPrototype()
           : global->initFunctionAndObjectClasses(cx);
}

static JSBool
ThrowTypeError(JSContext *cx, uintN argc, Value *vp)
{
    JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, NULL,
                                 JSMSG_THROW_TYPE_ERROR);
    return false;
}

namespace js {

JSObject *
GlobalObject::initFunctionAndObjectClasses(JSContext *cx)
{
    JS_THREADSAFE_ASSERT(cx->compartment != cx->runtime->atomsCompartment);
    JS_ASSERT(isNative());

    




    

    
    if (!cx->globalObject)
        JS_SetGlobalObject(cx, this);

    



    JSObject *objectProto = NewNonFunction<WithProto::Given>(cx, &ObjectClass, NULL, this);
    if (!objectProto || !objectProto->setSingletonType(cx))
        return NULL;
    types::TypeObject *objectType = objectProto->getNewType(cx, NULL,  true);
    if (!objectType || !objectType->getEmptyShape(cx, &ObjectClass, gc::FINALIZE_OBJECT0))
        return NULL;

    
    JSFunction *functionProto;
    {
        JSObject *proto = NewObject<WithProto::Given>(cx, &FunctionClass, objectProto, this);
        if (!proto || !proto->setSingletonType(cx))
            return NULL;
        types::TypeObject *functionType = proto->getNewType(cx, NULL,  true);
        if (!functionType || !functionType->getEmptyShape(cx, &FunctionClass, gc::FINALIZE_OBJECT0))
            return NULL;

        



        functionProto = js_NewFunction(cx, proto, NULL, 0, JSFUN_INTERPRETED, this, NULL);
        if (!functionProto)
            return NULL;
        JS_ASSERT(proto == functionProto);
        functionProto->flags |= JSFUN_PROTOTYPE;

        JSScript *script =
            JSScript::NewScript(cx, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, JSVERSION_DEFAULT);
        if (!script)
            return NULL;
        script->noScriptRval = true;
        script->code[0] = JSOP_STOP;
        script->code[1] = SRC_NULL;
        functionProto->setScript(script);
        functionProto->getType(cx)->interpretedFunction = functionProto;
        script->hasFunction = true;
    }

    
    jsid objectId = ATOM_TO_JSID(CLASS_ATOM(cx, Object));
    JSFunction *objectCtor;
    {
        JSObject *ctor = NewObject<WithProto::Given>(cx, &FunctionClass, functionProto, this);
        if (!ctor)
            return NULL;
        objectCtor = js_NewFunction(cx, ctor, js_Object, 1, JSFUN_CONSTRUCTOR, this,
                                    JSID_TO_ATOM(objectId));
        if (!objectCtor)
            return NULL;
        JS_ASSERT(ctor == objectCtor);

        objectCtor->setConstructorClass(&ObjectClass);
    }

    



    setObjectClassDetails(objectCtor, objectProto);

    
    jsid functionId = ATOM_TO_JSID(CLASS_ATOM(cx, Function));
    JSFunction *functionCtor;
    {
        JSObject *ctor =
            NewObject<WithProto::Given>(cx, &FunctionClass, functionProto, this);
        if (!ctor)
            return NULL;
        functionCtor = js_NewFunction(cx, ctor, Function, 1, JSFUN_CONSTRUCTOR, this,
                                      JSID_TO_ATOM(functionId));
        if (!functionCtor)
            return NULL;
        JS_ASSERT(ctor == functionCtor);

        functionCtor->setConstructorClass(&FunctionClass);
    }

    



    setFunctionClassDetails(functionCtor, functionProto);

    



    if (!LinkConstructorAndPrototype(cx, objectCtor, objectProto) ||
        !DefinePropertiesAndBrand(cx, objectProto, object_props, object_methods) ||
        !DefinePropertiesAndBrand(cx, objectCtor, NULL, object_static_methods) ||
        !LinkConstructorAndPrototype(cx, functionCtor, functionProto) ||
        !DefinePropertiesAndBrand(cx, functionProto, NULL, function_methods) ||
        !DefinePropertiesAndBrand(cx, functionCtor, NULL, NULL))
    {
        return NULL;
    }

    
    if (!addDataProperty(cx, objectId, JSProto_Object + JSProto_LIMIT * 2, 0))
        return NULL;
    if (!addDataProperty(cx, functionId, JSProto_Function + JSProto_LIMIT * 2, 0))
        return NULL;

    

    
    jsid id = ATOM_TO_JSID(cx->runtime->atomState.evalAtom);
    JSObject *evalobj = js_DefineFunction(cx, this, id, eval, 1, JSFUN_STUB_GSOPS);
    if (!evalobj)
        return NULL;
    setOriginalEval(evalobj);

    
    JSFunction *throwTypeError = js_NewFunction(cx, NULL, ThrowTypeError, 0, 0, this, NULL);
    if (!throwTypeError)
        return NULL;
    AutoIdVector ids(cx);
    if (!throwTypeError->preventExtensions(cx, &ids))
        return NULL;
    setThrowTypeError(throwTypeError);

    







    if (shouldSplicePrototype(cx) && !splicePrototype(cx, objectProto))
        return NULL;

    



    js_CallNewScriptHook(cx, functionProto->script(), functionProto);
    return functionProto;
}

GlobalObject *
GlobalObject::create(JSContext *cx, Class *clasp)
{
    JS_ASSERT(clasp->flags & JSCLASS_IS_GLOBAL);

    JSObject *obj = NewNonFunction<WithProto::Given>(cx, clasp, NULL, NULL);
    if (!obj || !obj->setSingletonType(cx))
        return NULL;

    GlobalObject *globalObj = obj->asGlobal();
    globalObj->makeVarObj();
    globalObj->syncSpecialEquality();

    
    JSObject *res = RegExpStatics::create(cx, globalObj);
    if (!res)
        return NULL;
    globalObj->setSlot(REGEXP_STATICS, ObjectValue(*res));
    globalObj->setFlags(0);

    return globalObj;
}

bool
GlobalObject::initStandardClasses(JSContext *cx)
{
    
    JS_ASSERT(numSlots() >= JSSLOT_FREE(getClass()));

    JSAtomState &state = cx->runtime->atomState;

    
    if (!defineProperty(cx, ATOM_TO_JSID(state.typeAtoms[JSTYPE_VOID]), UndefinedValue(),
                        JS_PropertyStub, JS_StrictPropertyStub, JSPROP_PERMANENT | JSPROP_READONLY))
    {
        return false;
    }

    if (!initFunctionAndObjectClasses(cx))
        return false;

    
    return js_InitArrayClass(cx, this) &&
           js_InitBooleanClass(cx, this) &&
           js_InitExceptionClasses(cx, this) &&
           js_InitMathClass(cx, this) &&
           js_InitNumberClass(cx, this) &&
           js_InitJSONClass(cx, this) &&
           js_InitRegExpClass(cx, this) &&
           js_InitStringClass(cx, this) &&
           js_InitTypedArrayClasses(cx, this) &&
#if JS_HAS_XML_SUPPORT
           js_InitXMLClasses(cx, this) &&
#endif
#if JS_HAS_GENERATORS
           js_InitIteratorClasses(cx, this) &&
#endif
           js_InitDateClass(cx, this) &&
           js_InitProxyClass(cx, this);
}

void
GlobalObject::clear(JSContext *cx)
{
    
    unbrand(cx);

    for (int key = JSProto_Null; key < JSProto_LIMIT * 3; key++)
        setSlot(key, UndefinedValue());

    
    getRegExpStatics()->clear();

    
    setSlot(RUNTIME_CODEGEN_ENABLED, UndefinedValue());

    




    setSlot(EVAL, UndefinedValue());
    setSlot(THROWTYPEERROR, UndefinedValue());

    



    int32 flags = getSlot(FLAGS).toInt32();
    flags |= FLAGS_CLEARED;
    setSlot(FLAGS, Int32Value(flags));
}

bool
GlobalObject::isRuntimeCodeGenEnabled(JSContext *cx)
{
    Value &v = getSlotRef(RUNTIME_CODEGEN_ENABLED);
    if (v.isUndefined()) {
        JSSecurityCallbacks *callbacks = JS_GetSecurityCallbacks(cx);

        



        v = BooleanValue((!callbacks || !callbacks->contentSecurityPolicyAllows) ||
                         callbacks->contentSecurityPolicyAllows(cx));
    }
    return !v.isFalse();
}

JSFunction *
GlobalObject::createConstructor(JSContext *cx, Native ctor, Class *clasp, JSAtom *name,
                                uintN length)
{
    JSFunction *fun = js_NewFunction(cx, NULL, ctor, length, JSFUN_CONSTRUCTOR, this, name);
    if (!fun)
        return NULL;

    



    fun->setConstructorClass(clasp);
    return fun;
}

static JSObject *
CreateBlankProto(JSContext *cx, Class *clasp, JSObject &proto, GlobalObject &global)
{
    JS_ASSERT(clasp != &ObjectClass);
    JS_ASSERT(clasp != &FunctionClass);

    JSObject *blankProto = NewNonFunction<WithProto::Given>(cx, clasp, &proto, &global);
    if (!blankProto || !blankProto->setSingletonType(cx))
        return NULL;

    



    types::TypeObject *type = blankProto->getNewType(cx);
    if (!type || !type->getEmptyShape(cx, clasp, gc::FINALIZE_OBJECT0))
        return NULL;

    return blankProto;
}

JSObject *
GlobalObject::createBlankPrototype(JSContext *cx, Class *clasp)
{
    JSObject *objectProto;
    if (!js_GetClassPrototype(cx, this, JSProto_Object, &objectProto))
        return NULL;

    return CreateBlankProto(cx, clasp, *objectProto, *this);
}

JSObject *
GlobalObject::createBlankPrototypeInheriting(JSContext *cx, Class *clasp, JSObject &proto)
{
    return CreateBlankProto(cx, clasp, proto, *this);
}

bool
LinkConstructorAndPrototype(JSContext *cx, JSObject *ctor, JSObject *proto)
{
    return ctor->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom),
                                ObjectValue(*proto), JS_PropertyStub, JS_StrictPropertyStub,
                                JSPROP_PERMANENT | JSPROP_READONLY) &&
           proto->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.constructorAtom),
                                 ObjectValue(*ctor), JS_PropertyStub, JS_StrictPropertyStub, 0);
}

bool
DefinePropertiesAndBrand(JSContext *cx, JSObject *obj, JSPropertySpec *ps, JSFunctionSpec *fs)
{
    if ((ps && !JS_DefineProperties(cx, obj, ps)) || (fs && !JS_DefineFunctions(cx, obj, fs)))
        return false;
    if (!cx->typeInferenceEnabled())
        obj->brand(cx);
    return true;
}

void
GlobalDebuggees_finalize(JSContext *cx, JSObject *obj)
{
    cx->delete_((GlobalObject::DebuggerVector *) obj->getPrivate());
}

static Class
GlobalDebuggees_class = {
    "GlobalDebuggee", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, GlobalDebuggees_finalize
};

GlobalObject::DebuggerVector *
GlobalObject::getDebuggers()
{
    Value debuggers = getReservedSlot(DEBUGGERS);
    if (debuggers.isUndefined())
        return NULL;
    JS_ASSERT(debuggers.toObject().getClass() == &GlobalDebuggees_class);
    return (DebuggerVector *) debuggers.toObject().getPrivate();
}

GlobalObject::DebuggerVector *
GlobalObject::getOrCreateDebuggers(JSContext *cx)
{
    assertSameCompartment(cx, this);
    DebuggerVector *debuggers = getDebuggers();
    if (debuggers)
        return debuggers;

    JSObject *obj = NewNonFunction<WithProto::Given>(cx, &GlobalDebuggees_class, NULL, this);
    if (!obj)
        return NULL;
    debuggers = cx->new_<DebuggerVector>();
    if (!debuggers)
        return NULL;
    obj->setPrivate(debuggers);
    setReservedSlot(DEBUGGERS, ObjectValue(*obj));
    return debuggers;
}

bool
GlobalObject::addDebugger(JSContext *cx, Debugger *dbg)
{
    DebuggerVector *debuggers = getOrCreateDebuggers(cx);
    if (!debuggers)
        return false;
#ifdef DEBUG
    for (Debugger **p = debuggers->begin(); p != debuggers->end(); p++)
        JS_ASSERT(*p != dbg);
#endif
    if (debuggers->empty() && !compartment()->addDebuggee(cx, this))
        return false;
    if (!debuggers->append(dbg)) {
        compartment()->removeDebuggee(cx, this);
        return false;
    }
    return true;
}

} 
