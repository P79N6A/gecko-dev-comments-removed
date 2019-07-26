






#include "jsapi-tests/tests.h"

BEGIN_TEST(testException_bug860435)
{
    JS::RootedValue fun(cx);

    EVAL("ReferenceError", fun.address());
    CHECK(fun.isObject());

    JS::RootedValue v(cx);
    JS_CallFunctionValue(cx, global, fun, 0, v.address(), v.address());
    CHECK(v.isObject());

    JS_GetProperty(cx, &v.toObject(), "stack", &v);
    CHECK(v.isString());
    return true;
}
END_TEST(testException_bug860435)
