






#ifndef gc_Nursery_h
#define gc_Nursery_h

#include "jsalloc.h"
#include "jspubtd.h"

#include "ds/BitArray.h"
#include "gc/Heap.h"
#include "gc/Memory.h"
#include "js/Class.h"
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
class NativeObject;
class Nursery;
class HeapSlot;
class ObjectGroup;

void SetGCZeal(JSRuntime*, uint8_t, uint32_t);

namespace gc {
struct Cell;
class MinorCollectionTracer;
class RelocationOverlay;
struct TenureCountCache;
} 

namespace jit {
class MacroAssembler;
} 

class TenuringTracer : public JSTracer
{
    friend class Nursery;
    Nursery& nursery_;

    
    size_t tenuredSize;

    
    
    
    gc::RelocationOverlay* head;
    gc::RelocationOverlay** tail;

    TenuringTracer(JSRuntime* rt, Nursery* nursery);

  public:
    const Nursery& nursery() const { return nursery_; }

    
    template <typename T> void traverse(T* thingp);

    void insertIntoFixupList(gc::RelocationOverlay* entry);

    
    void traceObject(JSObject* src);
    void traceObjectSlots(NativeObject* nobj, uint32_t start, uint32_t length);
    void traceSlots(JS::Value* vp, uint32_t nslots) { traceSlots(vp, vp + nslots); }

  private:
    Nursery& nursery() { return nursery_; }

    JSObject* moveToTenured(JSObject* src);
    size_t moveObjectToTenured(JSObject* dst, JSObject* src, gc::AllocKind dstKind);
    size_t moveElementsToTenured(NativeObject* dst, NativeObject* src, gc::AllocKind dstKind);
    size_t moveSlotsToTenured(NativeObject* dst, NativeObject* src, gc::AllocKind dstKind);

    void traceSlots(JS::Value* vp, JS::Value* end);
};

class Nursery
{
  public:
    static const size_t Alignment = gc::ChunkSize;
    static const size_t ChunkShift = gc::ChunkShift;

    explicit Nursery(JSRuntime* rt)
      : runtime_(rt),
        position_(0),
        currentStart_(0),
        currentEnd_(0),
        heapStart_(0),
        heapEnd_(0),
        currentChunk_(0),
        numActiveChunks_(0),
        numNurseryChunks_(0),
        profileThreshold_(0),
        enableProfiling_(false),
        freeMallocedBuffersTask(nullptr)
    {}
    ~Nursery();

    bool init(uint32_t maxNurseryBytes);

    bool exists() const { return numNurseryChunks_ != 0; }
    size_t numChunks() const { return numNurseryChunks_; }
    size_t nurserySize() const { return numNurseryChunks_ << ChunkShift; }

    void enable();
    void disable();
    bool isEnabled() const { return numActiveChunks_ != 0; }

    
    bool isEmpty() const;

    



    MOZ_ALWAYS_INLINE bool isInside(gc::Cell* cellp) const = delete;
    MOZ_ALWAYS_INLINE bool isInside(const void* p) const {
        return uintptr_t(p) >= heapStart_ && uintptr_t(p) < heapEnd_;
    }

    



    JSObject* allocateObject(JSContext* cx, size_t size, size_t numDynamic, const js::Class* clasp);

    
    void* allocateBuffer(JS::Zone* zone, uint32_t nbytes);

    



    void* allocateBuffer(JSObject* obj, uint32_t nbytes);

    
    void* reallocateBuffer(JSObject* obj, void* oldBuffer,
                           uint32_t oldBytes, uint32_t newBytes);

    
    void freeBuffer(void* buffer);

    typedef Vector<ObjectGroup*, 0, SystemAllocPolicy> ObjectGroupList;

    



    void collect(JSRuntime* rt, JS::gcreason::Reason reason, ObjectGroupList* pretenureGroups);

    




    MOZ_ALWAYS_INLINE bool getForwardedPointer(JSObject** ref) const;

    
    void forwardBufferPointer(HeapSlot** pSlotsElems);

    void maybeSetForwardingPointer(JSTracer* trc, void* oldData, void* newData, bool direct) {
        if (trc->isTenuringTracer() && isInside(oldData))
            setForwardingPointer(oldData, newData, direct);
    }

    
    void removeMallocedBuffer(void* buffer) {
        mallocedBuffers.remove(buffer);
    }

    void waitBackgroundFreeEnd();

    size_t sizeOfHeapCommitted() const {
        return numActiveChunks_ * gc::ChunkSize;
    }
    size_t sizeOfHeapDecommitted() const {
        return (numNurseryChunks_ - numActiveChunks_) * gc::ChunkSize;
    }
    size_t sizeOfMallocedBuffers(mozilla::MallocSizeOf mallocSizeOf) const {
        size_t total = 0;
        for (MallocedBuffersSet::Range r = mallocedBuffers.all(); !r.empty(); r.popFront())
            total += mallocSizeOf(r.front());
        total += mallocedBuffers.sizeOfExcludingThis(mallocSizeOf);
        return total;
    }

    MOZ_ALWAYS_INLINE uintptr_t start() const {
        return heapStart_;
    }

    MOZ_ALWAYS_INLINE uintptr_t heapEnd() const {
        return heapEnd_;
    }

#ifdef JS_GC_ZEAL
    void enterZealMode();
    void leaveZealMode();
#endif

  private:
    




    JSRuntime* runtime_;

    
    uintptr_t position_;

    
    uintptr_t currentStart_;

    
    uintptr_t currentEnd_;

    
    uintptr_t heapStart_;
    uintptr_t heapEnd_;

    
    int currentChunk_;

    
    int numActiveChunks_;

    
    int numNurseryChunks_;

    
    int64_t profileThreshold_;
    bool enableProfiling_;

    




    typedef HashSet<void*, PointerHasher<void*, 3>, SystemAllocPolicy> MallocedBuffersSet;
    MallocedBuffersSet mallocedBuffers;

    
    struct FreeMallocedBuffersTask;
    FreeMallocedBuffersTask* freeMallocedBuffersTask;

    






    typedef HashMap<void*, void*, PointerHasher<void*, 1>, SystemAllocPolicy> ForwardedBufferMap;
    ForwardedBufferMap forwardedBuffers;

    
    static const size_t MaxNurseryBufferSize = 1024;

    
    static const size_t NurseryChunkUsableSize = gc::ChunkSize - sizeof(gc::ChunkTrailer);

    struct NurseryChunkLayout {
        char data[NurseryChunkUsableSize];
        gc::ChunkTrailer trailer;
        uintptr_t start() { return uintptr_t(&data); }
        uintptr_t end() { return uintptr_t(&trailer); }
    };
    static_assert(sizeof(NurseryChunkLayout) == gc::ChunkSize,
                  "Nursery chunk size must match gc::Chunk size.");
    NurseryChunkLayout& chunk(int index) const {
        MOZ_ASSERT(index < numNurseryChunks_);
        MOZ_ASSERT(start());
        return reinterpret_cast<NurseryChunkLayout*>(start())[index];
    }

    MOZ_ALWAYS_INLINE void initChunk(int chunkno) {
        NurseryChunkLayout& c = chunk(chunkno);
        c.trailer.storeBuffer = JS::shadow::Runtime::asShadowRuntime(runtime())->gcStoreBufferPtr();
        c.trailer.location = gc::ChunkLocationBitNursery;
        c.trailer.runtime = runtime();
    }

    MOZ_ALWAYS_INLINE void setCurrentChunk(int chunkno) {
        MOZ_ASSERT(chunkno < numNurseryChunks_);
        MOZ_ASSERT(chunkno < numActiveChunks_);
        currentChunk_ = chunkno;
        position_ = chunk(chunkno).start();
        currentEnd_ = chunk(chunkno).end();
        initChunk(chunkno);
    }

    void updateDecommittedRegion();

    MOZ_ALWAYS_INLINE uintptr_t allocationEnd() const {
        MOZ_ASSERT(numActiveChunks_ > 0);
        return chunk(numActiveChunks_ - 1).end();
    }

    MOZ_ALWAYS_INLINE uintptr_t currentEnd() const {
        MOZ_ASSERT(runtime_);
        MOZ_ASSERT(currentEnd_ == chunk(currentChunk_).end());
        return currentEnd_;
    }
    void* addressOfCurrentEnd() const {
        MOZ_ASSERT(runtime_);
        return (void*)&currentEnd_;
    }

    uintptr_t position() const { return position_; }
    void* addressOfPosition() const { return (void*)&position_; }

    JSRuntime* runtime() const { return runtime_; }

    
    gc::TenuredCell* allocateFromTenured(JS::Zone* zone, gc::AllocKind thingKind);

    
    void* allocate(size_t size);

    



    void collectToFixedPoint(TenuringTracer& trc, gc::TenureCountCache& tenureCounts);

    
    void setForwardingPointer(void* oldData, void* newData, bool direct);

    void setSlotsForwardingPointer(HeapSlot* oldSlots, HeapSlot* newSlots, uint32_t nslots);
    void setElementsForwardingPointer(ObjectElements* oldHeader, ObjectElements* newHeader,
                                      uint32_t nelems);

    
    void freeMallocedBuffers();

    



    void sweep();

    
    void growAllocableSpace();
    void shrinkAllocableSpace();

    friend class TenuringTracer;
    friend class gc::MinorCollectionTracer;
    friend class jit::MacroAssembler;
};

} 

#endif 
