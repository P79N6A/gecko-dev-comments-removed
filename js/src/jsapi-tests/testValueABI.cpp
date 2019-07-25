#include "tests.h"








extern "C" {

extern JSBool
C_ValueToObject(JSContext *cx, jsval v, JSObject **obj);

extern jsval
C_GetEmptyStringValue(JSContext *cx);

}

BEGIN_TEST(testValueABI)
{
    JSObject* obj = JS_GetGlobalObject(cx);
    jsval v = OBJECT_TO_JSVAL(obj);
    obj = NULL;
    CHECK(C_ValueToObject(cx, v, &obj));
    JSBool equal;
    CHECK(JS_StrictlyEqual(cx, v, OBJECT_TO_JSVAL(obj), &equal));
    CHECK(equal);

    v = C_GetEmptyStringValue(cx);
    CHECK(JSVAL_IS_STRING(v));

    return true;
}
END_TEST(testValueABI)
