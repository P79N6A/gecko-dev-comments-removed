





#ifndef gc_Heap_h
#define gc_Heap_h

#include "mozilla/ArrayUtils.h"
#include "mozilla/Atomics.h"
#include "mozilla/Attributes.h"
#include "mozilla/PodOperations.h"

#include <stddef.h>
#include <stdint.h>

#include "jspubtd.h"
#include "jstypes.h"
#include "jsutil.h"

#include "ds/BitArray.h"
#include "gc/Memory.h"
#include "js/GCAPI.h"
#include "js/HeapAPI.h"
#include "js/TracingAPI.h"

struct JSCompartment;

struct JSRuntime;

namespace JS {
namespace shadow {
struct Runtime;
}
}

namespace js {

class AutoLockGC;
class FreeOp;

#ifdef DEBUG
extern bool
RuntimeFromMainThreadIsHeapMajorCollecting(JS::shadow::Zone *shadowZone);



extern bool
CurrentThreadIsIonCompiling();
#endif

namespace gc {

struct Arena;
class ArenaList;
class SortedArenaList;
struct ArenaHeader;
struct Chunk;

extern void
MarkKind(JSTracer *trc, void **thingp, JSGCTraceKind kind);






enum InitialHeap {
    DefaultHeap,
    TenuredHeap
};


enum AllocKind {
    FINALIZE_OBJECT0,
    FINALIZE_OBJECT0_BACKGROUND,
    FINALIZE_OBJECT2,
    FINALIZE_OBJECT2_BACKGROUND,
    FINALIZE_OBJECT4,
    FINALIZE_OBJECT4_BACKGROUND,
    FINALIZE_OBJECT8,
    FINALIZE_OBJECT8_BACKGROUND,
    FINALIZE_OBJECT12,
    FINALIZE_OBJECT12_BACKGROUND,
    FINALIZE_OBJECT16,
    FINALIZE_OBJECT16_BACKGROUND,
    FINALIZE_OBJECT_LAST = FINALIZE_OBJECT16_BACKGROUND,
    FINALIZE_SCRIPT,
    FINALIZE_LAZY_SCRIPT,
    FINALIZE_SHAPE,
    FINALIZE_ACCESSOR_SHAPE,
    FINALIZE_BASE_SHAPE,
    FINALIZE_TYPE_OBJECT,
    FINALIZE_FAT_INLINE_STRING,
    FINALIZE_STRING,
    FINALIZE_EXTERNAL_STRING,
    FINALIZE_SYMBOL,
    FINALIZE_JITCODE,
    FINALIZE_LAST = FINALIZE_JITCODE
};

static const unsigned FINALIZE_LIMIT = FINALIZE_LAST + 1;
static const unsigned FINALIZE_OBJECT_LIMIT = FINALIZE_OBJECT_LAST + 1;

static inline JSGCTraceKind
MapAllocToTraceKind(AllocKind kind)
{
    static const JSGCTraceKind map[] = {
        JSTRACE_OBJECT,     
        JSTRACE_OBJECT,     
        JSTRACE_OBJECT,     
        JSTRACE_OBJECT,     
        JSTRACE_OBJECT,     
        JSTRACE_OBJECT,     
        JSTRACE_OBJECT,     
        JSTRACE_OBJECT,     
        JSTRACE_OBJECT,     
        JSTRACE_OBJECT,     
        JSTRACE_OBJECT,     
        JSTRACE_OBJECT,     
        JSTRACE_SCRIPT,     
        JSTRACE_LAZY_SCRIPT,
        JSTRACE_SHAPE,      
        JSTRACE_SHAPE,      
        JSTRACE_BASE_SHAPE, 
        JSTRACE_TYPE_OBJECT,
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_SYMBOL,     
        JSTRACE_JITCODE,    
    };

    static_assert(MOZ_ARRAY_LENGTH(map) == FINALIZE_LIMIT,
                  "AllocKind-to-TraceKind mapping must be in sync");
    return map[kind];
}





static const size_t MAX_BACKGROUND_FINALIZE_KINDS = FINALIZE_LIMIT - FINALIZE_OBJECT_LIMIT / 2;

class TenuredCell;


struct Cell
{
  public:
    MOZ_ALWAYS_INLINE bool isTenured() const { return !IsInsideNursery(this); }
    MOZ_ALWAYS_INLINE const TenuredCell &asTenured() const;
    MOZ_ALWAYS_INLINE TenuredCell &asTenured();

    inline JSRuntime *runtimeFromMainThread() const;
    inline JS::shadow::Runtime *shadowRuntimeFromMainThread() const;

    
    
    inline JSRuntime *runtimeFromAnyThread() const;
    inline JS::shadow::Runtime *shadowRuntimeFromAnyThread() const;

    inline StoreBuffer *storeBuffer() const;

    static MOZ_ALWAYS_INLINE bool needWriteBarrierPre(JS::Zone *zone);

#ifdef DEBUG
    inline bool isAligned() const;
#endif

  protected:
    inline uintptr_t address() const;
    inline Chunk *chunk() const;
};



class TenuredCell : public Cell
{
  public:
    
    static MOZ_ALWAYS_INLINE TenuredCell *fromPointer(void *ptr);
    static MOZ_ALWAYS_INLINE const TenuredCell *fromPointer(const void *ptr);

    
    MOZ_ALWAYS_INLINE bool isMarked(uint32_t color = BLACK) const;
    MOZ_ALWAYS_INLINE bool markIfUnmarked(uint32_t color = BLACK) const;
    MOZ_ALWAYS_INLINE void unmark(uint32_t color) const;
    MOZ_ALWAYS_INLINE void copyMarkBitsFrom(const TenuredCell *src);

    
    
    static MOZ_ALWAYS_INLINE bool isNullLike(const Cell *thing) { return !thing; }

    
    inline ArenaHeader *arenaHeader() const;
    inline AllocKind getAllocKind() const;
    inline JSGCTraceKind getTraceKind() const;
    inline JS::Zone *zone() const;
    inline JS::Zone *zoneFromAnyThread() const;
    inline bool isInsideZone(JS::Zone *zone) const;

    MOZ_ALWAYS_INLINE JS::shadow::Zone *shadowZone() const {
        return JS::shadow::Zone::asShadowZone(zone());
    }
    MOZ_ALWAYS_INLINE JS::shadow::Zone *shadowZoneFromAnyThread() const {
        return JS::shadow::Zone::asShadowZone(zoneFromAnyThread());
    }

    static MOZ_ALWAYS_INLINE void readBarrier(TenuredCell *thing);
    static MOZ_ALWAYS_INLINE void writeBarrierPre(TenuredCell *thing);

    static MOZ_ALWAYS_INLINE void writeBarrierPost(TenuredCell *thing, void *cellp);
    static MOZ_ALWAYS_INLINE void writeBarrierPostRelocate(TenuredCell *thing, void *cellp);
    static MOZ_ALWAYS_INLINE void writeBarrierPostRemove(TenuredCell *thing, void *cellp);

#ifdef DEBUG
    inline bool isAligned() const;
#endif
};







const size_t ArenaCellCount = size_t(1) << (ArenaShift - CellShift);
const size_t ArenaBitmapBits = ArenaCellCount;
const size_t ArenaBitmapBytes = ArenaBitmapBits / 8;
const size_t ArenaBitmapWords = ArenaBitmapBits / JS_BITS_PER_WORD;













class FreeSpan
{
    friend class ArenaCellIterImpl;
    friend class CompactFreeSpan;
    friend class FreeList;

    uintptr_t   first;
    uintptr_t   last;

  public:
    
    
    void initBoundsUnchecked(uintptr_t first, uintptr_t last) {
        this->first = first;
        this->last = last;
    }

    void initBounds(uintptr_t first, uintptr_t last) {
        initBoundsUnchecked(first, last);
        checkSpan();
    }

    void initAsEmpty() {
        first = 0;
        last = 0;
        MOZ_ASSERT(isEmpty());
    }

    
    
    
    void initFinal(uintptr_t firstArg, uintptr_t lastArg, size_t thingSize) {
        first = firstArg;
        last = lastArg;
        FreeSpan *lastSpan = reinterpret_cast<FreeSpan*>(last);
        lastSpan->initAsEmpty();
        MOZ_ASSERT(!isEmpty());
        checkSpan(thingSize);
    }

    bool isEmpty() const {
        checkSpan();
        return !first;
    }

    static size_t offsetOfFirst() {
        return offsetof(FreeSpan, first);
    }

    static size_t offsetOfLast() {
        return offsetof(FreeSpan, last);
    }

    
    FreeSpan *nextSpanUnchecked() const {
        return reinterpret_cast<FreeSpan *>(last);
    }

    const FreeSpan *nextSpan() const {
        MOZ_ASSERT(!isEmpty());
        return nextSpanUnchecked();
    }

    uintptr_t arenaAddress() const {
        MOZ_ASSERT(!isEmpty());
        return first & ~ArenaMask;
    }

#ifdef DEBUG
    bool isWithinArena(uintptr_t arenaAddr) const {
        MOZ_ASSERT(!(arenaAddr & ArenaMask));
        MOZ_ASSERT(!isEmpty());
        return arenaAddress() == arenaAddr;
    }
#endif

    size_t length(size_t thingSize) const {
        checkSpan();
        MOZ_ASSERT((last - first) % thingSize == 0);
        return (last - first) / thingSize + 1;
    }

    bool inFreeList(uintptr_t thing) {
        for (const FreeSpan *span = this; !span->isEmpty(); span = span->nextSpan()) {
            
            if (thing < span->first)
                return false;

            
            if (thing <= span->last)
                return true;
        }
        return false;
    }

  private:
    
    
    void checkSpan(size_t thingSize = 0) const {
#ifdef DEBUG
        if (!first || !last) {
            MOZ_ASSERT(!first && !last);
            
            return;
        }

        
        
        MOZ_ASSERT(first <= last);
        MOZ_ASSERT((first & ~ArenaMask) == (last & ~ArenaMask));
        MOZ_ASSERT((last - first) % (thingSize ? thingSize : CellSize) == 0);

        
        
        FreeSpan *next = reinterpret_cast<FreeSpan*>(last);
        if (next->first) {
            MOZ_ASSERT(next->last);
            MOZ_ASSERT((first & ~ArenaMask) == (next->first & ~ArenaMask));
            MOZ_ASSERT(thingSize
                       ? last + 2 * thingSize <= next->first
                       : last < next->first);
        }
#endif
    }
};

class CompactFreeSpan
{
    uint16_t firstOffset_;
    uint16_t lastOffset_;

  public:
    CompactFreeSpan(size_t firstOffset, size_t lastOffset)
      : firstOffset_(firstOffset)
      , lastOffset_(lastOffset)
    {}

    void initAsEmpty() {
        firstOffset_ = 0;
        lastOffset_ = 0;
    }

    bool operator==(const CompactFreeSpan &other) const {
        return firstOffset_ == other.firstOffset_ &&
               lastOffset_  == other.lastOffset_;
    }

    void compact(FreeSpan span) {
        if (span.isEmpty()) {
            initAsEmpty();
        } else {
            static_assert(ArenaShift < 16, "Check that we can pack offsets into uint16_t.");
            uintptr_t arenaAddr = span.arenaAddress();
            firstOffset_ = span.first - arenaAddr;
            lastOffset_  = span.last  - arenaAddr;
        }
    }

    bool isEmpty() const {
        MOZ_ASSERT(!!firstOffset_ == !!lastOffset_);
        return !firstOffset_;
    }

    FreeSpan decompact(uintptr_t arenaAddr) const {
        MOZ_ASSERT(!(arenaAddr & ArenaMask));
        FreeSpan decodedSpan;
        if (isEmpty()) {
            decodedSpan.initAsEmpty();
        } else {
            MOZ_ASSERT(firstOffset_ <= lastOffset_);
            MOZ_ASSERT(lastOffset_ < ArenaSize);
            decodedSpan.initBounds(arenaAddr + firstOffset_, arenaAddr + lastOffset_);
        }
        return decodedSpan;
    }
};

class FreeList
{
    
    
    
    
    FreeSpan head;

  public:
    FreeList() {}

    static size_t offsetOfFirst() {
        return offsetof(FreeList, head) + offsetof(FreeSpan, first);
    }

    static size_t offsetOfLast() {
        return offsetof(FreeList, head) + offsetof(FreeSpan, last);
    }

    void *addressOfFirst() const {
        return (void*)&head.first;
    }

    void *addressOfLast() const {
        return (void*)&head.last;
    }

    void initAsEmpty() {
        head.initAsEmpty();
    }

    FreeSpan *getHead() { return &head; }
    void setHead(FreeSpan *span) { head = *span; }

    bool isEmpty() const {
        return head.isEmpty();
    }

#ifdef DEBUG
    uintptr_t arenaAddress() const {
        MOZ_ASSERT(!isEmpty());
        return head.arenaAddress();
    }
#endif

    ArenaHeader *arenaHeader() const {
        MOZ_ASSERT(!isEmpty());
        return reinterpret_cast<ArenaHeader *>(head.arenaAddress());
    }

#ifdef DEBUG
    bool isSameNonEmptySpan(const FreeSpan &another) const {
        MOZ_ASSERT(!isEmpty());
        MOZ_ASSERT(!another.isEmpty());
        return head.first == another.first && head.last == another.last;
    }
#endif

    MOZ_ALWAYS_INLINE TenuredCell *allocate(size_t thingSize) {
        MOZ_ASSERT(thingSize % CellSize == 0);
        head.checkSpan(thingSize);
        uintptr_t thing = head.first;
        if (thing < head.last) {
            
            
            head.first = thing + thingSize;
        } else if (MOZ_LIKELY(thing)) {
            
            
            
            setHead(reinterpret_cast<FreeSpan *>(thing));
        } else {
            
            return nullptr;
        }
        head.checkSpan(thingSize);
        JS_EXTRA_POISON(reinterpret_cast<void *>(thing), JS_ALLOCATED_TENURED_PATTERN, thingSize);
        return reinterpret_cast<TenuredCell *>(thing);
    }
};


struct ArenaHeader : public JS::shadow::ArenaHeader
{
    friend struct FreeLists;

    




    ArenaHeader     *next;

  private:
    



    CompactFreeSpan firstFreeSpan;

    









    size_t       allocKind          : 8;

    






















  public:
    size_t       hasDelayedMarking  : 1;
    size_t       allocatedDuringIncremental : 1;
    size_t       markOverflow : 1;
    size_t       auxNextLink : JS_BITS_PER_WORD - 8 - 1 - 1 - 1;
    static_assert(ArenaShift >= 8 + 1 + 1 + 1,
                  "ArenaHeader::auxNextLink packing assumes that ArenaShift has enough bits to "
                  "cover allocKind and hasDelayedMarking.");

    inline uintptr_t address() const;
    inline Chunk *chunk() const;

    bool allocated() const {
        MOZ_ASSERT(allocKind <= size_t(FINALIZE_LIMIT));
        return allocKind < size_t(FINALIZE_LIMIT);
    }

    void init(JS::Zone *zoneArg, AllocKind kind) {
        MOZ_ASSERT(!allocated());
        MOZ_ASSERT(!markOverflow);
        MOZ_ASSERT(!allocatedDuringIncremental);
        MOZ_ASSERT(!hasDelayedMarking);
        zone = zoneArg;

        static_assert(FINALIZE_LIMIT <= 255, "We must be able to fit the allockind into uint8_t.");
        allocKind = size_t(kind);

        



        firstFreeSpan.initAsEmpty();
    }

    void setAsNotAllocated() {
        allocKind = size_t(FINALIZE_LIMIT);
        markOverflow = 0;
        allocatedDuringIncremental = 0;
        hasDelayedMarking = 0;
        auxNextLink = 0;
    }

    inline uintptr_t arenaAddress() const;
    inline Arena *getArena();

    AllocKind getAllocKind() const {
        MOZ_ASSERT(allocated());
        return AllocKind(allocKind);
    }

    inline size_t getThingSize() const;

    bool hasFreeThings() const {
        return !firstFreeSpan.isEmpty();
    }

    inline bool isEmpty() const;

    void setAsFullyUsed() {
        firstFreeSpan.initAsEmpty();
    }

    inline FreeSpan getFirstFreeSpan() const;
    inline void setFirstFreeSpan(const FreeSpan *span);

#ifdef DEBUG
    void checkSynchronizedWithFreeList() const;
#endif

    inline ArenaHeader *getNextDelayedMarking() const;
    inline void setNextDelayedMarking(ArenaHeader *aheader);
    inline void unsetDelayedMarking();

    inline ArenaHeader *getNextAllocDuringSweep() const;
    inline void setNextAllocDuringSweep(ArenaHeader *aheader);
    inline void unsetAllocDuringSweep();

    inline void setNextArenaToUpdate(ArenaHeader *aheader);
    inline ArenaHeader *getNextArenaToUpdateAndUnlink();

    void unmarkAll();

#ifdef JSGC_COMPACTING
    size_t countUsedCells();
    size_t countFreeCells();
#endif
};

struct Arena
{
    













    ArenaHeader aheader;
    uint8_t     data[ArenaSize - sizeof(ArenaHeader)];

  private:
    static JS_FRIEND_DATA(const uint32_t) ThingSizes[];
    static JS_FRIEND_DATA(const uint32_t) FirstThingOffsets[];

  public:
    static void staticAsserts();

    static size_t thingSize(AllocKind kind) {
        return ThingSizes[kind];
    }

    static size_t firstThingOffset(AllocKind kind) {
        return FirstThingOffsets[kind];
    }

    static size_t thingsPerArena(size_t thingSize) {
        MOZ_ASSERT(thingSize % CellSize == 0);

        
        MOZ_ASSERT(thingSize >= sizeof(FreeSpan));

        return (ArenaSize - sizeof(ArenaHeader)) / thingSize;
    }

    static size_t thingsSpan(size_t thingSize) {
        return thingsPerArena(thingSize) * thingSize;
    }

    static bool isAligned(uintptr_t thing, size_t thingSize) {
        
        uintptr_t tailOffset = (ArenaSize - thing) & ArenaMask;
        return tailOffset % thingSize == 0;
    }

    uintptr_t address() const {
        return aheader.address();
    }

    uintptr_t thingsStart(AllocKind thingKind) {
        return address() + firstThingOffset(thingKind);
    }

    uintptr_t thingsEnd() {
        return address() + ArenaSize;
    }

    void setAsFullyUnused(AllocKind thingKind);

    template <typename T>
    size_t finalize(FreeOp *fop, AllocKind thingKind, size_t thingSize);
};

static_assert(sizeof(Arena) == ArenaSize, "The hardcoded arena size must match the struct size.");

inline size_t
ArenaHeader::getThingSize() const
{
    MOZ_ASSERT(allocated());
    return Arena::thingSize(getAllocKind());
}






struct ChunkTrailer
{
    
    uint32_t        location;
    uint32_t        padding;

    
    StoreBuffer     *storeBuffer;

    JSRuntime       *runtime;
};

static_assert(sizeof(ChunkTrailer) == 2 * sizeof(uintptr_t) + sizeof(uint64_t),
              "ChunkTrailer size is incorrect.");


struct ChunkInfo
{
    void init() {
        next = prev = nullptr;
        age = 0;
    }

  private:
    friend class ChunkPool;
    Chunk           *next;
    Chunk           *prev;

  public:
    
    ArenaHeader     *freeArenasHead;

#if JS_BITS_PER_WORD == 32
    



    char            padding[20];
#endif

    




    uint32_t        lastDecommittedArenaOffset;

    
    uint32_t        numArenasFree;

    
    uint32_t        numArenasFreeCommitted;

    
    uint32_t        age;

    
    ChunkTrailer    trailer;
};






























const size_t BytesPerArenaWithHeader = ArenaSize + ArenaBitmapBytes;
const size_t ChunkDecommitBitmapBytes = ChunkSize / ArenaSize / JS_BITS_PER_BYTE;
const size_t ChunkBytesAvailable = ChunkSize - sizeof(ChunkInfo) - ChunkDecommitBitmapBytes;
const size_t ArenasPerChunk = ChunkBytesAvailable / BytesPerArenaWithHeader;

#ifdef JS_GC_SMALL_CHUNK_SIZE
static_assert(ArenasPerChunk == 62, "Do not accidentally change our heap's density.");
#else
static_assert(ArenasPerChunk == 252, "Do not accidentally change our heap's density.");
#endif


struct ChunkBitmap
{
    volatile uintptr_t bitmap[ArenaBitmapWords * ArenasPerChunk];

  public:
    ChunkBitmap() { }

    MOZ_ALWAYS_INLINE void getMarkWordAndMask(const Cell *cell, uint32_t color,
                                              uintptr_t **wordp, uintptr_t *maskp)
    {
        GetGCThingMarkWordAndMask(cell, color, wordp, maskp);
    }

    MOZ_ALWAYS_INLINE MOZ_TSAN_BLACKLIST bool isMarked(const Cell *cell, uint32_t color) {
        uintptr_t *word, mask;
        getMarkWordAndMask(cell, color, &word, &mask);
        return *word & mask;
    }

    MOZ_ALWAYS_INLINE bool markIfUnmarked(const Cell *cell, uint32_t color) {
        uintptr_t *word, mask;
        getMarkWordAndMask(cell, BLACK, &word, &mask);
        if (*word & mask)
            return false;
        *word |= mask;
        if (color != BLACK) {
            



            getMarkWordAndMask(cell, color, &word, &mask);
            if (*word & mask)
                return false;
            *word |= mask;
        }
        return true;
    }

    MOZ_ALWAYS_INLINE void unmark(const Cell *cell, uint32_t color) {
        uintptr_t *word, mask;
        getMarkWordAndMask(cell, color, &word, &mask);
        *word &= ~mask;
    }

    MOZ_ALWAYS_INLINE void copyMarkBit(Cell *dst, const TenuredCell *src, uint32_t color) {
        uintptr_t *word, mask;
        getMarkWordAndMask(dst, color, &word, &mask);
        *word = (*word & ~mask) | (src->isMarked(color) ? mask : 0);
    }

    void clear() {
        memset((void *)bitmap, 0, sizeof(bitmap));
    }

    uintptr_t *arenaBits(ArenaHeader *aheader) {
        static_assert(ArenaBitmapBits == ArenaBitmapWords * JS_BITS_PER_WORD,
                      "We assume that the part of the bitmap corresponding to the arena "
                      "has the exact number of words so we do not need to deal with a word "
                      "that covers bits from two arenas.");

        uintptr_t *word, unused;
        getMarkWordAndMask(reinterpret_cast<Cell *>(aheader->address()), BLACK, &word, &unused);
        return word;
    }
};

static_assert(ArenaBitmapBytes * ArenasPerChunk == sizeof(ChunkBitmap),
              "Ensure our ChunkBitmap actually covers all arenas.");
static_assert(js::gc::ChunkMarkBitmapBits == ArenaBitmapBits * ArenasPerChunk,
              "Ensure that the mark bitmap has the right number of bits.");

typedef BitArray<ArenasPerChunk> PerArenaBitmap;

const size_t ChunkPadSize = ChunkSize
                            - (sizeof(Arena) * ArenasPerChunk)
                            - sizeof(ChunkBitmap)
                            - sizeof(PerArenaBitmap)
                            - sizeof(ChunkInfo);
static_assert(ChunkPadSize < BytesPerArenaWithHeader,
              "If the chunk padding is larger than an arena, we should have one more arena.");





struct Chunk
{
    Arena           arenas[ArenasPerChunk];

    
    uint8_t         padding[ChunkPadSize];

    ChunkBitmap     bitmap;
    PerArenaBitmap  decommittedArenas;
    ChunkInfo       info;

    static Chunk *fromAddress(uintptr_t addr) {
        addr &= ~ChunkMask;
        return reinterpret_cast<Chunk *>(addr);
    }

    static bool withinArenasRange(uintptr_t addr) {
        uintptr_t offset = addr & ChunkMask;
        return offset < ArenasPerChunk * ArenaSize;
    }

    static size_t arenaIndex(uintptr_t addr) {
        MOZ_ASSERT(withinArenasRange(addr));
        return (addr & ChunkMask) >> ArenaShift;
    }

    uintptr_t address() const {
        uintptr_t addr = reinterpret_cast<uintptr_t>(this);
        MOZ_ASSERT(!(addr & ChunkMask));
        return addr;
    }

    bool unused() const {
        return info.numArenasFree == ArenasPerChunk;
    }

    bool hasAvailableArenas() const {
        return info.numArenasFree != 0;
    }

    ArenaHeader *allocateArena(JSRuntime *rt, JS::Zone *zone, AllocKind kind,
                               const AutoLockGC &lock);

    enum ArenaDecommitState { IsCommitted = false, IsDecommitted = true };
    void releaseArena(JSRuntime *rt, ArenaHeader *aheader, const AutoLockGC &lock,
                      ArenaDecommitState state = IsCommitted);
    void recycleArena(ArenaHeader *aheader, SortedArenaList &dest, AllocKind thingKind,
                      size_t thingsPerArena);

    static Chunk *allocate(JSRuntime *rt);

    void decommitAllArenas(JSRuntime *rt);

  private:
    inline void init(JSRuntime *rt);

    
    unsigned findDecommittedArenaOffset();
    ArenaHeader* fetchNextDecommittedArena();

    void addArenaToFreeList(JSRuntime *rt, ArenaHeader *aheader);
    void addArenaToDecommittedList(JSRuntime *rt, const ArenaHeader *aheader);

  public:
    
    inline ArenaHeader* fetchNextFreeArena(JSRuntime *rt);
};

static_assert(sizeof(Chunk) == ChunkSize,
              "Ensure the hardcoded chunk size definition actually matches the struct.");
static_assert(js::gc::ChunkMarkBitmapOffset == offsetof(Chunk, bitmap),
              "The hardcoded API bitmap offset must match the actual offset.");
static_assert(js::gc::ChunkRuntimeOffset == offsetof(Chunk, info) +
                                            offsetof(ChunkInfo, trailer) +
                                            offsetof(ChunkTrailer, runtime),
              "The hardcoded API runtime offset must match the actual offset.");
static_assert(js::gc::ChunkLocationOffset == offsetof(Chunk, info) +
                                             offsetof(ChunkInfo, trailer) +
                                             offsetof(ChunkTrailer, location),
              "The hardcoded API location offset must match the actual offset.");





class HeapUsage
{
    



    HeapUsage *parent_;

    






    mozilla::Atomic<size_t, mozilla::ReleaseAcquire> gcBytes_;

  public:
    explicit HeapUsage(HeapUsage *parent)
      : parent_(parent),
        gcBytes_(0)
    {}

    size_t gcBytes() const { return gcBytes_; }

    void addGCArena() {
        gcBytes_ += ArenaSize;
        if (parent_)
            parent_->addGCArena();
    }
    void removeGCArena() {
        MOZ_ASSERT(gcBytes_ >= ArenaSize);
        gcBytes_ -= ArenaSize;
        if (parent_)
            parent_->removeGCArena();
    }

    
    void adopt(HeapUsage &other) {
        gcBytes_ += other.gcBytes_;
        other.gcBytes_ = 0;
    }
};

inline uintptr_t
ArenaHeader::address() const
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(this);
    MOZ_ASSERT(!(addr & ArenaMask));
    MOZ_ASSERT(Chunk::withinArenasRange(addr));
    return addr;
}

inline Chunk *
ArenaHeader::chunk() const
{
    return Chunk::fromAddress(address());
}

inline uintptr_t
ArenaHeader::arenaAddress() const
{
    return address();
}

inline Arena *
ArenaHeader::getArena()
{
    return reinterpret_cast<Arena *>(arenaAddress());
}

inline bool
ArenaHeader::isEmpty() const
{
    
    MOZ_ASSERT(allocated());
    size_t firstThingOffset = Arena::firstThingOffset(getAllocKind());
    size_t lastThingOffset = ArenaSize - getThingSize();
    const CompactFreeSpan emptyCompactSpan(firstThingOffset, lastThingOffset);
    return firstFreeSpan == emptyCompactSpan;
}

FreeSpan
ArenaHeader::getFirstFreeSpan() const
{
#ifdef DEBUG
    checkSynchronizedWithFreeList();
#endif
    return firstFreeSpan.decompact(arenaAddress());
}

void
ArenaHeader::setFirstFreeSpan(const FreeSpan *span)
{
    MOZ_ASSERT_IF(!span->isEmpty(), span->isWithinArena(arenaAddress()));
    firstFreeSpan.compact(*span);
}

inline ArenaHeader *
ArenaHeader::getNextDelayedMarking() const
{
    MOZ_ASSERT(hasDelayedMarking);
    return &reinterpret_cast<Arena *>(auxNextLink << ArenaShift)->aheader;
}

inline void
ArenaHeader::setNextDelayedMarking(ArenaHeader *aheader)
{
    MOZ_ASSERT(!(uintptr_t(aheader) & ArenaMask));
    MOZ_ASSERT(!auxNextLink && !hasDelayedMarking);
    hasDelayedMarking = 1;
    auxNextLink = aheader->arenaAddress() >> ArenaShift;
}

inline void
ArenaHeader::unsetDelayedMarking()
{
    MOZ_ASSERT(hasDelayedMarking);
    hasDelayedMarking = 0;
    auxNextLink = 0;
}

inline ArenaHeader *
ArenaHeader::getNextAllocDuringSweep() const
{
    MOZ_ASSERT(allocatedDuringIncremental);
    return &reinterpret_cast<Arena *>(auxNextLink << ArenaShift)->aheader;
}

inline void
ArenaHeader::setNextAllocDuringSweep(ArenaHeader *aheader)
{
    MOZ_ASSERT(!auxNextLink && !allocatedDuringIncremental);
    allocatedDuringIncremental = 1;
    auxNextLink = aheader->arenaAddress() >> ArenaShift;
}

inline void
ArenaHeader::unsetAllocDuringSweep()
{
    MOZ_ASSERT(allocatedDuringIncremental);
    allocatedDuringIncremental = 0;
    auxNextLink = 0;
}

inline ArenaHeader *
ArenaHeader::getNextArenaToUpdateAndUnlink()
{
    MOZ_ASSERT(!hasDelayedMarking && !allocatedDuringIncremental && !markOverflow);
    ArenaHeader *next = &reinterpret_cast<Arena *>(auxNextLink << ArenaShift)->aheader;
    auxNextLink = 0;
    return next;
}

inline void
ArenaHeader::setNextArenaToUpdate(ArenaHeader *aheader)
{
    MOZ_ASSERT(!hasDelayedMarking && !allocatedDuringIncremental && !markOverflow);
    MOZ_ASSERT(!auxNextLink);
    auxNextLink = aheader->arenaAddress() >> ArenaShift;
}

static void
AssertValidColor(const TenuredCell *thing, uint32_t color)
{
#ifdef DEBUG
    ArenaHeader *aheader = thing->arenaHeader();
    MOZ_ASSERT(color < aheader->getThingSize() / CellSize);
#endif
}

MOZ_ALWAYS_INLINE const TenuredCell &
Cell::asTenured() const
{
    MOZ_ASSERT(isTenured());
    return *static_cast<const TenuredCell *>(this);
}

MOZ_ALWAYS_INLINE TenuredCell &
Cell::asTenured()
{
    MOZ_ASSERT(isTenured());
    return *static_cast<TenuredCell *>(this);
}

inline JSRuntime *
Cell::runtimeFromMainThread() const
{
    JSRuntime *rt = chunk()->info.trailer.runtime;
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(rt));
    return rt;
}

inline JS::shadow::Runtime *
Cell::shadowRuntimeFromMainThread() const
{
    return reinterpret_cast<JS::shadow::Runtime*>(runtimeFromMainThread());
}

inline JSRuntime *
Cell::runtimeFromAnyThread() const
{
    return chunk()->info.trailer.runtime;
}

inline JS::shadow::Runtime *
Cell::shadowRuntimeFromAnyThread() const
{
    return reinterpret_cast<JS::shadow::Runtime*>(runtimeFromAnyThread());
}

inline uintptr_t
Cell::address() const
{
    uintptr_t addr = uintptr_t(this);
    MOZ_ASSERT(addr % CellSize == 0);
    MOZ_ASSERT(Chunk::withinArenasRange(addr));
    return addr;
}

Chunk *
Cell::chunk() const
{
    uintptr_t addr = uintptr_t(this);
    MOZ_ASSERT(addr % CellSize == 0);
    addr &= ~ChunkMask;
    return reinterpret_cast<Chunk *>(addr);
}

inline StoreBuffer *
Cell::storeBuffer() const
{
    return chunk()->info.trailer.storeBuffer;
}

inline bool
InFreeList(ArenaHeader *aheader, void *thing)
{
    if (!aheader->hasFreeThings())
        return false;

    FreeSpan firstSpan(aheader->getFirstFreeSpan());
    uintptr_t addr = reinterpret_cast<uintptr_t>(thing);

    MOZ_ASSERT(Arena::isAligned(addr, aheader->getThingSize()));

    return firstSpan.inFreeList(addr);
}

 MOZ_ALWAYS_INLINE bool
Cell::needWriteBarrierPre(JS::Zone *zone) {
    return JS::shadow::Zone::asShadowZone(zone)->needsIncrementalBarrier();
}

 MOZ_ALWAYS_INLINE TenuredCell *
TenuredCell::fromPointer(void *ptr)
{
    MOZ_ASSERT(static_cast<TenuredCell *>(ptr)->isTenured());
    return static_cast<TenuredCell *>(ptr);
}

 MOZ_ALWAYS_INLINE const TenuredCell *
TenuredCell::fromPointer(const void *ptr)
{
    MOZ_ASSERT(static_cast<const TenuredCell *>(ptr)->isTenured());
    return static_cast<const TenuredCell *>(ptr);
}

bool
TenuredCell::isMarked(uint32_t color ) const
{
    MOZ_ASSERT(arenaHeader()->allocated());
    AssertValidColor(this, color);
    return chunk()->bitmap.isMarked(this, color);
}

bool
TenuredCell::markIfUnmarked(uint32_t color ) const
{
    AssertValidColor(this, color);
    return chunk()->bitmap.markIfUnmarked(this, color);
}

void
TenuredCell::unmark(uint32_t color) const
{
    MOZ_ASSERT(color != BLACK);
    AssertValidColor(this, color);
    chunk()->bitmap.unmark(this, color);
}

void
TenuredCell::copyMarkBitsFrom(const TenuredCell *src)
{
    ChunkBitmap &bitmap = chunk()->bitmap;
    bitmap.copyMarkBit(this, src, BLACK);
    bitmap.copyMarkBit(this, src, GRAY);
}

inline ArenaHeader *
TenuredCell::arenaHeader() const
{
    MOZ_ASSERT(isTenured());
    uintptr_t addr = address();
    addr &= ~ArenaMask;
    return reinterpret_cast<ArenaHeader *>(addr);
}

AllocKind
TenuredCell::getAllocKind() const
{
    return arenaHeader()->getAllocKind();
}

JSGCTraceKind
TenuredCell::getTraceKind() const
{
    return MapAllocToTraceKind(getAllocKind());
}

JS::Zone *
TenuredCell::zone() const
{
    JS::Zone *zone = arenaHeader()->zone;
    MOZ_ASSERT(CurrentThreadCanAccessZone(zone));
    return zone;
}

JS::Zone *
TenuredCell::zoneFromAnyThread() const
{
    return arenaHeader()->zone;
}

bool
TenuredCell::isInsideZone(JS::Zone *zone) const
{
    return zone == arenaHeader()->zone;
}

 MOZ_ALWAYS_INLINE void
TenuredCell::readBarrier(TenuredCell *thing)
{
    MOZ_ASSERT(!CurrentThreadIsIonCompiling());
    MOZ_ASSERT(!isNullLike(thing));
    JS::shadow::Zone *shadowZone = thing->shadowZoneFromAnyThread();
    if (shadowZone->needsIncrementalBarrier()) {
        MOZ_ASSERT(!RuntimeFromMainThreadIsHeapMajorCollecting(shadowZone));
        void *tmp = thing;
        shadowZone->barrierTracer()->setTracingName("read barrier");
        MarkKind(shadowZone->barrierTracer(), &tmp,
                         MapAllocToTraceKind(thing->getAllocKind()));
        MOZ_ASSERT(tmp == thing);
    }
    JS::GCCellPtr cellptr(thing, thing->getTraceKind());
    if (JS::GCThingIsMarkedGray(thing))
        JS::UnmarkGrayGCThingRecursively(cellptr);
}

 MOZ_ALWAYS_INLINE void
TenuredCell::writeBarrierPre(TenuredCell *thing)
{
    MOZ_ASSERT(!CurrentThreadIsIonCompiling());
    if (isNullLike(thing) || !thing->shadowRuntimeFromAnyThread()->needsIncrementalBarrier())
        return;

    JS::shadow::Zone *shadowZone = thing->shadowZoneFromAnyThread();
    if (shadowZone->needsIncrementalBarrier()) {
        MOZ_ASSERT(!RuntimeFromMainThreadIsHeapMajorCollecting(shadowZone));
        void *tmp = thing;
        shadowZone->barrierTracer()->setTracingName("pre barrier");
        MarkKind(shadowZone->barrierTracer(), &tmp,
                         MapAllocToTraceKind(thing->getAllocKind()));
        MOZ_ASSERT(tmp == thing);
    }
}

static MOZ_ALWAYS_INLINE void
AssertValidToSkipBarrier(TenuredCell *thing)
{
    MOZ_ASSERT(!IsInsideNursery(thing));
    MOZ_ASSERT_IF(thing, MapAllocToTraceKind(thing->getAllocKind()) != JSTRACE_OBJECT);
}

 MOZ_ALWAYS_INLINE void
TenuredCell::writeBarrierPost(TenuredCell *thing, void *cellp)
{
    AssertValidToSkipBarrier(thing);
}

 MOZ_ALWAYS_INLINE void
TenuredCell::writeBarrierPostRelocate(TenuredCell *thing, void *cellp)
{
    AssertValidToSkipBarrier(thing);
}

 MOZ_ALWAYS_INLINE void
TenuredCell::writeBarrierPostRemove(TenuredCell *thing, void *cellp)
{
    AssertValidToSkipBarrier(thing);
}

#ifdef DEBUG
bool
Cell::isAligned() const
{
    if (!isTenured())
        return true;
    return asTenured().isAligned();
}

bool
TenuredCell::isAligned() const
{
    return Arena::isAligned(address(), arenaHeader()->getThingSize());
}
#endif

} 
} 

#endif 
