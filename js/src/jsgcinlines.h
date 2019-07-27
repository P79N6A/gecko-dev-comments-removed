





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
    MOZ_ASSERT_IF(isJSContext(), &asJSContext()->zone()->allocator == allocator_);
    return allocator_;
}

template <typename T>
inline bool
ThreadSafeContext::isThreadLocal(T thing) const
{
    if (!isForkJoinContext())
        return true;

    
    MOZ_ASSERT(!IsInsideNursery(thing));

    
    if (allocator_->arenas.containsArena(runtime_, thing->asTenured().arenaHeader()))
    {
        
        
        MOZ_ASSERT(!thing->zoneFromAnyThread()->needsIncrementalBarrier());
        MOZ_ASSERT(!thing->runtimeFromAnyThread()->needsIncrementalBarrier());

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

inline bool
ShouldNurseryAllocateObject(const Nursery &nursery, InitialHeap heap)
{
    return nursery.isEnabled() && heap != TenuredHeap;
}

inline JSGCTraceKind
GetGCThingTraceKind(const void *thing)
{
    MOZ_ASSERT(thing);
    const Cell *cell = static_cast<const Cell *>(thing);
    if (IsInsideNursery(cell))
        return JSTRACE_OBJECT;
    return MapAllocToTraceKind(cell->asTenured().getAllocKind());
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
        MOZ_ASSERT(!done());
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
        MOZ_ASSERT(!done());
        MOZ_ASSERT(thing);
        
        
        
        
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
        MOZ_ASSERT(aheader->zone->allocator.arenas.isSynchronizedFreeList(kind));
#endif
        initUnsynchronized(aheader);
    }

    
    
    void reset(ArenaHeader *aheader) {
        MOZ_ASSERT(isInited);
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
        MOZ_ASSERT(!done());
        return reinterpret_cast<TenuredCell *>(thing);
    }

    template<typename T> T *get() const {
        MOZ_ASSERT(!done());
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
        MOZ_ASSERT(aheader->zone->runtimeFromAnyThread()->isHeapBusy());
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
        MOZ_ASSERT(zone->allocator.arenas.isSynchronizedFreeList(kind));
        arenaIter.init(zone, kind);
        if (!arenaIter.done())
            cellIter.init(arenaIter.get());
    }

  public:
    bool done() const {
        return arenaIter.done();
    }

    template<typename T> T *get() const {
        MOZ_ASSERT(!done());
        return cellIter.get<T>();
    }

    Cell *getCell() const {
        MOZ_ASSERT(!done());
        return cellIter.getCell();
    }

    void next() {
        MOZ_ASSERT(!done());
        cellIter.next();
        if (cellIter.done()) {
            MOZ_ASSERT(!arenaIter.done());
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
        MOZ_ASSERT(zone->runtimeFromAnyThread()->gc.nursery.isEmpty());
        MOZ_ASSERT(zone->runtimeFromAnyThread()->isHeapBusy());
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
        JSRuntime *rt = zone->runtimeFromMainThread();

        





        if (IsBackgroundFinalized(kind) &&
            zone->allocator.arenas.needBackgroundFinalizeWait(kind))
        {
            rt->gc.waitBackgroundSweepEnd();
        }

        
        rt->gc.evictNursery();

        if (lists->isSynchronizedFreeList(kind)) {
            lists = nullptr;
        } else {
            MOZ_ASSERT(!rt->isHeapBusy());
            lists->copyFreeListToArena(kind);
        }

        
        noAlloc.disallowAlloc(rt);

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
    explicit GCZonesIter(JSRuntime *rt, ZoneSelector selector = WithAtoms)
      : zone(rt, selector)
    {
        if (!zone->isCollecting())
            next();
    }

    bool done() const { return zone.done(); }

    void next() {
        MOZ_ASSERT(!done());
        do {
            zone.next();
        } while (!zone.done() && !zone->isCollectingFromAnyThread());
    }

    JS::Zone *get() const {
        MOZ_ASSERT(!done());
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
        MOZ_ASSERT(rt->isHeapBusy());
        current = rt->gc.getCurrentZoneGroup();
    }

    bool done() const { return !current; }

    void next() {
        MOZ_ASSERT(!done());
        current = current->nextNodeInGroup();
    }

    JS::Zone *get() const {
        MOZ_ASSERT(!done());
        return current;
    }

    operator JS::Zone *() const { return get(); }
    JS::Zone *operator->() const { return get(); }
};

typedef CompartmentsIterT<GCZoneGroupIter> GCCompartmentGroupIter;





template <AllowGC allowGC>
inline JSObject *
TryNewNurseryObject(JSContext *cx, size_t thingSize, size_t nDynamicSlots, const js::Class *clasp)
{
    MOZ_ASSERT(!IsAtomsCompartment(cx->compartment()));
    JSRuntime *rt = cx->runtime();
    Nursery &nursery = rt->gc.nursery;
    JSObject *obj = nursery.allocateObject(cx, thingSize, nDynamicSlots, clasp);
    if (obj)
        return obj;
    if (allowGC && !rt->mainThread.suppressGC) {
        cx->minorGC(JS::gcreason::OUT_OF_NURSERY);

        
        if (nursery.isEnabled()) {
            JSObject *obj = nursery.allocateObject(cx, thingSize, nDynamicSlots, clasp);
            MOZ_ASSERT(obj);
            return obj;
        }
    }
    return nullptr;
}

static inline bool
PossiblyFail()
{
    JS_OOM_POSSIBLY_FAIL_BOOL();
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
    MOZ_ASSERT_IF(rt->isAtomsCompartment(ncx->compartment()),
                  kind == FINALIZE_STRING ||
                  kind == FINALIZE_FAT_INLINE_STRING ||
                  kind == FINALIZE_SYMBOL ||
                  kind == FINALIZE_JITCODE);
    MOZ_ASSERT(!rt->isHeapBusy());
    MOZ_ASSERT(rt->gc.isAllocAllowed());
#endif

    
    if (allowGC && !rt->mainThread.suppressGC)
        JS::AutoAssertOnGC::VerifyIsSafeToGC(rt);

    
    if (!PossiblyFail()) {
        js_ReportOutOfMemory(cx->asJSContext());
        return false;
    }

    if (allowGC) {
#ifdef JS_GC_ZEAL
        if (rt->gc.needZealousGC())
            rt->gc.runDebugGC();
#endif

        if (rt->hasPendingInterrupt()) {
            
            
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
    MOZ_ASSERT_IF(t && zone->wasGCStarted() && (zone->isGCMarking() || zone->isGCSweeping()),
                  t->asTenured().arenaHeader()->allocatedDuringIncremental);
#endif
}








template <AllowGC allowGC>
inline JSObject *
AllocateObject(ThreadSafeContext *cx, AllocKind kind, size_t nDynamicSlots, InitialHeap heap,
               const js::Class *clasp)
{
    size_t thingSize = Arena::thingSize(kind);

    MOZ_ASSERT(thingSize == Arena::thingSize(kind));
    MOZ_ASSERT(thingSize >= sizeof(JSObject));
    static_assert(sizeof(JSObject) >= CellSize,
                  "All allocations must be at least the allocator-imposed minimum size.");

    if (!CheckAllocatorState<allowGC>(cx, kind))
        return nullptr;

    if (cx->isJSContext() &&
        ShouldNurseryAllocateObject(cx->asJSContext()->nursery(), heap))
    {
        JSObject *obj = TryNewNurseryObject<allowGC>(cx->asJSContext(), thingSize, nDynamicSlots,
                                                     clasp);
        if (obj)
            return obj;
    }

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

    JSObject *obj = reinterpret_cast<JSObject *>(
            cx->allocator()->arenas.allocateFromFreeList(kind, thingSize));
    if (!obj)
        obj = reinterpret_cast<JSObject *>(GCRuntime::refillFreeListFromAnyThread<allowGC>(cx, kind));

    if (obj)
        obj->setInitialSlotsMaybeNonNative(slots);
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

    MOZ_ASSERT(thingSize == Arena::thingSize(kind));
    if (!CheckAllocatorState<allowGC>(cx, kind))
        return nullptr;

    T *t = static_cast<T *>(cx->allocator()->arenas.allocateFromFreeList(kind, thingSize));
    if (!t)
        t = static_cast<T *>(GCRuntime::refillFreeListFromAnyThread<allowGC>(cx, kind));

    CheckIncrementalZoneState(cx, t);
    js::gc::TraceTenuredAlloc(t, kind);
    return t;
}








template <AllowGC allowGC>
inline JSObject *
AllocateObjectForCacheHit(JSContext *cx, AllocKind kind, InitialHeap heap, const js::Class *clasp)
{
    if (ShouldNurseryAllocateObject(cx->nursery(), heap)) {
        size_t thingSize = Arena::thingSize(kind);

        MOZ_ASSERT(thingSize == Arena::thingSize(kind));
        if (!CheckAllocatorState<NoGC>(cx, kind))
            return nullptr;

        JSObject *obj = TryNewNurseryObject<NoGC>(cx, thingSize, 0, clasp);
        if (!obj && allowGC) {
            cx->minorGC(JS::gcreason::OUT_OF_NURSERY);
            return nullptr;
        }
        return obj;
    }

    JSObject *obj = AllocateObject<NoGC>(cx, kind, 0, heap, clasp);
    if (!obj && allowGC) {
        cx->runtime()->gc.maybeGC(cx->zone());
        return nullptr;
    }

    return obj;
}

inline bool
IsInsideGGCNursery(const js::gc::Cell *cell)
{
    if (!cell)
        return false;
    uintptr_t addr = uintptr_t(cell);
    addr &= ~js::gc::ChunkMask;
    addr |= js::gc::ChunkLocationOffset;
    uint32_t location = *reinterpret_cast<uint32_t *>(addr);
    MOZ_ASSERT(location != 0);
    return location & js::gc::ChunkLocationBitNursery;
}

} 

template <js::AllowGC allowGC>
inline JSObject *
NewGCObject(js::ThreadSafeContext *cx, js::gc::AllocKind kind, size_t nDynamicSlots,
            js::gc::InitialHeap heap, const js::Class *clasp)
{
    MOZ_ASSERT(kind >= js::gc::FINALIZE_OBJECT0 && kind <= js::gc::FINALIZE_OBJECT_LAST);
    return js::gc::AllocateObject<allowGC>(cx, kind, nDynamicSlots, heap, clasp);
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

inline Shape *
NewGCShape(ThreadSafeContext *cx)
{
    return gc::AllocateNonObject<Shape, CanGC>(cx);
}

inline Shape *
NewGCAccessorShape(ThreadSafeContext *cx)
{
    return gc::AllocateNonObject<AccessorShape, CanGC>(cx);
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

template <js::AllowGC allowGC>
inline js::BaseShape *
js_NewGCBaseShape(js::ThreadSafeContext *cx)
{
    return js::gc::AllocateNonObject<js::BaseShape, allowGC>(cx);
}

#endif 
