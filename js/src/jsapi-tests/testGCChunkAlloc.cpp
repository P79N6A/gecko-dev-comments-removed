







#include "tests.h"
#include "jsgcchunk.h"
#include "jscntxt.h"



static const int SYSTEM  = 0;
static const int USER    = 1;
static const int N_POOLS = 2;

class CustomGCChunkAllocator: public js::GCChunkAllocator {
  public:
    CustomGCChunkAllocator() { pool[SYSTEM] = NULL; pool[USER] = NULL; }
    void *pool[N_POOLS];
    
  private:

    virtual void *doAlloc() {
        if (!pool[SYSTEM] && !pool[USER])
            return NULL;
        void *chunk = NULL;
        if (pool[SYSTEM]) {
            chunk = pool[SYSTEM];
            pool[SYSTEM] = NULL;
        } else {
            chunk = pool[USER];
            pool[USER] = NULL;
        }
        return chunk;
    }
        
    virtual void doFree(void *chunk) {
        JS_ASSERT(!pool[SYSTEM] || !pool[USER]);
        if (!pool[SYSTEM]) {
            pool[SYSTEM] = chunk;
        } else {
            pool[USER] = chunk;
        }
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
    CHECK_EQUAL(errorCount, 1);
    CHECK(!customGCChunkAllocator.pool[SYSTEM]);
    CHECK(!customGCChunkAllocator.pool[USER]);
    JS_GC(cx);
    JS_ToggleOptions(cx, JSOPTION_JIT);
    EVAL("(function() {"
         "    var array = [];"
         "    for (var i = max >> 1; i != 0;) {"
         "        --i;"
         "        array.push({});"
         "    }"
         "})();", root.addr());
    CHECK_EQUAL(errorCount, 1);
    return true;
}

virtual JSRuntime * createRuntime() {
    



    JSRuntime *rt = JS_NewRuntime(2 * js::GC_CHUNK_SIZE);
    if (!rt)
        return NULL;

    customGCChunkAllocator.pool[SYSTEM] = js::AllocGCChunk();
    customGCChunkAllocator.pool[USER] = js::AllocGCChunk();
    JS_ASSERT(customGCChunkAllocator.pool[SYSTEM]);
    JS_ASSERT(customGCChunkAllocator.pool[USER]);

    rt->setCustomGCChunkAllocator(&customGCChunkAllocator);
    return rt;
}

virtual void destroyRuntime() {
    JS_DestroyRuntime(rt);

    
    JS_ASSERT(customGCChunkAllocator.pool[SYSTEM]);
    JS_ASSERT(customGCChunkAllocator.pool[USER]);
    js::FreeGCChunk(customGCChunkAllocator.pool[SYSTEM]);
    js::FreeGCChunk(customGCChunkAllocator.pool[USER]);
    customGCChunkAllocator.pool[SYSTEM] = NULL;
    customGCChunkAllocator.pool[USER] = NULL;
}

END_TEST(testGCChunkAlloc)
