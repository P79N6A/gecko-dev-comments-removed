






#include "jsapi-tests/tests.h"

BEGIN_TEST(testDeepFreeze_bug535703)
{
    JS::RootedValue v(cx);
    EVAL("var x = {}; x;", &v);
    JS::RootedObject obj(cx, v.toObjectOrNull());
    CHECK(JS_DeepFreezeObject(cx, obj));  
    EVAL("Object.isFrozen(x)", &v);
    CHECK(v.isTrue());
    return true;
}
END_TEST(testDeepFreeze_bug535703)

BEGIN_TEST(testDeepFreeze_deep)
{
    JS::RootedValue a(cx), o(cx);
    EXEC("var a = {}, o = a;\n"
         "for (var i = 0; i < 5000; i++)\n"
         "    a = {x: a, y: a};\n");
    EVAL("a", &a);
    EVAL("o", &o);

    JS::RootedObject aobj(cx, a.toObjectOrNull());
    CHECK(JS_DeepFreezeObject(cx, aobj));

    JS::RootedValue b(cx);
    EVAL("Object.isFrozen(a)", &b);
    CHECK(b.isTrue());
    EVAL("Object.isFrozen(o)", &b);
    CHECK(b.isTrue());
    return true;
}
END_TEST(testDeepFreeze_deep)

BEGIN_TEST(testDeepFreeze_loop)
{
    JS::RootedValue x(cx), y(cx);
    EXEC("var x = [], y = {x: x}; y.y = y; x.push(x, y);");
    EVAL("x", &x);
    EVAL("y", &y);

    JS::RootedObject xobj(cx, x.toObjectOrNull());
    CHECK(JS_DeepFreezeObject(cx, xobj));

    JS::RootedValue b(cx);
    EVAL("Object.isFrozen(x)", &b);
    CHECK(b.isTrue());
    EVAL("Object.isFrozen(y)", &b);
    CHECK(b.isTrue());
    return true;
}
END_TEST(testDeepFreeze_loop)
