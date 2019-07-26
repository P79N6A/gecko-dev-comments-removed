







#include "tests.h"
#include "jsdbgapi.h"

static int emptyTrapCallCount = 0;

static JSTrapStatus
EmptyTrapHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval,
                 jsval closureArg)
{
    js::RootedValue closure(cx, closureArg);
    JS_GC(JS_GetRuntime(cx));
    if (JSVAL_IS_STRING(closure))
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

    
    js::RootedScript script(cx, JS_CompileScript(cx, global, source, strlen(source), __FILE__, 1));
    CHECK(script);

    
    js::RootedValue v2(cx);
    CHECK(JS_ExecuteScript(cx, global, script, v2.address()));
    CHECK(v2.isObject());
    CHECK_EQUAL(emptyTrapCallCount, 0);

    
    CHECK(JS_SetDebugMode(cx, JS_TRUE));

    static const char trapClosureText[] = "some trap closure";

    
    
    js::RootedString trapClosure(cx);
    {
        jsbytecode *line2 = JS_LineNumberToPC(cx, script, 1);
        CHECK(line2);

        jsbytecode *line6 = JS_LineNumberToPC(cx, script, 5);
        CHECK(line2);

        trapClosure = JS_NewStringCopyZ(cx, trapClosureText);
        CHECK(trapClosure);
        JS_SetTrap(cx, script, line2, EmptyTrapHandler, STRING_TO_JSVAL(trapClosure));
        JS_SetTrap(cx, script, line6, EmptyTrapHandler, STRING_TO_JSVAL(trapClosure));

        JS_GC(rt);

        CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(trapClosure), trapClosureText));
    }

    
    CHECK(JS_ExecuteScript(cx, global, script, v2.address()));
    CHECK_EQUAL(emptyTrapCallCount, 11);

    JS_GC(rt);

    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(trapClosure), trapClosureText));

    return true;
}
END_TEST(testTrap_gc)

