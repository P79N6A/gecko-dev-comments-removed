





#ifndef jsgcinlines_h___
#define jsgcinlines_h___

#include "jsgc.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jslock.h"

#include "gc/Root.h"
#include "js/TemplateLib.h"
#include "vm/Shape.h"

namespace js {

class Shape;






struct AutoMarkInDeadZone
{
    AutoMarkInDeadZone(JS::Zone *zone)
      : zone(zone),
        scheduled(zone->scheduledForDestruction)
    {
        if (zone->rt->gcManipulatingDeadZones && zone->scheduledForDestruction) {
            zone->rt->gcObjectsMarkedInDeadZones++;
            zone->scheduledForDestruction = false;
        }
    }

    ~AutoMarkInDeadZone() {
        zone->scheduledForDestruction = scheduled;
    }

  private:
    JS::Zone *zone;
    bool scheduled;
};

namespace gc {


const size_t SLOTS_TO_THING_KIND_LIMIT = 17;

extern AllocKind slotsToThingKind[];


static inline AllocKind
GetGCObjectKind(size_t numSlots)
{
    if (numSlots >= SLOTS_TO_THING_KIND_LIMIT)
        return FINALIZE_OBJECT16;
    return slotsToThingKind[numSlots];
}

static inline AllocKind
GetGCObjectKind(Class *clasp)
{
    if (clasp == &FunctionClass)
        return JSFunction::FinalizeKind;
    uint32_t nslots = JSCLASS_RESERVED_SLOTS(clasp);
    if (clasp->flags & JSCLASS_HAS_PRIVATE)
        nslots++;
    return GetGCObjectKind(nslots);
}


static inline AllocKind
GetGCArrayKind(size_t numSlots)
{
    extern AllocKind slotsToThingKind[];

    





    JS_STATIC_ASSERT(ObjectElements::VALUES_PER_HEADER == 2);
    if (numSlots > JSObject::NELEMENTS_LIMIT || numSlots + 2 >= SLOTS_TO_THING_KIND_LIMIT)
        return FINALIZE_OBJECT2;
    return slotsToThingKind[numSlots + 2];
}

static inline AllocKind
GetGCObjectFixedSlotsKind(size_t numFixedSlots)
{
    extern AllocKind slotsToThingKind[];

    JS_ASSERT(numFixedSlots < SLOTS_TO_THING_KIND_LIMIT);
    return slotsToThingKind[numFixedSlots];
}

static inline AllocKind
GetBackgroundAllocKind(AllocKind kind)
{
    JS_ASSERT(!IsBackgroundFinalized(kind));
    JS_ASSERT(kind <= FINALIZE_OBJECT_LAST);
    return (AllocKind) (kind + 1);
}





static inline bool
TryIncrementAllocKind(AllocKind *kindp)
{
    size_t next = size_t(*kindp) + 2;
    if (next >= size_t(FINALIZE_OBJECT_LIMIT))
        return false;
    *kindp = AllocKind(next);
    return true;
}


static inline size_t
GetGCKindSlots(AllocKind thingKind)
{
    
    switch (thingKind) {
      case FINALIZE_OBJECT0:
      case FINALIZE_OBJECT0_BACKGROUND:
        return 0;
      case FINALIZE_OBJECT2:
      case FINALIZE_OBJECT2_BACKGROUND:
        return 2;
      case FINALIZE_OBJECT4:
      case FINALIZE_OBJECT4_BACKGROUND:
        return 4;
      case FINALIZE_OBJECT8:
      case FINALIZE_OBJECT8_BACKGROUND:
        return 8;
      case FINALIZE_OBJECT12:
      case FINALIZE_OBJECT12_BACKGROUND:
        return 12;
      case FINALIZE_OBJECT16:
      case FINALIZE_OBJECT16_BACKGROUND:
        return 16;
      default:
        JS_NOT_REACHED("Bad object finalize kind");
        return 0;
    }
}

static inline size_t
GetGCKindSlots(AllocKind thingKind, Class *clasp)
{
    size_t nslots = GetGCKindSlots(thingKind);

    
    if (clasp->flags & JSCLASS_HAS_PRIVATE) {
        JS_ASSERT(nslots > 0);
        nslots--;
    }

    



    if (clasp == &FunctionClass)
        nslots = 0;

    return nslots;
}

#ifdef JSGC_GENERATIONAL
template <typename NurseryType>
inline bool
ShouldNurseryAllocate(const NurseryType &nursery, AllocKind kind, InitialHeap heap)
{
    return nursery.isEnabled() && IsNurseryAllocable(kind) && heap != TenuredHeap;
}
#endif

inline bool
IsInsideNursery(JSRuntime *rt, const void *thing)
{
#ifdef JSGC_GENERATIONAL
#if JS_GC_ZEAL
    if (rt->gcVerifyPostData)
        return rt->gcVerifierNursery.isInside(thing);
#endif
#endif
    return false;
}

inline JSGCTraceKind
GetGCThingTraceKind(const void *thing)
{
    JS_ASSERT(thing);
    const Cell *cell = static_cast<const Cell *>(thing);
#ifdef JSGC_GENERATIONAL
    if (IsInsideNursery(cell->runtime(), cell))
        return JSTRACE_OBJECT;
#endif
    return MapAllocToTraceKind(cell->getAllocKind());
}

static inline void
GCPoke(JSRuntime *rt)
{
    rt->gcPoke = true;

#ifdef JS_GC_ZEAL
    
    if (rt->gcZeal() == js::gc::ZealPokeValue)
        rt->gcNextScheduled = 1;
#endif
}

class ArenaIter
{
    ArenaHeader *aheader;
    ArenaHeader *remainingHeader;

  public:
    ArenaIter() {
        init();
    }

    ArenaIter(JS::Zone *zone, AllocKind kind) {
        init(zone, kind);
    }

    void init() {
        aheader = NULL;
        remainingHeader = NULL;
    }

    void init(ArenaHeader *aheaderArg) {
        aheader = aheaderArg;
        remainingHeader = NULL;
    }

    void init(JS::Zone *zone, AllocKind kind) {
        aheader = zone->allocator.arenas.getFirstArena(kind);
        remainingHeader = zone->allocator.arenas.getFirstArenaToSweep(kind);
        if (!aheader) {
            aheader = remainingHeader;
            remainingHeader = NULL;
        }
    }

    bool done() {
        return !aheader;
    }

    ArenaHeader *get() {
        return aheader;
    }

    void next() {
        aheader = aheader->next;
        if (!aheader) {
            aheader = remainingHeader;
            remainingHeader = NULL;
        }
    }
};

class CellIterImpl
{
    size_t firstThingOffset;
    size_t thingSize;
    ArenaIter aiter;
    FreeSpan firstSpan;
    const FreeSpan *span;
    uintptr_t thing;
    Cell *cell;

  protected:
    CellIterImpl() {
    }

    void initSpan(JS::Zone *zone, AllocKind kind) {
        JS_ASSERT(zone->allocator.arenas.isSynchronizedFreeList(kind));
        firstThingOffset = Arena::firstThingOffset(kind);
        thingSize = Arena::thingSize(kind);
        firstSpan.initAsEmpty();
        span = &firstSpan;
        thing = span->first;
    }

    void init(ArenaHeader *singleAheader) {
        initSpan(singleAheader->zone, singleAheader->getAllocKind());
        aiter.init(singleAheader);
        next();
        aiter.init();
    }

    void init(JSCompartment *comp, AllocKind kind) {
        initSpan(comp->zone(), kind);
        aiter.init(comp->zone(), kind);
        next();
    }

  public:
    bool done() const {
        return !cell;
    }

    template<typename T> T *get() const {
        JS_ASSERT(!done());
        return static_cast<T *>(cell);
    }

    Cell *getCell() const {
        JS_ASSERT(!done());
        return cell;
    }

    void next() {
        for (;;) {
            if (thing != span->first)
                break;
            if (JS_LIKELY(span->hasNext())) {
                thing = span->last + thingSize;
                span = span->nextSpan();
                break;
            }
            if (aiter.done()) {
                cell = NULL;
                return;
            }
            ArenaHeader *aheader = aiter.get();
            firstSpan = aheader->getFirstFreeSpan();
            span = &firstSpan;
            thing = aheader->arenaAddress() | firstThingOffset;
            aiter.next();
        }
        cell = reinterpret_cast<Cell *>(thing);
        thing += thingSize;
    }
};

class CellIterUnderGC : public CellIterImpl
{
  public:
    CellIterUnderGC(JSCompartment *comp, AllocKind kind) {
        JS_ASSERT(comp->rt->isHeapBusy());
        init(comp, kind);
    }

    CellIterUnderGC(ArenaHeader *aheader) {
        JS_ASSERT(aheader->zone->rt->isHeapBusy());
        init(aheader);
    }
};

class CellIter : public CellIterImpl
{
    ArenaLists *lists;
    AllocKind kind;
#ifdef DEBUG
    size_t *counter;
#endif
  public:
    CellIter(JSCompartment *comp, AllocKind kind)
      : lists(&comp->zone()->allocator.arenas),
        kind(kind)
    {

        





        if (IsBackgroundFinalized(kind) &&
            comp->zone()->allocator.arenas.needBackgroundFinalizeWait(kind))
        {
            gc::FinishBackgroundFinalize(comp->rt);
        }
        if (lists->isSynchronizedFreeList(kind)) {
            lists = NULL;
        } else {
            JS_ASSERT(!comp->rt->isHeapBusy());
            lists->copyFreeListToArena(kind);
        }
#ifdef DEBUG
        counter = &comp->rt->noGCOrAllocationCheck;
        ++*counter;
#endif
        init(comp, kind);
    }

    ~CellIter() {
#ifdef DEBUG
        JS_ASSERT(*counter > 0);
        --*counter;
#endif
        if (lists)
            lists->clearFreeListInArena(kind);
    }
};





template <class ArenaOp, class CellOp>
void
ForEachArenaAndCell(JSCompartment *compartment, AllocKind thingKind,
                    ArenaOp arenaOp, CellOp cellOp)
{
    for (ArenaIter aiter(compartment, thingKind); !aiter.done(); aiter.next()) {
        ArenaHeader *aheader = aiter.get();
        arenaOp(aheader->getArena());
        for (CellIterUnderGC iter(aheader); !iter.done(); iter.next())
            cellOp(iter.getCell());
    }
}



inline void EmptyArenaOp(Arena *arena) {}
inline void EmptyCellOp(Cell *t) {}

class GCCompartmentsIter {
  private:
    JSCompartment **it, **end;

  public:
    GCCompartmentsIter(JSRuntime *rt) {
        JS_ASSERT(rt->isHeapBusy());
        it = rt->compartments.begin();
        end = rt->compartments.end();
        if (!(*it)->isCollecting())
            next();
    }

    bool done() const { return it == end; }

    void next() {
        JS_ASSERT(!done());
        do {
            it++;
        } while (it != end && !(*it)->isCollecting());
    }

    JSCompartment *get() const {
        JS_ASSERT(!done());
        return *it;
    }

    operator JSCompartment *() const { return get(); }
    JSCompartment *operator->() const { return get(); }
};

typedef GCCompartmentsIter GCZonesIter;


class GCCompartmentGroupIter {
  private:
    JSCompartment *current;

  public:
    GCCompartmentGroupIter(JSRuntime *rt) {
        JS_ASSERT(rt->isHeapBusy());
        current = rt->gcCurrentZoneGroup;
    }

    bool done() const { return !current; }

    void next() {
        JS_ASSERT(!done());
        current = current->nextNodeInGroup();
    }

    JSCompartment *get() const {
        JS_ASSERT(!done());
        return current;
    }

    operator JSCompartment *() const { return get(); }
    JSCompartment *operator->() const { return get(); }
};

typedef GCCompartmentGroupIter GCZoneGroupIter;








template <typename T, AllowGC allowGC>
inline T *
NewGCThing(JSContext *cx, AllocKind kind, size_t thingSize, InitialHeap heap)
{
    JS_ASSERT(thingSize == js::gc::Arena::thingSize(kind));
    JS_ASSERT_IF(cx->compartment == cx->runtime->atomsCompartment,
                 kind == FINALIZE_STRING ||
                 kind == FINALIZE_SHORT_STRING ||
                 kind == FINALIZE_IONCODE);
    JS_ASSERT(!cx->runtime->isHeapBusy());
    JS_ASSERT(!cx->runtime->noGCOrAllocationCheck);

    
    JS_OOM_POSSIBLY_FAIL_REPORT(cx);

#ifdef JS_GC_ZEAL
    if (cx->runtime->needZealousGC() && allowGC)
        js::gc::RunDebugGC(cx);
#endif

    if (allowGC)
        MaybeCheckStackRoots(cx,  false);

    JS::Zone *zone = cx->zone();
    T *t = static_cast<T *>(zone->allocator.arenas.allocateFromFreeList(kind, thingSize));
    if (!t)
        t = static_cast<T *>(js::gc::ArenaLists::refillFreeList<allowGC>(cx, kind));

    JS_ASSERT_IF(t && zone->wasGCStarted() && (zone->isGCMarking() || zone->isGCSweeping()),
                 t->arenaHeader()->allocatedDuringIncremental);

#if defined(JSGC_GENERATIONAL) && defined(JS_GC_ZEAL)
    if (cx->runtime->gcVerifyPostData &&
        ShouldNurseryAllocate(cx->runtime->gcVerifierNursery, kind, heap))
    {
        JS_ASSERT(!IsAtomsCompartment(cx->compartment));
        cx->runtime->gcVerifierNursery.insertPointer(t);
    }
#endif

    return t;
}







class AutoSuppressGC
{
    int32_t &suppressGC_;

  public:
    AutoSuppressGC(JSContext *cx)
      : suppressGC_(cx->runtime->mainThread.suppressGC)
    {
        suppressGC_++;
    }

    AutoSuppressGC(JSCompartment *comp)
      : suppressGC_(comp->rt->mainThread.suppressGC)
    {
        suppressGC_++;
    }

    ~AutoSuppressGC()
    {
        suppressGC_--;
    }
};

} 
} 

template <js::AllowGC allowGC>
inline JSObject *
js_NewGCObject(JSContext *cx, js::gc::AllocKind kind, js::gc::InitialHeap heap)
{
    JS_ASSERT(kind >= js::gc::FINALIZE_OBJECT0 && kind <= js::gc::FINALIZE_OBJECT_LAST);
    return js::gc::NewGCThing<JSObject, allowGC>(cx, kind, js::gc::Arena::thingSize(kind), heap);
}

template <js::AllowGC allowGC>
inline JSString *
js_NewGCString(JSContext *cx)
{
    return js::gc::NewGCThing<JSString, allowGC>(cx, js::gc::FINALIZE_STRING,
                                                 sizeof(JSString), js::gc::TenuredHeap);
}

template <js::AllowGC allowGC>
inline JSShortString *
js_NewGCShortString(JSContext *cx)
{
    return js::gc::NewGCThing<JSShortString, allowGC>(cx, js::gc::FINALIZE_SHORT_STRING,
                                                      sizeof(JSShortString), js::gc::TenuredHeap);
}

inline JSExternalString *
js_NewGCExternalString(JSContext *cx)
{
    return js::gc::NewGCThing<JSExternalString, js::CanGC>(cx, js::gc::FINALIZE_EXTERNAL_STRING,
                                                           sizeof(JSExternalString), js::gc::TenuredHeap);
}

inline JSScript *
js_NewGCScript(JSContext *cx)
{
    return js::gc::NewGCThing<JSScript, js::CanGC>(cx, js::gc::FINALIZE_SCRIPT,
                                                   sizeof(JSScript), js::gc::TenuredHeap);
}

inline js::RawShape
js_NewGCShape(JSContext *cx)
{
    return js::gc::NewGCThing<js::Shape, js::CanGC>(cx, js::gc::FINALIZE_SHAPE,
                                                    sizeof(js::Shape), js::gc::TenuredHeap);
}

template <js::AllowGC allowGC>
inline js::RawBaseShape
js_NewGCBaseShape(JSContext *cx)
{
    return js::gc::NewGCThing<js::BaseShape, allowGC>(cx, js::gc::FINALIZE_BASE_SHAPE,
                                                      sizeof(js::BaseShape), js::gc::TenuredHeap);
}

#endif 
