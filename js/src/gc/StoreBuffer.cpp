





#ifdef JSGC_GENERATIONAL

#include "gc/StoreBuffer.h"

#include "mozilla/Assertions.h"

#include "vm/ArgumentsObject.h"
#include "vm/ForkJoin.h"

#include "jsgcinlines.h"

using namespace js;
using namespace js::gc;
using mozilla::ReentrancyGuard;

gcstats::Statistics&
StoreBuffer::stats()
{
    return runtime_->gc.stats;
}



void
StoreBuffer::SlotsEdge::mark(JSTracer *trc)
{
    NativeObject *obj = object();

    
    if (!obj->isNative())
        return;

    if (IsInsideNursery(obj))
        return;

    if (kind() == ElementKind) {
        int32_t initLen = obj->getDenseInitializedLength();
        int32_t clampedStart = Min(start_, initLen);
        int32_t clampedEnd = Min(start_ + count_, initLen);
        gc::MarkArraySlots(trc, clampedEnd - clampedStart,
                           obj->getDenseElements() + clampedStart, "element");
    } else {
        int32_t start = Min(uint32_t(start_), obj->slotSpan());
        int32_t end = Min(uint32_t(start_) + count_, obj->slotSpan());
        MOZ_ASSERT(end >= start);
        MarkObjectSlots(trc, obj, start, end - start);
    }
}

void
StoreBuffer::WholeCellEdges::mark(JSTracer *trc)
{
    MOZ_ASSERT(edge->isTenured());
    JSGCTraceKind kind = GetGCThingTraceKind(edge);
    if (kind <= JSTRACE_OBJECT) {
        JSObject *object = static_cast<JSObject *>(edge);
        if (object->is<ArgumentsObject>())
            ArgumentsObject::trace(trc, object);
        MarkChildren(trc, object);
        return;
    }
    MOZ_ASSERT(kind == JSTRACE_JITCODE);
    static_cast<jit::JitCode *>(edge)->trace(trc);
}

void
StoreBuffer::CellPtrEdge::mark(JSTracer *trc)
{
    if (!*edge)
        return;

    MOZ_ASSERT(GetGCThingTraceKind(*edge) == JSTRACE_OBJECT);
    MarkObjectRoot(trc, reinterpret_cast<JSObject**>(edge), "store buffer edge");
}

void
StoreBuffer::ValueEdge::mark(JSTracer *trc)
{
    if (!deref())
        return;

    MarkValueRoot(trc, edge, "store buffer edge");
}



template <typename T>
void
StoreBuffer::MonoTypeBuffer<T>::handleOverflow(StoreBuffer *owner)
{
    if (!owner->isAboutToOverflow()) {
        



        gcstats::AutoPhase ap(owner->stats(), PHASE_COMPACT_STOREBUFFER_NO_PARENT);
        owner->stats().count(gcstats::STAT_STOREBUFFER_OVERFLOW);
        compact(owner);
        if (isLowOnSpace())
            owner->setAboutToOverflow();
    } else {
         



        if (storage_->availableInCurrentChunk() < sizeof(T)) {
            owner->stats().count(gcstats::STAT_STOREBUFFER_OVERFLOW);
            maybeCompact(owner, PHASE_COMPACT_STOREBUFFER_NO_PARENT);
        }
    }
}

template <typename T>
void
StoreBuffer::MonoTypeBuffer<T>::compactRemoveDuplicates(StoreBuffer *owner)
{
    typedef HashSet<T, typename T::Hasher, SystemAllocPolicy> DedupSet;

    DedupSet duplicates;
    if (!duplicates.init())
        return; 

    LifoAlloc::Enum insert(*storage_);
    for (LifoAlloc::Enum e(*storage_); !e.empty(); e.popFront<T>()) {
        T *edge = e.get<T>();
        if (!duplicates.has(*edge)) {
            insert.updateFront<T>(*edge);
            insert.popFront<T>();

            
            duplicates.put(*edge);
        }
    }
    storage_->release(insert.mark());

    duplicates.clear();
    owner->stats().count(gcstats::STAT_COMPACT_STOREBUFFER);
}

template <typename T>
void
StoreBuffer::MonoTypeBuffer<T>::compact(StoreBuffer *owner)
{
    MOZ_ASSERT(storage_);
    compactRemoveDuplicates(owner);
    usedAtLastCompact_ = storage_->used();
}

template <typename T>
void
StoreBuffer::MonoTypeBuffer<T>::maybeCompact(StoreBuffer *owner, int phase)
{
    MOZ_ASSERT(storage_);
    if (storage_->used() != usedAtLastCompact_) {
        gcstats::AutoPhase ap(owner->stats(), static_cast<gcstats::Phase>(phase));
        compact(owner);
    }
}

template <typename T>
void
StoreBuffer::MonoTypeBuffer<T>::mark(StoreBuffer *owner, JSTracer *trc)
{
    MOZ_ASSERT(owner->isEnabled());
    ReentrancyGuard g(*owner);
    if (!storage_)
        return;

    maybeCompact(owner, PHASE_COMPACT_STOREBUFFER_IN_MINOR_GC);

    for (LifoAlloc::Enum e(*storage_); !e.empty(); e.popFront<T>()) {
        T *edge = e.get<T>();
        edge->mark(trc);
    }
}



template <typename T>
void
StoreBuffer::RelocatableMonoTypeBuffer<T>::compactMoved(StoreBuffer *owner)
{
    LifoAlloc &storage = *this->storage_;
    EdgeSet invalidated;
    if (!invalidated.init())
        CrashAtUnhandlableOOM("RelocatableMonoTypeBuffer::compactMoved: Failed to init table.");

    
    for (LifoAlloc::Enum e(storage); !e.empty(); e.popFront<T>()) {
        T *edge = e.get<T>();
        if (edge->isTagged()) {
            if (!invalidated.put(edge->untagged().edge))
                CrashAtUnhandlableOOM("RelocatableMonoTypeBuffer::compactMoved: Failed to put removal.");
        } else {
            invalidated.remove(edge->untagged().edge);
        }
    }

    
    LifoAlloc::Enum insert(storage);
    for (LifoAlloc::Enum e(storage); !e.empty(); e.popFront<T>()) {
        T *edge = e.get<T>();
        if (!edge->isTagged() && !invalidated.has(edge->untagged().edge)) {
            insert.updateFront<T>(*edge);
            insert.popFront<T>();
        }
    }
    storage.release(insert.mark());

    invalidated.clear();

#ifdef DEBUG
    for (LifoAlloc::Enum e(storage); !e.empty(); e.popFront<T>())
        MOZ_ASSERT(!e.get<T>()->isTagged());
#endif
}

template <typename T>
void
StoreBuffer::RelocatableMonoTypeBuffer<T>::compact(StoreBuffer *owner)
{
    compactMoved(owner);
    StoreBuffer::MonoTypeBuffer<T>::compact(owner);
}



void
StoreBuffer::GenericBuffer::mark(StoreBuffer *owner, JSTracer *trc)
{
    MOZ_ASSERT(owner->isEnabled());
    ReentrancyGuard g(*owner);
    if (!storage_)
        return;

    for (LifoAlloc::Enum e(*storage_); !e.empty();) {
        unsigned size = *e.get<unsigned>();
        e.popFront<unsigned>();
        BufferableRef *edge = e.get<BufferableRef>(size);
        edge->mark(trc);
        e.popFront(size);
    }
}



bool
StoreBuffer::enable()
{
    if (enabled_)
        return true;

    if (!bufferVal.init() ||
        !bufferCell.init() ||
        !bufferSlot.init() ||
        !bufferWholeCell.init() ||
        !bufferRelocVal.init() ||
        !bufferRelocCell.init() ||
        !bufferGeneric.init())
    {
        return false;
    }

    enabled_ = true;
    return true;
}

void
StoreBuffer::disable()
{
    if (!enabled_)
        return;

    aboutToOverflow_ = false;

    enabled_ = false;
}

bool
StoreBuffer::clear()
{
    if (!enabled_)
        return true;

    aboutToOverflow_ = false;

    bufferVal.clear();
    bufferCell.clear();
    bufferSlot.clear();
    bufferWholeCell.clear();
    bufferRelocVal.clear();
    bufferRelocCell.clear();
    bufferGeneric.clear();

    return true;
}

void
StoreBuffer::markAll(JSTracer *trc)
{
    bufferVal.mark(this, trc);
    bufferCell.mark(this, trc);
    bufferSlot.mark(this, trc);
    bufferWholeCell.mark(this, trc);
    bufferRelocVal.mark(this, trc);
    bufferRelocCell.mark(this, trc);
    bufferGeneric.mark(this, trc);
}

void
StoreBuffer::setAboutToOverflow()
{
    aboutToOverflow_ = true;
    runtime_->gc.requestMinorGC(JS::gcreason::FULL_STORE_BUFFER);
}

bool
StoreBuffer::inParallelSection() const
{
    return InParallelSection();
}

void
StoreBuffer::addSizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf, JS::GCSizes
*sizes)
{
    sizes->storeBufferVals       += bufferVal.sizeOfExcludingThis(mallocSizeOf);
    sizes->storeBufferCells      += bufferCell.sizeOfExcludingThis(mallocSizeOf);
    sizes->storeBufferSlots      += bufferSlot.sizeOfExcludingThis(mallocSizeOf);
    sizes->storeBufferWholeCells += bufferWholeCell.sizeOfExcludingThis(mallocSizeOf);
    sizes->storeBufferRelocVals  += bufferRelocVal.sizeOfExcludingThis(mallocSizeOf);
    sizes->storeBufferRelocCells += bufferRelocCell.sizeOfExcludingThis(mallocSizeOf);
    sizes->storeBufferGenerics   += bufferGeneric.sizeOfExcludingThis(mallocSizeOf);
}

JS_PUBLIC_API(void)
JS::HeapCellPostBarrier(js::gc::Cell **cellp)
{
    MOZ_ASSERT(cellp);
    MOZ_ASSERT(*cellp);
    StoreBuffer *storeBuffer = (*cellp)->storeBuffer();
    if (storeBuffer)
        storeBuffer->putRelocatableCellFromAnyThread(cellp);
}

JS_PUBLIC_API(void)
JS::HeapCellRelocate(js::gc::Cell **cellp)
{
    
    MOZ_ASSERT(cellp);
    MOZ_ASSERT(*cellp);
    JSRuntime *runtime = (*cellp)->runtimeFromMainThread();
    runtime->gc.storeBuffer.removeRelocatableCellFromAnyThread(cellp);
}

JS_PUBLIC_API(void)
JS::HeapValuePostBarrier(JS::Value *valuep)
{
    MOZ_ASSERT(valuep);
    MOZ_ASSERT(valuep->isMarkable());
    if (valuep->isObject()) {
        StoreBuffer *storeBuffer = valuep->toObject().storeBuffer();
        if (storeBuffer)
            storeBuffer->putRelocatableValueFromAnyThread(valuep);
    }
}

JS_PUBLIC_API(void)
JS::HeapValueRelocate(JS::Value *valuep)
{
    
    MOZ_ASSERT(valuep);
    MOZ_ASSERT(valuep->isMarkable());
    if (valuep->isString() && valuep->toString()->isPermanentAtom())
        return;
    JSRuntime *runtime = static_cast<js::gc::Cell *>(valuep->toGCThing())->runtimeFromMainThread();
    runtime->gc.storeBuffer.removeRelocatableValueFromAnyThread(valuep);
}

template struct StoreBuffer::MonoTypeBuffer<StoreBuffer::ValueEdge>;
template struct StoreBuffer::MonoTypeBuffer<StoreBuffer::CellPtrEdge>;
template struct StoreBuffer::MonoTypeBuffer<StoreBuffer::SlotsEdge>;
template struct StoreBuffer::MonoTypeBuffer<StoreBuffer::WholeCellEdges>;
template struct StoreBuffer::RelocatableMonoTypeBuffer<StoreBuffer::ValueEdge>;
template struct StoreBuffer::RelocatableMonoTypeBuffer<StoreBuffer::CellPtrEdge>;

#endif 
