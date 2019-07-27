



#include "jsapi-tests/tests.h"











extern "C" {

extern bool
C_ValueToObject(JSContext *cx, jsval v, JSObject **obj);

extern jsval
C_GetEmptyStringValue(JSContext *cx);

extern size_t
C_jsvalAlignmentTest();

}

BEGIN_TEST(testValueABI_retparam)
{
    JS::RootedObject obj(cx, JS::CurrentGlobalOrNull(cx));
    RootedValue v(cx, ObjectValue(*obj));
    obj = nullptr;
    CHECK(C_ValueToObject(cx, v, obj.address()));
    bool equal;
    RootedValue v2(cx, ObjectValue(*obj));
    CHECK(JS_StrictlyEqual(cx, v, v2, &equal));
    CHECK(equal);

    v = C_GetEmptyStringValue(cx);
    CHECK(v.isString());

    return true;
}
END_TEST(testValueABI_retparam)

BEGIN_TEST(testValueABI_alignment)
{
    typedef struct { char c; jsval v; } AlignTest;
    CHECK(C_jsvalAlignmentTest() == sizeof(AlignTest));

    return true;
}
END_TEST(testValueABI_alignment)
