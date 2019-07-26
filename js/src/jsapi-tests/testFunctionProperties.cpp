






#include "jsapi-tests/tests.h"

BEGIN_TEST(testFunctionProperties)
{
    JS::RootedValue x(cx);
    EVAL("(function f() {})", &x);

    JS::RootedObject obj(cx, x.toObjectOrNull());

    JS::RootedValue y(cx);
    CHECK(JS_GetProperty(cx, obj, "arguments", &y));
    CHECK_SAME(y, JSVAL_NULL);

    CHECK(JS_GetProperty(cx, obj, "caller", &y));
    CHECK_SAME(y, JSVAL_NULL);

    return true;
}
END_TEST(testFunctionProperties)
