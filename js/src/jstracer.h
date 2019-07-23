





































#ifndef jstracer_h___
#define jstracer_h___

#include "jsstddef.h"
#include "jslock.h"











struct JSTraceMonitor {
    int         freq;
    JSObject*   recorder;
};

#define TRACE_TRIGGER_MASK 0x3f

void js_CallRecorder(JSContext* cx, const char* fn, uintN argc, jsval* argv);
void js_CallRecorder(JSContext* cx, const char* fn, jsval a);
void js_CallRecorder(JSContext* cx, const char* fn, jsval a, jsval b);

bool js_GetRecorderError(JSContext* cx);





static inline jsval
native_pointer_to_jsval(void* p)
{
    return INT_TO_JSVAL(((uint32)p) >> 2);
}

#endif 
