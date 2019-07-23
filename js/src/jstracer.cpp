





































#define jstracer_cpp___

#include "jsinterp.cpp"

JSBool
js_InitTracer(JSRuntime *rt)
{
    return JS_TRUE;
}











void
js_GrowLoopTableIfNeeded(JSRuntime* rt, uint32 index)
{
    JSTraceMonitor *tm = &rt->traceMonitor;
    uint32 oldSize = tm->loopTableSize;

    if (index >= oldSize) {
        uint32 newSize = oldSize << 1;
        jsval* t = tm->loopTable;
        if (t == NULL) {
            JS_ASSERT(oldSize == 0);
            newSize = 256;
            t = (jsval*)malloc(newSize * sizeof(jsval));
        } else {
            t = (jsval*)realloc(tm->loopTable, newSize * sizeof(jsval));
        }
        for (uint32 n = oldSize; n < newSize; ++n)
            t[n] = JSVAL_ZERO;
        tm->loopTable = t;
        tm->loopTableSize = newSize;
    }
}
