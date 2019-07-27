





#ifndef gc_Heap_h
#define gc_Heap_h

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
#include "js/HeapAPI.h"

struct JSCompartment;

struct JSRuntime;

namespace JS {
namespace shadow {
struct Runtime;
}
}

namespace js {

class FreeOp;

namespace gc {

struct Arena;
class ArenaList;
class SortedArenaList;
struct ArenaHeader;
struct Chunk;






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





static const size_t MAX_BACKGROUND_FINALIZE_KINDS = FINALIZE_LIMIT - FINALIZE_OBJECT_LIMIT / 2;




struct Cell
{
  public:
    inline ArenaHeader *arenaHeader() const;
    inline AllocKind tenuredGetAllocKind() const;
    MOZ_ALWAYS_INLINE bool isMarked(uint32_t color = BLACK) const;
    MOZ_ALWAYS_INLINE bool markIfUnmarked(uint32_t color = BLACK) const;
    MOZ_ALWAYS_INLINE void unmark(uint32_t color) const;

    inline JSRuntime *runtimeFromMainThread() const;
    inline JS::shadow::Runtime *shadowRuntimeFromMainThread() const;
    inline JS::Zone *tenuredZone() const;
    inline JS::Zone *tenuredZoneFromAnyThread() const;
    inline bool tenuredIsInsideZone(JS::Zone *zone) const;

    
    
    inline JSRuntime *runtimeFromAnyThread() const;
    inline JS::shadow::Runtime *shadowRuntimeFromAnyThread() const;

    inline StoreBuffer *storeBuffer() const;

#ifdef DEBUG
    inline bool isAligned() const;
    inline bool isTenured() const;
#endif

  protected:
    inline uintptr_t address() const;
    inline Chunk *chunk() const;
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
        JS_ASSERT(isEmpty());
    }

    
    
    
    void initFinal(uintptr_t firstArg, uintptr_t lastArg, size_t thingSize) {
        first = firstArg;
        last = lastArg;
        FreeSpan *lastSpan = reinterpret_cast<FreeSpan*>(last);
        lastSpan->initAsEmpty();
        JS_ASSERT(!isEmpty());
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
        JS_ASSERT(!isEmpty());
        return nextSpanUnchecked();
    }

    uintptr_t arenaAddress() const {
        JS_ASSERT(!isEmpty());
        return first & ~ArenaMask;
    }

#ifdef DEBUG
    bool isWithinArena(uintptr_t arenaAddr) const {
        JS_ASSERT(!(arenaAddr & ArenaMask));
        JS_ASSERT(!isEmpty());
        return arenaAddress() == arenaAddr;
    }
#endif

    size_t length(size_t thingSize) const {
        checkSpan();
        JS_ASSERT((last - first) % thingSize == 0);
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
            JS_ASSERT(!first && !last);
            
            return;
        }

        
        
        JS_ASSERT(first <= last);
        JS_ASSERT((first & ~ArenaMask) == (last & ~ArenaMask));
        JS_ASSERT((last - first) % (thingSize ? thingSize : CellSize) == 0);

        
        
        FreeSpan *next = reinterpret_cast<FreeSpan*>(last);
        if (next->first) {
            JS_ASSERT(next->last);
            JS_ASSERT((first & ~ArenaMask) == (next->first & ~ArenaMask));
            JS_ASSERT(thingSize
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
        JS_ASSERT(!!firstOffset_ == !!lastOffset_);
        return !firstOffset_;
    }

    FreeSpan decompact(uintptr_t arenaAddr) const {
        JS_ASSERT(!(arenaAddr & ArenaMask));
        FreeSpan decodedSpan;
        if (isEmpty()) {
            decodedSpan.initAsEmpty();
        } else {
            JS_ASSERT(firstOffset_ <= lastOffset_);
            JS_ASSERT(lastOffset_ < ArenaSize);
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
        JS_ASSERT(!isEmpty());
        return head.arenaAddress();
    }
#endif

    ArenaHeader *arenaHeader() const {
        JS_ASSERT(!isEmpty());
        return reinterpret_cast<ArenaHeader *>(head.arenaAddress());
    }

#ifdef DEBUG
    bool isSameNonEmptySpan(const FreeSpan &another) const {
        JS_ASSERT(!isEmpty());
        JS_ASSERT(!another.isEmpty());
        return head.first == another.first && head.last == another.last;
    }
#endif

    MOZ_ALWAYS_INLINE void *allocate(size_t thingSize) {
        JS_ASSERT(thingSize % CellSize == 0);
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
        return reinterpret_cast<void *>(thing);
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
        JS_ASSERT(allocKind <= size_t(FINALIZE_LIMIT));
        return allocKind < size_t(FINALIZE_LIMIT);
    }

    void init(JS::Zone *zoneArg, AllocKind kind) {
        JS_ASSERT(!allocated());
        JS_ASSERT(!markOverflow);
        JS_ASSERT(!allocatedDuringIncremental);
        JS_ASSERT(!hasDelayedMarking);
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
        JS_ASSERT(allocated());
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
        JS_ASSERT(thingSize % CellSize == 0);

        
        JS_ASSERT(thingSize >= sizeof(FreeSpan));

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
    JS_ASSERT(allocated());
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
    Chunk           *next;
    Chunk           **prevp;

    
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
        JS_ASSERT(withinArenasRange(addr));
        return (addr & ChunkMask) >> ArenaShift;
    }

    uintptr_t address() const {
        uintptr_t addr = reinterpret_cast<uintptr_t>(this);
        JS_ASSERT(!(addr & ChunkMask));
        return addr;
    }

    bool unused() const {
        return info.numArenasFree == ArenasPerChunk;
    }

    bool hasAvailableArenas() const {
        return info.numArenasFree != 0;
    }

    inline void addToAvailableList(JS::Zone *zone);
    inline void insertToAvailableList(Chunk **insertPoint);
    inline void removeFromAvailableList();

    ArenaHeader *allocateArena(JS::Zone *zone, AllocKind kind);

    void releaseArena(ArenaHeader *aheader);
    void recycleArena(ArenaHeader *aheader, SortedArenaList &dest, AllocKind thingKind,
                      size_t thingsPerArena);

    static Chunk *allocate(JSRuntime *rt);

    void decommitAllArenas(JSRuntime *rt);

    



    Chunk *getPrevious() {
        JS_ASSERT(info.prevp);
        return fromPointerToNext(info.prevp);
    }

    
    static Chunk *fromPointerToNext(Chunk **nextFieldPtr) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(nextFieldPtr);
        JS_ASSERT((addr & ChunkMask) == offsetof(Chunk, info.next));
        return reinterpret_cast<Chunk *>(addr - offsetof(Chunk, info.next));
    }

  private:
    inline void init(JSRuntime *rt);

    
    unsigned findDecommittedArenaOffset();
    ArenaHeader* fetchNextDecommittedArena();

  public:
    
    inline ArenaHeader* fetchNextFreeArena(JSRuntime *rt);

    inline void addArenaToFreeList(JSRuntime *rt, ArenaHeader *aheader);
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
    HeapUsage(HeapUsage *parent)
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
    JS_ASSERT(!(addr & ArenaMask));
    JS_ASSERT(Chunk::withinArenasRange(addr));
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
    
    JS_ASSERT(allocated());
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
    JS_ASSERT_IF(!span->isEmpty(), span->isWithinArena(arenaAddress()));
    firstFreeSpan.compact(*span);
}

inline ArenaHeader *
ArenaHeader::getNextDelayedMarking() const
{
    JS_ASSERT(hasDelayedMarking);
    return &reinterpret_cast<Arena *>(auxNextLink << ArenaShift)->aheader;
}

inline void
ArenaHeader::setNextDelayedMarking(ArenaHeader *aheader)
{
    JS_ASSERT(!(uintptr_t(aheader) & ArenaMask));
    JS_ASSERT(!auxNextLink && !hasDelayedMarking);
    hasDelayedMarking = 1;
    auxNextLink = aheader->arenaAddress() >> ArenaShift;
}

inline void
ArenaHeader::unsetDelayedMarking()
{
    JS_ASSERT(hasDelayedMarking);
    hasDelayedMarking = 0;
    auxNextLink = 0;
}

inline ArenaHeader *
ArenaHeader::getNextAllocDuringSweep() const
{
    JS_ASSERT(allocatedDuringIncremental);
    return &reinterpret_cast<Arena *>(auxNextLink << ArenaShift)->aheader;
}

inline void
ArenaHeader::setNextAllocDuringSweep(ArenaHeader *aheader)
{
    JS_ASSERT(!auxNextLink && !allocatedDuringIncremental);
    allocatedDuringIncremental = 1;
    auxNextLink = aheader->arenaAddress() >> ArenaShift;
}

inline void
ArenaHeader::unsetAllocDuringSweep()
{
    JS_ASSERT(allocatedDuringIncremental);
    allocatedDuringIncremental = 0;
    auxNextLink = 0;
}

static void
AssertValidColor(const void *thing, uint32_t color)
{
#ifdef DEBUG
    ArenaHeader *aheader = reinterpret_cast<const Cell *>(thing)->arenaHeader();
    JS_ASSERT(color < aheader->getThingSize() / CellSize);
#endif
}

inline ArenaHeader *
Cell::arenaHeader() const
{
    JS_ASSERT(isTenured());
    uintptr_t addr = address();
    addr &= ~ArenaMask;
    return reinterpret_cast<ArenaHeader *>(addr);
}

inline JSRuntime *
Cell::runtimeFromMainThread() const
{
    JSRuntime *rt = chunk()->info.trailer.runtime;
    JS_ASSERT(CurrentThreadCanAccessRuntime(rt));
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

bool
Cell::isMarked(uint32_t color ) const
{
    JS_ASSERT(isTenured());
    JS_ASSERT(arenaHeader()->allocated());
    AssertValidColor(this, color);
    return chunk()->bitmap.isMarked(this, color);
}

bool
Cell::markIfUnmarked(uint32_t color ) const
{
    JS_ASSERT(isTenured());
    AssertValidColor(this, color);
    return chunk()->bitmap.markIfUnmarked(this, color);
}

void
Cell::unmark(uint32_t color) const
{
    JS_ASSERT(isTenured());
    JS_ASSERT(color != BLACK);
    AssertValidColor(this, color);
    chunk()->bitmap.unmark(this, color);
}

JS::Zone *
Cell::tenuredZone() const
{
    JS::Zone *zone = arenaHeader()->zone;
    JS_ASSERT(CurrentThreadCanAccessZone(zone));
    JS_ASSERT(isTenured());
    return zone;
}

JS::Zone *
Cell::tenuredZoneFromAnyThread() const
{
    JS_ASSERT(isTenured());
    return arenaHeader()->zone;
}

bool
Cell::tenuredIsInsideZone(JS::Zone *zone) const
{
    JS_ASSERT(isTenured());
    return zone == arenaHeader()->zone;
}

#ifdef DEBUG
bool
Cell::isAligned() const
{
    return Arena::isAligned(address(), arenaHeader()->getThingSize());
}

bool
Cell::isTenured() const
{
#ifdef JSGC_GENERATIONAL
    return !IsInsideNursery(this);
#endif
    return true;
}
#endif

inline uintptr_t
Cell::address() const
{
    uintptr_t addr = uintptr_t(this);
    JS_ASSERT(addr % CellSize == 0);
    JS_ASSERT(Chunk::withinArenasRange(addr));
    return addr;
}

Chunk *
Cell::chunk() const
{
    uintptr_t addr = uintptr_t(this);
    JS_ASSERT(addr % CellSize == 0);
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

    JS_ASSERT(Arena::isAligned(addr, aheader->getThingSize()));

    return firstSpan.inFreeList(addr);
}

} 

gc::AllocKind
gc::Cell::tenuredGetAllocKind() const
{
    return arenaHeader()->getAllocKind();
}

} 

#endif 
