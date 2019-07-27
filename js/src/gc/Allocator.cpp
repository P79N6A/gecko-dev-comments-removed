





#include "gc/Allocator.h"

#include "jscntxt.h"

#include "gc/GCTrace.h"
#include "gc/Nursery.h"
#include "jit/JitCompartment.h"
#include "vm/Runtime.h"
#include "vm/String.h"

#include "jsobjinlines.h"

using namespace js;
using namespace gc;

static inline bool
ShouldNurseryAllocateObject(const Nursery &nursery, InitialHeap heap)
{
    return nursery.isEnabled() && heap != TenuredHeap;
}





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

static inline bool
PossiblyFail()
{
    JS_OOM_POSSIBLY_FAIL_BOOL();
    return true;
}

static inline bool
GCIfNeeded(ExclusiveContext *cx)
{
    if (cx->isJSContext()) {
        JSContext *ncx = cx->asJSContext();
        JSRuntime *rt = ncx->runtime();

#ifdef JS_GC_ZEAL
        if (rt->gc.needZealousGC())
            rt->gc.runDebugGC();
#endif

        
        
        if (rt->hasPendingInterrupt())
            rt->gc.gcIfRequested(ncx);

        
        
        
        if (rt->gc.isIncrementalGCInProgress() &&
            cx->zone()->usage.gcBytes() > cx->zone()->threshold.gcTriggerBytes())
        {
            PrepareZoneForGC(cx->zone());
            AutoKeepAtoms keepAtoms(cx->perThreadData);
            rt->gc.gc(GC_NORMAL, JS::gcreason::INCREMENTAL_TOO_SLOW);
        }
    }

    return true;
}

template <AllowGC allowGC>
static inline bool
CheckAllocatorState(ExclusiveContext *cx, AllocKind kind)
{
    if (allowGC) {
        if (!GCIfNeeded(cx))
            return false;
    }

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
        ReportOutOfMemory(ncx);
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

    if (!CheckAllocatorState<allowGC>(cx, kind))
        return nullptr;

    if (cx->isJSContext() &&
        ShouldNurseryAllocateObject(cx->asJSContext()->nursery(), heap))
    {
        JSObject *obj = TryNewNurseryObject<allowGC>(cx->asJSContext(), thingSize, nDynamicSlots,
                                                     clasp);
        if (obj)
            return obj;

        
        
        
        
        
        if (!allowGC)
            return nullptr;
    }

    HeapSlot *slots = nullptr;
    if (nDynamicSlots) {
        slots = cx->zone()->pod_malloc<HeapSlot>(nDynamicSlots);
        if (MOZ_UNLIKELY(!slots))
            return nullptr;
        Debug_SetSlotRangeToCrashOnTouch(slots, nDynamicSlots);
    }

    JSObject *obj = reinterpret_cast<JSObject *>(cx->arenas()->allocateFromFreeList(kind, thingSize));
    if (!obj)
        obj = reinterpret_cast<JSObject *>(GCRuntime::refillFreeListFromAnyThread<allowGC>(cx, kind));

    if (obj)
        obj->setInitialSlotsMaybeNonNative(slots);
    else
        js_free(slots);

    CheckIncrementalZoneState(cx, obj);
    TraceTenuredAlloc(obj, kind);
    return obj;
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
    if (!CheckAllocatorState<allowGC>(cx, kind))
        return nullptr;

    T *t = static_cast<T *>(cx->arenas()->allocateFromFreeList(kind, thingSize));
    if (!t)
        t = static_cast<T *>(GCRuntime::refillFreeListFromAnyThread<allowGC>(cx, kind));

    CheckIncrementalZoneState(cx, t);
    gc::TraceTenuredAlloc(t, kind);
    return t;
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
