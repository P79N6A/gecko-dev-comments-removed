






#include "jsapi-tests/tests.h"

BEGIN_TEST(testException_bug860435)
{
    JS::RootedValue fun(cx);

    EVAL("ReferenceError", fun.address());
    CHECK(fun.isObject());

    JS::RootedValue v(cx);
    JS_CallFunctionValue(cx, global, fun, JS::EmptyValueArray, v.address());
    CHECK(v.isObject());
    JS::RootedObject obj(cx, &v.toObject());

    JS_GetProperty(cx, obj, "stack", &v);
    CHECK(v.isString());
    return true;
}
END_TEST(testException_bug860435)
