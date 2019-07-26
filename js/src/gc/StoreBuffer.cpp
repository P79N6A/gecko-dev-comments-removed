





#ifdef JSGC_GENERATIONAL

#include "gc/StoreBuffer.h"

#include "mozilla/Assertions.h"

#include "vm/ForkJoin.h"

#include "jsgcinlines.h"

using namespace js;
using namespace js::gc;
using mozilla::ReentrancyGuard;



JS_ALWAYS_INLINE HeapSlot *
StoreBuffer::SlotEdge::slotLocation() const
{
    if (kind == HeapSlot::Element) {
        if (offset >= object->getDenseInitializedLength())
            return nullptr;
        return (HeapSlot *)&object->getDenseElement(offset);
    }
    if (offset >= object->slotSpan())
        return nullptr;
    return &object->getSlotRef(offset);
}

JS_ALWAYS_INLINE void *
StoreBuffer::SlotEdge::deref() const
{
    HeapSlot *loc = slotLocation();
    return (loc && loc->isGCThing()) ? loc->toGCThing() : nullptr;
}

JS_ALWAYS_INLINE void *
StoreBuffer::SlotEdge::location() const
{
    return (void *)slotLocation();
}

bool
StoreBuffer::SlotEdge::inRememberedSet(const Nursery &nursery) const
{
    return !nursery.isInside(object) && nursery.isInside(deref());
}

JS_ALWAYS_INLINE bool
StoreBuffer::SlotEdge::isNullEdge() const
{
    return !deref();
}

void
StoreBuffer::WholeCellEdges::mark(JSTracer *trc)
{
    JS_ASSERT(tenured->isTenured());
    JSGCTraceKind kind = GetGCThingTraceKind(tenured);
    if (kind <= JSTRACE_OBJECT) {
        MarkChildren(trc, static_cast<JSObject *>(tenured));
        return;
    }
#ifdef JS_ION
    JS_ASSERT(kind == JSTRACE_IONCODE);
    static_cast<jit::IonCode *>(tenured)->trace(trc);
#else
    MOZ_ASSUME_UNREACHABLE("Only objects can be in the wholeCellBuffer if IonMonkey is disabled.");
#endif
}



template <typename T>
void
StoreBuffer::MonoTypeBuffer<T>::compactRemoveDuplicates(StoreBuffer *owner)
{
    EdgeSet duplicates;
    if (!duplicates.init())
        return; 

    LifoAlloc::Enum insert(*storage_);
    for (LifoAlloc::Enum e(*storage_); !e.empty(); e.popFront<T>()) {
        T *edge = e.get<T>();
        if (!duplicates.has(edge->location())) {
            insert.updateFront<T>(*edge);
            insert.popFront<T>();

            
            duplicates.put(edge->location());
        }
    }
    storage_->release(insert.mark());

    duplicates.clear();
}

template <typename T>
void
StoreBuffer::MonoTypeBuffer<T>::compact(StoreBuffer *owner)
{
    if (!storage_)
        return;

    compactRemoveDuplicates(owner);
}

template <typename T>
void
StoreBuffer::MonoTypeBuffer<T>::mark(StoreBuffer *owner, JSTracer *trc)
{
    ReentrancyGuard g(*owner);
    if (!storage_)
        return;

    compact(owner);
    for (LifoAlloc::Enum e(*storage_); !e.empty(); e.popFront<T>()) {
        T *edge = e.get<T>();
        if (edge->isNullEdge())
            continue;
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
        MOZ_CRASH("RelocatableMonoTypeBuffer::compactMoved: Failed to init table.");

    
    for (LifoAlloc::Enum e(storage); !e.empty(); e.popFront<T>()) {
        T *edge = e.get<T>();
        if (edge->isTagged()) {
            if (!invalidated.put(edge->location()))
                MOZ_CRASH("RelocatableMonoTypeBuffer::compactMoved: Failed to put removal.");
        } else {
            invalidated.remove(edge->location());
        }
    }

    
    LifoAlloc::Enum insert(storage);
    for (LifoAlloc::Enum e(storage); !e.empty(); e.popFront<T>()) {
        T *edge = e.get<T>();
        if (!edge->isTagged() && !invalidated.has(edge->location())) {
            insert.updateFront<T>(*edge);
            insert.popFront<T>();
        }
    }
    storage.release(insert.mark());

    invalidated.clear();

#ifdef DEBUG
    for (LifoAlloc::Enum e(storage); !e.empty(); e.popFront<T>())
        JS_ASSERT(!e.get<T>()->isTagged());
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



void
StoreBuffer::CellPtrEdge::mark(JSTracer *trc)
{
    JS_ASSERT(GetGCThingTraceKind(*edge) == JSTRACE_OBJECT);
    MarkObjectRoot(trc, reinterpret_cast<JSObject**>(edge), "store buffer edge");
}

void
StoreBuffer::ValueEdge::mark(JSTracer *trc)
{
    MarkValueRoot(trc, edge, "store buffer edge");
}

void
StoreBuffer::SlotEdge::mark(JSTracer *trc)
{
    if (kind == HeapSlot::Element)
        MarkSlot(trc, (HeapSlot*)&object->getDenseElement(offset), "store buffer edge");
    else
        MarkSlot(trc, &object->getSlotRef(offset), "store buffer edge");
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
StoreBuffer::mark(JSTracer *trc)
{
    JS_ASSERT(isEnabled());

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
    runtime_->triggerOperationCallback(JSRuntime::TriggerCallbackMainThread);
}

bool
StoreBuffer::inParallelSection() const
{
    return InParallelSection();
}

JS_PUBLIC_API(void)
JS::HeapCellPostBarrier(js::gc::Cell **cellp)
{
    JS_ASSERT(*cellp);
    JSRuntime *runtime = (*cellp)->runtimeFromMainThread();
    runtime->gcStoreBuffer.putRelocatableCell(cellp);
}

JS_PUBLIC_API(void)
JS::HeapCellRelocate(js::gc::Cell **cellp)
{
    
    JS_ASSERT(*cellp);
    JSRuntime *runtime = (*cellp)->runtimeFromMainThread();
    runtime->gcStoreBuffer.removeRelocatableCell(cellp);
}

JS_PUBLIC_API(void)
JS::HeapValuePostBarrier(JS::Value *valuep)
{
    JS_ASSERT(JSVAL_IS_TRACEABLE(*valuep));
    JSRuntime *runtime = static_cast<js::gc::Cell *>(valuep->toGCThing())->runtimeFromMainThread();
    runtime->gcStoreBuffer.putRelocatableValue(valuep);
}

JS_PUBLIC_API(void)
JS::HeapValueRelocate(JS::Value *valuep)
{
    
    JS_ASSERT(JSVAL_IS_TRACEABLE(*valuep));
    JSRuntime *runtime = static_cast<js::gc::Cell *>(valuep->toGCThing())->runtimeFromMainThread();
    runtime->gcStoreBuffer.removeRelocatableValue(valuep);
}

template class StoreBuffer::MonoTypeBuffer<StoreBuffer::ValueEdge>;
template class StoreBuffer::MonoTypeBuffer<StoreBuffer::CellPtrEdge>;
template class StoreBuffer::MonoTypeBuffer<StoreBuffer::SlotEdge>;
template class StoreBuffer::MonoTypeBuffer<StoreBuffer::WholeCellEdges>;
template class StoreBuffer::RelocatableMonoTypeBuffer<StoreBuffer::ValueEdge>;
template class StoreBuffer::RelocatableMonoTypeBuffer<StoreBuffer::CellPtrEdge>;

#endif 
