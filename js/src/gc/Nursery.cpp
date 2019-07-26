






#ifdef JSGC_GENERATIONAL

#include "jscompartment.h"
#include "jsgc.h"

#include "gc/GCInternals.h"
#include "gc/Memory.h"
#include "vm/Debugger.h"

#include "gc/Barrier-inl.h"
#include "gc/Nursery-inl.h"

using namespace js;
using namespace gc;
using namespace mozilla;

bool
js::Nursery::enable()
{
    if (isEnabled())
        return true;

    if (!hugeSlots.init())
        return false;

    fallbackBitmap.clear(false);

    void *heap = MapAlignedPages(NurserySize, Alignment);
    if (!heap)
        return false;

    JSRuntime *rt = runtime();
    rt->gcNurseryStart_ = position_ = uintptr_t(heap);
    rt->gcNurseryEnd_ = start() + NurseryUsableSize;
    asLayout().runtime = rt;
    JS_POISON(asLayout().data, FreshNursery, sizeof(asLayout().data));

    JS_ASSERT(isEnabled());
    return true;
}

void
js::Nursery::disable()
{
    if (!isEnabled())
        return;

    hugeSlots.finish();
    JS_ASSERT(start());
    UnmapPages((void *)start(), NurserySize);
    runtime()->gcNurseryStart_ = runtime()->gcNurseryEnd_ = position_ = 0;
}

js::Nursery::~Nursery()
{
    disable();
}

void *
js::Nursery::allocate(size_t size)
{
    JS_ASSERT(size % ThingAlignment == 0);
    JS_ASSERT(position() % ThingAlignment == 0);

    if (position() + size > end())
        return NULL;

    void *thing = (void *)position();
    position_ = position() + size;

    JS_POISON(thing, AllocatedThing, size);
    return thing;
}


HeapSlot *
js::Nursery::allocateSlots(JSContext *cx, JSObject *obj, uint32_t nslots)
{
    JS_ASSERT(obj);
    JS_ASSERT(nslots > 0);

    if (!isInside(obj))
        return cx->pod_malloc<HeapSlot>(nslots);

    if (nslots > MaxNurserySlots)
        return allocateHugeSlots(cx, nslots);

    size_t size = sizeof(HeapSlot) * nslots;
    HeapSlot *slots = static_cast<HeapSlot *>(allocate(size));
    if (slots)
        return slots;

    return allocateHugeSlots(cx, nslots);
}

ObjectElements *
js::Nursery::allocateElements(JSContext *cx, JSObject *obj, uint32_t nelems)
{
    return reinterpret_cast<ObjectElements *>(allocateSlots(cx, obj, nelems));
}

HeapSlot *
js::Nursery::reallocateSlots(JSContext *cx, JSObject *obj, HeapSlot *oldSlots,
                             uint32_t oldCount, uint32_t newCount)
{
    size_t oldSize = oldCount * sizeof(HeapSlot);
    size_t newSize = newCount * sizeof(HeapSlot);

    if (!isInside(obj))
        return static_cast<HeapSlot *>(cx->realloc_(oldSlots, oldSize, newSize));

    if (!isInside(oldSlots)) {
        HeapSlot *newSlots = static_cast<HeapSlot *>(cx->realloc_(oldSlots, oldSize, newSize));
        if (oldSlots != newSlots) {
            hugeSlots.remove(oldSlots);
            
            (void)hugeSlots.put(newSlots);
        }
        return newSlots;
    }

    
    if (newCount < oldCount)
        return oldSlots;

    HeapSlot *newSlots = allocateSlots(cx, obj, newCount);
    PodCopy(newSlots, oldSlots, oldCount);
    return newSlots;
}

ObjectElements *
js::Nursery::reallocateElements(JSContext *cx, JSObject *obj, ObjectElements *oldHeader,
                                uint32_t oldCount, uint32_t newCount)
{
    HeapSlot *slots = reallocateSlots(cx, obj, reinterpret_cast<HeapSlot *>(oldHeader),
                                      oldCount, newCount);
    return reinterpret_cast<ObjectElements *>(slots);
}

HeapSlot *
js::Nursery::allocateHugeSlots(JSContext *cx, size_t nslots)
{
    HeapSlot *slots = cx->pod_malloc<HeapSlot>(nslots);
    
    (void)hugeSlots.put(slots);
    return slots;
}

void
js::Nursery::notifyInitialSlots(Cell *cell, HeapSlot *slots)
{
    if (isInside(cell) && !isInside(slots)) {
        
        (void)hugeSlots.put(slots);
    }
}

namespace js {
namespace gc {

class MinorCollectionTracer : public JSTracer
{
  public:
    Nursery *nursery;
    JSRuntime *runtime;
    AutoTraceSession session;

    




    RelocationOverlay *head;
    RelocationOverlay **tail;

    
    bool savedNeedsBarrier;
    AutoDisableProxyCheck disableStrictProxyChecking;

    
    JS_ALWAYS_INLINE void insertIntoFixupList(RelocationOverlay *entry) {
        *tail = entry;
        tail = &entry->next_;
        *tail = NULL;
    }

    MinorCollectionTracer(JSRuntime *rt, Nursery *nursery)
      : JSTracer(),
        nursery(nursery),
        runtime(rt),
        session(runtime, MinorCollecting),
        head(NULL),
        tail(&head),
        savedNeedsBarrier(runtime->needsBarrier()),
        disableStrictProxyChecking(runtime)
    {
        JS_TracerInit(this, runtime, Nursery::MinorGCCallback);
        eagerlyTraceWeakMaps = TraceWeakMapKeysValues;

        runtime->gcNumber++;
        runtime->setNeedsBarrier(false);
        for (ZonesIter zone(rt); !zone.done(); zone.next())
            zone->saveNeedsBarrier(false);
    }

    ~MinorCollectionTracer() {
        runtime->setNeedsBarrier(savedNeedsBarrier);
        for (ZonesIter zone(runtime); !zone.done(); zone.next())
            zone->restoreNeedsBarrier();
    }
};

} 
} 

static AllocKind
GetObjectAllocKindForCopy(JSObject *obj)
{
    if (obj->isArray()) {
        JS_ASSERT(obj->numFixedSlots() == 0);
        size_t nelements = obj->getDenseInitializedLength();
        return GetBackgroundAllocKind(GetGCArrayKind(nelements));
    }

    if (obj->isFunction())
        return obj->toFunction()->getAllocKind();

    AllocKind kind = GetGCObjectFixedSlotsKind(obj->numFixedSlots());
    if (CanBeFinalizedInBackground(kind, obj->getClass()))
        kind = GetBackgroundAllocKind(kind);
    return kind;
}

void *
js::Nursery::allocateFromTenured(Zone *zone, AllocKind thingKind)
{
    void *t = zone->allocator.arenas.allocateFromFreeList(thingKind, Arena::thingSize(thingKind));
    if (!t) {
        zone->allocator.arenas.checkEmptyFreeList(thingKind);
        t = zone->allocator.arenas.allocateFromArena(zone, thingKind);
    }

    



    if (zone->savedNeedsBarrier())
        static_cast<Cell *>(t)->markIfUnmarked();

    return t;
}

void *
js::Nursery::moveToTenured(MinorCollectionTracer *trc, JSObject *src)
{
    Zone *zone = src->zone();
    AllocKind dstKind = GetObjectAllocKindForCopy(src);
    JSObject *dst = static_cast<JSObject *>(allocateFromTenured(zone, dstKind));
    if (!dst)
        MOZ_CRASH();

    moveObjectToTenured(dst, src, dstKind);

    RelocationOverlay *overlay = reinterpret_cast<RelocationOverlay *>(src);
    overlay->forwardTo(dst);
    trc->insertIntoFixupList(overlay);

    return static_cast<void *>(dst);
}

void
js::Nursery::moveObjectToTenured(JSObject *dst, JSObject *src, AllocKind dstKind)
{
    size_t srcSize = Arena::thingSize(dstKind);

    




    if (src->isArray())
        srcSize = sizeof(ObjectImpl);

    js_memcpy(dst, src, srcSize);
    moveSlotsToTenured(dst, src, dstKind);
    moveElementsToTenured(dst, src, dstKind);

    
    if (&src->shape_ == dst->shape_->listp)
        dst->shape_->listp = &dst->shape_;
}

void
js::Nursery::moveSlotsToTenured(JSObject *dst, JSObject *src, AllocKind dstKind)
{
    
    if (!src->hasDynamicSlots())
        return;

    if (!isInside(src->slots)) {
        hugeSlots.remove(src->slots);
        return;
    }

    Allocator *alloc = &src->zone()->allocator;
    size_t count = src->numDynamicSlots();
    dst->slots = alloc->pod_malloc<HeapSlot>(count);
    PodCopy(dst->slots, src->slots, count);
}

void
js::Nursery::moveElementsToTenured(JSObject *dst, JSObject *src, AllocKind dstKind)
{
    if (src->hasEmptyElements())
        return;

    Allocator *alloc = &src->zone()->allocator;
    ObjectElements *srcHeader = src->getElementsHeader();
    ObjectElements *dstHeader;

    if (!isInside(srcHeader)) {
        JS_ASSERT(src->elements == dst->elements);
        hugeSlots.remove(reinterpret_cast<HeapSlot*>(srcHeader));
        return;
    }

    
    if (src->isArrayBuffer()) {
        size_t nbytes = sizeof(ObjectElements) + srcHeader->initializedLength;
        if (src->hasDynamicElements()) {
            dstHeader = static_cast<ObjectElements *>(alloc->malloc_(nbytes));
            if (!dstHeader)
                MOZ_CRASH();
        } else {
            dst->setFixedElements();
            dstHeader = dst->getElementsHeader();
        }
        js_memcpy(dstHeader, srcHeader, nbytes);
        dst->elements = dstHeader->elements();
        return;
    }

    size_t nslots = ObjectElements::VALUES_PER_HEADER + srcHeader->initializedLength;

    
    if (src->isArray() && nslots <= GetGCKindSlots(dstKind)) {
        dst->setFixedElements();
        dstHeader = dst->getElementsHeader();
        js_memcpy(dstHeader, srcHeader, nslots * sizeof(HeapSlot));
        dstHeader->capacity = GetGCKindSlots(dstKind) - ObjectElements::VALUES_PER_HEADER;
        return;
    }

    size_t nbytes = nslots * sizeof(HeapValue);
    dstHeader = static_cast<ObjectElements *>(alloc->malloc_(nbytes));
    if (!dstHeader)
        MOZ_CRASH();
    js_memcpy(dstHeader, srcHeader, nslots * sizeof(HeapSlot));
    dstHeader->capacity = srcHeader->initializedLength;
    dst->elements = dstHeader->elements();
}

static bool
ShouldMoveToTenured(MinorCollectionTracer *trc, void **thingp)
{
    Cell *cell = static_cast<Cell *>(*thingp);
    Nursery &nursery = *trc->nursery;
    return !nursery.isInside(thingp) && nursery.isInside(cell) &&
           !nursery.getForwardedPointer(thingp);
}

 void
js::Nursery::MinorGCCallback(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    MinorCollectionTracer *trc = static_cast<MinorCollectionTracer *>(jstrc);
    if (ShouldMoveToTenured(trc, thingp))
        *thingp = trc->nursery->moveToTenured(trc, static_cast<JSObject *>(*thingp));
}

void
js::Nursery::markFallback(Cell *cell)
{
    JS_ASSERT(uintptr_t(cell) >= start());
    size_t offset = uintptr_t(cell) - start();
    JS_ASSERT(offset < end() - start());
    JS_ASSERT(offset % ThingAlignment == 0);
    fallbackBitmap.set(offset / ThingAlignment);
}

void
js::Nursery::moveFallbackToTenured(gc::MinorCollectionTracer *trc)
{
    for (size_t i = 0; i < FallbackBitmapBits; ++i) {
        if (fallbackBitmap.get(i)) {
            JSObject *src = reinterpret_cast<JSObject *>(start() + i * ThingAlignment);
            moveToTenured(trc, src);
        }
    }
    fallbackBitmap.clear(false);
}

 void
js::Nursery::MinorFallbackMarkingCallback(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    MinorCollectionTracer *trc = static_cast<MinorCollectionTracer *>(jstrc);
    if (ShouldMoveToTenured(trc, thingp))
        trc->nursery->markFallback(static_cast<JSObject *>(*thingp));
}

 void
js::Nursery::MinorFallbackFixupCallback(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    MinorCollectionTracer *trc = static_cast<MinorCollectionTracer *>(jstrc);
    if (trc->nursery->isInside(*thingp))
        trc->nursery->getForwardedPointer(thingp);
}

static void
TraceHeapWithCallback(JSTracer *trc, JSTraceCallback callback)
{
    JSTraceCallback prior = trc->callback;

    AutoCopyFreeListToArenas copy(trc->runtime);
    trc->callback = callback;
    for (ZonesIter zone(trc->runtime); !zone.done(); zone.next()) {
        for (size_t i = 0; i < FINALIZE_LIMIT; ++i) {
            AllocKind kind = AllocKind(i);
            for (CellIterUnderGC cells(zone, kind); !cells.done(); cells.next())
                JS_TraceChildren(trc, cells.getCell(), MapAllocToTraceKind(kind));
        }
    }

    trc->callback = prior;
}

void
js::Nursery::markStoreBuffer(MinorCollectionTracer *trc)
{
    JSRuntime *rt = trc->runtime;
    if (!rt->gcStoreBuffer.hasOverflowed()) {
        rt->gcStoreBuffer.mark(trc);
        return;
    }

    










    TraceHeapWithCallback(trc, MinorFallbackMarkingCallback);
    moveFallbackToTenured(trc);
    TraceHeapWithCallback(trc, MinorFallbackFixupCallback);
}

void
js::Nursery::collect(JSRuntime *rt, JS::gcreason::Reason reason)
{
    JS_AbortIfWrongThread(rt);

    if (!isEnabled())
        return;

    if (position() == start())
        return;

    rt->gcHelperThread.waitBackgroundSweepEnd();

    
    MinorCollectionTracer trc(rt, this);
    MarkRuntime(&trc);
    Debugger::markAll(&trc);
    for (CompartmentsIter comp(rt); !comp.done(); comp.next()) {
        comp->markAllCrossCompartmentWrappers(&trc);
        comp->markAllInitialShapeTableEntries(&trc);
    }
    markStoreBuffer(&trc);

    





    for (RelocationOverlay *p = trc.head; p; p = p->next()) {
        JSObject *obj = static_cast<JSObject*>(p->forwardingAddress());
        JS_TraceChildren(&trc, obj, MapAllocToTraceKind(obj->tenuredGetAllocKind()));
    }

    
    sweep(rt->defaultFreeOp());
    rt->gcStoreBuffer.clear();

    




    if (rt->gcBytes >= rt->gcMaxBytes)
        disable();
}


void
js::Nursery::sweep(FreeOp *fop)
{
    for (HugeSlotsSet::Range r = hugeSlots.all(); !r.empty(); r.popFront())
        fop->free_(r.front());
    hugeSlots.clear();

    JS_POISON((void *)start(), SweptNursery, NurserySize - sizeof(JSRuntime *));

    position_ = start();
}

#endif 
