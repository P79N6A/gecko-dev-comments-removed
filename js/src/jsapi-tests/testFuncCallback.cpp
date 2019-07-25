#include "tests.h"
#include "jsfun.h"
#include "jscntxt.h"


#include "jstracer.h"

#ifdef MOZ_TRACE_JSCALLS

static int depth = 0;
static int enters = 0;
static int leaves = 0;
static int interpreted = 0;

static void
funcTransition(const JSFunction *,
               const JSScript *,
               const JSContext *cx,
               JSBool entering)
{
    if (entering) {
        ++depth;
        ++enters;
        if (! JS_ON_TRACE(cx))
            ++interpreted;
    } else {
        --depth;
        ++leaves;
    }
}

static JSBool called2 = false;

static void
funcTransition2(const JSFunction *, const JSScript*, const JSContext*, JSBool)
{
    called2 = true;
}

static int overlays = 0;
static JSFunctionCallback innerCallback = NULL;
static void
funcTransitionOverlay(const JSFunction *fun,
                      const JSScript *script,
                      const JSContext *cx,
                      JSBool entering)
{
    (*innerCallback)(fun, script, cx, entering);
    overlays++;
}
#endif

BEGIN_TEST(testFuncCallback_bug507012)
{
#ifdef MOZ_TRACE_JSCALLS
    
    JS_SetFunctionCallback(cx, funcTransition);

    EXEC("x = 0; function f (n) { if (n > 1) { f(n - 1); } }");
    interpreted = enters = leaves = depth = 0;

    
    EXEC("42");
    CHECK(enters == 1 && leaves == 1 && depth == 0);
    interpreted = enters = leaves = depth = 0;

    
    EXEC("f(1)");
    CHECK(enters == 2 && leaves == 2 && depth == 0);

    
    enters = 777;
    JS_SetFunctionCallback(cx, funcTransition2);
    EXEC("f(1)");
    CHECK(called2 && enters == 777);

    
    JS_SetFunctionCallback(cx, NULL);
    EXEC("f(1)");
    CHECK(enters == 777);
    interpreted = enters = leaves = depth = 0;

    
    JS_SetFunctionCallback(cx, funcTransition);
    enters = leaves = depth = 0;
    EXEC("f(3)");
    CHECK(enters == 1+3 && leaves == 1+3 && depth == 0);
    interpreted = enters = leaves = depth = 0;

    
    EXEC("function g () { ++x; }");
    interpreted = enters = leaves = depth = 0;
    EXEC("for (i = 0; i < 50; ++i) { g(); }");
    CHECK(enters == 50+1 && leaves == 50+1 && depth == 0);

    
    
    
    
#ifdef JS_TRACER
    if (TRACING_ENABLED(cx))
        CHECK(interpreted < enters);
#endif

    
    JS_SetFunctionCallback(cx, funcTransition);
    innerCallback = JS_GetFunctionCallback(cx);
    JS_SetFunctionCallback(cx, funcTransitionOverlay);

    EXEC("x = 0; function f (n) { if (n > 1) { f(n - 1); } }");
    interpreted = enters = leaves = depth = overlays = 0;

    EXEC("42.5");
    CHECK(enters == 1);
    CHECK(leaves == 1);
    CHECK(depth == 0);
    CHECK(overlays == 2); 
    interpreted = enters = leaves = depth = overlays = 0;
#endif

    return true;
}






virtual
JSContext *createContext()
{
    JSContext *cx = JSAPITest::createContext();
    if (cx)
        JS_SetOptions(cx, JS_GetOptions(cx) | JSOPTION_JIT);
    return cx;
}

END_TEST(testFuncCallback_bug507012)
