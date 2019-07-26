





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







class BufferableRef
{
  public:
    virtual void mark(JSTracer *trc) = 0;
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





class StoreBuffer
{
    
    static const size_t ChunkSize = 1 << 16; 

    
    static const size_t MinAvailableSize = (size_t)(ChunkSize * 1.0 / 8.0);

    




    template<typename T>
    class MonoTypeBuffer
    {
        friend class StoreBuffer;
        friend class mozilla::ReentrancyGuard;

        StoreBuffer *owner;

        LifoAlloc storage_;
        bool enabled_;
        mozilla::DebugOnly<bool> entered;

        explicit MonoTypeBuffer(StoreBuffer *owner)
          : owner(owner), storage_(ChunkSize), enabled_(false), entered(false)
        {}

        MonoTypeBuffer &operator=(const MonoTypeBuffer& other) MOZ_DELETE;

        void enable() { enabled_ = true; }
        void disable() { enabled_ = false; clear(); }
        void clear() { storage_.used() ? storage_.releaseAll() : storage_.freeAll(); }

        bool isAboutToOverflow() const {
            return !storage_.isEmpty() && storage_.availableInCurrentChunk() < MinAvailableSize;
        }

        
        void compactRemoveDuplicates();

        



        virtual void compact();

        
        void put(const T &t) {
            mozilla::ReentrancyGuard g(*this);
            JS_ASSERT(CurrentThreadCanAccessRuntime(owner->runtime));

            if (!enabled_)
                return;

            T *tp = storage_.new_<T>(t);
            if (!tp)
                MOZ_CRASH();

            if (isAboutToOverflow()) {
                compact();
                if (isAboutToOverflow())
                    owner->setAboutToOverflow();
            }
        }

        
        void mark(JSTracer *trc);
    };

    



    template <typename T>
    class RelocatableMonoTypeBuffer : public MonoTypeBuffer<T>
    {
        friend class StoreBuffer;

        explicit RelocatableMonoTypeBuffer(StoreBuffer *owner)
          : MonoTypeBuffer<T>(owner)
        {}

        
        void compactMoved();
        virtual void compact();

        
        void unput(const T &v) {
            JS_ASSERT(CurrentThreadCanAccessRuntime(this->owner->runtime));
            MonoTypeBuffer<T>::put(v.tagged());
        }
    };

    class GenericBuffer
    {
        friend class StoreBuffer;
        friend class mozilla::ReentrancyGuard;

        StoreBuffer *owner;
        LifoAlloc storage_;
        bool enabled_;
        mozilla::DebugOnly<bool> entered;

        explicit GenericBuffer(StoreBuffer *owner)
          : owner(owner), storage_(ChunkSize), enabled_(false), entered(false)
        {}

        GenericBuffer &operator=(const GenericBuffer& other) MOZ_DELETE;

        void enable() { enabled_ = true; }
        void disable() { enabled_ = false; clear(); }
        void clear() { storage_.used() ? storage_.releaseAll() : storage_.freeAll(); }

        bool isAboutToOverflow() const {
            return !storage_.isEmpty() && storage_.availableInCurrentChunk() < MinAvailableSize;
        }

        
        void mark(JSTracer *trc);

        template <typename T>
        void put(const T &t) {
            mozilla::ReentrancyGuard g(*this);
            JS_ASSERT(CurrentThreadCanAccessRuntime(owner->runtime));

            
            (void)static_cast<const BufferableRef*>(&t);

            if (!enabled_)
                return;

            unsigned size = sizeof(T);
            unsigned *sizep = storage_.newPod<unsigned>();
            if (!sizep)
                MOZ_CRASH();
            *sizep = size;

            T *tp = storage_.new_<T>(t);
            if (!tp)
                MOZ_CRASH();

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
        JS_ALWAYS_INLINE bool inRememberedSet(const Nursery &nursery) const;
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

    MonoTypeBuffer<ValueEdge> bufferVal;
    MonoTypeBuffer<CellPtrEdge> bufferCell;
    MonoTypeBuffer<SlotEdge> bufferSlot;
    MonoTypeBuffer<WholeCellEdges> bufferWholeCell;
    RelocatableMonoTypeBuffer<ValueEdge> bufferRelocVal;
    RelocatableMonoTypeBuffer<CellPtrEdge> bufferRelocCell;
    GenericBuffer bufferGeneric;

    
    EdgeSet edgeSet;

    JSRuntime *runtime;
    const Nursery &nursery_;

    bool aboutToOverflow;
    bool enabled;

  public:
    explicit StoreBuffer(JSRuntime *rt, const Nursery &nursery)
      : bufferVal(this), bufferCell(this), bufferSlot(this), bufferWholeCell(this),
        bufferRelocVal(this), bufferRelocCell(this), bufferGeneric(this),
        runtime(rt), nursery_(nursery), aboutToOverflow(false), enabled(false)
    {
        edgeSet.init();
    }

    bool enable();
    void disable();
    bool isEnabled() { return enabled; }

    bool clear();

    
    bool isAboutToOverflow() const { return aboutToOverflow; }

    
    void putValue(JS::Value *valuep) {
        ValueEdge edge(valuep);
        if (!edge.inRememberedSet(nursery_))
            return;
        bufferVal.put(edge);
    }
    void putCell(Cell **cellp) {
        CellPtrEdge edge(cellp);
        if (!edge.inRememberedSet(nursery_))
            return;
        bufferCell.put(edge);
    }
    void putSlot(JSObject *obj, int kind, uint32_t slot, void *target) {
        SlotEdge edge(obj, kind, slot);
        
        if (nursery_.isInside(obj) || !nursery_.isInside(target))
            return;
        bufferSlot.put(edge);
    }
    void putWholeCell(Cell *cell) {
        bufferWholeCell.put(WholeCellEdges(cell));
    }

    
    void putRelocatableValue(JS::Value *valuep) {
        ValueEdge edge(valuep);
        if (!edge.inRememberedSet(nursery_))
            return;
        bufferRelocVal.put(edge);
    }
    void putRelocatableCell(Cell **cellp) {
        CellPtrEdge edge(cellp);
        if (!edge.inRememberedSet(nursery_))
            return;
        bufferRelocCell.put(edge);
    }
    void removeRelocatableValue(JS::Value *valuep) {
        ValueEdge edge(valuep);
        if (!edge.inRememberedSet(nursery_))
            return;
        bufferRelocVal.unput(edge);
    }
    void removeRelocatableCell(Cell **cellp) {
        CellPtrEdge edge(cellp);
        if (!edge.inRememberedSet(nursery_))
            return;
        bufferRelocCell.unput(edge);
    }

    
    template <typename T>
    void putGeneric(const T &t) {
        bufferGeneric.put(t);
    }

    
    void putCallback(CallbackRef::MarkCallback callback, Cell *key, void *data) {
        if (nursery_.isInside(key))
            bufferGeneric.put(CallbackRef(callback, key, data));
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
