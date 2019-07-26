






#ifndef gc_Nursery_h
#define gc_Nursery_h

#ifdef JSGC_GENERATIONAL

#include "ds/BitArray.h"
#include "js/HashTable.h"

#include "jsgc.h"
#include "jspubtd.h"

namespace js {

class ObjectElements;

namespace gc {
class MinorCollectionTracer;
} 

namespace ion {
class CodeGenerator;
class MacroAssembler;
class ICStubCompiler;
class BaselineCompiler;
}

class Nursery
{
  public:
    const static int NumNurseryChunks = 8;
    const static int LastNurseryChunk = NumNurseryChunks - 1;
    const static size_t Alignment = gc::ChunkSize;
    const static size_t NurserySize = gc::ChunkSize * NumNurseryChunks;

    explicit Nursery(JSRuntime *rt)
      : runtime_(rt),
        position_(0),
        currentEnd_(0),
        currentChunk_(0),
        numActiveChunks_(0)
    {}
    ~Nursery();

    bool init();

    void enable();
    void disable();
    bool isEnabled() const { return numActiveChunks_ != 0; }

    template <typename T>
    JS_ALWAYS_INLINE bool isInside(const T *p) const {
        return uintptr_t(p) >= start() && uintptr_t(p) < heapEnd();
    }

    



    void *allocate(size_t size);

    
    HeapSlot *allocateSlots(JSContext *cx, JSObject *obj, uint32_t nslots);

    
    ObjectElements *allocateElements(JSContext *cx, JSObject *obj, uint32_t nelems);

    
    HeapSlot *reallocateSlots(JSContext *cx, JSObject *obj, HeapSlot *oldSlots,
                              uint32_t oldCount, uint32_t newCount);

    
    ObjectElements *reallocateElements(JSContext *cx, JSObject *obj, ObjectElements *oldHeader,
                                       uint32_t oldCount, uint32_t newCount);

    
    void notifyInitialSlots(gc::Cell *cell, HeapSlot *slots);

    
    void collect(JSRuntime *rt, JS::gcreason::Reason reason);

    




    template <typename T>
    JS_ALWAYS_INLINE bool getForwardedPointer(T **ref);

    
    void forwardBufferPointer(HeapSlot **pSlotsElems);

  private:
    




    JSRuntime *runtime_;

    
    uintptr_t position_;

    
    uintptr_t currentEnd_;

    
    int currentChunk_;

    
    int numActiveChunks_;

    




    typedef HashSet<HeapSlot *, PointerHasher<HeapSlot *, 3>, SystemAllocPolicy> HugeSlotsSet;
    HugeSlotsSet hugeSlots;

    
    const static size_t ThingAlignment = sizeof(Value);
    const static size_t FallbackBitmapBits = NurserySize / ThingAlignment;
    BitArray<FallbackBitmapBits> fallbackBitmap;

#ifdef DEBUG
    



    const static uint8_t FreshNursery = 0x2a;
    const static uint8_t SweptNursery = 0x2b;
    const static uint8_t AllocatedThing = 0x2c;
#endif

    
    const static size_t MaxNurserySlots = 100;

    
    const static size_t NurseryChunkUsableSize = gc::ChunkSize - sizeof(JSRuntime *);

    struct NurseryChunkLayout {
        char data[NurseryChunkUsableSize];
        JSRuntime *runtime;
        uintptr_t start() { return uintptr_t(&data); }
        uintptr_t end() { return uintptr_t(&runtime); }
    };
    NurseryChunkLayout &chunk(int index) const {
        JS_STATIC_ASSERT(sizeof(NurseryChunkLayout) == gc::ChunkSize);
        JS_ASSERT(index < NumNurseryChunks);
        JS_ASSERT(start());
        return reinterpret_cast<NurseryChunkLayout *>(start())[index];
    }

    JS_ALWAYS_INLINE uintptr_t start() const {
        JS_ASSERT(runtime_);
        return ((JS::shadow::Runtime *)runtime_)->gcNurseryStart_;
    }

    JS_ALWAYS_INLINE uintptr_t heapEnd() const {
        JS_ASSERT(runtime_);
        return ((JS::shadow::Runtime *)runtime_)->gcNurseryEnd_;
    }

    JS_ALWAYS_INLINE void setCurrentChunk(int chunkno) {
        JS_ASSERT(chunkno < NumNurseryChunks);
        JS_ASSERT(chunkno < numActiveChunks_);
        currentChunk_ = chunkno;
        position_ = chunk(chunkno).start();
        currentEnd_ = chunk(chunkno).end();
    }

    JS_ALWAYS_INLINE uintptr_t allocationEnd() const {
        JS_ASSERT(numActiveChunks_ > 0);
        return chunk(numActiveChunks_ - 1).end();
    }

    JS_ALWAYS_INLINE uintptr_t currentEnd() const {
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

    
    void *allocateFromTenured(Zone *zone, gc::AllocKind thingKind);

    



    void collectToFixedPoint(gc::MinorCollectionTracer *trc);
    JS_ALWAYS_INLINE void traceObject(gc::MinorCollectionTracer *trc, JSObject *src);
    JS_ALWAYS_INLINE void markSlots(gc::MinorCollectionTracer *trc, HeapSlot *vp, uint32_t nslots);
    JS_ALWAYS_INLINE void markSlots(gc::MinorCollectionTracer *trc, HeapSlot *vp, HeapSlot *end);
    JS_ALWAYS_INLINE void markSlot(gc::MinorCollectionTracer *trc, HeapSlot *slotp);
    void *moveToTenured(gc::MinorCollectionTracer *trc, JSObject *src);
    size_t moveObjectToTenured(JSObject *dst, JSObject *src, gc::AllocKind dstKind);
    size_t moveElementsToTenured(JSObject *dst, JSObject *src, gc::AllocKind dstKind);
    size_t moveSlotsToTenured(JSObject *dst, JSObject *src, gc::AllocKind dstKind);

    
    void setSlotsForwardingPointer(HeapSlot *oldSlots, HeapSlot *newSlots, uint32_t nslots);
    void setElementsForwardingPointer(ObjectElements *oldHeader, ObjectElements *newHeader,
                                      uint32_t nelems);

    
    void markFallback(gc::Cell *cell);
    void moveFallbackToTenured(gc::MinorCollectionTracer *trc);

    void markStoreBuffer(gc::MinorCollectionTracer *trc);

    




    void sweep(FreeOp *fop);

    
    void growAllocableSpace();
    void shrinkAllocableSpace();

    static void MinorGCCallback(JSTracer *trc, void **thingp, JSGCTraceKind kind);
    static void MinorFallbackMarkingCallback(JSTracer *trc, void **thingp, JSGCTraceKind kind);
    static void MinorFallbackFixupCallback(JSTracer *trc, void **thingp, JSGCTraceKind kind);

    friend class gc::MinorCollectionTracer;
    friend class ion::CodeGenerator;
    friend class ion::MacroAssembler;
    friend class ion::ICStubCompiler;
    friend class ion::BaselineCompiler;
};

} 

#endif 
#endif 
