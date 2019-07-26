





#ifndef gc_StoreBuffer_h
#define gc_StoreBuffer_h

#ifdef JSGC_GENERATIONAL

#ifndef JSGC_USE_EXACT_ROOTING
# error "Generational GC requires exact rooting."
#endif

#include "mozilla/ReentrancyGuard.h"

#include "jsalloc.h"
#include "jsgc.h"
#include "jsobj.h"

#include "gc/Nursery.h"

namespace js {
namespace gc {

class AccumulateEdgesTracer;







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
        map->rekey(prior, key);
    }
};

typedef HashSet<void *, PointerHasher<void *, 3>, SystemAllocPolicy> EdgeSet;





class StoreBuffer
{
    




    template<typename T>
    class MonoTypeBuffer
    {
        friend class StoreBuffer;
        friend class mozilla::ReentrancyGuard;

        StoreBuffer *owner;

        T *base;      
        T *pos;       
        T *top;       

        



        T *highwater;

        




        EdgeSet duplicates;

        bool entered;

        MonoTypeBuffer(StoreBuffer *owner)
          : owner(owner), base(NULL), pos(NULL), top(NULL), entered(false)
        {
            duplicates.init();
        }

        MonoTypeBuffer &operator=(const MonoTypeBuffer& other) MOZ_DELETE;

        bool enable(uint8_t *region, size_t len);
        void disable();
        void clear();

        bool isEmpty() const { return pos == base; }
        bool isFull() const { JS_ASSERT(pos <= top); return pos == top; }
        bool isAboutToOverflow() const { return pos >= highwater; }

        
        void compactNotInSet(const Nursery &nursery);
        void compactRemoveDuplicates();

        



        virtual void compact();

        
        void put(const T &v) {
            mozilla::ReentrancyGuard g(*this);
            JS_ASSERT(!owner->inParallelSection());

            
            if (!pos)
                return;

            




            *pos++ = v;
            if (isAboutToOverflow()) {
                owner->setAboutToOverflow();
                if (isFull()) {
                    compact();
                    if (isFull()) {
                        owner->setOverflowed();
                        clear();
                    }
                }
            }
        }

        
        void mark(JSTracer *trc);
    };

    



    template <typename T>
    class RelocatableMonoTypeBuffer : public MonoTypeBuffer<T>
    {
        friend class StoreBuffer;

        RelocatableMonoTypeBuffer(StoreBuffer *owner)
          : MonoTypeBuffer<T>(owner)
        {}

        
        void compactMoved();
        virtual void compact();

        
        void unput(const T &v) {
            JS_ASSERT(!this->owner->inParallelSection());
            MonoTypeBuffer<T>::put(v.tagged());
        }
    };

    class GenericBuffer
    {
        friend class StoreBuffer;
        friend class mozilla::ReentrancyGuard;

        StoreBuffer *owner;

        uint8_t *base; 
        uint8_t *pos;  
        uint8_t *top;  

        bool entered;

        GenericBuffer(StoreBuffer *owner)
          : owner(owner), base(NULL), pos(NULL), top(NULL), entered(false)
        {}

        GenericBuffer &operator=(const GenericBuffer& other) MOZ_DELETE;

        bool enable(uint8_t *region, size_t len);
        void disable();
        void clear();

        
        void mark(JSTracer *trc);

        template <typename T>
        void put(const T &t) {
            mozilla::ReentrancyGuard g(*this);
            JS_ASSERT(!owner->inParallelSection());

            
            (void)static_cast<const BufferableRef*>(&t);

            
            if (!pos)
                return;

            
            if (unsigned(top - pos) < unsigned(sizeof(unsigned) + sizeof(T))) {
                owner->setOverflowed();
                return;
            }

            *((unsigned *)pos) = sizeof(T);
            pos += sizeof(unsigned);

            T *p = (T *)pos;
            new (p) T(t);
            pos += sizeof(T);
        }
    };

    class CellPtrEdge
    {
        friend class StoreBuffer;
        friend class StoreBuffer::MonoTypeBuffer<CellPtrEdge>;
        friend class StoreBuffer::RelocatableMonoTypeBuffer<CellPtrEdge>;

        Cell **edge;

        CellPtrEdge(Cell **v) : edge(v) {}
        bool operator==(const CellPtrEdge &other) const { return edge == other.edge; }
        bool operator!=(const CellPtrEdge &other) const { return edge != other.edge; }

        void *location() const { return (void *)edge; }

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

        Value *edge;

        ValueEdge(Value *v) : edge(v) {}
        bool operator==(const ValueEdge &other) const { return edge == other.edge; }
        bool operator!=(const ValueEdge &other) const { return edge != other.edge; }

        void *deref() const { return edge->isGCThing() ? edge->toGCThing() : NULL; }
        void *location() const { return (void *)edge; }

        bool inRememberedSet(const Nursery &nursery) const {
            return !nursery.isInside(edge) && nursery.isInside(deref());
        }

        bool isNullEdge() const {
            return !deref();
        }

        void mark(JSTracer *trc);

        ValueEdge tagged() const { return ValueEdge((Value *)(uintptr_t(edge) | 1)); }
        ValueEdge untagged() const { return ValueEdge((Value *)(uintptr_t(edge) & ~1)); }
        bool isTagged() const { return bool(uintptr_t(edge) & 1); }
    };

    struct SlotEdge
    {
        friend class StoreBuffer;
        friend class StoreBuffer::MonoTypeBuffer<SlotEdge>;

        JSObject *object;
        uint32_t offset;
        HeapSlot::Kind kind;

        SlotEdge(JSObject *object, HeapSlot::Kind kind, uint32_t offset)
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

    JSRuntime *runtime;
    const Nursery &nursery_;

    void *buffer;

    bool aboutToOverflow;
    bool overflowed;
    bool enabled;

    
    static const size_t ValueBufferSize = 1 * 1024 * sizeof(ValueEdge);
    static const size_t CellBufferSize = 8 * 1024 * sizeof(CellPtrEdge);
    static const size_t SlotBufferSize = 2 * 1024 * sizeof(SlotEdge);
    static const size_t WholeCellBufferSize = 2 * 1024 * sizeof(WholeCellEdges);
    static const size_t RelocValueBufferSize = 1 * 1024 * sizeof(ValueEdge);
    static const size_t RelocCellBufferSize = 1 * 1024 * sizeof(CellPtrEdge);
    static const size_t GenericBufferSize = 1 * 1024 * sizeof(int);
    static const size_t TotalSize = ValueBufferSize + CellBufferSize +
                                    SlotBufferSize + WholeCellBufferSize +
                                    RelocValueBufferSize + RelocCellBufferSize +
                                    GenericBufferSize;

  public:
    explicit StoreBuffer(JSRuntime *rt, const Nursery &nursery)
      : bufferVal(this), bufferCell(this), bufferSlot(this), bufferWholeCell(this),
        bufferRelocVal(this), bufferRelocCell(this), bufferGeneric(this),
        runtime(rt), nursery_(nursery), buffer(NULL), aboutToOverflow(false), overflowed(false),
        enabled(false)
    {}

    bool enable();
    void disable();
    bool isEnabled() { return enabled; }

    bool clear();

    
    bool isAboutToOverflow() const { return aboutToOverflow; }
    bool hasOverflowed() const { return overflowed; }

    
    void putValue(Value *v) {
        bufferVal.put(v);
    }
    void putCell(Cell **o) {
        bufferCell.put(o);
    }
    void putSlot(JSObject *obj, HeapSlot::Kind kind, uint32_t slot) {
        bufferSlot.put(SlotEdge(obj, kind, slot));
    }
    void putWholeCell(Cell *cell) {
        bufferWholeCell.put(WholeCellEdges(cell));
    }

    
    void putRelocatableValue(Value *v) {
        bufferRelocVal.put(v);
    }
    void putRelocatableCell(Cell **c) {
        bufferRelocCell.put(c);
    }
    void removeRelocatableValue(Value *v) {
        bufferRelocVal.unput(v);
    }
    void removeRelocatableCell(Cell **c) {
        bufferRelocCell.unput(c);
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
