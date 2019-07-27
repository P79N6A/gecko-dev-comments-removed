






#include "jscntxt.h"

#include "js/OldDebugAPI.h"
#include "jsapi-tests/tests.h"

using namespace js;

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
