





































#ifndef jstracer_h___
#define jstracer_h___

#include "jsstddef.h"
#include "jslock.h"











struct JSTraceMonitor {
    int         freq;
    JSObject*   recorder;
    bool        error;
};

#define TRACE_TRIGGER_MASK 0x3f

jsval
js_CallRecorder(JSContext* cx, const char* name, uintN argc, jsval* argv);

jsval
js_CallRecorder(JSContext* cx, const char* name, jsval a);

jsval
js_CallRecorder(JSContext* cx, const char* name, jsval a, jsval b);








static inline jsval
native_pointer_to_jsval(void* p)
{
    return INT_TO_JSVAL(JS_PTR_TO_UINT32(p));
}

#endif 
