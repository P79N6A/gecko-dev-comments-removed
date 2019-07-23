#include "tests.h"
#include "jsdbgapi.h"

static int callCount[2] = {0, 0};

static void *
callHook(JSContext *cx, JSStackFrame *fp, JSBool before, JSBool *ok, void *closure)
{
    callCount[before]++;
    JS_GetFrameThis(cx, fp);  
    return cx;  
}

BEGIN_TEST(testDebugger_bug519719)
{
    JS_SetCallHook(rt, callHook, NULL);
    EXEC("function call(fn) { fn(0); }\n"
         "function f(g) { for (var i = 0; i < 9; i++) call(g); }\n"
         "f(Math.sin);\n"    
         "f(Math.cos);\n");  
    CHECK(callCount[0] == 20);
    CHECK(callCount[1] == 20);
    return true;
}
END_TEST(testDebugger_bug519719)
