







































#include "GlobalObject.h"

#include "jscntxt.h"
#include "jsexn.h"
#include "jsmath.h"
#include "json.h"

#include "jsobjinlines.h"
#include "jsregexpinlines.h"

using namespace js;

JSObject *
js_InitObjectClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto = js_InitClass(cx, obj, NULL, &ObjectClass, js_Object, 1,
                                   object_props, object_methods, NULL, object_static_methods);
    if (!proto)
        return NULL;

    
    proto->getNewType(cx, NULL,  true);

    
    jsid id = ATOM_TO_JSID(cx->runtime->atomState.evalAtom);
    JSObject *evalobj = js_DefineFunction(cx, obj, id, eval, 1, JSFUN_STUB_GSOPS);
    if (!evalobj)
        return NULL;
    if (obj->isGlobal())
        obj->asGlobal()->setOriginalEval(evalobj);

    return proto;
}

JSObject *
js_InitFunctionAndObjectClasses(JSContext *cx, JSObject *obj)
{
    JS_THREADSAFE_ASSERT(cx->compartment != cx->runtime->atomsCompartment);

    
    if (!cx->globalObject)
        JS_SetGlobalObject(cx, obj);

    
    JSAtom **classAtoms = cx->runtime->atomState.classAtoms;
    AutoResolving resolving1(cx, obj, ATOM_TO_JSID(classAtoms[JSProto_Function]));
    AutoResolving resolving2(cx, obj, ATOM_TO_JSID(classAtoms[JSProto_Object]));

    
    JSObject *fun_proto;
    if (!js_GetClassPrototype(cx, obj, JSProto_Function, &fun_proto))
        return NULL;
    if (!fun_proto) {
        fun_proto = js_InitFunctionClass(cx, obj);
        if (!fun_proto)
            return NULL;
    } else {
        JSObject *ctor = JS_GetConstructor(cx, fun_proto);
        if (!ctor)
            return NULL;
        if (!obj->defineProperty(cx, ATOM_TO_JSID(CLASS_ATOM(cx, Function)),
                                 ObjectValue(*ctor), 0, 0, 0)) {
            return NULL;
        }
    }

    
    JSObject *obj_proto;
    if (!js_GetClassPrototype(cx, obj, JSProto_Object, &obj_proto))
        return NULL;
    if (!obj_proto)
        obj_proto = js_InitObjectClass(cx, obj);
    if (!obj_proto)
        return NULL;

    




    if (fun_proto->shouldSplicePrototype(cx) && !fun_proto->splicePrototype(cx, obj_proto))
        return NULL;
    if (obj->shouldSplicePrototype(cx) && !obj->splicePrototype(cx, obj_proto))
        return NULL;

    return fun_proto;
}

namespace js {

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

    
    JSObject *res = regexp_statics_construct(cx, globalObj);
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
                        PropertyStub, StrictPropertyStub, JSPROP_PERMANENT | JSPROP_READONLY))
    {
        return false;
    }

    if (!js_InitFunctionAndObjectClasses(cx, this))
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

    
    RegExpStatics::extractFrom(this)->clear();

    
    setSlot(RUNTIME_CODEGEN_ENABLED, UndefinedValue());

    



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
                                ObjectValue(*proto), PropertyStub, StrictPropertyStub,
                                JSPROP_PERMANENT | JSPROP_READONLY) &&
           proto->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.constructorAtom),
                                 ObjectValue(*ctor), PropertyStub, StrictPropertyStub, 0);
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
    PropertyStub, PropertyStub, PropertyStub, StrictPropertyStub,
    EnumerateStub, ResolveStub, ConvertStub, GlobalDebuggees_finalize
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
