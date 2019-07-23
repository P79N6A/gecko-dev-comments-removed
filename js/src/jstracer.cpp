





































#define jstracer_cpp___

#include "jsinterp.cpp"

bool
js_InitTracer(JSRuntime* rt)
{
    return true;
}

bool
js_AllocateLoopTableSlots(JSContext* cx, uint32 nloops, uint32 *basep)
{
    return false;
}

void
js_FreeLoopTableSlots(JSContext* cx, uint32 base, uint32 nloops)
{
}











bool
js_GrowLoopTable(JSContext* cx, uint32 index)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    uint32 oldSize = tm->loopTableSize;

    if (index >= oldSize) {
        uint32 newSize = oldSize << 1;
        jsval* t = tm->loopTable;
        if (t) {
            t = (jsval*) JS_realloc(cx, t, newSize * sizeof(jsval));
        } else {
            JS_ASSERT(oldSize == 0);
            newSize = 256;
            t = (jsval*) JS_malloc(cx, newSize * sizeof(jsval));
        }
        if (!t)
            return false;
        for (uint32 n = oldSize; n < newSize; ++n)
            t[n] = JSVAL_ZERO;
        tm->loopTable = t;
        tm->loopTableSize = newSize;
    }
    return true;
}
