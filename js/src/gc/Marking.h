





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
TraceEdge(JSTracer *trc, BarrieredBase<T> *thingp, const char *name);




template <typename T>
void
TraceRoot(JSTracer *trc, T *thingp, const char *name);




template <typename T>
void
TraceManuallyBarrieredEdge(JSTracer *trc, T *thingp, const char *name);


template <typename T>
void
TraceRange(JSTracer *trc, size_t len, BarrieredBase<T> *thingp, const char *name);


template <typename T>
void
TraceRootRange(JSTracer *trc, size_t len, T *thingp, const char *name);



template <typename T>
void
TraceCrossCompartmentEdge(JSTracer *trc, JSObject *src, BarrieredBase<T> *dst,
                          const char *name);


template <typename T>
void
TraceManuallyBarrieredCrossCompartmentEdge(JSTracer *trc, JSObject *src, T *dst,
                                           const char *name);

namespace gc {















































#define DeclMarker(base, type)                                                                    \
void Mark##base(JSTracer *trc, BarrieredBase<type*> *thing, const char *name);                    \
void Mark##base##Root(JSTracer *trc, type **thingp, const char *name);                            \
void Mark##base##Unbarriered(JSTracer *trc, type **thingp, const char *name);                     \
void Mark##base##Range(JSTracer *trc, size_t len, HeapPtr<type*> *thing, const char *name);       \
void Mark##base##RootRange(JSTracer *trc, size_t len, type **thing, const char *name);            \
bool Is##base##Marked(type **thingp);                                                             \
bool Is##base##Marked(BarrieredBase<type*> *thingp);                                              \
bool Is##base##MarkedFromAnyThread(type **thingp);                                                \
bool Is##base##MarkedFromAnyThread(BarrieredBase<type*> *thingp);                                 \
bool Is##base##AboutToBeFinalized(type **thingp);                                                 \
bool Is##base##AboutToBeFinalizedFromAnyThread(type **thingp);                                    \
bool Is##base##AboutToBeFinalized(BarrieredBase<type*> *thingp);                                  \
type *Update##base##IfRelocated(JSRuntime *rt, BarrieredBase<type*> *thingp);                     \
type *Update##base##IfRelocated(JSRuntime *rt, type **thingp);

DeclMarker(BaseShape, BaseShape)
DeclMarker(BaseShape, UnownedBaseShape)
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
MarkPermanentAtom(JSTracer *trc, JSAtom *atom, const char *name);

void
MarkWellKnownSymbol(JSTracer *trc, JS::Symbol *sym);




MOZ_ALWAYS_INLINE bool
IsNullTaggedPointer(void *p)
{
    return uintptr_t(p) < 32;
}








void
MarkKind(JSTracer *trc, void **thingp, JSGCTraceKind kind);

void
MarkGCThingRoot(JSTracer *trc, void **thingp, const char *name);

void
MarkGCThingUnbarriered(JSTracer *trc, void **thingp, const char *name);



void
MarkId(JSTracer *trc, BarrieredBase<jsid> *id, const char *name);

void
MarkIdRoot(JSTracer *trc, jsid *id, const char *name);

void
MarkIdUnbarriered(JSTracer *trc, jsid *id, const char *name);

void
MarkIdRange(JSTracer *trc, size_t len, HeapId *vec, const char *name);

void
MarkIdRootRange(JSTracer *trc, size_t len, jsid *vec, const char *name);



bool
IsValueMarked(Value *v);

bool
IsValueAboutToBeFinalized(Value *v);

bool
IsValueAboutToBeFinalizedFromAnyThread(Value *v);



bool
IsSlotMarked(HeapSlot *s);

void
MarkObjectSlots(JSTracer *trc, NativeObject *obj, uint32_t start, uint32_t nslots);








void
MarkCycleCollectorChildren(JSTracer *trc, Shape *shape);

void
PushArena(GCMarker *gcmarker, ArenaHeader *aheader);



inline bool
IsMarked(BarrieredBase<Value> *v)
{
    if (!v->isMarkable())
        return true;
    return IsValueMarked(v->unsafeGet());
}

inline bool
IsMarked(BarrieredBase<JSObject*> *objp)
{
    return IsObjectMarked(objp);
}

inline bool
IsMarked(BarrieredBase<JSScript*> *scriptp)
{
    return IsScriptMarked(scriptp);
}

inline bool
IsAboutToBeFinalized(BarrieredBase<Value> *v)
{
    if (!v->isMarkable())
        return false;
    return IsValueAboutToBeFinalized(v->unsafeGet());
}

inline bool
IsAboutToBeFinalized(BarrieredBase<JSObject*> *objp)
{
    return IsObjectAboutToBeFinalized(objp);
}

inline bool
IsAboutToBeFinalized(BarrieredBase<JSScript*> *scriptp)
{
    return IsScriptAboutToBeFinalized(scriptp);
}

inline bool
IsAboutToBeFinalized(const js::jit::VMFunction **vmfunc)
{
    



    return false;
}

inline bool
IsAboutToBeFinalized(ReadBarrieredJitCode code)
{
    return IsJitCodeAboutToBeFinalized(code.unsafeGet());
}

inline Cell *
ToMarkable(const Value &v)
{
    if (v.isMarkable())
        return (Cell *)v.toGCThing();
    return nullptr;
}

inline Cell *
ToMarkable(Cell *cell)
{
    return cell;
}





template <typename Map, typename Key>
class HashKeyRef : public BufferableRef
{
    Map *map;
    Key key;

  public:
    HashKeyRef(Map *m, const Key &k) : map(m), key(k) {}

    void mark(JSTracer *trc) {
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
TraceChildren(JSTracer *trc, void *thing, JSGCTraceKind kind);

bool
UnmarkGrayShapeRecursively(Shape *shape);

} 

#endif 
