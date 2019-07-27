







#include "jsapi-tests/tests.h"

static unsigned errorCount = 0;

static void
ErrorCounter(JSContext *cx, const char *message, JSErrorReport *report)
{
    ++errorCount;
}

BEGIN_TEST(testGCOutOfMemory)
{
    JS_SetErrorReporter(rt, ErrorCounter);

    JS::RootedValue root(cx);

    
    static const char source[] =
        "var max = 0; (function() {"
        "    var array = [];"
        "    for (; ; ++max)"
        "        array.push({});"
        "    array = []; array.push(0);"
        "})();";
    JS::CompileOptions opts(cx);
    bool ok = JS::Evaluate(cx, opts, source, strlen(source), &root);

    
    CHECK(!ok);
    CHECK(!JS_IsExceptionPending(cx));
    CHECK_EQUAL(errorCount, 1u);
    JS_GC(rt);

    
    
    EVAL("(function() {"
         "    var array = [];"
         "    for (var i = max >> 2; i != 0;) {"
         "        --i;"
         "        array.push({});"
         "    }"
         "})();", &root);
    CHECK_EQUAL(errorCount, 1u);
    return true;
}

virtual JSRuntime * createRuntime() override {
    
    
    
    
    
    
    
    JSRuntime *rt = JS_NewRuntime(768 * 1024, 128 * 1024);
    if (!rt)
        return nullptr;
    setNativeStackQuota(rt);
    return rt;
}

virtual void destroyRuntime() override {
    JS_DestroyRuntime(rt);
}

END_TEST(testGCOutOfMemory)
