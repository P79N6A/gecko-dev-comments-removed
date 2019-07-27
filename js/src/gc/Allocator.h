





#ifndef gc_Allocator_h
#define gc_Allocator_h

#include "gc/Heap.h"
#include "js/RootingAPI.h"

namespace js {
struct Class;

template <typename, AllowGC allowGC = CanGC>
JSObject *
Allocate(ExclusiveContext *cx, gc::AllocKind kind, size_t nDynamicSlots, gc::InitialHeap heap,
         const Class *clasp);

template <typename T, AllowGC allowGC = CanGC>
T *
Allocate(ExclusiveContext *cx);

} 

#endif 
