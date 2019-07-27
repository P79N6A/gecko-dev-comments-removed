





#include "builtin/WeakSetObject.h"

#include "jsapi.h"
#include "jscntxt.h"
#include "jsiter.h"

#include "builtin/SelfHostingDefines.h"
#include "vm/GlobalObject.h"

#include "jsobjinlines.h"

using namespace js;
using namespace JS;

const Class WeakSetObject::class_ = {
    "WeakSet",
    JSCLASS_IMPLEMENTS_BARRIERS | JSCLASS_HAS_CACHED_PROTO(JSProto_WeakSet) |
    JSCLASS_HAS_RESERVED_SLOTS(WeakSetObject::RESERVED_SLOTS),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

const JSPropertySpec WeakSetObject::properties[] = {
    JS_PS_END
};

const JSFunctionSpec WeakSetObject::methods[] = {
    JS_SELF_HOSTED_FN("add",    "WeakSet_add",    1, 0),
    JS_SELF_HOSTED_FN("clear",  "WeakSet_clear",  0, 0),
    JS_SELF_HOSTED_FN("delete", "WeakSet_delete", 1, 0),
    JS_SELF_HOSTED_FN("has",    "WeakSet_has",    1, 0),
    JS_FS_END
};

JSObject *
WeakSetObject::initClass(JSContext *cx, JSObject *obj)
{
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());
    
    Rooted<JSObject*> proto(cx, global->createBlankPrototype(cx, &class_));
    if (!proto)
        return nullptr;
    proto->setReservedSlot(WEAKSET_MAP_SLOT, UndefinedValue());

    Rooted<JSFunction*> ctor(cx, global->createConstructor(cx, construct, ClassName(JSProto_WeakSet, cx), 1));
    if (!ctor ||
        !LinkConstructorAndPrototype(cx, ctor, proto) ||
        !DefinePropertiesAndFunctions(cx, proto, properties, methods) ||
        !GlobalObject::initBuiltinConstructor(cx, global, JSProto_WeakSet, ctor, proto))
    {
        return nullptr;
    }
    return proto;
}

WeakSetObject*
WeakSetObject::create(JSContext *cx)
{
    RootedObject obj(cx, NewBuiltinClassInstance(cx, &class_));
    if (!obj)
        return nullptr;

    RootedObject map(cx, NewWeakMapObject(cx));
    if (!map)
        return nullptr;

    obj->setReservedSlot(WEAKSET_MAP_SLOT, ObjectValue(*map));
    return &obj->as<WeakSetObject>();
}

bool
WeakSetObject::construct(JSContext *cx, unsigned argc, Value *vp)
{
    Rooted<WeakSetObject*> obj(cx, WeakSetObject::create(cx));
    if (!obj)
        return false;

    
    CallArgs args = CallArgsFromVp(argc, vp);

    if (!args.isConstructing()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_FUNCTION, "WeakSet");
        return false;
    }

    if (args.hasDefined(0)) {
        RootedObject map(cx, &obj->getReservedSlot(WEAKSET_MAP_SLOT).toObject());

        ForOfIterator iter(cx);
        if (!iter.init(args[0]))
            return false;

        RootedValue keyVal(cx);
        RootedObject keyObject(cx);
        RootedValue placeholder(cx, BooleanValue(true));
        while (true) {
            bool done;
            if (!iter.next(&keyVal, &done))
                return false;
            if (done)
                break;

            if (keyVal.isPrimitive()) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_NONNULL_OBJECT);
                return false;
            }

            keyObject = &keyVal.toObject();
            if (!SetWeakMapEntry(cx, map, keyObject, placeholder))
                return false;
        }
    }

    args.rval().setObject(*obj);
    return true;
}


JSObject *
js_InitWeakSetClass(JSContext *cx, HandleObject obj)
{
    return WeakSetObject::initClass(cx, obj);
}
