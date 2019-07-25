



#include "tests.h"
#include "jsscript.h"
#include "jsxdrapi.h"

BEGIN_TEST(testXDR_bug506491)
{
    const char *s =
        "function makeClosure(s, name, value) {\n"
        "    eval(s);\n"
        "    return let (n = name, v = value) function () { return String(v); };\n"
        "}\n"
        "var f = makeClosure('0;', 'status', 'ok');\n";

    
    JSObject *scriptObj = JS_CompileScript(cx, global, s, strlen(s), __FILE__, __LINE__);
    CHECK(scriptObj);

    
    JSXDRState *w = JS_XDRNewMem(cx, JSXDR_ENCODE);
    CHECK(w);
    CHECK(JS_XDRScriptObject(w, &scriptObj));
    uint32 nbytes;
    void *p = JS_XDRMemGetData(w, &nbytes);
    CHECK(p);
    void *frozen = JS_malloc(cx, nbytes);
    CHECK(frozen);
    memcpy(frozen, p, nbytes);
    JS_XDRDestroy(w);

    
    scriptObj = NULL;
    JSXDRState *r = JS_XDRNewMem(cx, JSXDR_DECODE);
    JS_XDRMemSetData(r, frozen, nbytes);
    CHECK(JS_XDRScriptObject(r, &scriptObj));
    JS_XDRDestroy(r);  

    
    jsvalRoot v2(cx);
    CHECK(JS_ExecuteScript(cx, global, scriptObj, v2.addr()));

    
    JS_GC(cx);

    
    EVAL("f() === 'ok';\n", v2.addr());
    jsvalRoot trueval(cx, JSVAL_TRUE);
    CHECK_SAME(v2, trueval);
    return true;
}
END_TEST(testXDR_bug506491)

BEGIN_TEST(testXDR_bug516827)
{
    
    JSObject *scriptObj = JS_CompileScript(cx, global, "", 0, __FILE__, __LINE__);
    CHECK(scriptObj);

    
    JSXDRState *w = JS_XDRNewMem(cx, JSXDR_ENCODE);
    CHECK(w);
    CHECK(JS_XDRScriptObject(w, &scriptObj));
    uint32 nbytes;
    void *p = JS_XDRMemGetData(w, &nbytes);
    CHECK(p);
    void *frozen = JS_malloc(cx, nbytes);
    CHECK(frozen);
    memcpy(frozen, p, nbytes);
    JS_XDRDestroy(w);

    
    scriptObj = NULL;
    JSXDRState *r = JS_XDRNewMem(cx, JSXDR_DECODE);
    JS_XDRMemSetData(r, frozen, nbytes);
    CHECK(JS_XDRScriptObject(r, &scriptObj));
    JS_XDRDestroy(r);  

    
    CHECK(JS_ExecuteScript(cx, global, scriptObj, NULL));
    return true;
}
END_TEST(testXDR_bug516827)
