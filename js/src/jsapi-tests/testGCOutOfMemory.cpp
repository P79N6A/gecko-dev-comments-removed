







#include "tests.h"
#include "jscntxt.h"

static unsigned errorCount = 0;

static void
ErrorCounter(JSContext *cx, const char *message, JSErrorReport *report)
{
    ++errorCount;
}

BEGIN_TEST(testGCOutOfMemory)
{
    JS_SetErrorReporter(cx, ErrorCounter);

    JS::RootedValue root(cx);

    static const char source[] =
        "var max = 0; (function() {"
        "    var array = [];"
        "    for (; ; ++max)"
        "        array.push({});"
        "    array = []; array.push(0);"
        "})();";
    JSBool ok = JS_EvaluateScript(cx, global, source, strlen(source), "", 1,
                                  root.address());

    
    CHECK(!ok);
    CHECK(!JS_IsExceptionPending(cx));
    CHECK_EQUAL(errorCount, 1);
    JS_GC(rt);

    
    return true;

    EVAL("(function() {"
         "    var array = [];"
         "    for (var i = max >> 2; i != 0;) {"
         "        --i;"
         "        array.push({});"
         "    }"
         "})();", root.address());
    CHECK_EQUAL(errorCount, 1);
    return true;
}

virtual JSRuntime * createRuntime() {
    return JS_NewRuntime(768 * 1024, JS_USE_HELPER_THREADS);
}

virtual void destroyRuntime() {
    JS_DestroyRuntime(rt);
}

END_TEST(testGCOutOfMemory)
