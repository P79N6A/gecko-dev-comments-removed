






#ifndef gc_ForkJoinNursery_h
#define gc_ForkJoinNursery_h

#ifdef JSGC_FJGENERATIONAL

#ifndef JSGC_GENERATIONAL
#error "JSGC_GENERATIONAL is required for the ForkJoinNursery"
#endif

#include "jsalloc.h"
#include "jspubtd.h"

#include "gc/Heap.h"
#include "gc/Memory.h"
#include "gc/Nursery.h"

#include "js/HashTable.h"
#include "js/TracingAPI.h"

namespace js {
class ObjectElements;
class HeapSlot;
class ForkJoinShared;
}

namespace js {
namespace gc {

class ForkJoinGCShared;
class ForkJoinNursery;
class ForkJoinNurseryCollectionTracer;
class RelocationOverlay;










class ForkJoinNurseryCollectionTracer : public JSTracer
{
    friend class ForkJoinNursery;

  public:
    ForkJoinNurseryCollectionTracer(JSRuntime *rt, ForkJoinNursery *nursery);

  private:
    ForkJoinNursery *const nursery_;
};



struct ForkJoinNurseryChunk
{
    
    static const size_t UsableSize = ChunkSize - sizeof(ChunkTrailer);

    char data[UsableSize];
    ChunkTrailer trailer;
    uintptr_t start() { return uintptr_t(&data); }
    uintptr_t end() { return uintptr_t(&trailer); }
};




class ForkJoinGCShared
{
  public:
    explicit ForkJoinGCShared(ForkJoinShared *shared) : shared_(shared) {}

    JSRuntime *runtime();
    JS::Zone *zone();

    
    JSObject *updatable();

    
    ForkJoinNurseryChunk *allocateNurseryChunk();

    
    void freeNurseryChunk(ForkJoinNurseryChunk *p);

    
    void spewGC(const char *fmt, ...);

  private:
    ForkJoinShared *const shared_;
};





class ForkJoinNursery
{
    friend class ForkJoinNurseryCollectionTracer;
    friend class RelocationOverlay;

    static_assert(sizeof(ForkJoinNurseryChunk) == ChunkSize,
                  "ForkJoinNursery chunk size must match Chunk size.");
  public:
    ForkJoinNursery(ForkJoinContext *cx, ForkJoinGCShared *shared, Allocator *tenured);
    ~ForkJoinNursery();

    
    bool initialize();

    
    
    void minorGC();

    
    
    void evacuatingGC();

    
    
    
    
    
    
    
    
    
    
    JSObject *allocateObject(size_t size, size_t numDynamic, bool& tooLarge);

    
    
    
    
    
    
    
    
    HeapSlot *allocateSlots(JSObject *obj, uint32_t nslots);
    HeapSlot *reallocateSlots(JSObject *obj, HeapSlot *oldSlots,
                              uint32_t oldCount, uint32_t newCount);
    ObjectElements *allocateElements(JSObject *obj, uint32_t nelems);
    ObjectElements *reallocateElements(JSObject *obj, ObjectElements *oldHeader,
                                       uint32_t oldCount, uint32_t newCount);

    
    void freeSlots(HeapSlot *slots);

    
    static void MinorGCCallback(JSTracer *trcArg, void **thingp, JSGCTraceKind kind);

    
    static void forwardBufferPointer(JSTracer *trc, HeapSlot **pSlotsElems);

    
    MOZ_ALWAYS_INLINE bool isInsideNewspace(const void *obj);

    
    MOZ_ALWAYS_INLINE bool isInsideFromspace(const void *obj);

    MOZ_ALWAYS_INLINE bool isForwarded(Cell *cell);

    template <typename T>
    MOZ_ALWAYS_INLINE bool getForwardedPointer(T **ref);

    static size_t offsetOfPosition() {
        return offsetof(ForkJoinNursery, position_);
    }

    static size_t offsetOfCurrentEnd() {
        return offsetof(ForkJoinNursery, currentEnd_);
    }

  private:
    
    
    
    
    
    static const size_t MaxNurserySlots = 2048;

    
    
    
    
    
    
    
    
    static const size_t MaxNurseryChunks = 16;

    
    
    
    static const int NurseryLoadFactor = 3;

    
    
    
    void *allocate(size_t size);

    
    HeapSlot *allocateHugeSlots(JSObject *obj, size_t nslots);

    
    
    
    HeapSlot *reallocateHugeSlots(JSObject *obj, HeapSlot *oldSlots,
                                  uint32_t oldCount, uint32_t newCount);

    
    void sweepHugeSlots();

    
    
    
    bool setCurrentChunk(int index);

    enum PJSCollectionOp {
        Evacuate = 1,
        Collect = 2,
        Recreate = 4
    };

    
    void pjsCollection(int op );
    bool initNewspace();
    void flip();
    void forwardFromRoots(ForkJoinNurseryCollectionTracer *trc);
    void forwardFromUpdatable(ForkJoinNurseryCollectionTracer *trc);
    void forwardFromStack(ForkJoinNurseryCollectionTracer *trc);
    void forwardFromTenured(ForkJoinNurseryCollectionTracer *trc);
    void forwardFromRematerializedFrames(ForkJoinNurseryCollectionTracer *trc);
    void collectToFixedPoint(ForkJoinNurseryCollectionTracer *trc);
    void freeFromspace();
    void computeNurserySizeAfterGC(size_t live, const char **msg);

    AllocKind getObjectAllocKind(JSObject *src);
    void *allocateInTospaceInfallible(size_t thingSize);
    void *allocateInTospace(AllocKind thingKind);
    template <typename T> T *allocateInTospace(size_t nelem);
    MOZ_ALWAYS_INLINE bool shouldMoveObject(void **thingp);
    void *moveObjectToTospace(JSObject *src);
    size_t copyObjectToTospace(JSObject *dst, JSObject *src, gc::AllocKind dstKind);
    size_t copyElementsToTospace(JSObject *dst, JSObject *src, gc::AllocKind dstKind);
    size_t copySlotsToTospace(JSObject *dst, JSObject *src, gc::AllocKind dstKind);
    MOZ_ALWAYS_INLINE void insertIntoFixupList(RelocationOverlay *entry);

    void setSlotsForwardingPointer(HeapSlot *oldSlots, HeapSlot *newSlots, uint32_t nslots);
    void setElementsForwardingPointer(ObjectElements *oldHeader, ObjectElements *newHeader,
                                      uint32_t nelems);

    MOZ_ALWAYS_INLINE void traceObject(ForkJoinNurseryCollectionTracer *trc, JSObject *obj);
    MOZ_ALWAYS_INLINE void markSlots(HeapSlot *vp, uint32_t nslots);
    MOZ_ALWAYS_INLINE void markSlots(HeapSlot *vp, HeapSlot *end);
    MOZ_ALWAYS_INLINE void markSlot(HeapSlot *slotp);

    ForkJoinContext *const cx_;      
    Allocator *const tenured_;       
    ForkJoinGCShared *const shared_; 
    JS::Zone *evacuationZone_;       
                                     

    uintptr_t currentStart_;         
    uintptr_t currentEnd_;           
    uintptr_t position_;             
    unsigned currentChunk_;          
    unsigned numActiveChunks_;       
    unsigned numFromspaceChunks_;    
    bool mustEvacuate_;              

    bool isEvacuating_;              
    size_t movedSize_;               
    RelocationOverlay *head_;        
    RelocationOverlay **tail_;       

    typedef HashSet<HeapSlot *, PointerHasher<HeapSlot *, 3>, SystemAllocPolicy> HugeSlotsSet;

    HugeSlotsSet hugeSlots[2];       

    int hugeSlotsNew;                
    int hugeSlotsFrom;               

    ForkJoinNurseryChunk *newspace[MaxNurseryChunks];  
    ForkJoinNurseryChunk *fromspace[MaxNurseryChunks]; 
};

} 
} 

#endif 

#endif 
