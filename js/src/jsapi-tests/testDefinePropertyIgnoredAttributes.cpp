






#include "jsapi-tests/tests.h"

static bool
Getter(JSContext* cx, unsigned argc, JS::Value* vp)
{
    JS::CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setBoolean(true);
    return true;
}

enum PropertyDescriptorKind {
    DataDescriptor, AccessorDescriptor
};

static bool
CheckDescriptor(JS::Handle<JSPropertyDescriptor> desc, PropertyDescriptorKind kind,
                bool enumerable, bool writable, bool configurable)
{
    if (!desc.object())
        return false;
    if (!(kind == DataDescriptor ? desc.isDataDescriptor() : desc.isAccessorDescriptor()))
        return false;
    if (desc.enumerable() != enumerable)
        return false;
    if (kind == DataDescriptor && desc.writable() != writable)
        return false;
    if (desc.configurable() != configurable)
        return false;
    return true;
}

BEGIN_TEST(testDefinePropertyIgnoredAttributes)
{
    JS::RootedObject obj(cx, JS_NewPlainObject(cx));
    JS::Rooted<JSPropertyDescriptor> desc(cx);
    JS::RootedValue defineValue(cx);

    
    
    
    CHECK(JS_DefineProperty(cx, obj, "foo", defineValue,
                            JSPROP_IGNORE_ENUMERATE | JSPROP_IGNORE_PERMANENT | JSPROP_SHARED,
                            Getter));

    CHECK(JS_GetOwnPropertyDescriptor(cx, obj, "foo", &desc));

    
    CHECK(CheckDescriptor(desc, AccessorDescriptor, false, true, false));

    
    CHECK(JS_DefineProperty(cx, obj, "bar", defineValue,
                            JSPROP_IGNORE_ENUMERATE | JSPROP_SHARED,
                            Getter));
    CHECK(JS_GetOwnPropertyDescriptor(cx, obj, "bar", &desc));
    CHECK(CheckDescriptor(desc, AccessorDescriptor, false, true, true));

    
    
    CHECK(JS_DefineProperty(cx, obj, "bar", defineValue,
                            JSPROP_IGNORE_PERMANENT | JSPROP_ENUMERATE | JSPROP_SHARED,
                            Getter));
    CHECK(JS_GetOwnPropertyDescriptor(cx, obj, "bar", &desc));
    CHECK(CheckDescriptor(desc, AccessorDescriptor, true, true, true));

    
    defineValue.setObject(*obj);
    CHECK(JS_DefineProperty(cx, obj, "baz", defineValue,
                            JSPROP_IGNORE_ENUMERATE |
                            JSPROP_IGNORE_READONLY |
                            JSPROP_IGNORE_PERMANENT));
    CHECK(JS_GetOwnPropertyDescriptor(cx, obj, "baz", &desc));
    CHECK(CheckDescriptor(desc, DataDescriptor, false, false, false));

    
    CHECK(JS_DefineProperty(cx, obj, "quux", defineValue,
                            JSPROP_IGNORE_ENUMERATE | JSPROP_IGNORE_READONLY));
    CHECK(JS_GetOwnPropertyDescriptor(cx, obj, "quux", &desc));
    CHECK(CheckDescriptor(desc, DataDescriptor, false, false, true));

    
    defineValue.setUndefined();
    CHECK(JS_DefineProperty(cx, obj, "quux", defineValue,
                            JSPROP_IGNORE_ENUMERATE |
                            JSPROP_IGNORE_PERMANENT |
                            JSPROP_IGNORE_VALUE));

    CHECK(JS_GetOwnPropertyDescriptor(cx, obj, "quux", &desc));
    CHECK(CheckDescriptor(desc, DataDescriptor, false, true, true));
    CHECK_SAME(JS::ObjectValue(*obj), desc.value());

    return true;
}
END_TEST(testDefinePropertyIgnoredAttributes)
