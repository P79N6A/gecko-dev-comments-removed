



#include "jsapi-tests/tests.h"

#ifdef MOZ_TRACE_JSCALLS

static int depth = 0;
static int enters = 0;
static int leaves = 0;
static int interpreted = 0;

static void
funcTransition(const JSFunction *,
               const JSScript *,
               const JSContext *cx,
               int entering)
{
    if (entering > 0) {
        ++depth;
        ++enters;
        ++interpreted;
    } else {
        --depth;
        ++leaves;
    }
}

static bool called2 = false;

static void
funcTransition2(const JSFunction *, const JSScript*, const JSContext*, int)
{
    called2 = true;
}

static int overlays = 0;
static JSFunctionCallback innerCallback = nullptr;
static void
funcTransitionOverlay(const JSFunction *fun,
                      const JSScript *script,
                      const JSContext *cx,
                      int entering)
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
    CHECK_EQUAL(enters, 1);
    CHECK_EQUAL(leaves, 1);
    CHECK_EQUAL(depth, 0);
    interpreted = enters = leaves = depth = 0;

    
    EXEC("f(1)");
    CHECK_EQUAL(enters, 1+1);
    CHECK_EQUAL(leaves, 1+1);
    CHECK_EQUAL(depth, 0);

    
    enters = 777;
    JS_SetFunctionCallback(cx, funcTransition2);
    EXEC("f(1)");
    CHECK(called2);
    CHECK_EQUAL(enters, 777);

    
    JS_SetFunctionCallback(cx, nullptr);
    EXEC("f(1)");
    CHECK_EQUAL(enters, 777);
    interpreted = enters = leaves = depth = 0;

    
    JS_SetFunctionCallback(cx, funcTransition);
    enters = leaves = depth = 0;
    EXEC("f(3)");
    CHECK_EQUAL(enters, 1+3);
    CHECK_EQUAL(leaves, 1+3);
    CHECK_EQUAL(depth, 0);
    interpreted = enters = leaves = depth = 0;

    
    
    EXEC("function g () { ++x; }");
    interpreted = enters = leaves = depth = 0;
    EXEC("for (i = 0; i < 5000; ++i) { g(); }");
    CHECK_EQUAL(enters, 1+5000);
    CHECK_EQUAL(leaves, 1+5000);
    CHECK_EQUAL(depth, 0);

    
    JS_SetFunctionCallback(cx, funcTransition);
    innerCallback = JS_GetFunctionCallback(cx);
    JS_SetFunctionCallback(cx, funcTransitionOverlay);

    EXEC("x = 0; function f (n) { if (n > 1) { f(n - 1); } }");
    interpreted = enters = leaves = depth = overlays = 0;

    EXEC("42.5");
    CHECK_EQUAL(enters, 1);
    CHECK_EQUAL(leaves, 1);
    CHECK_EQUAL(depth, 0);
    CHECK_EQUAL(overlays, enters + leaves);
    interpreted = enters = leaves = depth = overlays = 0;
#endif

    
    
    
    

    return true;
}



virtual
JSContext *createContext()
{
    JSContext *cx = JSAPITest::createContext();
    if (!cx)
        return nullptr;
    ContextOptionsRef(cx).setBaseline(true)
                         .setIon(true);
    return cx;
}

END_TEST(testFuncCallback_bug507012)
