





































#ifndef jstracer_h___
#define jstracer_h___

#include "jsstddef.h"
#include "jslock.h"













struct JSTraceMonitor {
    jsval               *loopTable;
    uint32              loopTableSize;
    JSObject            *recorder;
};

#define TRACE_THRESHOLD 10

uint32 js_AllocateLoopTableSlot(JSRuntime *rt);
void   js_FreeLoopTableSlot(JSRuntime *rt, uint32 slot);
JSBool js_GrowLoopTable(JSContext *cx, uint32 index);
jsval  js_CallRecorder(JSContext* cx, const char* fn, uintN argc, jsval* argv);






static inline jsval
native_pointer_to_jsval(void* p) {
    JS_ASSERT(INT_FITS_IN_JSVAL((int)p));
    return INT_TO_JSVAL((int)p);
}

#endif 
