







#include "tests.h"

BEGIN_TEST(testDeepFreeze_bug535703)
{
    JS::RootedValue v(cx);
    EVAL("var x = {}; x;", v.address());
    JS::RootedObject obj(cx, JSVAL_TO_OBJECT(v));
    CHECK(JS_DeepFreezeObject(cx, obj));  
    EVAL("Object.isFrozen(x)", v.address());
    CHECK_SAME(v, JSVAL_TRUE);
    return true;
}
END_TEST(testDeepFreeze_bug535703)

BEGIN_TEST(testDeepFreeze_deep)
{
    JS::RootedValue a(cx), o(cx);
    EXEC("var a = {}, o = a;\n"
         "for (var i = 0; i < 5000; i++)\n"
         "    a = {x: a, y: a};\n");
    EVAL("a", a.address());
    EVAL("o", o.address());

    JS::RootedObject aobj(cx, JSVAL_TO_OBJECT(a));
    CHECK(JS_DeepFreezeObject(cx, aobj));

    JS::RootedValue b(cx);
    EVAL("Object.isFrozen(a)", b.address());
    CHECK_SAME(b, JSVAL_TRUE);
    EVAL("Object.isFrozen(o)", b.address());
    CHECK_SAME(b, JSVAL_TRUE);
    return true;
}
END_TEST(testDeepFreeze_deep)

BEGIN_TEST(testDeepFreeze_loop)
{
    JS::RootedValue x(cx), y(cx);
    EXEC("var x = [], y = {x: x}; y.y = y; x.push(x, y);");
    EVAL("x", x.address());
    EVAL("y", y.address());

    JS::RootedObject xobj(cx, JSVAL_TO_OBJECT(x));
    CHECK(JS_DeepFreezeObject(cx, xobj));

    JS::RootedValue b(cx);
    EVAL("Object.isFrozen(x)", b.address());
    CHECK_SAME(b, JSVAL_TRUE);
    EVAL("Object.isFrozen(y)", b.address());
    CHECK_SAME(b, JSVAL_TRUE);
    return true;
}
END_TEST(testDeepFreeze_loop)
