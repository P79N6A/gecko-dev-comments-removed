






#include "jsapi-tests/tests.h"

static bool
NativeGetterSetter(JSContext *cx, unsigned argc, jsval *vp)
{
    return true;
}

BEGIN_TEST(testDefineGetterSetterNonEnumerable)
{
    static const char PROPERTY_NAME[] = "foo";

    JS::RootedValue vobj(cx);
    JS::RootedObject obj(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
    CHECK(obj);
    vobj = OBJECT_TO_JSVAL(obj);

    JSFunction *funGet = JS_NewFunction(cx, NativeGetterSetter, 0, 0, JS::NullPtr(), "get");
    CHECK(funGet);
    JS::RootedObject funGetObj(cx, JS_GetFunctionObject(funGet));
    JS::RootedValue vget(cx, OBJECT_TO_JSVAL(funGetObj));

    JSFunction *funSet = JS_NewFunction(cx, NativeGetterSetter, 1, 0, JS::NullPtr(), "set");
    CHECK(funSet);
    JS::RootedObject funSetObj(cx, JS_GetFunctionObject(funSet));
    JS::RootedValue vset(cx, OBJECT_TO_JSVAL(funSetObj));

    JS::RootedObject vObject(cx, vobj.toObjectOrNull());
    CHECK(JS_DefineProperty(cx, vObject, PROPERTY_NAME,
                            JS::UndefinedHandleValue,
                            JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED | JSPROP_ENUMERATE,
                            JS_DATA_TO_FUNC_PTR(JSNative, (JSObject*) funGetObj),
                            JS_DATA_TO_FUNC_PTR(JSNative, (JSObject*) funSetObj)));

    CHECK(JS_DefineProperty(cx, vObject, PROPERTY_NAME,
                            JS::UndefinedHandleValue,
                            JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED | JSPROP_PERMANENT,
                            JS_DATA_TO_FUNC_PTR(JSNative, (JSObject*) funGetObj),
                            JS_DATA_TO_FUNC_PTR(JSNative, (JSObject*) funSetObj)));

    JS::Rooted<JSPropertyDescriptor> desc(cx);
    CHECK(JS_GetOwnPropertyDescriptor(cx, vObject, PROPERTY_NAME, &desc));
    CHECK(desc.object());
    CHECK(desc.hasGetterObject());
    CHECK(desc.hasSetterObject());
    CHECK(desc.isPermanent());
    CHECK(!desc.isEnumerable());

    return true;
}
END_TEST(testDefineGetterSetterNonEnumerable)
