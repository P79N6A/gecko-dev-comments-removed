







#include "tests.h"

BEGIN_TEST(testDefineProperty_bug564344)
{
    JS::RootedValue x(cx);
    EVAL("function f() {}\n"
         "var x = {p: f};\n"
         "x.p();  // brand x's scope\n"
         "x;", x.address());

    JS::RootedObject obj(cx, JSVAL_TO_OBJECT(x));
    for (int i = 0; i < 2; i++)
        CHECK(JS_DefineProperty(cx, obj, "q", JSVAL_VOID, NULL, NULL, JSPROP_SHARED));
    return true;
}
END_TEST(testDefineProperty_bug564344)
