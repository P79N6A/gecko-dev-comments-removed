





































#ifndef jstracer_h___
#define jstracer_h___

#include "jsstddef.h"
#include "jslock.h"













struct JSTraceMonitor {
    jsval              *loopTable;
    uint32              loopTableSize;
};

#define TRACE_THRESHOLD 10

JSBool js_InitTracer(JSRuntime *rt);
uint32 js_AllocateLoopTableSlot(JSContext *cx);
JSBool js_GrowLoopTable(JSContext *cx, uint32 index);

#endif 
