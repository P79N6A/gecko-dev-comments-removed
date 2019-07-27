





#include "builtin/Reflect.h"

using namespace js;

JSObject*
js::InitReflect(JSContext* cx, HandleObject obj)
{
    static const JSFunctionSpec static_methods[] = {
        JS_FS_END
    };

    RootedObject proto(cx, obj->as<GlobalObject>().getOrCreateObjectPrototype(cx));
    if (!proto)
        return nullptr;

    RootedObject reflect(cx, NewObjectWithGivenProto<PlainObject>(cx, proto, SingletonObject));
    if (!reflect)
        return nullptr;
    if (!JS_DefineFunctions(cx, reflect, static_methods))
        return nullptr;

    RootedValue value(cx, ObjectValue(*reflect));
    if (!DefineProperty(cx, obj, cx->names().Reflect, value, nullptr, nullptr, JSPROP_RESOLVING))
        return nullptr;

    obj->as<GlobalObject>().setConstructor(JSProto_Reflect, value);

    return reflect;
}
