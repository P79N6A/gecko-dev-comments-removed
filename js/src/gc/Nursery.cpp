






#include "gc/Nursery-inl.h"

#include "mozilla/IntegerPrintfMacros.h"
#include "mozilla/Move.h"

#include "jscompartment.h"
#include "jsgc.h"
#include "jsutil.h"
#include "prmjtime.h"

#include "gc/GCInternals.h"
#include "gc/Memory.h"
#include "jit/JitFrames.h"
#include "vm/ArrayObject.h"
#include "vm/Debugger.h"
#if defined(DEBUG)
#include "vm/ScopeObject.h"
#endif
#include "vm/TypedArrayObject.h"
#include "vm/TypeInference.h"

#include "jsobjinlines.h"

#include "vm/NativeObject-inl.h"

using namespace js;
using namespace gc;

using mozilla::ArrayLength;
using mozilla::PodCopy;
using mozilla::PodZero;

struct js::Nursery::FreeHugeSlotsTask : public GCParallelTask
{
    explicit FreeHugeSlotsTask(FreeOp* fop) : fop_(fop) {}
    bool init() { return slots_.init(); }
    void transferSlotsToFree(HugeSlotsSet& slotsToFree);
    ~FreeHugeSlotsTask() override { join(); }

  private:
    FreeOp* fop_;
    HugeSlotsSet slots_;

    virtual void run() override;
};

bool
js::Nursery::init(uint32_t maxNurseryBytes)
{
    
    numNurseryChunks_ = maxNurseryBytes >> ChunkShift;

    
    if (numNurseryChunks_ == 0)
        return true;

    if (!hugeSlots.init())
        return false;

    void* heap = MapAlignedPages(nurserySize(), Alignment);
    if (!heap)
        return false;

    freeHugeSlotsTask = js_new<FreeHugeSlotsTask>(runtime()->defaultFreeOp());
    if (!freeHugeSlotsTask || !freeHugeSlotsTask->init())
        return false;

    heapStart_ = uintptr_t(heap);
    heapEnd_ = heapStart_ + nurserySize();
    currentStart_ = start();
    numActiveChunks_ = 1;
    JS_POISON(heap, JS_FRESH_NURSERY_PATTERN, nurserySize());
    setCurrentChunk(0);
    updateDecommittedRegion();

    char* env = getenv("JS_GC_PROFILE_NURSERY");
    if (env) {
        if (0 == strcmp(env, "help")) {
            fprintf(stderr, "JS_GC_PROFILE_NURSERY=N\n\n"
                    "\tReport minor GC's taking more than N microseconds.");
            exit(0);
        }
        enableProfiling_ = true;
        profileThreshold_ = atoi(env);
    }

    MOZ_ASSERT(isEnabled());
    return true;
}

js::Nursery::~Nursery()
{
    if (start())
        UnmapPages((void*)start(), nurserySize());

    js_delete(freeHugeSlotsTask);
}

void
js::Nursery::updateDecommittedRegion()
{
#ifndef JS_GC_ZEAL
    if (numActiveChunks_ < numNurseryChunks_) {
        
        
# ifndef XP_MACOSX
        uintptr_t decommitStart = chunk(numActiveChunks_).start();
        uintptr_t decommitSize = heapEnd() - decommitStart;
        MOZ_ASSERT(decommitStart == AlignBytes(decommitStart, Alignment));
        MOZ_ASSERT(decommitSize == AlignBytes(decommitStart, Alignment));
        MarkPagesUnused((void*)decommitStart, decommitSize);
# endif
    }
#endif
}

void
js::Nursery::enable()
{
    MOZ_ASSERT(isEmpty());
    MOZ_ASSERT(!runtime()->gc.isVerifyPreBarriersEnabled());
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
    MOZ_ASSERT(isEmpty());
    if (!isEnabled())
        return;
    numActiveChunks_ = 0;
    currentEnd_ = 0;
    updateDecommittedRegion();
}

bool
js::Nursery::isEmpty() const
{
    MOZ_ASSERT(runtime_);
    if (!isEnabled())
        return true;
    MOZ_ASSERT_IF(runtime_->gcZeal() != ZealGenerationalGCValue, currentStart_ == start());
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
        MOZ_ASSERT(isEmpty());
        setCurrentChunk(0);
        currentStart_ = start();
    }
}
#endif 

void
js::Nursery::verifyFinalizerList()
{
#ifdef DEBUG
    for (ListItem* current = finalizers_; current; current = current->next()) {
        JSObject* obj = current->get();
        RelocationOverlay* overlay = RelocationOverlay::fromCell(obj);
        if (overlay->isForwarded())
            obj = static_cast<JSObject*>(overlay->forwardingAddress());
        MOZ_ASSERT(obj);
        MOZ_ASSERT(obj->group());
        MOZ_ASSERT(obj->group()->clasp());
        MOZ_ASSERT(obj->group()->clasp()->finalize);
        MOZ_ASSERT(obj->group()->clasp()->flags & JSCLASS_FINALIZE_FROM_NURSERY);
    }
#endif 
}

JSObject*
js::Nursery::allocateObject(JSContext* cx, size_t size, size_t numDynamic, const js::Class* clasp)
{
    
    MOZ_ASSERT(size >= sizeof(RelocationOverlay));
    verifyFinalizerList();

    
    ListItem* listEntry = nullptr;
    if (clasp->finalize) {
        listEntry = static_cast<ListItem*>(allocate(sizeof(ListItem)));
        if (!listEntry)
            return nullptr;
    }

    
    JSObject* obj = static_cast<JSObject*>(allocate(size));
    if (!obj)
        return nullptr;

    
    HeapSlot* slots = nullptr;
    if (numDynamic) {
        
        if (numDynamic <= MaxNurserySlots)
            slots = static_cast<HeapSlot*>(allocate(numDynamic * sizeof(HeapSlot)));

        
        if (!slots)
            slots = allocateHugeSlots(cx->zone(), numDynamic);

        

        if (!slots)
            return nullptr;
    }

    
    obj->setInitialSlotsMaybeNonNative(slots);

    
    if (clasp->finalize) {
        MOZ_ASSERT(listEntry);
        new (listEntry) ListItem(finalizers_, obj);
        finalizers_ = listEntry;
    }

    TraceNurseryAlloc(obj, size);
    return obj;
}

void*
js::Nursery::allocate(size_t size)
{
    MOZ_ASSERT(isEnabled());
    MOZ_ASSERT(!runtime()->isHeapBusy());
    MOZ_ASSERT(position() >= currentStart_);

    if (currentEnd() < position() + size) {
        if (currentChunk_ + 1 == numActiveChunks_)
            return nullptr;
        setCurrentChunk(currentChunk_ + 1);
    }

    void* thing = (void*)position();
    position_ = position() + size;

    JS_EXTRA_POISON(thing, JS_ALLOCATED_NURSERY_PATTERN, size);
    return thing;
}


HeapSlot*
js::Nursery::allocateSlots(JSObject* obj, uint32_t nslots)
{
    MOZ_ASSERT(obj);
    MOZ_ASSERT(nslots > 0);

    if (!IsInsideNursery(obj))
        return obj->zone()->pod_malloc<HeapSlot>(nslots);

    if (nslots > MaxNurserySlots)
        return allocateHugeSlots(obj->zone(), nslots);

    size_t size = sizeof(HeapSlot) * nslots;
    HeapSlot* slots = static_cast<HeapSlot*>(allocate(size));
    if (slots)
        return slots;

    return allocateHugeSlots(obj->zone(), nslots);
}

ObjectElements*
js::Nursery::allocateElements(JSObject* obj, uint32_t nelems)
{
    MOZ_ASSERT(nelems >= ObjectElements::VALUES_PER_HEADER);
    return reinterpret_cast<ObjectElements*>(allocateSlots(obj, nelems));
}

HeapSlot*
js::Nursery::reallocateSlots(JSObject* obj, HeapSlot* oldSlots,
                             uint32_t oldCount, uint32_t newCount)
{
    if (!IsInsideNursery(obj))
        return obj->zone()->pod_realloc<HeapSlot>(oldSlots, oldCount, newCount);

    if (!isInside(oldSlots)) {
        HeapSlot* newSlots = obj->zone()->pod_realloc<HeapSlot>(oldSlots, oldCount, newCount);
        if (newSlots && oldSlots != newSlots) {
            hugeSlots.remove(oldSlots);
            
            (void)hugeSlots.put(newSlots);
        }
        return newSlots;
    }

    
    if (newCount < oldCount)
        return oldSlots;

    HeapSlot* newSlots = allocateSlots(obj, newCount);
    if (newSlots)
        PodCopy(newSlots, oldSlots, oldCount);
    return newSlots;
}

ObjectElements*
js::Nursery::reallocateElements(JSObject* obj, ObjectElements* oldHeader,
                                uint32_t oldCount, uint32_t newCount)
{
    HeapSlot* slots = reallocateSlots(obj, reinterpret_cast<HeapSlot*>(oldHeader),
                                      oldCount, newCount);
    return reinterpret_cast<ObjectElements*>(slots);
}

void
js::Nursery::freeSlots(HeapSlot* slots)
{
    if (!isInside(slots)) {
        hugeSlots.remove(slots);
        js_free(slots);
    }
}

HeapSlot*
js::Nursery::allocateHugeSlots(JS::Zone* zone, size_t nslots)
{
    HeapSlot* slots = zone->pod_malloc<HeapSlot>(nslots);
    
    if (slots)
        (void)hugeSlots.put(slots);
    return slots;
}

namespace js {
namespace gc {

class MinorCollectionTracer : public JS::CallbackTracer
{
  public:
    Nursery* nursery;
    AutoTraceSession session;

    
    size_t tenuredSize;

    




    RelocationOverlay* head;
    RelocationOverlay** tail;

    
    bool savedRuntimeNeedBarrier;
    AutoDisableProxyCheck disableStrictProxyChecking;
    AutoEnterOOMUnsafeRegion oomUnsafeRegion;

    
    MOZ_ALWAYS_INLINE void insertIntoFixupList(RelocationOverlay* entry) {
        *tail = entry;
        tail = &entry->next_;
        *tail = nullptr;
    }

    MinorCollectionTracer(JSRuntime* rt, Nursery* nursery)
      : JS::CallbackTracer(rt, Nursery::MinorGCCallback, TraceWeakMapKeysValues),
        nursery(nursery),
        session(rt, MinorCollecting),
        tenuredSize(0),
        head(nullptr),
        tail(&head),
        savedRuntimeNeedBarrier(rt->needsIncrementalBarrier()),
        disableStrictProxyChecking(rt)
    {
        rt->gc.incGcNumber();

        








        rt->setNeedsIncrementalBarrier(false);
    }

    ~MinorCollectionTracer() {
        runtime()->setNeedsIncrementalBarrier(savedRuntimeNeedBarrier);
    }
};

} 
} 

MOZ_ALWAYS_INLINE TenuredCell*
js::Nursery::allocateFromTenured(Zone* zone, AllocKind thingKind)
{
    TenuredCell* t = zone->arenas.allocateFromFreeList(thingKind, Arena::thingSize(thingKind));
    if (t)
        return t;
    zone->arenas.checkEmptyFreeList(thingKind);
    AutoMaybeStartBackgroundAllocation maybeStartBackgroundAllocation;
    return zone->arenas.allocateFromArena(zone, thingKind, maybeStartBackgroundAllocation);
}

void
Nursery::setForwardingPointer(void* oldData, void* newData, bool direct)
{
    MOZ_ASSERT(isInside(oldData));
    MOZ_ASSERT(!isInside(newData));

    if (direct) {
        *reinterpret_cast<void**>(oldData) = newData;
    } else {
        if (!forwardedBuffers.initialized() && !forwardedBuffers.init())
            CrashAtUnhandlableOOM("Nursery::setForwardingPointer");
#ifdef DEBUG
        if (ForwardedBufferMap::Ptr p = forwardedBuffers.lookup(oldData))
            MOZ_ASSERT(p->value() == newData);
#endif
        if (!forwardedBuffers.put(oldData, newData))
            CrashAtUnhandlableOOM("Nursery::setForwardingPointer");
    }
}

void
Nursery::setSlotsForwardingPointer(HeapSlot* oldSlots, HeapSlot* newSlots, uint32_t nslots)
{
    
    
    MOZ_ASSERT(nslots > 0);
    setForwardingPointer(oldSlots, newSlots,  true);
}

void
Nursery::setElementsForwardingPointer(ObjectElements* oldHeader, ObjectElements* newHeader,
                                      uint32_t nelems)
{
    
    setForwardingPointer(oldHeader->elements(), newHeader->elements(),
                         nelems > ObjectElements::VALUES_PER_HEADER);
}

#ifdef DEBUG
static bool IsWriteableAddress(void* ptr)
{
    volatile uint64_t* vPtr = reinterpret_cast<volatile uint64_t*>(ptr);
    *vPtr = *vPtr;
    return true;
}
#endif

void
js::Nursery::forwardBufferPointer(HeapSlot** pSlotsElems)
{
    HeapSlot* old = *pSlotsElems;

    if (!isInside(old))
        return;

    
    
    do {
        if (forwardedBuffers.initialized()) {
            if (ForwardedBufferMap::Ptr p = forwardedBuffers.lookup(old)) {
                *pSlotsElems = reinterpret_cast<HeapSlot*>(p->value());
                break;
            }
        }

        *pSlotsElems = *reinterpret_cast<HeapSlot**>(old);
    } while (false);

    MOZ_ASSERT(!isInside(*pSlotsElems));
    MOZ_ASSERT(IsWriteableAddress(*pSlotsElems));
}



struct TenureCount
{
    ObjectGroup* group;
    int count;
};




struct Nursery::TenureCountCache
{
    TenureCount entries[16];

    TenureCountCache() { PodZero(this); }

    TenureCount& findEntry(ObjectGroup* group) {
        return entries[PointerHasher<ObjectGroup*, 3>::hash(group) % ArrayLength(entries)];
    }
};

void
js::Nursery::collectToFixedPoint(MinorCollectionTracer* trc, TenureCountCache& tenureCounts)
{
    for (RelocationOverlay* p = trc->head; p; p = p->next()) {
        JSObject* obj = static_cast<JSObject*>(p->forwardingAddress());
        traceObject(trc, obj);

        TenureCount& entry = tenureCounts.findEntry(obj->group());
        if (entry.group == obj->group()) {
            entry.count++;
        } else if (!entry.group) {
            entry.group = obj->group();
            entry.count = 1;
        }
    }
}

MOZ_ALWAYS_INLINE void
js::Nursery::traceObject(MinorCollectionTracer* trc, JSObject* obj)
{
    const Class* clasp = obj->getClass();
    if (clasp->trace)
        clasp->trace(trc, obj);

    MOZ_ASSERT(obj->isNative() == clasp->isNative());
    if (!clasp->isNative())
        return;
    NativeObject* nobj = &obj->as<NativeObject>();

    
    
    if (!nobj->hasEmptyElements() && !nobj->denseElementsAreCopyOnWrite())
        markSlots(trc, nobj->getDenseElements(), nobj->getDenseInitializedLength());

    HeapSlot* fixedStart;
    HeapSlot* fixedEnd;
    HeapSlot* dynStart;
    HeapSlot* dynEnd;
    nobj->getSlotRange(0, nobj->slotSpan(), &fixedStart, &fixedEnd, &dynStart, &dynEnd);
    markSlots(trc, fixedStart, fixedEnd);
    markSlots(trc, dynStart, dynEnd);
}

MOZ_ALWAYS_INLINE void
js::Nursery::markSlots(MinorCollectionTracer* trc, HeapSlot* vp, uint32_t nslots)
{
    markSlots(trc, vp, vp + nslots);
}

MOZ_ALWAYS_INLINE void
js::Nursery::markSlots(MinorCollectionTracer* trc, HeapSlot* vp, HeapSlot* end)
{
    for (; vp != end; ++vp)
        markSlot(trc, vp);
}

MOZ_ALWAYS_INLINE void
js::Nursery::markSlot(MinorCollectionTracer* trc, HeapSlot* slotp)
{
    if (!slotp->isObject())
        return;

    JSObject* obj = &slotp->toObject();
    if (!IsInsideNursery(obj))
        return;

    if (getForwardedPointer(&obj)) {
        slotp->unsafeGet()->setObject(*obj);
        return;
    }

    JSObject* tenured = static_cast<JSObject*>(moveToTenured(trc, obj));
    slotp->unsafeGet()->setObject(*tenured);
}

void*
js::Nursery::moveToTenured(MinorCollectionTracer* trc, JSObject* src)
{

    AllocKind dstKind = src->allocKindForTenure(*this);
    Zone* zone = src->zone();
    JSObject* dst = reinterpret_cast<JSObject*>(allocateFromTenured(zone, dstKind));
    if (!dst)
        CrashAtUnhandlableOOM("Failed to allocate object while tenuring.");

    trc->tenuredSize += moveObjectToTenured(trc, dst, src, dstKind);

    RelocationOverlay* overlay = RelocationOverlay::fromCell(src);
    overlay->forwardTo(dst);
    trc->insertIntoFixupList(overlay);

    TracePromoteToTenured(src, dst);
    return static_cast<void*>(dst);
}

MOZ_ALWAYS_INLINE size_t
js::Nursery::moveObjectToTenured(MinorCollectionTracer* trc,
                                 JSObject* dst, JSObject* src, AllocKind dstKind)
{
    size_t srcSize = Arena::thingSize(dstKind);
    size_t tenuredSize = srcSize;

    








    if (src->is<ArrayObject>())
        tenuredSize = srcSize = sizeof(NativeObject);

    js_memcpy(dst, src, srcSize);
    if (src->isNative()) {
        NativeObject* ndst = &dst->as<NativeObject>();
        NativeObject* nsrc = &src->as<NativeObject>();
        tenuredSize += moveSlotsToTenured(ndst, nsrc, dstKind);
        tenuredSize += moveElementsToTenured(ndst, nsrc, dstKind);

        
        
        if (&nsrc->shape_ == ndst->shape_->listp) {
            MOZ_ASSERT(nsrc->shape_->inDictionary());
            ndst->shape_->listp = &ndst->shape_;
        }
    }

    if (src->is<InlineTypedObject>())
        InlineTypedObject::objectMovedDuringMinorGC(trc, dst, src);

    return tenuredSize;
}

MOZ_ALWAYS_INLINE size_t
js::Nursery::moveSlotsToTenured(NativeObject* dst, NativeObject* src, AllocKind dstKind)
{
    
    if (!src->hasDynamicSlots())
        return 0;

    if (!isInside(src->slots_)) {
        hugeSlots.remove(src->slots_);
        return 0;
    }

    Zone* zone = src->zone();
    size_t count = src->numDynamicSlots();
    dst->slots_ = zone->pod_malloc<HeapSlot>(count);
    if (!dst->slots_)
        CrashAtUnhandlableOOM("Failed to allocate slots while tenuring.");
    PodCopy(dst->slots_, src->slots_, count);
    setSlotsForwardingPointer(src->slots_, dst->slots_, count);
    return count * sizeof(HeapSlot);
}

MOZ_ALWAYS_INLINE size_t
js::Nursery::moveElementsToTenured(NativeObject* dst, NativeObject* src, AllocKind dstKind)
{
    if (src->hasEmptyElements() || src->denseElementsAreCopyOnWrite())
        return 0;

    Zone* zone = src->zone();
    ObjectElements* srcHeader = src->getElementsHeader();
    ObjectElements* dstHeader;

    
    if (!isInside(srcHeader)) {
        MOZ_ASSERT(src->elements_ == dst->elements_);
        hugeSlots.remove(reinterpret_cast<HeapSlot*>(srcHeader));
        return 0;
    }

    size_t nslots = ObjectElements::VALUES_PER_HEADER + srcHeader->capacity;

    
    if (src->is<ArrayObject>() && nslots <= GetGCKindSlots(dstKind)) {
        dst->as<ArrayObject>().setFixedElements();
        dstHeader = dst->as<ArrayObject>().getElementsHeader();
        js_memcpy(dstHeader, srcHeader, nslots * sizeof(HeapSlot));
        setElementsForwardingPointer(srcHeader, dstHeader, nslots);
        return nslots * sizeof(HeapSlot);
    }

    MOZ_ASSERT(nslots >= 2);
    dstHeader = reinterpret_cast<ObjectElements*>(zone->pod_malloc<HeapSlot>(nslots));
    if (!dstHeader)
        CrashAtUnhandlableOOM("Failed to allocate elements while tenuring.");
    js_memcpy(dstHeader, srcHeader, nslots * sizeof(HeapSlot));
    setElementsForwardingPointer(srcHeader, dstHeader, nslots);
    dst->elements_ = dstHeader->elements();
    return nslots * sizeof(HeapSlot);
}

static bool
ShouldMoveToTenured(MinorCollectionTracer* trc, void** thingp)
{
    Cell* cell = static_cast<Cell*>(*thingp);
    Nursery& nursery = *trc->nursery;
    return !nursery.isInside(thingp) && IsInsideNursery(cell) &&
           !nursery.getForwardedPointer(thingp);
}

 void
js::Nursery::MinorGCCallback(JS::CallbackTracer* jstrc, void** thingp, JSGCTraceKind kind)
{
    MinorCollectionTracer* trc = static_cast<MinorCollectionTracer*>(jstrc);
    if (ShouldMoveToTenured(trc, thingp))
        *thingp = trc->nursery->moveToTenured(trc, static_cast<JSObject*>(*thingp));
}

#define TIME_START(name) int64_t timstampStart_##name = enableProfiling_ ? PRMJ_Now() : 0
#define TIME_END(name) int64_t timstampEnd_##name = enableProfiling_ ? PRMJ_Now() : 0
#define TIME_TOTAL(name) (timstampEnd_##name - timstampStart_##name)

void
js::Nursery::collect(JSRuntime* rt, JS::gcreason::Reason reason, ObjectGroupList* pretenureGroups)
{
    if (rt->mainThread.suppressGC)
        return;

    JS_AbortIfWrongThread(rt);

    StoreBuffer& sb = rt->gc.storeBuffer;
    if (!isEnabled() || isEmpty()) {
        





        sb.clear();
        return;
    }

    rt->gc.incMinorGcNumber();

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

    TIME_START(markRuntime);
    rt->gc.markRuntime(&trc);
    TIME_END(markRuntime);

    TIME_START(markDebugger);
    {
        gcstats::AutoPhase ap(rt->gc.stats, gcstats::PHASE_MARK_ROOTS);
        Debugger::markAll(&trc);
    }
    TIME_END(markDebugger);

    TIME_START(clearNewObjectCache);
    rt->newObjectCache.clearNurseryObjects(rt);
    TIME_END(clearNewObjectCache);

    
    
    
    
    TIME_START(collectToFP);
    TenureCountCache tenureCounts;
    collectToFixedPoint(&trc, tenureCounts);
    TIME_END(collectToFP);

    
    TIME_START(sweepArrayBufferViewList);
    for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
        if (c->innerViews.needsSweepAfterMinorGC())
            c->innerViews.sweepAfterMinorGC(rt);
    }
    TIME_END(sweepArrayBufferViewList);

    
    TIME_START(updateJitActivations);
    js::jit::UpdateJitActivationsForMinorGC(rt, &trc);
    forwardedBuffers.finish();
    TIME_END(updateJitActivations);

    
    TIME_START(runFinalizers);
    runFinalizers();
    TIME_END(runFinalizers);

    TIME_START(freeHugeSlots);
    freeHugeSlots();
    TIME_END(freeHugeSlots);

    TIME_START(sweep);
    sweep();
    TIME_END(sweep);

    TIME_START(clearStoreBuffer);
    rt->gc.storeBuffer.clear();
    TIME_END(clearStoreBuffer);

    
    TIME_START(checkHashTables);
#ifdef JS_GC_ZEAL
    if (rt->gcZeal() == ZealCheckHashTablesOnMinorGC)
        CheckHashTablesAfterMovingGC(rt);
#endif
    TIME_END(checkHashTables);

    
    TIME_START(resize);
    double promotionRate = trc.tenuredSize / double(allocationEnd() - start());
    if (promotionRate > 0.05)
        growAllocableSpace();
    else if (promotionRate < 0.01)
        shrinkAllocableSpace();
    TIME_END(resize);

    
    
    
    
    TIME_START(pretenure);
    if (pretenureGroups && (promotionRate > 0.8 || reason == JS::gcreason::FULL_STORE_BUFFER)) {
        for (size_t i = 0; i < ArrayLength(tenureCounts.entries); i++) {
            const TenureCount& entry = tenureCounts.entries[i];
            if (entry.count >= 3000)
                pretenureGroups->append(entry.group); 
        }
    }
    TIME_END(pretenure);

    
    
    
    if (rt->gc.usage.gcBytes() >= rt->gc.tunables.gcMaxBytes())
        disable();

    TIME_END(total);

    TraceMinorGCEnd();

    int64_t totalTime = TIME_TOTAL(total);
    if (enableProfiling_ && totalTime >= profileThreshold_) {
        static bool printedHeader = false;
        if (!printedHeader) {
            fprintf(stderr,
                    "MinorGC: Reason               PRate  Size Time   mkVals mkClls mkSlts mkWCll mkRVal mkRCll mkGnrc ckTbls mkRntm mkDbgr clrNOC collct swpABO updtIn runFin frSlts clrSB  sweep resize pretnr\n");
            printedHeader = true;
        }

#define FMT " %6" PRIu64
        fprintf(stderr,
                "MinorGC: %20s %5.1f%% %4d" FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT FMT "\n",
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
                TIME_TOTAL(runFinalizers),
                TIME_TOTAL(freeHugeSlots),
                TIME_TOTAL(clearStoreBuffer),
                TIME_TOTAL(sweep),
                TIME_TOTAL(resize),
                TIME_TOTAL(pretenure));
#undef FMT
    }
}

#undef TIME_START
#undef TIME_END
#undef TIME_TOTAL

void
js::Nursery::FreeHugeSlotsTask::transferSlotsToFree(HugeSlotsSet& slotsToFree)
{
    
    
    MOZ_ASSERT(!isRunning());
    MOZ_ASSERT(slots_.empty());
    mozilla::Swap(slots_, slotsToFree);
}

void
js::Nursery::FreeHugeSlotsTask::run()
{
    for (HugeSlotsSet::Range r = slots_.all(); !r.empty(); r.popFront())
        fop_->free_(r.front());
    slots_.clear();
}

void
js::Nursery::freeHugeSlots()
{
    if (hugeSlots.empty())
        return;

    bool started;
    {
        AutoLockHelperThreadState lock;
        freeHugeSlotsTask->joinWithLockHeld();
        freeHugeSlotsTask->transferSlotsToFree(hugeSlots);
        started = freeHugeSlotsTask->startWithLockHeld();
    }

    if (!started)
        freeHugeSlotsTask->runFromMainThread(runtime());

    MOZ_ASSERT(hugeSlots.empty());
}

void
js::Nursery::waitBackgroundFreeEnd()
{
    freeHugeSlotsTask->join();
}

void
js::Nursery::runFinalizers()
{
    verifyFinalizerList();

    FreeOp* fop = runtime()->defaultFreeOp();
    for (ListItem* current = finalizers_; current; current = current->next()) {
        JSObject* obj = current->get();
        RelocationOverlay* overlay = RelocationOverlay::fromCell(obj);
        if (!overlay->isForwarded())
            obj->getClass()->finalize(fop, obj);
    }
    finalizers_ = nullptr;
}

void
js::Nursery::sweep()
{
#ifdef JS_GC_ZEAL
    
    JS_POISON((void*)start(), JS_SWEPT_NURSERY_PATTERN, nurserySize());
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
        JS_POISON((void*)start(), JS_SWEPT_NURSERY_PATTERN, allocationEnd() - start());
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
