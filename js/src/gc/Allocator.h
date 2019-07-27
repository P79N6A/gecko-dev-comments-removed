





#ifndef gc_Allocator_h
#define gc_Allocator_h

#include "gc/Heap.h"
#include "js/RootingAPI.h"

namespace JS {
class Symbol;
} 
class JSExternalString;
class JSFatInlineString;
class JSObject;
class JSScript;
class JSString;

namespace js {
struct Class;
class BaseShape;
class LazyScript;
class ObjectGroup;
class Shape;
namespace jit {
class JitCode;
} 

template <typename, AllowGC allowGC = CanGC>
JSObject *
Allocate(ExclusiveContext *cx, gc::AllocKind kind, size_t nDynamicSlots, gc::InitialHeap heap,
         const Class *clasp);

template <typename T, AllowGC allowGC = CanGC>
T *
Allocate(ExclusiveContext *cx);

namespace gc {

template <AllowGC allowGC>
NativeObject *
AllocateObjectForCacheHit(JSContext *cx, AllocKind kind, InitialHeap heap, const Class *clasp);

} 
} 

#endif 
