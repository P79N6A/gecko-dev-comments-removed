





































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

#ifdef DEBUG
    JSBool ok =
#endif
    JS_ExecuteScript(cx, JS_GetGlobalObject(cx), script, &result);
    JS_ASSERT(ok && JSVAL_IS_OBJECT(result));
    return tm->recorder = JSVAL_TO_OBJECT(result);
}

void
js_CallRecorder(JSContext* cx, const char* fn, uintN argc, jsval* argv)
{
    jsval rval;
#ifdef DEBUG
    JSBool ok =
#endif
    JS_CallFunctionName(cx, js_GetRecorder(cx), fn, argc, argv, &rval);
    JS_ASSERT(ok);
}

void
js_CallRecorder(JSContext* cx, const char* name, jsval a)
{
    jsval args[] = { a };
    js_CallRecorder(cx, name, 1, args);
}

void
js_CallRecorder(JSContext* cx, const char* name, jsval a, jsval b)
{
    jsval args[] = { a, b };
    js_CallRecorder(cx, name, 2, args);
}

bool 
js_GetRecorderError(JSContext* cx)
{
    jsval error;
    return (JS_GetProperty(cx, JS_TRACE_MONITOR(cx).recorder, 
                           "error", &error) != true) 
           || (error != JSVAL_FALSE);
}
