







#include "tests.h"

BEGIN_TEST(testDefineProperty_bug564344)
{
    jsvalRoot x(cx);
    EVAL("function f() {}\n"
         "var x = {p: f};\n"
         "x.p();  // brand x's scope\n"
         "x;", x.addr());

    JSObject *obj = JSVAL_TO_OBJECT(x.value());
    for (int i = 0; i < 2; i++)
        CHECK(JS_DefineProperty(cx, obj, "q", JSVAL_VOID, NULL, NULL, JSPROP_SHARED));
    return true;
}
END_TEST(testDefineProperty_bug564344)
