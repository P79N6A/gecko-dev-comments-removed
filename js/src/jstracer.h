





































#ifndef jstracer_h___
#define jstracer_h___

#include "jsstddef.h"
#include "jslock.h"











struct JSTraceMonitor {
    jsval*      loopTable;
    uint32      loopTableSize;
    JSObject*   recorder;
};

#define TRACE_THRESHOLD 10

uint32 js_AllocateLoopTableSlots(JSContext* cx, uint32 nloops);
void   js_FreeLoopTableSlots(JSContext* cx, uint32 base, uint32 nloops);
bool   js_GrowLoopTable(JSContext* cx, uint32 slot);
jsval  js_CallRecorder(JSContext* cx, const char* fn, uintN argc, jsval* argv);






static inline jsval
native_pointer_to_jsval(void* p)
{
    JS_ASSERT(INT_FITS_IN_JSVAL((int)p));
    return INT_TO_JSVAL((int)p);
}

#endif 
