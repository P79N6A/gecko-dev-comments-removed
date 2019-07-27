





#include "gc/Allocator.h"

#include "mozilla/UniquePtr.h"

#include "jscntxt.h"

#include "gc/GCTrace.h"
#include "gc/Nursery.h"
#include "jit/JitCompartment.h"
#include "vm/Runtime.h"
#include "vm/String.h"

#include "jsobjinlines.h"

using namespace js;
using namespace gc;

typedef mozilla::UniquePtr<HeapSlot, JS::FreePolicy> UniqueSlots;

static inline UniqueSlots
MakeSlotArray(ExclusiveContext *cx, size_t count)
{
    HeapSlot *slots = nullptr;
    if (count) {
        slots = cx->zone()->pod_malloc<HeapSlot>(count);
        if (slots)
            Debug_SetSlotRangeToCrashOnTouch(slots, count);
    }
    return UniqueSlots(slots);
}

static inline bool
ShouldNurseryAllocateObject(const Nursery &nursery, InitialHeap heap)
{
    return nursery.isEnabled() && heap != TenuredHeap;
}

template <typename T, AllowGC allowGC>
T *
TryNewTenuredThing(ExclusiveContext *cx, AllocKind kind, size_t thingSize);





template <AllowGC allowGC>
inline JSObject *
TryNewNurseryObject(JSContext *cx, size_t thingSize, size_t nDynamicSlots, const Class *clasp)
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

template <AllowGC allowGC>
inline JSObject *
TryNewTenuredObject(ExclusiveContext *cx, AllocKind kind, size_t thingSize, size_t nDynamicSlots)
{
    UniqueSlots slots = MakeSlotArray(cx, nDynamicSlots);
    if (nDynamicSlots && !slots)
        return nullptr;

    JSObject *obj = TryNewTenuredThing<JSObject, allowGC>(cx, kind, thingSize);

    if (obj)
        obj->setInitialSlotsMaybeNonNative(slots.release());

    return obj;
}

static inline bool
GCIfNeeded(JSContext *cx)
{
    JSRuntime *rt = cx->runtime();

#ifdef JS_GC_ZEAL
    if (rt->gc.needZealousGC())
        rt->gc.runDebugGC();
#endif

    
    
    if (rt->hasPendingInterrupt())
        rt->gc.gcIfRequested(cx);

    
    
    
    if (rt->gc.isIncrementalGCInProgress() &&
        cx->zone()->usage.gcBytes() > cx->zone()->threshold.gcTriggerBytes())
    {
        PrepareZoneForGC(cx->zone());
        AutoKeepAtoms keepAtoms(cx->perThreadData);
        rt->gc.gc(GC_NORMAL, JS::gcreason::INCREMENTAL_TOO_SLOW);
    }

    return true;
}

template <AllowGC allowGC>
static inline bool
CheckAllocatorState(JSContext *cx, AllocKind kind)
{
    if (allowGC) {
        if (!GCIfNeeded(cx))
            return false;
    }

    JSRuntime *rt = cx->runtime();
#if defined(JS_GC_ZEAL) || defined(DEBUG)
    MOZ_ASSERT_IF(rt->isAtomsCompartment(cx->compartment()),
                  kind == FINALIZE_STRING ||
                  kind == FINALIZE_FAT_INLINE_STRING ||
                  kind == FINALIZE_SYMBOL ||
                  kind == FINALIZE_JITCODE);
    MOZ_ASSERT(!rt->isHeapBusy());
    MOZ_ASSERT(rt->gc.isAllocAllowed());
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
static inline void
CheckIncrementalZoneState(ExclusiveContext *cx, T *t)
{
#ifdef DEBUG
    if (!cx->isJSContext())
        return;

    Zone *zone = cx->asJSContext()->zone();
    MOZ_ASSERT_IF(t && zone->wasGCStarted() && (zone->isGCMarking() || zone->isGCSweeping()),
                  t->asTenured().arenaHeader()->allocatedDuringIncremental);
#endif
}








template <typename T, AllowGC allowGC >
JSObject *
js::Allocate(ExclusiveContext *cx, AllocKind kind, size_t nDynamicSlots, InitialHeap heap,
             const Class *clasp)
{
    static_assert(mozilla::IsConvertible<T *, JSObject *>::value, "must be JSObject derived");
    MOZ_ASSERT(kind >= FINALIZE_OBJECT0 && kind <= FINALIZE_OBJECT_LAST);
    size_t thingSize = Arena::thingSize(kind);

    MOZ_ASSERT(thingSize == Arena::thingSize(kind));
    MOZ_ASSERT(thingSize >= sizeof(JSObject_Slots0));
    static_assert(sizeof(JSObject_Slots0) >= CellSize,
                  "All allocations must be at least the allocator-imposed minimum size.");

    
    if (!cx->isJSContext())
        return TryNewTenuredObject<NoGC>(cx, kind, thingSize, nDynamicSlots);

    JSContext *ncx = cx->asJSContext();
    if (!CheckAllocatorState<allowGC>(ncx, kind))
        return nullptr;

    if (ShouldNurseryAllocateObject(ncx->nursery(), heap)) {
        JSObject *obj = TryNewNurseryObject<allowGC>(ncx, thingSize, nDynamicSlots, clasp);
        if (obj)
            return obj;

        
        
        
        
        
        if (!allowGC)
            return nullptr;
    }

    return TryNewTenuredObject<allowGC>(cx, kind, thingSize, nDynamicSlots);
}
template JSObject *js::Allocate<JSObject, NoGC>(ExclusiveContext *cx, gc::AllocKind kind,
                                                size_t nDynamicSlots, gc::InitialHeap heap,
                                                const Class *clasp);
template JSObject *js::Allocate<JSObject, CanGC>(ExclusiveContext *cx, gc::AllocKind kind,
                                                 size_t nDynamicSlots, gc::InitialHeap heap,
                                                 const Class *clasp);

template <typename T, AllowGC allowGC >
T *
js::Allocate(ExclusiveContext *cx)
{
    static_assert(!mozilla::IsConvertible<T*, JSObject*>::value, "must not be JSObject derived");
    static_assert(sizeof(T) >= CellSize,
                  "All allocations must be at least the allocator-imposed minimum size.");

    AllocKind kind = MapTypeToFinalizeKind<T>::kind;
    size_t thingSize = sizeof(T);
    MOZ_ASSERT(thingSize == Arena::thingSize(kind));

    if (cx->isJSContext()) {
        if (!CheckAllocatorState<allowGC>(cx->asJSContext(), kind))
            return nullptr;
    }

    return TryNewTenuredThing<T, allowGC>(cx, kind, thingSize);
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
    template type *js::Allocate<type, NoGC>(ExclusiveContext *cx);\
    template type *js::Allocate<type, CanGC>(ExclusiveContext *cx);
FOR_ALL_NON_OBJECT_GC_LAYOUTS(DECL_ALLOCATOR_INSTANCES)
#undef DECL_ALLOCATOR_INSTANCES

template <typename T, AllowGC allowGC>
T *
TryNewTenuredThing(ExclusiveContext *cx, AllocKind kind, size_t thingSize)
{
    T *t = reinterpret_cast<T *>(cx->arenas()->allocateFromFreeList(kind, thingSize));
    if (!t)
        t = reinterpret_cast<T *>(GCRuntime::refillFreeListFromAnyThread<allowGC>(cx, kind));

    CheckIncrementalZoneState(cx, t);
    TraceTenuredAlloc(t, kind);
    return t;
}
