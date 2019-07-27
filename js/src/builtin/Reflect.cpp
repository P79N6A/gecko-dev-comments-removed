





#include "builtin/Reflect.h"

#include "jscntxt.h"

#include "vm/Stack.h"

using namespace js;









static bool
InitArgsFromArrayLike(JSContext* cx, HandleValue v, InvokeArgs* args, bool construct)
{
    
    RootedObject obj(cx, NonNullObject(cx, v));
    if (!obj)
        return false;

    
    uint32_t len;
    if (!GetLengthProperty(cx, obj, &len))
        return false;

    
    if (len > ARGS_LENGTH_MAX) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_TOO_MANY_FUN_APPLY_ARGS);
        return false;
    }
    if (!args->init(len, construct))
        return false;

    
    for (uint32_t index = 0; index < len; index++) {
        if (!GetElement(cx, obj, obj, index, (*args)[index]))
            return false;
    }

    
    return true;
}


static bool
Reflect_apply(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    if (!IsCallable(args.get(0))) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_NOT_FUNCTION,
                             "Reflect.apply argument");
        return false;
    }

    
    FastInvokeGuard fig(cx, args.get(0));
    InvokeArgs& invokeArgs = fig.args();
    if (!InitArgsFromArrayLike(cx, args.get(2), &invokeArgs, false))
        return false;
    invokeArgs.setCallee(args.get(0));
    invokeArgs.setThis(args.get(1));

    
    if (!fig.invoke(cx))
        return false;
    args.rval().set(invokeArgs.rval());
    return true;
}


static bool
Reflect_defineProperty(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject obj(cx, NonNullObject(cx, args.get(0)));
    if (!obj)
        return false;

    
    RootedValue propertyKey(cx, args.get(1));
    RootedId key(cx);
    if (!ToPropertyKey(cx, propertyKey, &key))
        return false;

    
    Rooted<JSPropertyDescriptor> desc(cx);
    if (!ToPropertyDescriptor(cx, args.get(2), true, &desc))
        return false;

    
    ObjectOpResult result;
    if (!DefineProperty(cx, obj, key, desc, result))
        return false;
    args.rval().setBoolean(bool(result));
    return true;
}


static bool
Reflect_deleteProperty(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject target(cx, NonNullObject(cx, args.get(0)));
    if (!target)
        return false;

    
    RootedValue propertyKey(cx, args.get(1));
    RootedId key(cx);
    if (!ToPropertyKey(cx, propertyKey, &key))
        return false;

    
    ObjectOpResult result;
    if (!DeleteProperty(cx, target, key, result))
        return false;
    args.rval().setBoolean(bool(result));
    return true;
}

#if 0







static bool
Reflect_enumerate(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject obj(cx, NonNullObject(cx, args.get(0)));
    if (!obj)
        return false;

    
    RootedObject iterator(cx);
    if (!Enumerate(cx, obj, &iterator))
        return false;
    args.rval().setObject(*iterator);
    return true;
}
#endif






static bool
Reflect_get(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject obj(cx, NonNullObject(cx, args.get(0)));
    if (!obj)
        return false;

    
    RootedValue propertyKey(cx, args.get(1));
    RootedId key(cx);
    if (!ToPropertyKey(cx, propertyKey, &key))
        return false;

    
    RootedValue receiver(cx, argc > 2 ? args[2] : args.get(0));

    
    
    RootedObject receiverObj(cx, NonNullObject(cx, receiver));
    if (!receiverObj)
        return false;

    
    return GetProperty(cx, obj, receiverObj, key, args.rval());
}


static bool
Reflect_getOwnPropertyDescriptor(JSContext* cx, unsigned argc, Value* vp)
{
    
    CallArgs args = CallArgsFromVp(argc, vp);
    if (!NonNullObject(cx, args.get(0)))
        return false;

    
    
    return js::obj_getOwnPropertyDescriptor(cx, argc, vp);
}


bool
js::Reflect_getPrototypeOf(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject target(cx, NonNullObject(cx, args.get(0)));
    if (!target)
        return false;

    
    RootedObject proto(cx);
    if (!GetPrototype(cx, target, &proto))
        return false;
    args.rval().setObjectOrNull(proto);
    return true;
}


bool
js::Reflect_isExtensible(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject target(cx, NonNullObject(cx, args.get(0)));
    if (!target)
        return false;

    
    bool extensible;
    if (!IsExtensible(cx, target, &extensible))
        return false;
    args.rval().setBoolean(extensible);
    return true;
}


static bool
Reflect_ownKeys(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    if (!NonNullObject(cx, args.get(0)))
        return false;

    
    return GetOwnPropertyKeys(cx, args, JSITER_OWNONLY | JSITER_HIDDEN | JSITER_SYMBOLS);
}


static bool
Reflect_preventExtensions(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject target(cx, NonNullObject(cx, args.get(0)));
    if (!target)
        return false;

    
    ObjectOpResult result;
    if (!PreventExtensions(cx, target, result))
        return false;
    args.rval().setBoolean(bool(result));
    return true;
}


static bool
Reflect_set(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject target(cx, NonNullObject(cx, args.get(0)));
    if (!target)
        return false;

    
    RootedValue propertyKey(cx, args.get(1));
    RootedId key(cx);
    if (!ToPropertyKey(cx, propertyKey, &key))
        return false;

    
    RootedValue receiver(cx, argc > 3 ? args[3] : args.get(0));

    
    ObjectOpResult result;
    RootedValue value(cx, args.get(2));
    if (!SetProperty(cx, target, key, value, receiver, result))
        return false;
    args.rval().setBoolean(bool(result));
    return true;
}







static bool
Reflect_setPrototypeOf(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject obj(cx, NonNullObject(cx, args.get(0)));
    if (!obj)
        return false;

    
    if (!args.get(1).isObjectOrNull()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_NOT_EXPECTED_TYPE,
                             "Reflect.setPrototypeOf", "an object or null",
                             InformalValueTypeName(args.get(1)));
        return false;
    }
    RootedObject proto(cx, args.get(1).toObjectOrNull());

    
    ObjectOpResult result;
    if (!SetPrototype(cx, obj, proto, result))
        return false;
    args.rval().setBoolean(bool(result));
    return true;
}

static const JSFunctionSpec methods[] = {
    JS_FN("apply", Reflect_apply, 3, 0),
    
    JS_FN("defineProperty", Reflect_defineProperty, 3, 0),
    JS_FN("deleteProperty", Reflect_deleteProperty, 2, 0),
    
    JS_FN("get", Reflect_get, 2, 0),
    JS_FN("getOwnPropertyDescriptor", Reflect_getOwnPropertyDescriptor, 2, 0),
    JS_FN("getPrototypeOf", Reflect_getPrototypeOf, 1, 0),
    JS_SELF_HOSTED_FN("has", "Reflect_has", 2, 0),
    JS_FN("isExtensible", Reflect_isExtensible, 1, 0),
    JS_FN("ownKeys", Reflect_ownKeys, 1, 0),
    JS_FN("preventExtensions", Reflect_preventExtensions, 1, 0),
    JS_FN("set", Reflect_set, 3, 0),
    JS_FN("setPrototypeOf", Reflect_setPrototypeOf, 2, 0),
    JS_FS_END
};




JSObject*
js::InitReflect(JSContext* cx, HandleObject obj)
{
    RootedObject proto(cx, obj->as<GlobalObject>().getOrCreateObjectPrototype(cx));
    if (!proto)
        return nullptr;

    RootedObject reflect(cx, NewObjectWithGivenProto<PlainObject>(cx, proto, SingletonObject));
    if (!reflect)
        return nullptr;
    if (!JS_DefineFunctions(cx, reflect, methods))
        return nullptr;

    RootedValue value(cx, ObjectValue(*reflect));
    if (!DefineProperty(cx, obj, cx->names().Reflect, value, nullptr, nullptr, JSPROP_RESOLVING))
        return nullptr;

    obj->as<GlobalObject>().setConstructor(JSProto_Reflect, value);

    return reflect;
}
