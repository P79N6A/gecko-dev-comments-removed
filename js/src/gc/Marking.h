





#ifndef gc_Marking_h
#define gc_Marking_h

#include "gc/Barrier.h"

class JSAtom;
class JSLinearString;

namespace js {

class ArgumentsObject;
class ArrayBufferObject;
class ArrayBufferViewObject;
class SharedArrayBufferObject;
class BaseShape;
class DebugScopeObject;
class GCMarker;
class GlobalObject;
class LazyScript;
class NestedScopeObject;
class SavedFrame;
class ScopeObject;
class Shape;
class UnownedBaseShape;

template<class> class HeapPtr;

namespace jit {
class JitCode;
struct IonScript;
struct VMFunction;
}





template <typename T>
void
TraceEdge(JSTracer* trc, BarrieredBase<T>* thingp, const char* name);




template <typename T>
void
TraceRoot(JSTracer* trc, T* thingp, const char* name);




template <typename T>
void
TraceManuallyBarrieredEdge(JSTracer* trc, T* thingp, const char* name);


template <typename T>
void
TraceRange(JSTracer* trc, size_t len, BarrieredBase<T>* vec, const char* name);


template <typename T>
void
TraceRootRange(JSTracer* trc, size_t len, T* vec, const char* name);



template <typename T>
void
TraceCrossCompartmentEdge(JSTracer* trc, JSObject* src, BarrieredBase<T>* dst,
                          const char* name);


template <typename T>
void
TraceManuallyBarrieredCrossCompartmentEdge(JSTracer* trc, JSObject* src, T* dst,
                                           const char* name);

namespace gc {















































#define DeclMarker(base, type)                                                                    \
void Mark##base(JSTracer* trc, BarrieredBase<type*>* thing, const char* name);                    \
void Mark##base##Root(JSTracer* trc, type** thingp, const char* name);                            \
void Mark##base##Unbarriered(JSTracer* trc, type** thingp, const char* name);                     \
void Mark##base##Range(JSTracer* trc, size_t len, HeapPtr<type*>* thing, const char* name);       \
void Mark##base##RootRange(JSTracer* trc, size_t len, type** thing, const char* name);            \
bool Is##base##Marked(type** thingp);                                                             \
bool Is##base##Marked(BarrieredBase<type*>* thingp);                                              \
bool Is##base##AboutToBeFinalized(type** thingp);                                                 \
bool Is##base##AboutToBeFinalized(BarrieredBase<type*>* thingp);                                  \
type* Update##base##IfRelocated(JSRuntime* rt, BarrieredBase<type*>* thingp);                     \
type* Update##base##IfRelocated(JSRuntime* rt, type** thingp);

DeclMarker(JitCode, jit::JitCode)
DeclMarker(Object, NativeObject)
DeclMarker(Object, ArrayObject)
DeclMarker(Object, ArgumentsObject)
DeclMarker(Object, ArrayBufferObject)
DeclMarker(Object, ArrayBufferObjectMaybeShared)
DeclMarker(Object, ArrayBufferViewObject)
DeclMarker(Object, DebugScopeObject)
DeclMarker(Object, GlobalObject)
DeclMarker(Object, JSObject)
DeclMarker(Object, JSFunction)
DeclMarker(Object, NestedScopeObject)
DeclMarker(Object, PlainObject)
DeclMarker(Object, SavedFrame)
DeclMarker(Object, ScopeObject)
DeclMarker(Object, SharedArrayBufferObject)
DeclMarker(Object, SharedTypedArrayObject)
DeclMarker(Script, JSScript)
DeclMarker(LazyScript, LazyScript)
DeclMarker(Shape, Shape)
DeclMarker(String, JSAtom)
DeclMarker(String, JSString)
DeclMarker(String, JSFlatString)
DeclMarker(String, JSLinearString)
DeclMarker(String, PropertyName)
DeclMarker(Symbol, JS::Symbol)
DeclMarker(ObjectGroup, ObjectGroup)

#undef DeclMarker

void
MarkPermanentAtom(JSTracer* trc, JSAtom* atom, const char* name);

void
MarkWellKnownSymbol(JSTracer* trc, JS::Symbol* sym);




MOZ_ALWAYS_INLINE bool
IsNullTaggedPointer(void* p)
{
    return uintptr_t(p) < 32;
}








void
MarkKind(JSTracer* trc, void** thingp, JSGCTraceKind kind);

void
MarkGCThingRoot(JSTracer* trc, void** thingp, const char* name);

void
MarkGCThingUnbarriered(JSTracer* trc, void** thingp, const char* name);



void
MarkObjectSlots(JSTracer* trc, NativeObject* obj, uint32_t start, uint32_t nslots);








void
MarkCycleCollectorChildren(JSTracer* trc, Shape* shape);

void
PushArena(GCMarker* gcmarker, ArenaHeader* aheader);



template <typename T>
bool
IsMarkedUnbarriered(T* thingp);

template <typename T>
bool
IsMarked(BarrieredBase<T>* thingp);

template <typename T>
bool
IsMarked(ReadBarriered<T>* thingp);

template <typename T>
bool
IsAboutToBeFinalizedUnbarriered(T* thingp);

template <typename T>
bool
IsAboutToBeFinalized(BarrieredBase<T>* thingp);

template <typename T>
bool
IsAboutToBeFinalized(ReadBarriered<T>* thingp);

inline bool
IsAboutToBeFinalized(const js::jit::VMFunction** vmfunc)
{
    



    return false;
}

inline Cell*
ToMarkable(const Value& v)
{
    if (v.isMarkable())
        return (Cell*)v.toGCThing();
    return nullptr;
}

inline Cell*
ToMarkable(Cell* cell)
{
    return cell;
}





template <typename Map, typename Key>
class HashKeyRef : public BufferableRef
{
    Map* map;
    Key key;

  public:
    HashKeyRef(Map* m, const Key& k) : map(m), key(k) {}

    void mark(JSTracer* trc) {
        Key prior = key;
        typename Map::Ptr p = map->lookup(key);
        if (!p)
            return;
        trc->setTracingLocation(&*p);
        TraceManuallyBarrieredEdge(trc, &key, "HashKeyRef");
        map->rekeyIfMoved(prior, key);
    }
};

} 

void
TraceChildren(JSTracer* trc, void* thing, JSGCTraceKind kind);

bool
UnmarkGrayShapeRecursively(Shape* shape);

} 

#endif 
