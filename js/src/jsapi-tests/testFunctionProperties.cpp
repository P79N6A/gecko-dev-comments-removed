







#include "tests.h"

BEGIN_TEST(testFunctionProperties)
{
    JS::RootedValue x(cx);
    EVAL("(function f() {})", x.address());

    JS::RootedObject obj(cx, JSVAL_TO_OBJECT(x));

    JS::RootedValue y(cx);
    CHECK(JS_GetProperty(cx, obj, "arguments", y.address()));
    CHECK_SAME(y, JSVAL_NULL);

    CHECK(JS_GetProperty(cx, obj, "caller", y.address()));
    CHECK_SAME(y, JSVAL_NULL);

    return true;
}
END_TEST(testFunctionProperties)
