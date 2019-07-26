



#include "tests.h"











extern "C" {

extern JSBool
C_ValueToObject(JSContext *cx, jsval v, JSObject **obj);

extern jsval
C_GetEmptyStringValue(JSContext *cx);

extern size_t
C_jsvalAlignmentTest();

}

BEGIN_TEST(testValueABI_retparam)
{
    JS::RootedObject obj(cx, JS_GetGlobalObject(cx));
    jsval v = OBJECT_TO_JSVAL(obj);
    obj = NULL;
    CHECK(C_ValueToObject(cx, v, obj.address()));
    JSBool equal;
    CHECK(JS_StrictlyEqual(cx, v, OBJECT_TO_JSVAL(obj), &equal));
    CHECK(equal);

    v = C_GetEmptyStringValue(cx);
    CHECK(JSVAL_IS_STRING(v));

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
