





































#ifndef jstracer_h___
#define jstracer_h___

#include "jsstddef.h"
#include "jslock.h"











struct JSTraceMonitor {
    int         freq;
    JSObject*   recorder;
};

#define TRACE_TRIGGER_MASK 0x3f

jsval  js_CallRecorder(JSContext* cx, const char* fn, uintN argc, jsval* argv);






static inline jsval
native_pointer_to_jsval(void* p)
{
    JS_ASSERT(INT_FITS_IN_JSVAL((int)p));
    return INT_TO_JSVAL((int)p);
}

#endif 
