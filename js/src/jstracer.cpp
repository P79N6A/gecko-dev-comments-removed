





































#define jstracer_cpp___

#include "jsinterp.cpp"

JSBool
js_InitTracer(JSRuntime *rt)
{
#ifdef JS_THREADSAFE    
    JSTraceMonitor *tm = &rt->traceMonitor;
    JS_ASSERT(!tm->lock);
    tm->lock = JS_NEW_LOCK();
    if (!tm->lock)
        return JS_FALSE;
#endif    
    return JS_TRUE;
}











void
js_GrowLoopTableIfNeeded(JSRuntime* rt, uint32 index)
{
    JSTraceMonitor *tm = &rt->traceMonitor;
    JS_ACQUIRE_LOCK(&tm->lock);
    uint32 oldSize;
    if (index >= (oldSize = tm->loopTableSize)) {
        uint32 newSize = oldSize << 1;
        jsval* t = tm->loopTable;
        if (t == NULL) {
            JS_ASSERT(oldSize == 0);
            newSize = 256;
            t = (jsval*)malloc(newSize * sizeof(jsval));
        } else 
            t = (jsval*)realloc(tm->loopTable, newSize * sizeof(jsval));
        for (uint32 n = oldSize; n < newSize; ++n)
            t[n] = JSVAL_ZERO;
        tm->loopTable = t;
        tm->loopTableSize = newSize;
    }
    JS_RELEASE_LOCK(&tm->lock);
}
