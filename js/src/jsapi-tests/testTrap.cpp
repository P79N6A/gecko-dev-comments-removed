






#include "js/OldDebugAPI.h"
#include "jsapi-tests/tests.h"

static int emptyTrapCallCount = 0;

static JSTrapStatus
EmptyTrapHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval,
                 jsval closureArg)
{
    JS::RootedValue closure(cx, closureArg);
    JS_GC(JS_GetRuntime(cx));
    if (closure.isString())
        ++emptyTrapCallCount;
    return JSTRAP_CONTINUE;
}

BEGIN_TEST(testTrap_gc)
{
    static const char source[] =
"var i = 0;\n"
"var sum = 0;\n"
"while (i < 10) {\n"
"    sum += i;\n"
"    ++i;\n"
"}\n"
"({ result: sum });\n"
        ;

    
    JS::CompileOptions options(cx);
    options.setFileAndLine(__FILE__, 1);
    JS::RootedScript script(cx);
    CHECK(JS_CompileScript(cx, global, source, strlen(source), options, &script));
    CHECK(script);

    
    JS::RootedValue v2(cx);
    CHECK(JS_ExecuteScript(cx, global, script, &v2));
    CHECK(v2.isObject());
    CHECK_EQUAL(emptyTrapCallCount, 0);

    
    CHECK(JS_SetDebugMode(cx, true));

    static const char trapClosureText[] = "some trap closure";

    
    
    JS::RootedString trapClosure(cx);
    {
        jsbytecode *line2 = JS_LineNumberToPC(cx, script, 1);
        CHECK(line2);

        jsbytecode *line6 = JS_LineNumberToPC(cx, script, 5);
        CHECK(line2);

        trapClosure = JS_NewStringCopyZ(cx, trapClosureText);
        CHECK(trapClosure);
        JS::RootedValue closureValue(cx, JS::StringValue(trapClosure));
        JS_SetTrap(cx, script, line2, EmptyTrapHandler, closureValue);
        JS_SetTrap(cx, script, line6, EmptyTrapHandler, closureValue);

        JS_GC(rt);

        CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(trapClosure), trapClosureText));
    }

    
    CHECK(JS_ExecuteScript(cx, global, script, &v2));
    CHECK_EQUAL(emptyTrapCallCount, 11);

    JS_GC(rt);

    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(trapClosure), trapClosureText));

    return true;
}
END_TEST(testTrap_gc)

