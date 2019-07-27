






#ifndef gc_Nursery_h
#define gc_Nursery_h

#ifdef JSGC_GENERATIONAL

#include "jsalloc.h"
#include "jspubtd.h"

#include "ds/BitArray.h"
#include "gc/Heap.h"
#include "gc/Memory.h"
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
void SetGCZeal(JSRuntime *, uint8_t, uint32_t);

namespace gc {
struct Cell;
class Collector;
class MinorCollectionTracer;
class ForkJoinNursery;
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
    static const size_t Alignment = gc::ChunkSize;
    static const size_t ChunkShift = gc::ChunkShift;

    explicit Nursery(JSRuntime *rt)
      : runtime_(rt),
        position_(0),
        currentStart_(0),
        currentEnd_(0),
        heapStart_(0),
        heapEnd_(0),
        currentChunk_(0),
        numActiveChunks_(0),
        numNurseryChunks_(0)
    {}
    ~Nursery();

    bool init(uint32_t numNurseryChunks);

    bool exists() const { return numNurseryChunks_ != 0; }
    size_t numChunks() const { return numNurseryChunks_; }
    size_t nurserySize() const { return numNurseryChunks_ << ChunkShift; }

    void enable();
    void disable();
    bool isEnabled() const { return numActiveChunks_ != 0; }

    
    bool isEmpty() const;

    



    MOZ_ALWAYS_INLINE bool isInside(gc::Cell *cellp) const MOZ_DELETE;
    MOZ_ALWAYS_INLINE bool isInside(const void *p) const {
        return uintptr_t(p) >= heapStart_ && uintptr_t(p) < heapEnd_;
    }

    



    JSObject *allocateObject(JSContext *cx, size_t size, size_t numDynamic);

    
    HeapSlot *allocateSlots(JSObject *obj, uint32_t nslots);

    
    ObjectElements *allocateElements(JSObject *obj, uint32_t nelems);

    
    HeapSlot *reallocateSlots(JSObject *obj, HeapSlot *oldSlots,
                              uint32_t oldCount, uint32_t newCount);

    
    ObjectElements *reallocateElements(JSObject *obj, ObjectElements *oldHeader,
                                       uint32_t oldCount, uint32_t newCount);

    
    void freeSlots(HeapSlot *slots);

    typedef Vector<types::TypeObject *, 0, SystemAllocPolicy> TypeObjectList;

    



    void collect(JSRuntime *rt, JS::gcreason::Reason reason, TypeObjectList *pretenureTypes);

    




    template <typename T>
    MOZ_ALWAYS_INLINE bool getForwardedPointer(T **ref);

    
    void forwardBufferPointer(HeapSlot **pSlotsElems);

    static void forwardBufferPointer(JSTracer* trc, HeapSlot **pSlotsElems);

    size_t sizeOfHeapCommitted() const {
        return numActiveChunks_ * gc::ChunkSize;
    }
    size_t sizeOfHeapDecommitted() const {
        return (numNurseryChunks_ - numActiveChunks_) * gc::ChunkSize;
    }
    size_t sizeOfHugeSlots(mozilla::MallocSizeOf mallocSizeOf) const {
        size_t total = 0;
        for (HugeSlotsSet::Range r = hugeSlots.all(); !r.empty(); r.popFront())
            total += mallocSizeOf(r.front());
        total += hugeSlots.sizeOfExcludingThis(mallocSizeOf);
        return total;
    }

    MOZ_ALWAYS_INLINE uintptr_t start() const {
        return heapStart_;
    }

    MOZ_ALWAYS_INLINE uintptr_t heapEnd() const {
        return heapEnd_;
    }

    static bool IsMinorCollectionTracer(JSTracer *trc) {
        return trc->callback == MinorGCCallback;
    }

#ifdef JS_GC_ZEAL
    void enterZealMode();
    void leaveZealMode();
#endif

  private:
    




    JSRuntime *runtime_;

    
    uintptr_t position_;

    
    uintptr_t currentStart_;

    
    uintptr_t currentEnd_;

    
    uintptr_t heapStart_;
    uintptr_t heapEnd_;

    
    int currentChunk_;

    
    int numActiveChunks_;

    
    int numNurseryChunks_;

    




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
        JS_ASSERT(index < numNurseryChunks_);
        JS_ASSERT(start());
        return reinterpret_cast<NurseryChunkLayout *>(start())[index];
    }

    MOZ_ALWAYS_INLINE void initChunk(int chunkno) {
        NurseryChunkLayout &c = chunk(chunkno);
        c.trailer.storeBuffer = JS::shadow::Runtime::asShadowRuntime(runtime())->gcStoreBufferPtr();
        c.trailer.location = gc::ChunkLocationBitNursery;
        c.trailer.runtime = runtime();
    }

    MOZ_ALWAYS_INLINE void setCurrentChunk(int chunkno) {
        JS_ASSERT(chunkno < numNurseryChunks_);
        JS_ASSERT(chunkno < numActiveChunks_);
        currentChunk_ = chunkno;
        position_ = chunk(chunkno).start();
        currentEnd_ = chunk(chunkno).end();
        initChunk(chunkno);
    }

    void updateDecommittedRegion();

    MOZ_ALWAYS_INLINE uintptr_t allocationEnd() const {
        JS_ASSERT(numActiveChunks_ > 0);
        return chunk(numActiveChunks_ - 1).end();
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

    
    HeapSlot *allocateHugeSlots(JS::Zone *zone, size_t nslots);

    
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
    void forwardTypedArrayPointers(JSObject *dst, JSObject *src);

    
    void setSlotsForwardingPointer(HeapSlot *oldSlots, HeapSlot *newSlots, uint32_t nslots);
    void setElementsForwardingPointer(ObjectElements *oldHeader, ObjectElements *newHeader,
                                      uint32_t nelems);

    
    void freeHugeSlots();

    



    void sweep();

    
    void growAllocableSpace();
    void shrinkAllocableSpace();

    static void MinorGCCallback(JSTracer *trc, void **thingp, JSGCTraceKind kind);

    friend class gc::MinorCollectionTracer;
    friend class jit::MacroAssembler;
};

} 

#endif 
#endif 
