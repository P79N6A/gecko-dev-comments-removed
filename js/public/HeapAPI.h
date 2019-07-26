





#ifndef js_heap_api_h___
#define js_heap_api_h___

#include "gc/Heap.h"

namespace js {

static inline JSCompartment *
GetGCThingCompartment(void *thing)
{
    JS_ASSERT(thing);
    return reinterpret_cast<gc::Cell *>(thing)->compartment();
}

static inline JSCompartment *
GetObjectCompartment(JSObject *obj)
{
    return GetGCThingCompartment(obj);
}

} 

#endif 
