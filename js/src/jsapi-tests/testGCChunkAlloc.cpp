







#include "tests.h"
#include "jsgcchunk.h"
#include "jscntxt.h"



class CustomGCChunkAllocator: public js::GCChunkAllocator {
  public:
    CustomGCChunkAllocator() : pool(NULL) {}
    void *pool;
    
  private:

    virtual void *doAlloc() {
        if (!pool)
            return NULL;
        void *chunk = pool;
        pool = NULL;
        return chunk;
    }
        
    virtual void doFree(void *chunk) {
        JS_ASSERT(!pool);
        pool = chunk;
    }
};

static CustomGCChunkAllocator customGCChunkAllocator;

static unsigned errorCount = 0;

static void
ErrorCounter(JSContext *cx, const char *message, JSErrorReport *report)
{
    ++errorCount;
}

BEGIN_TEST(testGCChunkAlloc)
{
    JS_SetErrorReporter(cx, ErrorCounter);

    jsvalRoot root(cx);

    




    JS_ToggleOptions(cx, JSOPTION_JIT);

    static const char source[] =
        "var max = 0; (function() {"
        "    var array = [];"
        "    for (; ; ++max)"
        "        array.push({});"
        "})();";
    JSBool ok = JS_EvaluateScript(cx, global, source, strlen(source), "", 1,
                                  root.addr());

    
    CHECK(!ok);
    CHECK(!JS_IsExceptionPending(cx));
    CHECK(errorCount == 1);
    CHECK(!customGCChunkAllocator.pool);
    JS_GC(cx);
    JS_ToggleOptions(cx, JSOPTION_JIT);
    EVAL("(function() {"
         "    var array = [];"
         "    for (var i = max >> 1; i != 0;) {"
         "        --i;"
         "        array.push({});"
         "    }"
         "})();", root.addr());
    CHECK(errorCount == 1);
    return true;
}

virtual JSRuntime * createRuntime() {
    



    JSRuntime *rt = JS_NewRuntime(2 * js::GC_CHUNK_SIZE);
    if (!rt)
        return NULL;

    customGCChunkAllocator.pool = js::AllocGCChunk();
    JS_ASSERT(customGCChunkAllocator.pool);

    rt->setCustomGCChunkAllocator(&customGCChunkAllocator);
    return rt;
}

virtual void destroyRuntime() {
    JS_DestroyRuntime(rt);

    
    JS_ASSERT(customGCChunkAllocator.pool);
    js::FreeGCChunk(customGCChunkAllocator.pool);
    customGCChunkAllocator.pool = NULL;
}

END_TEST(testGCChunkAlloc)
