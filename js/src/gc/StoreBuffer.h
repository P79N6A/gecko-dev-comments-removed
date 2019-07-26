





#ifndef gc_StoreBuffer_h
#define gc_StoreBuffer_h

#ifdef JSGC_GENERATIONAL

#ifndef JSGC_USE_EXACT_ROOTING
# error "Generational GC requires exact rooting."
#endif

#include "mozilla/DebugOnly.h"
#include "mozilla/ReentrancyGuard.h"

#include "jsalloc.h"

#include "ds/LifoAlloc.h"
#include "gc/Nursery.h"
#include "js/Tracer.h"

namespace js {
namespace gc {

extern void
CrashAtUnhandlableOOM(const char *);







class BufferableRef
{
  public:
    virtual void mark(JSTracer *trc) = 0;
    bool inRememberedSet(const Nursery &) const { return true; }
};





template <typename Map, typename Key>
class HashKeyRef : public BufferableRef
{
    Map *map;
    Key key;

  public:
    HashKeyRef(Map *m, const Key &k) : map(m), key(k) {}

    void mark(JSTracer *trc) {
        Key prior = key;
        typename Map::Ptr p = map->lookup(key);
        if (!p)
            return;
        JS_SET_TRACING_LOCATION(trc, (void*)&*p);
        Mark(trc, &key, "HashKeyRef");
        map->rekeyIfMoved(prior, key);
    }
};

typedef HashSet<void *, PointerHasher<void *, 3>, SystemAllocPolicy> EdgeSet;


static const size_t LifoAllocBlockSize = 1 << 16; 





class StoreBuffer
{
    friend class mozilla::ReentrancyGuard;

    
    static const size_t MinAvailableSize = (size_t)(LifoAllocBlockSize * 1.0 / 8.0);

    




    template<typename T>
    class MonoTypeBuffer
    {
        friend class StoreBuffer;

        LifoAlloc *storage_;

        explicit MonoTypeBuffer() : storage_(nullptr) {}
        ~MonoTypeBuffer() { js_delete(storage_); }

        MonoTypeBuffer &operator=(const MonoTypeBuffer& other) MOZ_DELETE;

        bool init() {
            if (!storage_)
                storage_ = js_new<LifoAlloc>(LifoAllocBlockSize);
            clear();
            return bool(storage_);
        }

        void clear() {
            if (!storage_)
                return;

            storage_->used() ? storage_->releaseAll() : storage_->freeAll();
        }

        bool isAboutToOverflow() const {
            return !storage_->isEmpty() && storage_->availableInCurrentChunk() < MinAvailableSize;
        }

        
        void compactRemoveDuplicates(StoreBuffer *owner);

        



        virtual void compact(StoreBuffer *owner);

        
        void put(StoreBuffer *owner, const T &t) {
            JS_ASSERT(storage_);

            T *tp = storage_->new_<T>(t);
            if (!tp)
                CrashAtUnhandlableOOM("Failed to allocate for MonoTypeBuffer::put.");

            if (isAboutToOverflow()) {
                compact(owner);
                if (isAboutToOverflow())
                    owner->setAboutToOverflow();
            }
        }

        
        void mark(StoreBuffer *owner, JSTracer *trc);
    };

    



    template <typename T>
    class RelocatableMonoTypeBuffer : public MonoTypeBuffer<T>
    {
        friend class StoreBuffer;

        
        void compactMoved(StoreBuffer *owner);
        virtual void compact(StoreBuffer *owner) MOZ_OVERRIDE;

        
        void unput(StoreBuffer *owner, const T &v) {
            MonoTypeBuffer<T>::put(owner, v.tagged());
        }
    };

    class GenericBuffer
    {
        friend class StoreBuffer;

        LifoAlloc *storage_;

        explicit GenericBuffer() : storage_(nullptr) {}
        ~GenericBuffer() { js_delete(storage_); }

        GenericBuffer &operator=(const GenericBuffer& other) MOZ_DELETE;

        bool init() {
            if (!storage_)
                storage_ = js_new<LifoAlloc>(LifoAllocBlockSize);
            clear();
            return bool(storage_);
        }

        void clear() {
            if (!storage_)
                return;

            storage_->used() ? storage_->releaseAll() : storage_->freeAll();
        }

        bool isAboutToOverflow() const {
            return !storage_->isEmpty() && storage_->availableInCurrentChunk() < MinAvailableSize;
        }

        
        void mark(StoreBuffer *owner, JSTracer *trc);

        template <typename T>
        void put(StoreBuffer *owner, const T &t) {
            JS_ASSERT(storage_);

            
            (void)static_cast<const BufferableRef*>(&t);

            unsigned size = sizeof(T);
            unsigned *sizep = storage_->newPod<unsigned>();
            if (!sizep)
                CrashAtUnhandlableOOM("Failed to allocate for GenericBuffer::put.");
            *sizep = size;

            T *tp = storage_->new_<T>(t);
            if (!tp)
                CrashAtUnhandlableOOM("Failed to allocate for GenericBuffer::put.");

            if (isAboutToOverflow())
                owner->setAboutToOverflow();
        }
    };

    class CellPtrEdge
    {
        friend class StoreBuffer;
        friend class StoreBuffer::MonoTypeBuffer<CellPtrEdge>;
        friend class StoreBuffer::RelocatableMonoTypeBuffer<CellPtrEdge>;

        Cell **edge;

        explicit CellPtrEdge(Cell **v) : edge(v) {}
        bool operator==(const CellPtrEdge &other) const { return edge == other.edge; }
        bool operator!=(const CellPtrEdge &other) const { return edge != other.edge; }

        void *location() const { return (void *)untagged().edge; }

        bool inRememberedSet(const Nursery &nursery) const {
            return !nursery.isInside(edge) && nursery.isInside(*edge);
        }

        bool isNullEdge() const {
            return !*edge;
        }

        void mark(JSTracer *trc);

        CellPtrEdge tagged() const { return CellPtrEdge((Cell **)(uintptr_t(edge) | 1)); }
        CellPtrEdge untagged() const { return CellPtrEdge((Cell **)(uintptr_t(edge) & ~1)); }
        bool isTagged() const { return bool(uintptr_t(edge) & 1); }
    };

    class ValueEdge
    {
        friend class StoreBuffer;
        friend class StoreBuffer::MonoTypeBuffer<ValueEdge>;
        friend class StoreBuffer::RelocatableMonoTypeBuffer<ValueEdge>;

        JS::Value *edge;

        explicit ValueEdge(JS::Value *v) : edge(v) {}
        bool operator==(const ValueEdge &other) const { return edge == other.edge; }
        bool operator!=(const ValueEdge &other) const { return edge != other.edge; }

        void *deref() const { return edge->isGCThing() ? edge->toGCThing() : nullptr; }
        void *location() const { return (void *)untagged().edge; }

        bool inRememberedSet(const Nursery &nursery) const {
            return !nursery.isInside(edge) && nursery.isInside(deref());
        }

        bool isNullEdge() const {
            return !deref();
        }

        void mark(JSTracer *trc);

        ValueEdge tagged() const { return ValueEdge((JS::Value *)(uintptr_t(edge) | 1)); }
        ValueEdge untagged() const { return ValueEdge((JS::Value *)(uintptr_t(edge) & ~1)); }
        bool isTagged() const { return bool(uintptr_t(edge) & 1); }
    };

    struct SlotEdge
    {
        friend class StoreBuffer;
        friend class StoreBuffer::MonoTypeBuffer<SlotEdge>;

        JSObject *object;
        uint32_t offset;
        int kind; 

        SlotEdge(JSObject *object, int kind, uint32_t offset)
          : object(object), offset(offset), kind(kind)
        {}

        bool operator==(const SlotEdge &other) const {
            return object == other.object && offset == other.offset && kind == other.kind;
        }

        bool operator!=(const SlotEdge &other) const {
            return object != other.object || offset != other.offset || kind != other.kind;
        }

        JS_ALWAYS_INLINE HeapSlot *slotLocation() const;

        JS_ALWAYS_INLINE void *deref() const;
        JS_ALWAYS_INLINE void *location() const;
        bool inRememberedSet(const Nursery &nursery) const;
        JS_ALWAYS_INLINE bool isNullEdge() const;

        void mark(JSTracer *trc);
    };

    class WholeCellEdges
    {
        friend class StoreBuffer;
        friend class StoreBuffer::MonoTypeBuffer<WholeCellEdges>;

        Cell *tenured;

        WholeCellEdges(Cell *cell) : tenured(cell) {
            JS_ASSERT(tenured->isTenured());
        }

        bool operator==(const WholeCellEdges &other) const { return tenured == other.tenured; }
        bool operator!=(const WholeCellEdges &other) const { return tenured != other.tenured; }

        bool inRememberedSet(const Nursery &nursery) const { return true; }

        
        void *location() const { return (void *)tenured; }

        bool isNullEdge() const { return false; }

        void mark(JSTracer *trc);
    };

    class CallbackRef : public BufferableRef
    {
      public:
        typedef void (*MarkCallback)(JSTracer *trc, void *key, void *data);

        CallbackRef(MarkCallback cb, void *k, void *d) : callback(cb), key(k), data(d) {}

        virtual void mark(JSTracer *trc) {
            callback(trc, key, data);
        }

      private:
        MarkCallback callback;
        void *key;
        void *data;
    };

    template <typename Edge>
    bool isOkayToUseBuffer(const Edge &edge) const {
        



        if (!isEnabled())
            return false;

        



        if (!CurrentThreadCanAccessRuntime(runtime_)) {
            JS_ASSERT(!edge.inRememberedSet(nursery_));
            return false;
        }

        return true;
    }

    template <typename Buffer, typename Edge>
    void put(Buffer &buffer, const Edge &edge) {
        if (!isOkayToUseBuffer(edge))
            return;
        mozilla::ReentrancyGuard g(*this);
        if (edge.inRememberedSet(nursery_))
            buffer.put(this, edge);
    }

    template <typename Buffer, typename Edge>
    void unput(Buffer &buffer, const Edge &edge) {
        if (!isOkayToUseBuffer(edge))
            return;
        mozilla::ReentrancyGuard g(*this);
        buffer.unput(this, edge);
    }

    MonoTypeBuffer<ValueEdge> bufferVal;
    MonoTypeBuffer<CellPtrEdge> bufferCell;
    MonoTypeBuffer<SlotEdge> bufferSlot;
    MonoTypeBuffer<WholeCellEdges> bufferWholeCell;
    RelocatableMonoTypeBuffer<ValueEdge> bufferRelocVal;
    RelocatableMonoTypeBuffer<CellPtrEdge> bufferRelocCell;
    GenericBuffer bufferGeneric;

    JSRuntime *runtime_;
    const Nursery &nursery_;

    bool aboutToOverflow_;
    bool enabled_;
    mozilla::DebugOnly<bool> entered; 

  public:
    explicit StoreBuffer(JSRuntime *rt, const Nursery &nursery)
      : bufferVal(), bufferCell(), bufferSlot(), bufferWholeCell(),
        bufferRelocVal(), bufferRelocCell(), bufferGeneric(),
        runtime_(rt), nursery_(nursery), aboutToOverflow_(false), enabled_(false),
        entered(false)
    {
    }

    bool enable();
    void disable();
    bool isEnabled() const { return enabled_; }

    bool clear();

    
    bool isAboutToOverflow() const { return aboutToOverflow_; }

    
    void putValue(JS::Value *valuep) { put(bufferVal, ValueEdge(valuep)); }
    void putCell(Cell **cellp) { put(bufferCell, CellPtrEdge(cellp)); }
    void putSlot(JSObject *obj, int kind, uint32_t slot, void *target) {
        put(bufferSlot, SlotEdge(obj, kind, slot));
    }
    void putWholeCell(Cell *cell) {
        JS_ASSERT(cell->isTenured());
        put(bufferWholeCell, WholeCellEdges(cell));
    }

    
    void putRelocatableValue(JS::Value *valuep) { put(bufferRelocVal, ValueEdge(valuep)); }
    void putRelocatableCell(Cell **cellp) { put(bufferRelocCell, CellPtrEdge(cellp)); }
    void removeRelocatableValue(JS::Value *valuep) { unput(bufferRelocVal, ValueEdge(valuep)); }
    void removeRelocatableCell(Cell **cellp) { unput(bufferRelocCell, CellPtrEdge(cellp)); }

    
    template <typename T>
    void putGeneric(const T &t) { put(bufferGeneric, t);}

    
    void putCallback(CallbackRef::MarkCallback callback, Cell *key, void *data) {
        put(bufferGeneric, CallbackRef(callback, key, data));
    }

    
    void mark(JSTracer *trc);

    
    bool inParallelSection() const;

    
    void setAboutToOverflow();
    void setOverflowed();
};

} 
} 

#endif 

#endif 
