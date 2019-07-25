







































#include "GlobalObject.h"

#include "jscntxt.h"
#include "jsexn.h"
#include "json.h"

#include "jsobjinlines.h"
#include "jsregexpinlines.h"

using namespace js;

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

    
    fun_proto->setProto(obj_proto);
    if (!obj->getProto())
        obj->setProto(obj_proto);

    return fun_proto;
}

namespace js {

GlobalObject *
GlobalObject::create(JSContext *cx, Class *clasp)
{
    JS_ASSERT(clasp->flags & JSCLASS_IS_GLOBAL);

    JSObject *obj = NewNonFunction<WithProto::Given>(cx, clasp, NULL, NULL);
    if (!obj)
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

        



        v.setBoolean((!callbacks || !callbacks->contentSecurityPolicyAllows) ||
                     callbacks->contentSecurityPolicyAllows(cx));
    }
    return !v.isFalse();
}

void
GlobalDebuggees_finalize(JSContext *cx, JSObject *obj)
{
    cx->delete_((GlobalObject::DebugVector *) obj->getPrivate());
}

static Class
GlobalDebuggees_class = {
    "GlobalDebuggee", JSCLASS_HAS_PRIVATE,
    PropertyStub, PropertyStub, PropertyStub, StrictPropertyStub,
    EnumerateStub, ResolveStub, ConvertStub, GlobalDebuggees_finalize
};

GlobalObject::DebugVector *
GlobalObject::getDebuggers()
{
    Value debuggers = getReservedSlot(DEBUGGERS);
    if (debuggers.isUndefined())
        return NULL;
    JS_ASSERT(debuggers.toObject().clasp == &GlobalDebuggees_class);
    return (DebugVector *) debuggers.toObject().getPrivate();
}

GlobalObject::DebugVector *
GlobalObject::getOrCreateDebuggers(JSContext *cx)
{
    assertSameCompartment(cx, this);
    DebugVector *vec = getDebuggers();
    if (vec)
        return vec;

    JSObject *obj = NewNonFunction<WithProto::Given>(cx, &GlobalDebuggees_class, NULL, NULL);
    if (!obj)
        return NULL;
    vec = cx->new_<DebugVector>();
    if (!vec)
        return NULL;
    obj->setPrivate(vec);
    if (!js_SetReservedSlot(cx, this, DEBUGGERS, ObjectValue(*obj)))
        return NULL;
    return vec;
}

bool
GlobalObject::addDebug(JSContext *cx, Debug *dbg)
{
    DebugVector *vec = getOrCreateDebuggers(cx);
    if (!vec)
        return false;
#ifdef DEBUG
    for (Debug **p = vec->begin(); p != vec->end(); p++)
        JS_ASSERT(*p != dbg);
#endif
    if (vec->empty() && !compartment()->addDebuggee(cx, this))
        return false;
    if (!vec->append(dbg)) {
        compartment()->removeDebuggee(cx, this);
        return false;
    }
    return true;
}

} 
