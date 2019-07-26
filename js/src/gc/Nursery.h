






#ifndef jsgc_nursery_h___
#define jsgc_nursery_h___

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

class Nursery
{
  public:
    const static size_t Alignment = gc::ChunkSize;
    const static size_t NurserySize = gc::ChunkSize;
    const static size_t NurseryMask = NurserySize - 1;

    explicit Nursery(JSRuntime *rt)
      : runtime_(rt),
        position_(0)
    {}
    ~Nursery();

    bool enable();
    void disable();
    bool isEnabled() const { return bool(start()); }

    template <typename T>
    JS_ALWAYS_INLINE bool isInside(const T *p) const {
        return uintptr_t(p) >= start() && uintptr_t(p) < end();
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

  private:
    




    JSRuntime *runtime_;

    
    uintptr_t position_;

    




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

    
    const static size_t NurseryUsableSize = NurserySize - sizeof(JSRuntime *);

    struct Layout {
        char data[NurseryUsableSize];
        JSRuntime *runtime;
    };
    Layout &asLayout() {
        JS_STATIC_ASSERT(sizeof(Layout) == NurserySize);
        JS_ASSERT(start());
        return *reinterpret_cast<Layout *>(start());
    }

    JS_ALWAYS_INLINE uintptr_t start() const {
        JS_ASSERT(runtime_);
        return ((JS::shadow::Runtime *)runtime_)->gcNurseryStart_;
    }

    JS_ALWAYS_INLINE uintptr_t end() const {
        JS_ASSERT(runtime_);
        return ((JS::shadow::Runtime *)runtime_)->gcNurseryEnd_;
    }

    uintptr_t position() const { return position_; }

    JSRuntime *runtime() const { return runtime_; }

    
    HeapSlot *allocateHugeSlots(JSContext *cx, size_t nslots);

    
    void *allocateFromTenured(Zone *zone, gc::AllocKind thingKind);

    



    void *moveToTenured(gc::MinorCollectionTracer *trc, JSObject *src);
    void moveObjectToTenured(JSObject *dst, JSObject *src, gc::AllocKind dstKind);
    void moveElementsToTenured(JSObject *dst, JSObject *src, gc::AllocKind dstKind);
    void moveSlotsToTenured(JSObject *dst, JSObject *src, gc::AllocKind dstKind);

    
    void markFallback(gc::Cell *cell);
    void moveFallbackToTenured(gc::MinorCollectionTracer *trc);

    void markStoreBuffer(gc::MinorCollectionTracer *trc);

    




    void sweep(FreeOp *fop);

    static void MinorGCCallback(JSTracer *trc, void **thingp, JSGCTraceKind kind);
    static void MinorFallbackMarkingCallback(JSTracer *trc, void **thingp, JSGCTraceKind kind);
    static void MinorFallbackFixupCallback(JSTracer *trc, void **thingp, JSGCTraceKind kind);

    friend class gc::MinorCollectionTracer;
};

} 

#endif 
#endif 
