






#include "jsapi-tests/tests.h"

BEGIN_TEST(testDefineProperty_bug564344)
{
    JS::RootedValue x(cx);
    EVAL("function f() {}\n"
         "var x = {p: f};\n"
         "x.p();  // brand x's scope\n"
         "x;", &x);

    JS::RootedObject obj(cx, x.toObjectOrNull());
    for (int i = 0; i < 2; i++)
        CHECK(JS_DefineProperty(cx, obj, "q", JS::UndefinedHandleValue, JSPROP_SHARED));
    return true;
}
END_TEST(testDefineProperty_bug564344)
