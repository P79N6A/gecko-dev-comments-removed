





#ifndef gc_Marking_h
#define gc_Marking_h

#include "gc/Barrier.h"

namespace js {





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
TraceGenericPointerRoot(JSTracer* trc, Cell** thingp, const char* name);

void
TraceManuallyBarrieredGenericPointerEdge(JSTracer* trc, Cell** thingp, const char* name);



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
