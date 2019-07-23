





































#ifndef jstracer_h___
#define jstracer_h___

#include "jsstddef.h"
#include "jslock.h"













struct JSTraceMonitor {
    jsval*      loopTable;
    uint32      loopTableSize;
};

#define TRACE_THRESHOLD 10

bool js_InitTracer(JSRuntime* rt);
bool js_AllocateLoopTableSlots(JSContext* cx, uint32 nloops, uint32 *basep);
void js_FreeLoopTableSlots(JSContext* cx, uint32 base, uint32 nloops);
bool js_GrowLoopTable(JSContext* cx, uint32 index);

#endif 
