





#include "gc/Allocator.h"

#include "jscntxt.h"

#include "gc/GCInternals.h"
#include "gc/GCTrace.h"
#include "gc/Nursery.h"
#include "jit/JitCompartment.h"
#include "vm/Runtime.h"
#include "vm/String.h"

#include "jsobjinlines.h"

using namespace js;
using namespace gc;

bool
GCRuntime::gcIfNeededPerAllocation(JSContext* cx)
{
#ifdef JS_GC_ZEAL
    if (needZealousGC())
        runDebugGC();
#endif

    
    
    if (rt->hasPendingInterrupt())
        gcIfRequested(cx);

    
    
    
    if (isIncrementalGCInProgress() &&
        cx->zone()->usage.gcBytes() > cx->zone()->threshold.gcTriggerBytes())
    {
        PrepareZoneForGC(cx->zone());
        AutoKeepAtoms keepAtoms(cx->perThreadData);
        gc(GC_NORMAL, JS::gcreason::INCREMENTAL_TOO_SLOW);
    }

    return true;
}

template <AllowGC allowGC>
bool
GCRuntime::checkAllocatorState(JSContext* cx, AllocKind kind)
{
    if (allowGC) {
        if (!gcIfNeededPerAllocation(cx))
            return false;
    }

#if defined(JS_GC_ZEAL) || defined(DEBUG)
    MOZ_ASSERT_IF(rt->isAtomsCompartment(cx->compartment()),
                  kind == AllocKind::STRING ||
                  kind == AllocKind::FAT_INLINE_STRING ||
                  kind == AllocKind::SYMBOL ||
                  kind == AllocKind::JITCODE);
    MOZ_ASSERT(!rt->isHeapBusy());
    MOZ_ASSERT(isAllocAllowed());
#endif

    
    if (allowGC && !rt->mainThread.suppressGC)
        JS::AutoAssertOnGC::VerifyIsSafeToGC(rt);

    
    if (js::oom::ShouldFailWithOOM()) {
        ReportOutOfMemory(cx);
        return false;
    }

    return true;
}

template <typename T>
 void
GCRuntime::checkIncrementalZoneState(ExclusiveContext* cx, T* t)
{
#ifdef DEBUG
    if (!cx->isJSContext())
        return;

    Zone* zone = cx->asJSContext()->zone();
    MOZ_ASSERT_IF(t && zone->wasGCStarted() && (zone->isGCMarking() || zone->isGCSweeping()),
                  t->asTenured().arenaHeader()->allocatedDuringIncremental);
#endif
}

template <typename T, AllowGC allowGC >
JSObject*
js::Allocate(ExclusiveContext* cx, AllocKind kind, size_t nDynamicSlots, InitialHeap heap,
             const Class* clasp)
{
    static_assert(mozilla::IsConvertible<T*, JSObject*>::value, "must be JSObject derived");
    MOZ_ASSERT(IsObjectAllocKind(kind));
    size_t thingSize = Arena::thingSize(kind);

    MOZ_ASSERT(thingSize == Arena::thingSize(kind));
    MOZ_ASSERT(thingSize >= sizeof(JSObject_Slots0));
    static_assert(sizeof(JSObject_Slots0) >= CellSize,
                  "All allocations must be at least the allocator-imposed minimum size.");

    
    if (!cx->isJSContext())
        return GCRuntime::tryNewTenuredObject<NoGC>(cx, kind, thingSize, nDynamicSlots);

    JSContext* ncx = cx->asJSContext();
    JSRuntime* rt = ncx->runtime();
    if (!rt->gc.checkAllocatorState<allowGC>(ncx, kind))
        return nullptr;

    if (ncx->nursery().isEnabled() && heap != TenuredHeap) {
        JSObject* obj = rt->gc.tryNewNurseryObject<allowGC>(ncx, thingSize, nDynamicSlots, clasp);
        if (obj)
            return obj;

        
        
        
        
        
        if (!allowGC)
            return nullptr;
    }

    return GCRuntime::tryNewTenuredObject<allowGC>(cx, kind, thingSize, nDynamicSlots);
}
template JSObject* js::Allocate<JSObject, NoGC>(ExclusiveContext* cx, gc::AllocKind kind,
                                                size_t nDynamicSlots, gc::InitialHeap heap,
                                                const Class* clasp);
template JSObject* js::Allocate<JSObject, CanGC>(ExclusiveContext* cx, gc::AllocKind kind,
                                                 size_t nDynamicSlots, gc::InitialHeap heap,
                                                 const Class* clasp);



template <AllowGC allowGC>
JSObject*
GCRuntime::tryNewNurseryObject(JSContext* cx, size_t thingSize, size_t nDynamicSlots, const Class* clasp)
{
    MOZ_ASSERT(!IsAtomsCompartment(cx->compartment()));
    JSObject* obj = nursery.allocateObject(cx, thingSize, nDynamicSlots, clasp);
    if (obj)
        return obj;

    if (allowGC && !rt->mainThread.suppressGC) {
        minorGC(cx, JS::gcreason::OUT_OF_NURSERY);

        
        if (nursery.isEnabled()) {
            JSObject* obj = nursery.allocateObject(cx, thingSize, nDynamicSlots, clasp);
            MOZ_ASSERT(obj);
            return obj;
        }
    }
    return nullptr;
}

template <AllowGC allowGC>
JSObject*
GCRuntime::tryNewTenuredObject(ExclusiveContext* cx, AllocKind kind, size_t thingSize,
                               size_t nDynamicSlots)
{
    HeapSlot* slots = nullptr;
    if (nDynamicSlots) {
        slots = cx->zone()->pod_malloc<HeapSlot>(nDynamicSlots);
        if (MOZ_UNLIKELY(!slots))
            return nullptr;
        Debug_SetSlotRangeToCrashOnTouch(slots, nDynamicSlots);
    }

    JSObject* obj = tryNewTenuredThing<JSObject, allowGC>(cx, kind, thingSize);

    if (obj)
        obj->setInitialSlotsMaybeNonNative(slots);
    else
        js_free(slots);

    return obj;
}

template <typename T, AllowGC allowGC >
T*
js::Allocate(ExclusiveContext* cx)
{
    static_assert(!mozilla::IsConvertible<T*, JSObject*>::value, "must not be JSObject derived");
    static_assert(sizeof(T) >= CellSize,
                  "All allocations must be at least the allocator-imposed minimum size.");

    AllocKind kind = MapTypeToFinalizeKind<T>::kind;
    size_t thingSize = sizeof(T);
    MOZ_ASSERT(thingSize == Arena::thingSize(kind));

    if (cx->isJSContext()) {
        JSContext* ncx = cx->asJSContext();
        if (!ncx->runtime()->gc.checkAllocatorState<allowGC>(ncx, kind))
            return nullptr;
    }

    return GCRuntime::tryNewTenuredThing<T, allowGC>(cx, kind, thingSize);
}

#define FOR_ALL_NON_OBJECT_GC_LAYOUTS(macro) \
    macro(JS::Symbol) \
    macro(JSExternalString) \
    macro(JSFatInlineString) \
    macro(JSScript) \
    macro(JSString) \
    macro(js::AccessorShape) \
    macro(js::BaseShape) \
    macro(js::LazyScript) \
    macro(js::ObjectGroup) \
    macro(js::Shape) \
    macro(js::jit::JitCode)

#define DECL_ALLOCATOR_INSTANCES(type) \
    template type* js::Allocate<type, NoGC>(ExclusiveContext* cx);\
    template type* js::Allocate<type, CanGC>(ExclusiveContext* cx);
FOR_ALL_NON_OBJECT_GC_LAYOUTS(DECL_ALLOCATOR_INSTANCES)
#undef DECL_ALLOCATOR_INSTANCES

template <typename T, AllowGC allowGC>
 T*
GCRuntime::tryNewTenuredThing(ExclusiveContext* cx, AllocKind kind, size_t thingSize)
{
    T* t = reinterpret_cast<T*>(cx->arenas()->allocateFromFreeList(kind, thingSize));
    if (!t)
        t = reinterpret_cast<T*>(refillFreeListFromAnyThread<allowGC>(cx, kind, thingSize));

    checkIncrementalZoneState(cx, t);
    TraceTenuredAlloc(t, kind);
    return t;
}

template <AllowGC allowGC>
 void*
GCRuntime::refillFreeListFromAnyThread(ExclusiveContext* cx, AllocKind thingKind, size_t thingSize)
{
    MOZ_ASSERT(cx->arenas()->freeLists[thingKind].isEmpty());

    if (cx->isJSContext())
        return refillFreeListFromMainThread<allowGC>(cx->asJSContext(), thingKind, thingSize);

    return refillFreeListOffMainThread(cx, thingKind);
}

template <AllowGC allowGC>
 void*
GCRuntime::refillFreeListFromMainThread(JSContext* cx, AllocKind thingKind, size_t thingSize)
{
    JSRuntime* rt = cx->runtime();
    MOZ_ASSERT(!rt->isHeapBusy(), "allocating while under GC");
    MOZ_ASSERT_IF(allowGC, !rt->currentThreadHasExclusiveAccess());

    
    void* thing = tryRefillFreeListFromMainThread(cx, thingKind);
    if (MOZ_LIKELY(thing))
        return thing;

    
    {
        
        
        if (!allowGC)
            return nullptr;

        JS::PrepareForFullGC(rt);
        AutoKeepAtoms keepAtoms(cx->perThreadData);
        rt->gc.gc(GC_SHRINK, JS::gcreason::LAST_DITCH);
    }

    
    
    
    thing = cx->arenas()->allocateFromFreeList(thingKind, thingSize);
    if (!thing)
        thing = tryRefillFreeListFromMainThread(cx, thingKind);
    if (thing)
        return thing;

    
    MOZ_ASSERT(allowGC, "A fallible allocation must not report OOM on failure.");
    ReportOutOfMemory(cx);
    return nullptr;
}

 void*
GCRuntime::tryRefillFreeListFromMainThread(JSContext* cx, AllocKind thingKind)
{
    ArenaLists* arenas = cx->arenas();
    Zone* zone = cx->zone();

    AutoMaybeStartBackgroundAllocation maybeStartBGAlloc;

    void* thing = arenas->allocateFromArena(zone, thingKind, maybeStartBGAlloc);
    if (MOZ_LIKELY(thing))
        return thing;

    
    
    
    
    
    cx->runtime()->gc.waitBackgroundSweepOrAllocEnd();

    thing = arenas->allocateFromArena(zone, thingKind, maybeStartBGAlloc);
    if (thing)
        return thing;

    return nullptr;
}

 void*
GCRuntime::refillFreeListOffMainThread(ExclusiveContext* cx, AllocKind thingKind)
{
    ArenaLists* arenas = cx->arenas();
    Zone* zone = cx->zone();
    JSRuntime* rt = zone->runtimeFromAnyThread();

    AutoMaybeStartBackgroundAllocation maybeStartBGAlloc;

    
    
    
    AutoLockHelperThreadState lock;
    while (rt->isHeapBusy())
        HelperThreadState().wait(GlobalHelperThreadState::PRODUCER);

    void* thing = arenas->allocateFromArena(zone, thingKind, maybeStartBGAlloc);
    if (thing)
        return thing;

    ReportOutOfMemory(cx);
    return nullptr;
}

TenuredCell*
ArenaLists::allocateFromArena(JS::Zone* zone, AllocKind thingKind,
                              AutoMaybeStartBackgroundAllocation& maybeStartBGAlloc)
{
    JSRuntime* rt = zone->runtimeFromAnyThread();
    mozilla::Maybe<AutoLockGC> maybeLock;

    
    if (backgroundFinalizeState[thingKind] != BFS_DONE)
        maybeLock.emplace(rt);

    ArenaList& al = arenaLists[thingKind];
    ArenaHeader* aheader = al.takeNextArena();
    if (aheader) {
        
        MOZ_ASSERT(!aheader->isEmpty());

        return allocateFromArenaInner<HasFreeThings>(zone, aheader, thingKind);
    }

    
    
    if (maybeLock.isNothing())
        maybeLock.emplace(rt);

    Chunk* chunk = rt->gc.pickChunk(maybeLock.ref(), maybeStartBGAlloc);
    if (!chunk)
        return nullptr;

    
    
    aheader = rt->gc.allocateArena(chunk, zone, thingKind, maybeLock.ref());
    if (!aheader)
        return nullptr;

    MOZ_ASSERT(!maybeLock->wasUnlocked());
    MOZ_ASSERT(al.isCursorAtEnd());
    al.insertAtCursor(aheader);

    return allocateFromArenaInner<IsEmpty>(zone, aheader, thingKind);
}

template <ArenaLists::ArenaAllocMode hasFreeThings>
TenuredCell*
ArenaLists::allocateFromArenaInner(JS::Zone* zone, ArenaHeader* aheader, AllocKind kind)
{
    size_t thingSize = Arena::thingSize(kind);

    FreeSpan span;
    if (hasFreeThings) {
        MOZ_ASSERT(aheader->hasFreeThings());
        span = aheader->getFirstFreeSpan();
        aheader->setAsFullyUsed();
    } else {
        MOZ_ASSERT(!aheader->hasFreeThings());
        Arena* arena = aheader->getArena();
        span.initFinal(arena->thingsStart(kind), arena->thingsEnd() - thingSize, thingSize);
    }
    freeLists[kind].setHead(&span);

    if (MOZ_UNLIKELY(zone->wasGCStarted()))
        zone->runtimeFromAnyThread()->gc.arenaAllocatedDuringGC(zone, aheader);
    TenuredCell* thing = freeLists[kind].allocate(thingSize);
    MOZ_ASSERT(thing); 
    return thing;
}

void
GCRuntime::arenaAllocatedDuringGC(JS::Zone* zone, ArenaHeader* arena)
{
    if (zone->needsIncrementalBarrier()) {
        arena->allocatedDuringIncremental = true;
        marker.delayMarkingArena(arena);
    } else if (zone->isGCSweeping()) {
        arena->setNextAllocDuringSweep(arenasAllocatedDuringSweep);
        arenasAllocatedDuringSweep = arena;
    }
}


