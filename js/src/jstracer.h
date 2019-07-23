





































#ifndef jstracer_h__
#define jstracer_h__

#include "jsstddef.h"
#include "jslock.h"













struct JSTraceMonitor {
#ifdef JS_THREADSAFE    
    JSLock             *lock;
#endif    
    jsval              *loopTable;
    uint32              loopTableSize;
};

#define TRACE_THRESHOLD 10

JSBool js_InitTracer(JSRuntime *rt);
uint32 js_AllocateLoopTableSlot(JSRuntime *rt);
void   js_GrowLoopTableIfNeeded(JSContext *cx, uint32 index);

#endif