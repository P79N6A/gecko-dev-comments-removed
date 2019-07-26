






#ifndef gc_Nursery_h
#define gc_Nursery_h

#ifdef JSGC_GENERATIONAL

#include "jsalloc.h"
#include "jspubtd.h"

#include "ds/BitArray.h"
#include "gc/Heap.h"
#include "js/GCAPI.h"
#include "js/HashTable.h"
#include "js/HeapAPI.h"
#include "js/Value.h"
#include "js/Vector.h"

namespace JS {
struct Zone;
}

namespace js {

class ObjectElements;
class HeapSlot;

namespace gc {
class Cell;
class MinorCollectionTracer;
} 

namespace types {
struct TypeObject;
}

namespace jit {
class CodeGenerator;
class MacroAssembler;
class ICStubCompiler;
class BaselineCompiler;
}

class Nursery
{
  public:
    static const int NumNurseryChunks = 16;
    static const int LastNurseryChunk = NumNurseryChunks - 1;
    static const size_t Alignment = gc::ChunkSize;
    static const size_t NurserySize = gc::ChunkSize * NumNurseryChunks;

    explicit Nursery(JSRuntime *rt)
      : runtime_(rt),
        position_(0),
        currentStart_(0),
        currentEnd_(0),
        currentChunk_(0),
        numActiveChunks_(0)
    {}
    ~Nursery();

    bool init();

    void enable();
    void disable();
    bool isEnabled() const { return numActiveChunks_ != 0; }

    
    bool isEmpty() const;

    template <typename T>
    MOZ_ALWAYS_INLINE bool isInside(const T *p) const {
        return gc::IsInsideNursery((JS::shadow::Runtime *)runtime_, p);
    }

    



    JSObject *allocateObject(JSContext *cx, size_t size, size_t numDynamic);

    
    HeapSlot *allocateSlots(JSContext *cx, JSObject *obj, uint32_t nslots);

    
    ObjectElements *allocateElements(JSContext *cx, JSObject *obj, uint32_t nelems);

    
    HeapSlot *reallocateSlots(JSContext *cx, JSObject *obj, HeapSlot *oldSlots,
                              uint32_t oldCount, uint32_t newCount);

    
    ObjectElements *reallocateElements(JSContext *cx, JSObject *obj, ObjectElements *oldHeader,
                                       uint32_t oldCount, uint32_t newCount);

    
    void freeSlots(JSContext *cx, HeapSlot *slots);

    
    void notifyInitialSlots(gc::Cell *cell, HeapSlot *slots);

    typedef Vector<types::TypeObject *, 0, SystemAllocPolicy> TypeObjectList;

    



    void collect(JSRuntime *rt, JS::gcreason::Reason reason, TypeObjectList *pretenureTypes);

    




    template <typename T>
    MOZ_ALWAYS_INLINE bool getForwardedPointer(T **ref);

    
    void forwardBufferPointer(HeapSlot **pSlotsElems);

    size_t sizeOfHeap() { return start() ? NurserySize : 0; }

#ifdef JS_GC_ZEAL
    



    static const uint8_t FreshNursery = 0x2a;
    static const uint8_t SweptNursery = 0x2b;
    static const uint8_t AllocatedThing = 0x2c;
    void enterZealMode() {
        if (isEnabled())
            numActiveChunks_ = NumNurseryChunks;
    }
    void leaveZealMode() {
        if (isEnabled()) {
            JS_ASSERT(isEmpty());
            setCurrentChunk(0);
        }
    }
#endif

  private:
    




    JSRuntime *runtime_;

    
    uintptr_t position_;

    
    uintptr_t currentStart_;

    
    uintptr_t currentEnd_;

    
    int currentChunk_;

    
    int numActiveChunks_;

    




    typedef HashSet<HeapSlot *, PointerHasher<HeapSlot *, 3>, SystemAllocPolicy> HugeSlotsSet;
    HugeSlotsSet hugeSlots;

    
    static const size_t MaxNurserySlots = 128;

    
    static const size_t NurseryChunkUsableSize = gc::ChunkSize - sizeof(gc::ChunkTrailer);

    struct NurseryChunkLayout {
        char data[NurseryChunkUsableSize];
        gc::ChunkTrailer trailer;
        uintptr_t start() { return uintptr_t(&data); }
        uintptr_t end() { return uintptr_t(&trailer); }
    };
    static_assert(sizeof(NurseryChunkLayout) == gc::ChunkSize,
                  "Nursery chunk size must match gc::Chunk size.");
    NurseryChunkLayout &chunk(int index) const {
        JS_ASSERT(index < NumNurseryChunks);
        JS_ASSERT(start());
        return reinterpret_cast<NurseryChunkLayout *>(start())[index];
    }

    MOZ_ALWAYS_INLINE uintptr_t start() const {
        JS_ASSERT(runtime_);
        return ((JS::shadow::Runtime *)runtime_)->gcNurseryStart_;
    }

    MOZ_ALWAYS_INLINE uintptr_t heapEnd() const {
        JS_ASSERT(runtime_);
        return ((JS::shadow::Runtime *)runtime_)->gcNurseryEnd_;
    }

    MOZ_ALWAYS_INLINE void setCurrentChunk(int chunkno) {
        JS_ASSERT(chunkno < NumNurseryChunks);
        JS_ASSERT(chunkno < numActiveChunks_);
        currentChunk_ = chunkno;
        position_ = chunk(chunkno).start();
        currentStart_ = chunk(0).start();
        currentEnd_ = chunk(chunkno).end();
    }

    MOZ_ALWAYS_INLINE uintptr_t allocationEnd() const {
        JS_ASSERT(numActiveChunks_ > 0);
        return chunk(numActiveChunks_ - 1).end();
    }

    MOZ_ALWAYS_INLINE bool isFullyGrown() const {
        return numActiveChunks_ == NumNurseryChunks;
    }

    MOZ_ALWAYS_INLINE uintptr_t currentEnd() const {
        JS_ASSERT(runtime_);
        JS_ASSERT(currentEnd_ == chunk(currentChunk_).end());
        return currentEnd_;
    }
    void *addressOfCurrentEnd() const {
        JS_ASSERT(runtime_);
        return (void *)&currentEnd_;
    }

    uintptr_t position() const { return position_; }
    void *addressOfPosition() const { return (void*)&position_; }

    JSRuntime *runtime() const { return runtime_; }

    
    HeapSlot *allocateHugeSlots(JSContext *cx, size_t nslots);

    
    void *allocateFromTenured(JS::Zone *zone, gc::AllocKind thingKind);

    struct TenureCountCache;

    
    void *allocate(size_t size);

    



    void collectToFixedPoint(gc::MinorCollectionTracer *trc, TenureCountCache &tenureCounts);
    MOZ_ALWAYS_INLINE void traceObject(gc::MinorCollectionTracer *trc, JSObject *src);
    MOZ_ALWAYS_INLINE void markSlots(gc::MinorCollectionTracer *trc, HeapSlot *vp, uint32_t nslots);
    MOZ_ALWAYS_INLINE void markSlots(gc::MinorCollectionTracer *trc, HeapSlot *vp, HeapSlot *end);
    MOZ_ALWAYS_INLINE void markSlot(gc::MinorCollectionTracer *trc, HeapSlot *slotp);
    void *moveToTenured(gc::MinorCollectionTracer *trc, JSObject *src);
    size_t moveObjectToTenured(JSObject *dst, JSObject *src, gc::AllocKind dstKind);
    size_t moveElementsToTenured(JSObject *dst, JSObject *src, gc::AllocKind dstKind);
    size_t moveSlotsToTenured(JSObject *dst, JSObject *src, gc::AllocKind dstKind);

    
    void setSlotsForwardingPointer(HeapSlot *oldSlots, HeapSlot *newSlots, uint32_t nslots);
    void setElementsForwardingPointer(ObjectElements *oldHeader, ObjectElements *newHeader,
                                      uint32_t nelems);

    
    void freeHugeSlots(JSRuntime *rt);

    



    void sweep(JSRuntime *rt);

    
    void growAllocableSpace();
    void shrinkAllocableSpace();

    static void MinorGCCallback(JSTracer *trc, void **thingp, JSGCTraceKind kind);

    friend class gc::MinorCollectionTracer;
    friend class jit::CodeGenerator;
    friend class jit::MacroAssembler;
    friend class jit::ICStubCompiler;
    friend class jit::BaselineCompiler;
};

} 

#endif 
#endif 
