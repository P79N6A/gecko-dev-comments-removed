





































#define jstracer_cpp___

#include "jsinterp.cpp"

JSBool
js_InitTracer(JSRuntime *rt) {
#ifdef JS_THREADSAFE    
    JSTraceMonitor *tm = &rt->traceMonitor;
    JS_ASSERT(!tm->lock);
    tm->lock = JS_NEW_LOCK();
    if (!tm->lock)
        goto bad;
    return JS_TRUE;
bad:
    return JS_FALSE;
#else
    return JS_TRUE;
#endif    
}







uint32 js_AllocateLoopTableSlot(JSRuntime *rt) {
    uint32 slot = JS_ATOMIC_INCREMENT(&rt->loopTableIndexGen);
    JS_ASSERT(slot < JS_BITMASK(24));
    return slot;
}











void
js_GrowLoopTableIfNeeded(JSContext *cx, uint32 index) {
    JSTraceMonitor *tm = &JS_TRACE_MONITOR(cx);
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
