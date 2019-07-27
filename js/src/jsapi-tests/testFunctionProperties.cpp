






#include "jsapi-tests/tests.h"

BEGIN_TEST(testFunctionProperties)
{
    JS::RootedValue x(cx);
    EVAL("(function f() {})", &x);

    JS::RootedObject obj(cx, x.toObjectOrNull());

    JS::RootedValue y(cx);
    CHECK(JS_GetProperty(cx, obj, "arguments", &y));
    CHECK(y.isNull());

    CHECK(JS_GetProperty(cx, obj, "caller", &y));
    CHECK(y.isNull());

    return true;
}
END_TEST(testFunctionProperties)
