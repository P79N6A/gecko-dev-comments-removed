






#include "jsapi-tests/tests.h"

static const unsigned IgnoreWithValue = JSPROP_IGNORE_ENUMERATE | JSPROP_IGNORE_READONLY |
                               JSPROP_IGNORE_PERMANENT;
static const unsigned IgnoreAll = IgnoreWithValue | JSPROP_IGNORE_VALUE;

static const unsigned AllowConfigure = IgnoreAll & ~JSPROP_IGNORE_PERMANENT;
static const unsigned AllowEnumerate = IgnoreAll & ~JSPROP_IGNORE_ENUMERATE;
static const unsigned AllowWritable  = IgnoreAll & ~JSPROP_IGNORE_READONLY;
static const unsigned ValueWithConfigurable = IgnoreWithValue & ~JSPROP_IGNORE_PERMANENT;

static bool
Getter(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JS::CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setBoolean(true);
    return true;
}

static bool
CheckDescriptor(JS::Handle<JSPropertyDescriptor> desc, bool enumerable,
                bool writable, bool configurable)
{
    if (!desc.object())
        return false;
    if (desc.isEnumerable() != enumerable)
        return false;
    if (desc.isReadonly() == writable)
        return false;
    if (desc.isPermanent() == configurable)
        return false;
    return true;
}

BEGIN_TEST(testDefinePropertyIgnoredAttributes)
{
    JS::RootedObject obj(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
    JS::Rooted<JSPropertyDescriptor> desc(cx);
    JS::RootedValue defineValue(cx);

    
    CHECK(JS_DefineProperty(cx, obj, "foo", defineValue,
                            IgnoreAll | JSPROP_NATIVE_ACCESSORS | JSPROP_SHARED,
                            (JSPropertyOp)Getter));

    CHECK(JS_GetPropertyDescriptor(cx, obj, "foo", &desc));

    
    
    CHECK(CheckDescriptor(desc, false, true, false));

    
    CHECK(JS_DefineProperty(cx, obj, "bar", defineValue,
                            AllowConfigure | JSPROP_NATIVE_ACCESSORS | JSPROP_SHARED,
                            (JSPropertyOp)Getter));
    CHECK(JS_GetPropertyDescriptor(cx, obj, "bar", &desc));
    CHECK(CheckDescriptor(desc, false, true, true));

    
    
    CHECK(JS_DefineProperty(cx, obj, "bar", defineValue,
                            AllowEnumerate |
                            JSPROP_ENUMERATE |
                            JSPROP_NATIVE_ACCESSORS |
                            JSPROP_SHARED,
                            (JSPropertyOp)Getter));
    CHECK(JS_GetPropertyDescriptor(cx, obj, "bar", &desc));
    CHECK(CheckDescriptor(desc, true, true, true));

    
    defineValue.setObject(*obj);
    CHECK(JS_DefineProperty(cx, obj, "baz", defineValue, IgnoreWithValue));
    CHECK(JS_GetPropertyDescriptor(cx, obj, "baz", &desc));
    CHECK(CheckDescriptor(desc, false, false, false));

    
    CHECK(JS_DefineProperty(cx, obj, "quox", defineValue, ValueWithConfigurable));
    CHECK(JS_GetPropertyDescriptor(cx, obj, "quox", &desc));
    CHECK(CheckDescriptor(desc, false, false, true));

    
    defineValue.setUndefined();
    CHECK(JS_DefineProperty(cx, obj, "quox", defineValue, AllowWritable));
    CHECK(JS_GetPropertyDescriptor(cx, obj, "quox", &desc));
    CHECK(CheckDescriptor(desc, false, true, true));
    CHECK_SAME(ObjectValue(*obj), desc.value());

    return true;
}
END_TEST(testDefinePropertyIgnoredAttributes)
