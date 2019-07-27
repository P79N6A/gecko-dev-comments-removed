






#include "jscntxt.h"

#include "js/OldDebugAPI.h"
#include "jsapi-tests/tests.h"

using namespace js;

BEGIN_TEST(testDebugger_debuggerObjectVsDebugMode)
{
    CHECK(JS_DefineDebuggerObject(cx, global));
    JS::RootedObject debuggee(cx, JS_NewGlobalObject(cx, getGlobalClass(), nullptr, JS::FireOnNewGlobalHook));
    CHECK(debuggee);

    {
        JSAutoCompartment ae(cx, debuggee);
        CHECK(JS_SetDebugMode(cx, true));
        CHECK(JS_InitStandardClasses(cx, debuggee));
    }

    JS::RootedObject debuggeeWrapper(cx, debuggee);
    CHECK(JS_WrapObject(cx, &debuggeeWrapper));
    JS::RootedValue v(cx, JS::ObjectValue(*debuggeeWrapper));
    CHECK(JS_SetProperty(cx, global, "debuggee", v));

    EVAL("var dbg = new Debugger(debuggee);\n"
         "var hits = 0;\n"
         "dbg.onDebuggerStatement = function () { hits++; };\n"
         "debuggee.eval('debugger;');\n"
         "hits;\n",
         &v);
    CHECK_SAME(v, JSVAL_ONE);

    {
        JSAutoCompartment ae(cx, debuggee);
        CHECK(JS_SetDebugMode(cx, false));
    }

    EVAL("debuggee.eval('debugger; debugger; debugger;');\n"
         "hits;\n",
         &v);
    CHECK_SAME(v, INT_TO_JSVAL(4));

    return true;
}
END_TEST(testDebugger_debuggerObjectVsDebugMode)

BEGIN_TEST(testDebugger_newScriptHook)
{
    
    CHECK(JS_DefineDebuggerObject(cx, global));
    JS::RootedObject g(cx, JS_NewGlobalObject(cx, getGlobalClass(), nullptr, JS::FireOnNewGlobalHook));
    CHECK(g);
    {
        JSAutoCompartment ae(cx, g);
        CHECK(JS_InitStandardClasses(cx, g));
    }

    JS::RootedObject gWrapper(cx, g);
    CHECK(JS_WrapObject(cx, &gWrapper));
    JS::RootedValue v(cx, JS::ObjectValue(*gWrapper));
    CHECK(JS_SetProperty(cx, global, "g", v));

    EXEC("var dbg = Debugger(g);\n"
         "var hits = 0;\n"
         "dbg.onNewScript = function (s) {\n"
         "    hits += Number(s instanceof Debugger.Script);\n"
         "};\n");

    
    
    
    
    
    
    return testIndirectEval(g, "Math.abs(0)");
}

bool testIndirectEval(JS::HandleObject scope, const char *code)
{
    EXEC("hits = 0;");

    {
        JSAutoCompartment ae(cx, scope);
        JSString *codestr = JS_NewStringCopyZ(cx, code);
        CHECK(codestr);
        JS::RootedValue arg(cx, JS::StringValue(codestr));
        JS::RootedValue v(cx);
        CHECK(JS_CallFunctionName(cx, scope, "eval", HandleValueArray(arg), &v));
    }

    JS::RootedValue hitsv(cx);
    EVAL("hits", &hitsv);
    CHECK_SAME(hitsv, INT_TO_JSVAL(1));
    return true;
}
END_TEST(testDebugger_newScriptHook)
