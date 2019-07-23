





































#ifndef jstracer_h___
#define jstracer_h___

#include "jsstddef.h"
#include "jslock.h"













struct JSTraceMonitor {
    jsval               *loopTable;
    uint32              loopTableSize;
};

#define TRACE_THRESHOLD 10

uint32 js_AllocateLoopTableSlot(JSRuntime *rt);
void   js_FreeLoopTableSlot(JSRuntime *rt, uint32 slot);
JSBool js_GrowLoopTable(JSContext *cx, uint32 index);

#endif 
