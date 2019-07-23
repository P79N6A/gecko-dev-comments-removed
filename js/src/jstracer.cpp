





































#define jstracer_cpp___

#include "jsinterp.cpp"

uint32
js_AllocateLoopTableSlots(JSContext* cx, uint32 nloops)
{
    jsword* cursorp = &cx->runtime->loopTableCursor;
    jsword cursor, fencepost;

    do {
        cursor = *cursorp;
        fencepost = cursor + nloops;
        if (fencepost > LOOP_TABLE_LIMIT) {
            
            return LOOP_TABLE_NO_SLOT;
        }
    } while (!js_CompareAndSwap(cursorp, cursor, fencepost));
    return (uint32) cursor;
}

void
js_FreeLoopTableSlots(JSContext* cx, uint32 base, uint32 nloops)
{
    jsword* cursorp = &cx->runtime->loopTableCursor;
    jsword cursor;

    cursor = *cursorp;
    JS_ASSERT(cursor >= (jsword) (base + nloops));

    if ((uint32) cursor == base + nloops &&
        js_CompareAndSwap(cursorp, cursor, base)) {
        return;
    }

    
}











bool
js_GrowLoopTable(JSContext* cx, uint32 slot)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    uint32 oldSize = tm->loopTableSize;

    if (slot >= oldSize) {
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
