





































#define jstracer_cpp___

#include "jsinterp.cpp"

uint32
js_AllocateLoopTableSlot(JSRuntime* rt)
{
    uint32 slot = JS_ATOMIC_INCREMENT(&rt->loopTableSlotGen);
    JS_ASSERT(slot != 0);
    return slot - 1;
}

void
js_FreeLoopTableSlot(JSRuntime* rt, uint32 slot)
{
}











JSBool
js_GrowLoopTable(JSContext* cx, uint32 index)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    uint32 oldSize = tm->loopTableSize;

    if (index >= oldSize) {
        uint32 newSize = oldSize << 1;
        jsval* t = tm->loopTable;
        if (t) {
            t = (jsval *) JS_realloc(cx, t, newSize * sizeof(jsval));
        } else {
            JS_ASSERT(oldSize == 0);
            newSize = 256;
            t = (jsval *) JS_malloc(cx, newSize * sizeof(jsval));
        }
        if (!t)
            return JS_FALSE;
        for (uint32 n = oldSize; n < newSize; ++n)
            t[n] = JSVAL_ZERO;
        tm->loopTable = t;
        tm->loopTableSize = newSize;
    }
    return JS_TRUE;
}

JSObject*
js_GetRecorder(JSContext* cx) 
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    if (tm->recorder)
        return tm->recorder;
    JSScript* script = JS_CompileFile(cx, JS_GetGlobalObject(cx), "recorder.js");
    JS_ASSERT(script != NULL);
    jsval result;
    JSBool ok = JS_ExecuteScript(cx, JS_GetGlobalObject(cx), script, &result);
    JS_ASSERT(ok && JSVAL_IS_OBJECT(result));
    return tm->recorder = JSVAL_TO_OBJECT(result);
}

jsval
js_CallRecorder(JSContext* cx, const char* fn, uintN argc, jsval* argv)
{
    jsval rval;
    JSBool ok;
    ok = JS_CallFunctionName(cx, js_GetRecorder(cx), fn, argc, argv, &rval);
    JS_ASSERT(ok);
    return rval;
}