





#ifndef jsgcinlines_h
#define jsgcinlines_h

#include "jsgc.h"

#include "gc/GCTrace.h"
#include "gc/Zone.h"
#include "vm/ForkJoin.h"

namespace js {

class Shape;

inline Allocator *
ThreadSafeContext::allocator() const
{
    JS_ASSERT_IF(isJSContext(), &asJSContext()->zone()->allocator == allocator_);
    return allocator_;
}

template <typename T>
inline bool
ThreadSafeContext::isThreadLocal(T thing) const
{
    if (!isForkJoinContext())
        return true;

#ifdef JSGC_FJGENERATIONAL
    ForkJoinContext *cx = static_cast<ForkJoinContext*>(const_cast<ThreadSafeContext*>(this));
    if (cx->nursery().isInsideNewspace(thing))
        return true;
#endif

    
    JS_ASSERT(!IsInsideNursery(thing));

    
    if (allocator_->arenas.containsArena(runtime_, thing->asTenured()->arenaHeader()))
    {
        
        
        JS_ASSERT(!thing->zoneFromAnyThread()->needsIncrementalBarrier());
        JS_ASSERT(!thing->runtimeFromAnyThread()->needsIncrementalBarrier());

        return true;
    }

    return false;
}

namespace gc {

static inline AllocKind
GetGCObjectKind(const Class *clasp)
{
    if (clasp == FunctionClassPtr)
        return JSFunction::FinalizeKind;
    uint32_t nslots = JSCLASS_RESERVED_SLOTS(clasp);
    if (clasp->flags & JSCLASS_HAS_PRIVATE)
        nslots++;
    return GetGCObjectKind(nslots);
}

#ifdef JSGC_GENERATIONAL
inline bool
ShouldNurseryAllocate(const Nursery &nursery, AllocKind kind, InitialHeap heap)
{
    return nursery.isEnabled() && IsNurseryAllocable(kind) && heap != TenuredHeap;
}
#endif

#ifdef JSGC_FJGENERATIONAL
inline bool
ShouldFJNurseryAllocate(const ForkJoinNursery &nursery, AllocKind kind, InitialHeap heap)
{
    return IsFJNurseryAllocable(kind) && heap != TenuredHeap;
}
#endif

inline JSGCTraceKind
GetGCThingTraceKind(const void *thing)
{
    JS_ASSERT(thing);
    const Cell *cell = static_cast<const Cell *>(thing);
#ifdef JSGC_GENERATIONAL
    if (IsInsideNursery(cell))
        return JSTRACE_OBJECT;
#endif
    return MapAllocToTraceKind(cell->asTenured()->getAllocKind());
}

inline void
GCRuntime::poke()
{
    poked = true;

#ifdef JS_GC_ZEAL
    
    if (zealMode == ZealPokeValue)
        nextScheduled = 1;
#endif
}

class ArenaIter
{
    ArenaHeader *aheader;
    ArenaHeader *unsweptHeader;
    ArenaHeader *sweptHeader;

  public:
    ArenaIter() {
        aheader = nullptr;
        unsweptHeader = nullptr;
        sweptHeader = nullptr;
    }

    ArenaIter(JS::Zone *zone, AllocKind kind) {
        init(zone, kind);
    }

    void init(Allocator *allocator, AllocKind kind) {
        aheader = allocator->arenas.getFirstArena(kind);
        unsweptHeader = allocator->arenas.getFirstArenaToSweep(kind);
        sweptHeader = allocator->arenas.getFirstSweptArena(kind);
        if (!unsweptHeader) {
            unsweptHeader = sweptHeader;
            sweptHeader = nullptr;
        }
        if (!aheader) {
            aheader = unsweptHeader;
            unsweptHeader = sweptHeader;
            sweptHeader = nullptr;
        }
    }

    void init(JS::Zone *zone, AllocKind kind) {
        init(&zone->allocator, kind);
    }

    bool done() const {
        return !aheader;
    }

    ArenaHeader *get() const {
        return aheader;
    }

    void next() {
        JS_ASSERT(!done());
        aheader = aheader->next;
        if (!aheader) {
            aheader = unsweptHeader;
            unsweptHeader = sweptHeader;
            sweptHeader = nullptr;
        }
    }
};

class ArenaCellIterImpl
{
    
    size_t firstThingOffset;
    size_t thingSize;
#ifdef DEBUG
    bool isInited;
#endif

    
    FreeSpan span;
    uintptr_t thing;
    uintptr_t limit;

    
    
    void moveForwardIfFree() {
        JS_ASSERT(!done());
        JS_ASSERT(thing);
        
        
        
        
        if (thing == span.first) {
            thing = span.last + thingSize;
            span = *span.nextSpan();
        }
    }

  public:
    ArenaCellIterImpl()
      : firstThingOffset(0)     
      , thingSize(0)            
      , limit(0)
    {
    }

    void initUnsynchronized(ArenaHeader *aheader) {
        AllocKind kind = aheader->getAllocKind();
#ifdef DEBUG
        isInited = true;
#endif
        firstThingOffset = Arena::firstThingOffset(kind);
        thingSize = Arena::thingSize(kind);
        reset(aheader);
    }

    void init(ArenaHeader *aheader) {
#ifdef DEBUG
        AllocKind kind = aheader->getAllocKind();
        JS_ASSERT(aheader->zone->allocator.arenas.isSynchronizedFreeList(kind));
#endif
        initUnsynchronized(aheader);
    }

    
    
    void reset(ArenaHeader *aheader) {
        JS_ASSERT(isInited);
        span = aheader->getFirstFreeSpan();
        uintptr_t arenaAddr = aheader->arenaAddress();
        thing = arenaAddr + firstThingOffset;
        limit = arenaAddr + ArenaSize;
        moveForwardIfFree();
    }

    bool done() const {
        return thing == limit;
    }

    TenuredCell *getCell() const {
        JS_ASSERT(!done());
        return reinterpret_cast<TenuredCell *>(thing);
    }

    template<typename T> T *get() const {
        JS_ASSERT(!done());
        return static_cast<T *>(getCell());
    }

    void next() {
        MOZ_ASSERT(!done());
        thing += thingSize;
        if (thing < limit)
            moveForwardIfFree();
    }
};

template<>
JSObject *
ArenaCellIterImpl::get<JSObject>() const;

class ArenaCellIterUnderGC : public ArenaCellIterImpl
{
  public:
    explicit ArenaCellIterUnderGC(ArenaHeader *aheader) {
        JS_ASSERT(aheader->zone->runtimeFromAnyThread()->isHeapBusy());
        init(aheader);
    }
};

class ArenaCellIterUnderFinalize : public ArenaCellIterImpl
{
  public:
    explicit ArenaCellIterUnderFinalize(ArenaHeader *aheader) {
        initUnsynchronized(aheader);
    }
};

class ZoneCellIterImpl
{
    ArenaIter arenaIter;
    ArenaCellIterImpl cellIter;

  protected:
    ZoneCellIterImpl() {}

    void init(JS::Zone *zone, AllocKind kind) {
        JS_ASSERT(zone->allocator.arenas.isSynchronizedFreeList(kind));
        arenaIter.init(zone, kind);
        if (!arenaIter.done())
            cellIter.init(arenaIter.get());
    }

  public:
    bool done() const {
        return arenaIter.done();
    }

    template<typename T> T *get() const {
        JS_ASSERT(!done());
        return cellIter.get<T>();
    }

    Cell *getCell() const {
        JS_ASSERT(!done());
        return cellIter.getCell();
    }

    void next() {
        JS_ASSERT(!done());
        cellIter.next();
        if (cellIter.done()) {
            JS_ASSERT(!arenaIter.done());
            arenaIter.next();
            if (!arenaIter.done())
                cellIter.reset(arenaIter.get());
        }
    }
};

class ZoneCellIterUnderGC : public ZoneCellIterImpl
{
  public:
    ZoneCellIterUnderGC(JS::Zone *zone, AllocKind kind) {
#ifdef JSGC_GENERATIONAL
        JS_ASSERT(zone->runtimeFromAnyThread()->gc.nursery.isEmpty());
#endif
        JS_ASSERT(zone->runtimeFromAnyThread()->isHeapBusy());
        init(zone, kind);
    }
};

class ZoneCellIter : public ZoneCellIterImpl
{
    JS::AutoAssertNoAlloc noAlloc;
    ArenaLists *lists;
    AllocKind kind;

  public:
    ZoneCellIter(JS::Zone *zone, AllocKind kind)
      : lists(&zone->allocator.arenas),
        kind(kind)
    {
        





        if (IsBackgroundFinalized(kind) &&
            zone->allocator.arenas.needBackgroundFinalizeWait(kind))
        {
            zone->runtimeFromMainThread()->gc.waitBackgroundSweepEnd();
        }

#ifdef JSGC_GENERATIONAL
        
        JSRuntime *rt = zone->runtimeFromMainThread();
        rt->gc.evictNursery();
#endif

        if (lists->isSynchronizedFreeList(kind)) {
            lists = nullptr;
        } else {
            JS_ASSERT(!zone->runtimeFromMainThread()->isHeapBusy());
            lists->copyFreeListToArena(kind);
        }

        
        noAlloc.disallowAlloc(zone->runtimeFromMainThread());

        init(zone, kind);
    }

    ~ZoneCellIter() {
        if (lists)
            lists->clearFreeListInArena(kind);
    }
};

class GCZonesIter
{
  private:
    ZonesIter zone;

  public:
    explicit GCZonesIter(JSRuntime *rt) : zone(rt, WithAtoms) {
        if (!zone->isCollecting())
            next();
    }

    bool done() const { return zone.done(); }

    void next() {
        JS_ASSERT(!done());
        do {
            zone.next();
        } while (!zone.done() && !zone->isCollecting());
    }

    JS::Zone *get() const {
        JS_ASSERT(!done());
        return zone;
    }

    operator JS::Zone *() const { return get(); }
    JS::Zone *operator->() const { return get(); }
};

typedef CompartmentsIterT<GCZonesIter> GCCompartmentsIter;


class GCZoneGroupIter {
  private:
    JS::Zone *current;

  public:
    explicit GCZoneGroupIter(JSRuntime *rt) {
        JS_ASSERT(rt->isHeapBusy());
        current = rt->gc.getCurrentZoneGroup();
    }

    bool done() const { return !current; }

    void next() {
        JS_ASSERT(!done());
        current = current->nextNodeInGroup();
    }

    JS::Zone *get() const {
        JS_ASSERT(!done());
        return current;
    }

    operator JS::Zone *() const { return get(); }
    JS::Zone *operator->() const { return get(); }
};

typedef CompartmentsIterT<GCZoneGroupIter> GCCompartmentGroupIter;

#ifdef JSGC_GENERATIONAL




template <AllowGC allowGC>
inline JSObject *
TryNewNurseryObject(JSContext *cx, size_t thingSize, size_t nDynamicSlots)
{
    JS_ASSERT(!IsAtomsCompartment(cx->compartment()));
    JSRuntime *rt = cx->runtime();
    Nursery &nursery = rt->gc.nursery;
    JSObject *obj = nursery.allocateObject(cx, thingSize, nDynamicSlots);
    if (obj)
        return obj;
    if (allowGC && !rt->mainThread.suppressGC) {
        cx->minorGC(JS::gcreason::OUT_OF_NURSERY);

        
        if (nursery.isEnabled()) {
            JSObject *obj = nursery.allocateObject(cx, thingSize, nDynamicSlots);
            JS_ASSERT(obj);
            return obj;
        }
    }
    return nullptr;
}
#endif 

#ifdef JSGC_FJGENERATIONAL
template <AllowGC allowGC>
inline JSObject *
TryNewNurseryObject(ForkJoinContext *cx, size_t thingSize, size_t nDynamicSlots)
{
    ForkJoinNursery &nursery = cx->nursery();
    bool tooLarge = false;
    JSObject *obj = nursery.allocateObject(thingSize, nDynamicSlots, tooLarge);
    if (obj)
        return obj;

    if (!tooLarge && allowGC) {
        nursery.minorGC();
        obj = nursery.allocateObject(thingSize, nDynamicSlots, tooLarge);
        if (obj)
            return obj;
    }

    return nullptr;
}
#endif 

static inline bool
PossiblyFail()
{
    JS_OOM_POSSIBLY_FAIL();
    return true;
}

template <AllowGC allowGC>
static inline bool
CheckAllocatorState(ThreadSafeContext *cx, AllocKind kind)
{
    if (!cx->isJSContext())
        return true;

    JSContext *ncx = cx->asJSContext();
    JSRuntime *rt = ncx->runtime();
#if defined(JS_GC_ZEAL) || defined(DEBUG)
    JS_ASSERT_IF(rt->isAtomsCompartment(ncx->compartment()),
                 kind == FINALIZE_STRING ||
                 kind == FINALIZE_FAT_INLINE_STRING ||
                 kind == FINALIZE_SYMBOL ||
                 kind == FINALIZE_JITCODE);
    JS_ASSERT(!rt->isHeapBusy());
    JS_ASSERT(rt->gc.isAllocAllowed());
#endif

    
    if (allowGC && !rt->mainThread.suppressGC)
        JS::AutoAssertOnGC::VerifyIsSafeToGC(rt);

    
    if (!PossiblyFail()) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    if (allowGC) {
#ifdef JS_GC_ZEAL
        if (rt->gc.needZealousGC())
            rt->gc.runDebugGC();
#endif

        if (rt->interrupt) {
            
            
            ncx->gcIfNeeded();
        }
    }

    return true;
}

template <typename T>
static inline void
CheckIncrementalZoneState(ThreadSafeContext *cx, T *t)
{
#ifdef DEBUG
    if (!cx->isJSContext())
        return;

    Zone *zone = cx->asJSContext()->zone();
    JS_ASSERT_IF(t && zone->wasGCStarted() && (zone->isGCMarking() || zone->isGCSweeping()),
                 t->asTenured()->arenaHeader()->allocatedDuringIncremental);
#endif
}








template <AllowGC allowGC>
inline JSObject *
AllocateObject(ThreadSafeContext *cx, AllocKind kind, size_t nDynamicSlots, InitialHeap heap)
{
    size_t thingSize = Arena::thingSize(kind);

    JS_ASSERT(thingSize == Arena::thingSize(kind));
    JS_ASSERT(thingSize >= sizeof(JSObject));
    static_assert(sizeof(JSObject) >= CellSize,
                  "All allocations must be at least the allocator-imposed minimum size.");

    if (!CheckAllocatorState<allowGC>(cx, kind))
        return nullptr;

#ifdef JSGC_GENERATIONAL
    if (cx->isJSContext() &&
        ShouldNurseryAllocate(cx->asJSContext()->nursery(), kind, heap)) {
        JSObject *obj = TryNewNurseryObject<allowGC>(cx->asJSContext(), thingSize, nDynamicSlots);
        if (obj)
            return obj;
    }
#endif
#ifdef JSGC_FJGENERATIONAL
    if (cx->isForkJoinContext() &&
        ShouldFJNurseryAllocate(cx->asForkJoinContext()->nursery(), kind, heap))
    {
        JSObject *obj =
            TryNewNurseryObject<allowGC>(cx->asForkJoinContext(), thingSize, nDynamicSlots);
        if (obj)
            return obj;
    }
#endif

    HeapSlot *slots = nullptr;
    if (nDynamicSlots) {
        if (cx->isExclusiveContext())
            slots = cx->asExclusiveContext()->zone()->pod_malloc<HeapSlot>(nDynamicSlots);
        else
            slots = js_pod_malloc<HeapSlot>(nDynamicSlots);
        if (MOZ_UNLIKELY(!slots))
            return nullptr;
        js::Debug_SetSlotRangeToCrashOnTouch(slots, nDynamicSlots);
    }

    JSObject *obj = static_cast<JSObject *>(cx->allocator()->arenas.allocateFromFreeList(kind, thingSize));
    if (!obj)
        obj = static_cast<JSObject *>(js::gc::ArenaLists::refillFreeList<allowGC>(cx, kind));

    if (obj)
        obj->setInitialSlots(slots);
    else
        js_free(slots);

    CheckIncrementalZoneState(cx, obj);
    js::gc::TraceTenuredAlloc(obj, kind);
    return obj;
}

template <typename T, AllowGC allowGC>
inline T *
AllocateNonObject(ThreadSafeContext *cx)
{
    static_assert(sizeof(T) >= CellSize,
                  "All allocations must be at least the allocator-imposed minimum size.");

    AllocKind kind = MapTypeToFinalizeKind<T>::kind;
    size_t thingSize = sizeof(T);

    JS_ASSERT(thingSize == Arena::thingSize(kind));
    if (!CheckAllocatorState<allowGC>(cx, kind))
        return nullptr;

    T *t = static_cast<T *>(cx->allocator()->arenas.allocateFromFreeList(kind, thingSize));
    if (!t)
        t = static_cast<T *>(js::gc::ArenaLists::refillFreeList<allowGC>(cx, kind));

    CheckIncrementalZoneState(cx, t);
    js::gc::TraceTenuredAlloc(t, kind);
    return t;
}










template <AllowGC allowGC>
inline JSObject *
AllocateObjectForCacheHit(JSContext *cx, AllocKind kind, InitialHeap heap)
{
#ifdef JSGC_GENERATIONAL
    if (ShouldNurseryAllocate(cx->nursery(), kind, heap)) {
        size_t thingSize = Arena::thingSize(kind);

        JS_ASSERT(thingSize == Arena::thingSize(kind));
        if (!CheckAllocatorState<NoGC>(cx, kind))
            return nullptr;

        JSObject *obj = TryNewNurseryObject<NoGC>(cx, thingSize, 0);
        if (!obj && allowGC) {
            cx->minorGC(JS::gcreason::OUT_OF_NURSERY);
            return nullptr;
        }
        return obj;
    }
#endif

    JSObject *obj = AllocateObject<NoGC>(cx, kind, 0, heap);
    if (!obj && allowGC) {
        cx->runtime()->gc.maybeGC(cx->zone());
        return nullptr;
    }

    return obj;
}

inline bool
IsInsideGGCNursery(const js::gc::Cell *cell)
{
#ifdef JSGC_GENERATIONAL
    if (!cell)
        return false;
    uintptr_t addr = uintptr_t(cell);
    addr &= ~js::gc::ChunkMask;
    addr |= js::gc::ChunkLocationOffset;
    uint32_t location = *reinterpret_cast<uint32_t *>(addr);
    JS_ASSERT(location != 0);
    return location & js::gc::ChunkLocationBitNursery;
#else
    return false;
#endif
}

} 

template <js::AllowGC allowGC>
inline JSObject *
NewGCObject(js::ThreadSafeContext *cx, js::gc::AllocKind kind, size_t nDynamicSlots, js::gc::InitialHeap heap)
{
    JS_ASSERT(kind >= js::gc::FINALIZE_OBJECT0 && kind <= js::gc::FINALIZE_OBJECT_LAST);
    return js::gc::AllocateObject<allowGC>(cx, kind, nDynamicSlots, heap);
}

template <js::AllowGC allowGC>
inline jit::JitCode *
NewJitCode(js::ThreadSafeContext *cx)
{
    return gc::AllocateNonObject<jit::JitCode, allowGC>(cx);
}

inline
types::TypeObject *
NewTypeObject(js::ThreadSafeContext *cx)
{
    return gc::AllocateNonObject<types::TypeObject, js::CanGC>(cx);
}

template <js::AllowGC allowGC>
inline JSString *
NewGCString(js::ThreadSafeContext *cx)
{
    return js::gc::AllocateNonObject<JSString, allowGC>(cx);
}

template <js::AllowGC allowGC>
inline JSFatInlineString *
NewGCFatInlineString(js::ThreadSafeContext *cx)
{
    return js::gc::AllocateNonObject<JSFatInlineString, allowGC>(cx);
}

inline JSExternalString *
NewGCExternalString(js::ThreadSafeContext *cx)
{
    return js::gc::AllocateNonObject<JSExternalString, js::CanGC>(cx);
}

} 

inline JSScript *
js_NewGCScript(js::ThreadSafeContext *cx)
{
    return js::gc::AllocateNonObject<JSScript, js::CanGC>(cx);
}

inline js::LazyScript *
js_NewGCLazyScript(js::ThreadSafeContext *cx)
{
    return js::gc::AllocateNonObject<js::LazyScript, js::CanGC>(cx);
}

inline js::Shape *
js_NewGCShape(js::ThreadSafeContext *cx)
{
    return js::gc::AllocateNonObject<js::Shape, js::CanGC>(cx);
}

template <js::AllowGC allowGC>
inline js::BaseShape *
js_NewGCBaseShape(js::ThreadSafeContext *cx)
{
    return js::gc::AllocateNonObject<js::BaseShape, allowGC>(cx);
}

#endif 
