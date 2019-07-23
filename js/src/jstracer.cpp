





































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
    JS_ASSERT(ok && !JSVAL_IS_PRIMITIVE(result));
    return tm->recorder = JSVAL_TO_OBJECT(result);
}

void
js_CallRecorder(JSContext* cx, const char* name, uintN argc, jsval* argv)
{
    jsval rval;
    JSBool ok =
    JS_CallFunctionName(cx, js_GetRecorder(cx), name, argc, argv, &rval);
    if (!ok) {
#ifdef DEBUG
        printf("recorder: unsupported instruction '%s'\n", name);
#endif        
        js_TriggerRecorderError(cx);
    }        
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

void
js_CallRecorder(JSContext* cx, const char* name, jsval a, jsval b, jsval c)
{
    jsval args[] = { a, b, c };
    js_CallRecorder(cx, name, 3, args);
}

void
js_CallRecorder(JSContext* cx, const char* name, jsval a, jsval b, jsval c, jsval d)
{
    jsval args[] = { a, b, c, d };
    js_CallRecorder(cx, name, 4, args);
}

void
js_TriggerRecorderError(JSContext* cx)
{
    jsval error = JSVAL_TRUE;
    JS_SetProperty(cx, js_GetRecorder(cx), "error", &error); 
}

bool 
js_GetRecorderError(JSContext* cx)
{
    jsval error;
    bool ok = JS_GetProperty(cx, js_GetRecorder(cx), "error", &error);
    return ok && (error != JSVAL_FALSE);
}
