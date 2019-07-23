





































#define jstracer_cpp___

#include "jsinterp.cpp"

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
