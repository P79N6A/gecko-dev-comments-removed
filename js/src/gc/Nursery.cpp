






#ifdef JSGC_GENERATIONAL

#include "gc/Nursery-inl.h"

#include "mozilla/IntegerPrintfMacros.h"

#include "jscompartment.h"
#include "jsgc.h"
#include "jsinfer.h"
#include "jsutil.h"
#include "prmjtime.h"

#include "gc/GCInternals.h"
#include "gc/Memory.h"
#include "jit/IonFrames.h"
#include "vm/ArrayObject.h"
#include "vm/Debugger.h"
#if defined(DEBUG)
#include "vm/ScopeObject.h"
#endif
#include "vm/TypedArrayObject.h"

#include "jsgcinlines.h"

#include "vm/ObjectImpl-inl.h"

using namespace js;
using namespace gc;

using mozilla::ArrayLength;
using mozilla::PodCopy;
using mozilla::PodZero;



#ifdef PROFILE_NURSERY



static int64_t GCReportThreshold = INT64_MAX;
#endif

bool
js::Nursery::init(uint32_t maxNurseryBytes)
{
    
    numNurseryChunks_ = maxNurseryBytes >> ChunkShift;

    
    if (numNurseryChunks_ == 0)
        return true;

    if (!hugeSlots.init())
        return false;

    void *heap = MapAlignedPages(nurserySize(), Alignment);
    if (!heap)
        return false;

    heapStart_ = uintptr_t(heap);
    heapEnd_ = heapStart_ + nurserySize();
    currentStart_ = start();
    numActiveChunks_ = 1;
    JS_POISON(heap, JS_FRESH_NURSERY_PATTERN, nurserySize());
    setCurrentChunk(0);
    updateDecommittedRegion();

#ifdef PROFILE_NURSERY
    char *env = getenv("JS_MINORGC_TIME");
    if (env)
        GCReportThreshold = atoi(env);
#endif

    JS_ASSERT(isEnabled());
    return true;
}

js::Nursery::~Nursery()
{
    if (start())
        UnmapPages((void *)start(), nurserySize());
}

void
js::Nursery::updateDecommittedRegion()
{
#ifndef JS_GC_ZEAL
    if (numActiveChunks_ < numNurseryChunks_) {
        
        
# ifndef XP_MACOSX
        uintptr_t decommitStart = chunk(numActiveChunks_).start();
        uintptr_t decommitSize = heapEnd() - decommitStart;
        JS_ASSERT(decommitStart == AlignBytes(decommitStart, Alignment));
        JS_ASSERT(decommitSize == AlignBytes(decommitStart, Alignment));
        MarkPagesUnused((void *)decommitStart, decommitSize);
# endif
    }
#endif
}

void
js::Nursery::enable()
{
    JS_ASSERT(isEmpty());
    if (isEnabled())
        return;
    numActiveChunks_ = 1;
    setCurrentChunk(0);
    currentStart_ = position();
#ifdef JS_GC_ZEAL
    if (runtime()->gcZeal() == ZealGenerationalGCValue)
        enterZealMode();
#endif
}

void
js::Nursery::disable()
{
    JS_ASSERT(isEmpty());
    if (!isEnabled())
        return;
    numActiveChunks_ = 0;
    currentEnd_ = 0;
    updateDecommittedRegion();
}

bool
js::Nursery::isEmpty() const
{
    JS_ASSERT(runtime_);
    if (!isEnabled())
        return true;
    JS_ASSERT_IF(runtime_->gcZeal() != ZealGenerationalGCValue, currentStart_ == start());
    return position() == currentStart_;
}

#ifdef JS_GC_ZEAL
void
js::Nursery::enterZealMode() {
    if (isEnabled())
        numActiveChunks_ = numNurseryChunks_;
}

void
js::Nursery::leaveZealMode() {
    if (isEnabled()) {
        JS_ASSERT(isEmpty());
        setCurrentChunk(0);
        currentStart_ = start();
    }
}
#endif 

JSObject *
js::Nursery::allocateObject(JSContext *cx, size_t size, size_t numDynamic)
{
    
    JS_ASSERT(size >= sizeof(RelocationOverlay));

    
    if (numDynamic && numDynamic <= MaxNurserySlots) {
        size_t totalSize = size + sizeof(HeapSlot) * numDynamic;
        JSObject *obj = static_cast<JSObject *>(allocate(totalSize));
        if (obj) {
            obj->setInitialSlots(reinterpret_cast<HeapSlot *>(size_t(obj) + size));
            TraceNurseryAlloc(obj, size);
            return obj;
        }
        
    }

    HeapSlot *slots = nullptr;
    if (numDynamic) {
        slots = allocateHugeSlots(cx, numDynamic);
        if (MOZ_UNLIKELY(!slots))
            return nullptr;
    }

    JSObject *obj = static_cast<JSObject *>(allocate(size));

    if (obj)
        obj->setInitialSlots(slots);
    else
        freeSlots(cx, slots);

    TraceNurseryAlloc(obj, size);
    return obj;
}

void *
js::Nursery::allocate(size_t size)
{
    JS_ASSERT(isEnabled());
    JS_ASSERT(!runtime()->isHeapBusy());
    JS_ASSERT(position() >= currentStart_);

    if (currentEnd() < position() + size) {
        if (currentChunk_ + 1 == numActiveChunks_)
            return nullptr;
        setCurrentChunk(currentChunk_ + 1);
    }

    void *thing = (void *)position();
    position_ = position() + size;

    JS_EXTRA_POISON(thing, JS_ALLOCATED_NURSERY_PATTERN, size);
    return thing;
}


HeapSlot *
js::Nursery::allocateSlots(JSContext *cx, JSObject *obj, uint32_t nslots)
{
    JS_ASSERT(obj);
    JS_ASSERT(nslots > 0);

    if (!IsInsideNursery(obj))
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
    JS_ASSERT(nelems >= ObjectElements::VALUES_PER_HEADER);
    return reinterpret_cast<ObjectElements *>(allocateSlots(cx, obj, nelems));
}

HeapSlot *
js::Nursery::reallocateSlots(JSContext *cx, JSObject *obj, HeapSlot *oldSlots,
                             uint32_t oldCount, uint32_t newCount)
{
    size_t oldSize = oldCount * sizeof(HeapSlot);
    size_t newSize = newCount * sizeof(HeapSlot);

    if (!IsInsideNursery(obj))
        return static_cast<HeapSlot *>(cx->realloc_(oldSlots, oldSize, newSize));

    if (!isInside(oldSlots)) {
        HeapSlot *newSlots = static_cast<HeapSlot *>(cx->realloc_(oldSlots, oldSize, newSize));
        if (newSlots && oldSlots != newSlots) {
            hugeSlots.remove(oldSlots);
            
            (void)hugeSlots.put(newSlots);
        }
        return newSlots;
    }

    
    if (newCount < oldCount)
        return oldSlots;

    HeapSlot *newSlots = allocateSlots(cx, obj, newCount);
    if (newSlots)
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

void
js::Nursery::freeSlots(JSContext *cx, HeapSlot *slots)
{
    if (!isInside(slots)) {
        hugeSlots.remove(slots);
        js_free(slots);
    }
}

HeapSlot *
js::Nursery::allocateHugeSlots(JSContext *cx, size_t nslots)
{
    HeapSlot *slots = cx->pod_malloc<HeapSlot>(nslots);
    
    if (slots)
        (void)hugeSlots.put(slots);
    return slots;
}

namespace js {
namespace gc {

class MinorCollectionTracer : public JSTracer
{
  public:
    Nursery *nursery;
    AutoTraceSession session;

    
    size_t tenuredSize;

    




    RelocationOverlay *head;
    RelocationOverlay **tail;

    
    bool savedRuntimeNeedBarrier;
    AutoDisableProxyCheck disableStrictProxyChecking;
    AutoEnterOOMUnsafeRegion oomUnsafeRegion;
    ArrayBufferVector liveArrayBuffers;

    
    MOZ_ALWAYS_INLINE void insertIntoFixupList(RelocationOverlay *entry) {
        *tail = entry;
        tail = &entry->next_;
        *tail = nullptr;
    }

    MinorCollectionTracer(JSRuntime *rt, Nursery *nursery)
      : JSTracer(rt, Nursery::MinorGCCallback, TraceWeakMapKeysValues),
        nursery(nursery),
        session(rt, MinorCollecting),
        tenuredSize(0),
        head(nullptr),
        tail(&head),
        savedRuntimeNeedBarrier(rt->needsBarrier()),
        disableStrictProxyChecking(rt)
    {
        rt->gc.incGcNumber();

        







        rt->setNeedsBarrier(false);

        




        if (rt->gc.state() != NO_INCREMENTAL) {
            for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
                if (!ArrayBufferObject::saveArrayBufferList(c, liveArrayBuffers))
                    CrashAtUnhandlableOOM("OOM while saving live array buffers");
                ArrayBufferObject::resetArrayBufferList(c);
            }
        }
    }

    ~MinorCollectionTracer() {
        runtime()->setNeedsBarrier(savedRuntimeNeedBarrier);
        if (runtime()->gc.state() != NO_INCREMENTAL)
            ArrayBufferObject::restoreArrayBufferLists(liveArrayBuffers);
    }
};

} 
} 

static AllocKind
GetObjectAllocKindForCopy(JSRuntime *rt, JSObject *obj)
{
    if (obj->is<ArrayObject>()) {
        JS_ASSERT(obj->numFixedSlots() == 0);

        
        if (!rt->gc.nursery.isInside(obj->getElementsHeader()))
            return FINALIZE_OBJECT0_BACKGROUND;

        size_t nelements = obj->getDenseCapacity();
        return GetBackgroundAllocKind(GetGCArrayKind(nelements));
    }

    if (obj->is<JSFunction>())
        return obj->as<JSFunction>().getAllocKind();

    



    if (obj->is<TypedArrayObject>() && !obj->as<TypedArrayObject>().buffer()) {
        size_t nbytes = obj->as<TypedArrayObject>().byteLength();
        return GetBackgroundAllocKind(TypedArrayObject::AllocKindForLazyBuffer(nbytes));
    }

    AllocKind kind = GetGCObjectFixedSlotsKind(obj->numFixedSlots());
    JS_ASSERT(!IsBackgroundFinalized(kind));
    JS_ASSERT(CanBeFinalizedInBackground(kind, obj->getClass()));
    return GetBackgroundAllocKind(kind);
}

void *
js::Nursery::allocateFromTenured(Zone *zone, AllocKind thingKind)
{
    void *t = zone->allocator.arenas.allocateFromFreeList(thingKind, Arena::thingSize(thingKind));
    if (t)
        return t;
    zone->allocator.arenas.checkEmptyFreeList(thingKind);
    return zone->allocator.arenas.allocateFromArena(zone, thingKind);
}

void
js::Nursery::setSlotsForwardingPointer(HeapSlot *oldSlots, HeapSlot *newSlots, uint32_t nslots)
{
    JS_ASSERT(nslots > 0);
    JS_ASSERT(isInside(oldSlots));
    JS_ASSERT(!isInside(newSlots));
    *reinterpret_cast<HeapSlot **>(oldSlots) = newSlots;
}

void
js::Nursery::setElementsForwardingPointer(ObjectElements *oldHeader, ObjectElements *newHeader,
                                          uint32_t nelems)
{
    



    if (nelems - ObjectElements::VALUES_PER_HEADER < 1)
        return;
    JS_ASSERT(isInside(oldHeader));
    JS_ASSERT(!isInside(newHeader));
    *reinterpret_cast<HeapSlot **>(oldHeader->elements()) = newHeader->elements();
}

#ifdef DEBUG
static bool IsWriteableAddress(void *ptr)
{
    volatile uint64_t *vPtr = reinterpret_cast<volatile uint64_t *>(ptr);
    *vPtr = *vPtr;
    return true;
}
#endif

void
js::Nursery::forwardBufferPointer(HeapSlot **pSlotsElems)
{
    HeapSlot *old = *pSlotsElems;

    if (!isInside(old))
        return;

    







    *pSlotsElems = *reinterpret_cast<HeapSlot **>(old);
    JS_ASSERT(!isInside(*pSlotsElems));
    JS_ASSERT(IsWriteableAddress(*pSlotsElems));
}



struct TenureCount
{
    types::TypeObject *type;
    int count;
};




struct Nursery::TenureCountCache
{
    TenureCount entries[16];

    TenureCountCache() { PodZero(this); }

    TenureCount &findEntry(types::TypeObject *type) {
        return entries[PointerHasher<types::TypeObject *, 3>::hash(type) % ArrayLength(entries)];
    }
};

void
js::Nursery::collectToFixedPoint(MinorCollectionTracer *trc, TenureCountCache &tenureCounts)
{
    for (RelocationOverlay *p = trc->head; p; p = p->next()) {
        JSObject *obj = static_cast<JSObject*>(p->forwardingAddress());
        traceObject(trc, obj);

        TenureCount &entry = tenureCounts.findEntry(obj->type());
        if (entry.type == obj->type()) {
            entry.count++;
        } else if (!entry.type) {
            entry.type = obj->type();
            entry.count = 1;
        }
    }
}

MOZ_ALWAYS_INLINE void
js::Nursery::traceObject(MinorCollectionTracer *trc, JSObject *obj)
{
    const Class *clasp = obj->getClass();
    if (clasp->trace)
        clasp->trace(trc, obj);

    if (!obj->isNative())
        return;

    if (!obj->hasEmptyElements())
        markSlots(trc, obj->getDenseElements(), obj->getDenseInitializedLength());

    HeapSlot *fixedStart, *fixedEnd, *dynStart, *dynEnd;
    obj->getSlotRange(0, obj->slotSpan(), &fixedStart, &fixedEnd, &dynStart, &dynEnd);
    markSlots(trc, fixedStart, fixedEnd);
    markSlots(trc, dynStart, dynEnd);
}

MOZ_ALWAYS_INLINE void
js::Nursery::markSlots(MinorCollectionTracer *trc, HeapSlot *vp, uint32_t nslots)
{
    markSlots(trc, vp, vp + nslots);
}

MOZ_ALWAYS_INLINE void
js::Nursery::markSlots(MinorCollectionTracer *trc, HeapSlot *vp, HeapSlot *end)
{
    for (; vp != end; ++vp)
        markSlot(trc, vp);
}

MOZ_ALWAYS_INLINE void
js::Nursery::markSlot(MinorCollectionTracer *trc, HeapSlot *slotp)
{
    if (!slotp->isObject())
        return;

    JSObject *obj = &slotp->toObject();
    if (!IsInsideNursery(obj))
        return;

    if (getForwardedPointer(&obj)) {
        slotp->unsafeGet()->setObject(*obj);
        return;
    }

    JSObject *tenured = static_cast<JSObject*>(moveToTenured(trc, obj));
    slotp->unsafeGet()->setObject(*tenured);
}

void *
js::Nursery::moveToTenured(MinorCollectionTracer *trc, JSObject *src)
{
    Zone *zone = src->zone();
    AllocKind dstKind = GetObjectAllocKindForCopy(trc->runtime(), src);
    JSObject *dst = static_cast<JSObject *>(allocateFromTenured(zone, dstKind));
    if (!dst)
        CrashAtUnhandlableOOM("Failed to allocate object while tenuring.");

    trc->tenuredSize += moveObjectToTenured(dst, src, dstKind);

    RelocationOverlay *overlay = reinterpret_cast<RelocationOverlay *>(src);
    overlay->forwardTo(dst);
    trc->insertIntoFixupList(overlay);

    TracePromoteToTenured(src, dst);
    return static_cast<void *>(dst);
}

size_t
js::Nursery::moveObjectToTenured(JSObject *dst, JSObject *src, AllocKind dstKind)
{
    size_t srcSize = Arena::thingSize(dstKind);
    size_t tenuredSize = srcSize;

    








    if (src->is<ArrayObject>())
        tenuredSize = srcSize = sizeof(ObjectImpl);

    js_memcpy(dst, src, srcSize);
    tenuredSize += moveSlotsToTenured(dst, src, dstKind);
    tenuredSize += moveElementsToTenured(dst, src, dstKind);

    if (src->is<TypedArrayObject>())
        forwardTypedArrayPointers(dst, src);

    
    if (&src->shape_ == dst->shape_->listp)
        dst->shape_->listp = &dst->shape_;

    return tenuredSize;
}

void
js::Nursery::forwardTypedArrayPointers(JSObject *dst, JSObject *src)
{
    




    TypedArrayObject &typedArray = src->as<TypedArrayObject>();
    JS_ASSERT_IF(typedArray.buffer(), !isInside(src->getPrivate()));
    if (typedArray.buffer())
        return;

    void *srcData = src->fixedData(TypedArrayObject::FIXED_DATA_START);
    void *dstData = dst->fixedData(TypedArrayObject::FIXED_DATA_START);
    JS_ASSERT(src->getPrivate() == srcData);
    dst->setPrivate(dstData);

    




    size_t nslots = 1;
    setSlotsForwardingPointer(reinterpret_cast<HeapSlot*>(srcData),
                              reinterpret_cast<HeapSlot*>(dstData),
                              nslots);
}

size_t
js::Nursery::moveSlotsToTenured(JSObject *dst, JSObject *src, AllocKind dstKind)
{
    
    if (!src->hasDynamicSlots())
        return 0;

    if (!isInside(src->slots)) {
        hugeSlots.remove(src->slots);
        return 0;
    }

    Zone *zone = src->zone();
    size_t count = src->numDynamicSlots();
    dst->slots = zone->pod_malloc<HeapSlot>(count);
    if (!dst->slots)
        CrashAtUnhandlableOOM("Failed to allocate slots while tenuring.");
    PodCopy(dst->slots, src->slots, count);
    setSlotsForwardingPointer(src->slots, dst->slots, count);
    return count * sizeof(HeapSlot);
}

size_t
js::Nursery::moveElementsToTenured(JSObject *dst, JSObject *src, AllocKind dstKind)
{
    if (src->hasEmptyElements())
        return 0;

    Zone *zone = src->zone();
    ObjectElements *srcHeader = src->getElementsHeader();
    ObjectElements *dstHeader;

    
    if (!isInside(srcHeader)) {
        JS_ASSERT(src->elements == dst->elements);
        hugeSlots.remove(reinterpret_cast<HeapSlot*>(srcHeader));
        return 0;
    }

    size_t nslots = ObjectElements::VALUES_PER_HEADER + srcHeader->capacity;

    
    if (src->is<ArrayObject>() && nslots <= GetGCKindSlots(dstKind)) {
        dst->setFixedElements();
        dstHeader = dst->getElementsHeader();
        js_memcpy(dstHeader, srcHeader, nslots * sizeof(HeapSlot));
        setElementsForwardingPointer(srcHeader, dstHeader, nslots);
        return nslots * sizeof(HeapSlot);
    }

    JS_ASSERT(nslots >= 2);
    size_t nbytes = nslots * sizeof(HeapValue);
    dstHeader = static_cast<ObjectElements *>(zone->malloc_(nbytes));
    if (!dstHeader)
        CrashAtUnhandlableOOM("Failed to allocate elements while tenuring.");
    js_memcpy(dstHeader, srcHeader, nslots * sizeof(HeapSlot));
    setElementsForwardingPointer(srcHeader, dstHeader, nslots);
    dst->elements = dstHeader->elements();
    return nslots * sizeof(HeapSlot);
}

static bool
ShouldMoveToTenured(MinorCollectionTracer *trc, void **thingp)
{
    Cell *cell = static_cast<Cell *>(*thingp);
    Nursery &nursery = *trc->nursery;
    return !nursery.isInside(thingp) && IsInsideNursery(cell) &&
           !nursery.getForwardedPointer(thingp);
}

 void
js::Nursery::MinorGCCallback(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    MinorCollectionTracer *trc = static_cast<MinorCollectionTracer *>(jstrc);
    if (ShouldMoveToTenured(trc, thingp))
        *thingp = trc->nursery->moveToTenured(trc, static_cast<JSObject *>(*thingp));
}

static void
CheckHashTablesAfterMovingGC(JSRuntime *rt)
{
#ifdef JS_GC_ZEAL
    if (rt->gcZeal() == ZealCheckHashTablesOnMinorGC) {
        
        for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
            c->checkNewTypeObjectTableAfterMovingGC();
            c->checkInitialShapesTableAfterMovingGC();
            c->checkWrapperMapAfterMovingGC();
            if (c->debugScopes)
                c->debugScopes->checkHashTablesAfterMovingGC(rt);
        }
    }
#endif
}

#ifdef PROFILE_NURSERY
#define TIME_START(name) int64_t timstampStart_##name = PRMJ_Now()
#define TIME_END(name) int64_t timstampEnd_##name = PRMJ_Now()
#define TIME_TOTAL(name) (timstampEnd_##name - timstampStart_##name)
#else
#define TIME_START(name)
#define TIME_END(name)
#define TIME_TOTAL(name)
#endif

void
js::Nursery::collect(JSRuntime *rt, JS::gcreason::Reason reason, TypeObjectList *pretenureTypes)
{
    if (rt->mainThread.suppressGC)
        return;

    JS_AbortIfWrongThread(rt);

    StoreBuffer &sb = rt->gc.storeBuffer;
    if (!isEnabled() || isEmpty()) {
        





        sb.clear();
        return;
    }

    rt->gc.stats.count(gcstats::STAT_MINOR_GC);

    TraceMinorGCStart();

    TIME_START(total);

    AutoStopVerifyingBarriers av(rt, false);

    
    MinorCollectionTracer trc(rt, this);

    
    TIME_START(markValues);
    sb.markValues(&trc);
    TIME_END(markValues);

    TIME_START(markCells);
    sb.markCells(&trc);
    TIME_END(markCells);

    TIME_START(markSlots);
    sb.markSlots(&trc);
    TIME_END(markSlots);

    TIME_START(markWholeCells);
    sb.markWholeCells(&trc);
    TIME_END(markWholeCells);

    TIME_START(markRelocatableValues);
    sb.markRelocatableValues(&trc);
    TIME_END(markRelocatableValues);

    TIME_START(markRelocatableCells);
    sb.markRelocatableCells(&trc);
    TIME_END(markRelocatableCells);

    TIME_START(markGenericEntries);
    sb.markGenericEntries(&trc);
    TIME_END(markGenericEntries);

    TIME_START(checkHashTables);
    CheckHashTablesAfterMovingGC(rt);
    TIME_END(checkHashTables);

    TIME_START(markRuntime);
    rt->gc.markRuntime(&trc);
    TIME_END(markRuntime);

    TIME_START(markDebugger);
    Debugger::markAll(&trc);
    TIME_END(markDebugger);

    TIME_START(clearNewObjectCache);
    rt->newObjectCache.clearNurseryObjects(rt);
    TIME_END(clearNewObjectCache);

    TIME_START(clearRegExpTestCache);
    rt->regExpTestCache.purge();
    TIME_END(clearRegExpTestCache);

    
    
    
    
    TIME_START(collectToFP);
    TenureCountCache tenureCounts;
    collectToFixedPoint(&trc, tenureCounts);
    TIME_END(collectToFP);

    
    TIME_START(sweepArrayBufferViewList);
    for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
        if (!c->gcLiveArrayBuffers.empty())
            ArrayBufferObject::sweep(c);
    }
    TIME_END(sweepArrayBufferViewList);

    
    TIME_START(updateJitActivations);
    js::jit::UpdateJitActivationsForMinorGC<Nursery>(&rt->mainThread, &trc);
    TIME_END(updateJitActivations);

    
    TIME_START(resize);
    double promotionRate = trc.tenuredSize / double(allocationEnd() - start());
    if (promotionRate > 0.05)
        growAllocableSpace();
    else if (promotionRate < 0.01)
        shrinkAllocableSpace();
    TIME_END(resize);

    
    
    
    
    TIME_START(pretenure);
    if (pretenureTypes && (promotionRate > 0.8 || reason == JS::gcreason::FULL_STORE_BUFFER)) {
        for (size_t i = 0; i < ArrayLength(tenureCounts.entries); i++) {
            const TenureCount &entry = tenureCounts.entries[i];
            if (entry.count >= 3000)
                pretenureTypes->append(entry.type); 
        }
    }
    TIME_END(pretenure);

    
    TIME_START(freeHugeSlots);
    freeHugeSlots();
    TIME_END(freeHugeSlots);

    TIME_START(sweep);
    sweep();
    TIME_END(sweep);

    TIME_START(clearStoreBuffer);
    rt->gc.storeBuffer.clear();
    TIME_END(clearStoreBuffer);

    
    
    
    if (rt->gc.usage.gcBytes() >= rt->gc.tunables.gcMaxBytes())
        disable();

    TIME_END(total);

    TraceMinorGCEnd();

#ifdef PROFILE_NURSERY
    int64_t totalTime = TIME_TOTAL(total);

    if (totalTime >= GCReportThreshold) {
        static bool printedHeader = false;
        if (!printedHeader) {
            fprintf(stderr,
                    "MinorGC: Reason               PRate  Size Time   mkVals mkClls mkSlts mkWCll mkRVal mkRCll mkGnrc ckTbls mkRntm mkDbgr clrNOC collct swpABO updtIn resize pretnr frSlts clrSB  sweep\n");
            printedHeader = true;
        }

#define FMT " %6" PRIu64
        fprintf(stderr,
                "MinorGC: %20s %5.1f%% %4d" FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT "\n",
                js::gcstats::ExplainReason(reason),
                promotionRate * 100,
                numActiveChunks_,
                totalTime,
                TIME_TOTAL(markValues),
                TIME_TOTAL(markCells),
                TIME_TOTAL(markSlots),
                TIME_TOTAL(markWholeCells),
                TIME_TOTAL(markRelocatableValues),
                TIME_TOTAL(markRelocatableCells),
                TIME_TOTAL(markGenericEntries),
                TIME_TOTAL(checkHashTables),
                TIME_TOTAL(markRuntime),
                TIME_TOTAL(markDebugger),
                TIME_TOTAL(clearNewObjectCache),
                TIME_TOTAL(collectToFP),
                TIME_TOTAL(sweepArrayBufferViewList),
                TIME_TOTAL(updateJitActivations),
                TIME_TOTAL(resize),
                TIME_TOTAL(pretenure),
                TIME_TOTAL(freeHugeSlots),
                TIME_TOTAL(clearStoreBuffer),
                TIME_TOTAL(sweep));
#undef FMT
    }
#endif
}

#undef TIME_START
#undef TIME_END
#undef TIME_TOTAL

void
js::Nursery::freeHugeSlots()
{
    FreeOp *fop = runtime()->defaultFreeOp();
    for (HugeSlotsSet::Range r = hugeSlots.all(); !r.empty(); r.popFront())
        fop->free_(r.front());
    hugeSlots.clear();
}

void
js::Nursery::sweep()
{
#ifdef JS_GC_ZEAL
    
    JS_POISON((void *)start(), JS_SWEPT_NURSERY_PATTERN, nurserySize());
    for (int i = 0; i < numNurseryChunks_; ++i)
        initChunk(i);

    if (runtime()->gcZeal() == ZealGenerationalGCValue) {
        MOZ_ASSERT(numActiveChunks_ == numNurseryChunks_);

        
        if (currentChunk_ + 1 == numNurseryChunks_)
            setCurrentChunk(0);
    } else
#endif
    {
#ifdef JS_CRASH_DIAGNOSTICS
        JS_POISON((void *)start(), JS_SWEPT_NURSERY_PATTERN, allocationEnd() - start());
        for (int i = 0; i < numActiveChunks_; ++i)
            chunk(i).trailer.runtime = runtime();
#endif
        setCurrentChunk(0);
    }

    
    currentStart_ = position();
}

void
js::Nursery::growAllocableSpace()
{
#ifdef JS_GC_ZEAL
    MOZ_ASSERT_IF(runtime()->gcZeal() == ZealGenerationalGCValue,
                  numActiveChunks_ == numNurseryChunks_);
#endif
    numActiveChunks_ = Min(numActiveChunks_ * 2, numNurseryChunks_);
}

void
js::Nursery::shrinkAllocableSpace()
{
#ifdef JS_GC_ZEAL
    if (runtime()->gcZeal() == ZealGenerationalGCValue)
        return;
#endif
    numActiveChunks_ = Max(numActiveChunks_ - 1, 1);
    updateDecommittedRegion();
}

#endif 
