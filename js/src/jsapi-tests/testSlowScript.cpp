



#include "jsapi-tests/tests.h"

static bool
InterruptCallback(JSContext* cx)
{
    return false;
}

static unsigned sRemain;

static bool
RequestInterruptCallback(JSContext* cx, unsigned argc, jsval* vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    if (!sRemain--)
        JS_RequestInterruptCallback(JS_GetRuntime(cx));
    args.rval().setUndefined();
    return true;
}

BEGIN_TEST(testSlowScript)
{
    JS_SetInterruptCallback(cx, InterruptCallback);
    JS_DefineFunction(cx, global, "requestInterruptCallback", RequestInterruptCallback, 0, 0);

    test("while (true)"
         "  for (i in [0,0,0,0])"
         "    requestInterruptCallback();");

    test("while (true)"
         "  for (i in [0,0,0,0])"
         "    for (j in [0,0,0,0])"
         "      requestInterruptCallback();");

    test("while (true)"
         "  for (i in [0,0,0,0])"
         "    for (j in [0,0,0,0])"
         "      for (k in [0,0,0,0])"
         "        requestInterruptCallback();");

    test("function f() { while (true) yield requestInterruptCallback() }"
         "for (i in f()) ;");

    test("function f() { while (true) yield 1 }"
         "for (i in f())"
         "  requestInterruptCallback();");

    test("(function() {"
         "  while (true)"
         "    let (x = 1) { eval('requestInterruptCallback()'); }"
         "})()");

    return true;
}

bool
test(const char* bytes)
{
    jsval v;

    JS_SetOptions(cx, JS_GetOptions(cx) & ~(JSOPTION_METHODJIT | JSOPTION_METHODJIT_ALWAYS));
    sRemain = 0;
    CHECK(!evaluate(bytes, __FILE__, __LINE__, &v));
    CHECK(!JS_IsExceptionPending(cx));

    sRemain = 1000;
    CHECK(!evaluate(bytes, __FILE__, __LINE__, &v));
    CHECK(!JS_IsExceptionPending(cx));

    JS_SetOptions(cx, JS_GetOptions(cx) | JSOPTION_METHODJIT | JSOPTION_METHODJIT_ALWAYS);

    sRemain = 0;
    CHECK(!evaluate(bytes, __FILE__, __LINE__, &v));
    CHECK(!JS_IsExceptionPending(cx));

    sRemain = 1000;
    CHECK(!evaluate(bytes, __FILE__, __LINE__, &v));
    CHECK(!JS_IsExceptionPending(cx));

    return true;
}
END_TEST(testSlowScript)
