





#ifndef gc_heap_h___
#define gc_heap_h___

#include "mozilla/Attributes.h"
#include "mozilla/StandardInteger.h"

#include <stddef.h>

#include "jstypes.h"
#include "jsutil.h"

#include "ds/BitArray.h"
#include "js/HeapAPI.h"

struct JSCompartment;

extern "C" {
struct JSRuntime;
}

namespace js {

class FreeOp;

namespace gc {

struct Arena;
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
    FINALIZE_SHAPE,
    FINALIZE_BASE_SHAPE,
    FINALIZE_TYPE_OBJECT,
    FINALIZE_SHORT_STRING,
    FINALIZE_STRING,
    FINALIZE_EXTERNAL_STRING,
    FINALIZE_IONCODE,
    FINALIZE_LAST = FINALIZE_IONCODE
};

static const unsigned FINALIZE_LIMIT = FINALIZE_LAST + 1;
static const unsigned FINALIZE_OBJECT_LIMIT = FINALIZE_OBJECT_LAST + 1;





static const size_t MAX_BACKGROUND_FINALIZE_KINDS = FINALIZE_LIMIT - FINALIZE_OBJECT_LIMIT / 2;




struct Cell
{
    inline ArenaHeader *arenaHeader() const;
    inline AllocKind tenuredGetAllocKind() const;
    MOZ_ALWAYS_INLINE bool isMarked(uint32_t color = BLACK) const;
    MOZ_ALWAYS_INLINE bool markIfUnmarked(uint32_t color = BLACK) const;
    MOZ_ALWAYS_INLINE void unmark(uint32_t color) const;

    inline JSRuntime *runtime() const;
    inline Zone *tenuredZone() const;

#ifdef DEBUG
    inline bool isAligned() const;
    bool isTenured() const;
#endif

  protected:
    inline uintptr_t address() const;
    inline Chunk *chunk() const;
};





const static uint32_t FreeCommittedArenasThreshold = (32 << 20) / ArenaSize;







const size_t ArenaCellCount = size_t(1) << (ArenaShift - CellShift);
const size_t ArenaBitmapBits = ArenaCellCount;
const size_t ArenaBitmapBytes = ArenaBitmapBits / 8;
const size_t ArenaBitmapWords = ArenaBitmapBits / JS_BITS_PER_WORD;



























struct FreeSpan
{
    uintptr_t   first;
    uintptr_t   last;

  public:
    FreeSpan() {}

    FreeSpan(uintptr_t first, uintptr_t last)
      : first(first), last(last) {
        checkSpan();
    }

    



    static size_t encodeOffsets(size_t firstOffset, size_t lastOffset) {
        
        JS_STATIC_ASSERT(ArenaShift < 16);
        JS_ASSERT(firstOffset <= ArenaSize);
        JS_ASSERT(lastOffset < ArenaSize);
        JS_ASSERT(firstOffset <= ((lastOffset + 1) & ~size_t(1)));
        return firstOffset | (lastOffset << 16);
    }

    



    static const size_t FullArenaOffsets = ArenaSize | ((ArenaSize - 1) << 16);

    static FreeSpan decodeOffsets(uintptr_t arenaAddr, size_t offsets) {
        JS_ASSERT(!(arenaAddr & ArenaMask));

        size_t firstOffset = offsets & 0xFFFF;
        size_t lastOffset = offsets >> 16;
        JS_ASSERT(firstOffset <= ArenaSize);
        JS_ASSERT(lastOffset < ArenaSize);

        



        return FreeSpan(arenaAddr + firstOffset, arenaAddr | lastOffset);
    }

    void initAsEmpty(uintptr_t arenaAddr = 0) {
        JS_ASSERT(!(arenaAddr & ArenaMask));
        first = arenaAddr + ArenaSize;
        last = arenaAddr | (ArenaSize  - 1);
        JS_ASSERT(isEmpty());
    }

    bool isEmpty() const {
        checkSpan();
        return first > last;
    }

    bool hasNext() const {
        checkSpan();
        return !(last & uintptr_t(1));
    }

    const FreeSpan *nextSpan() const {
        JS_ASSERT(hasNext());
        return reinterpret_cast<FreeSpan *>(last);
    }

    FreeSpan *nextSpanUnchecked(size_t thingSize) const {
#ifdef DEBUG
        uintptr_t lastOffset = last & ArenaMask;
        JS_ASSERT(!(lastOffset & 1));
        JS_ASSERT((ArenaSize - lastOffset) % thingSize == 0);
#endif
        return reinterpret_cast<FreeSpan *>(last);
    }

    uintptr_t arenaAddressUnchecked() const {
        return last & ~ArenaMask;
    }

    uintptr_t arenaAddress() const {
        checkSpan();
        return arenaAddressUnchecked();
    }

    ArenaHeader *arenaHeader() const {
        return reinterpret_cast<ArenaHeader *>(arenaAddress());
    }

    bool isSameNonEmptySpan(const FreeSpan *another) const {
        JS_ASSERT(!isEmpty());
        JS_ASSERT(!another->isEmpty());
        return first == another->first && last == another->last;
    }

    bool isWithinArena(uintptr_t arenaAddr) const {
        JS_ASSERT(!(arenaAddr & ArenaMask));

        
        return arenaAddress() == arenaAddr;
    }

    size_t encodeAsOffsets() const {
        



        uintptr_t arenaAddr = arenaAddress();
        return encodeOffsets(first - arenaAddr, last & ArenaMask);
    }

    
    MOZ_ALWAYS_INLINE void *allocate(size_t thingSize) {
        JS_ASSERT(thingSize % CellSize == 0);
        checkSpan();
        uintptr_t thing = first;
        if (thing < last) {
            
            first = thing + thingSize;
        } else if (JS_LIKELY(thing == last)) {
            



            *this = *reinterpret_cast<FreeSpan *>(thing);
        } else {
            return NULL;
        }
        checkSpan();
        return reinterpret_cast<void *>(thing);
    }

    
    MOZ_ALWAYS_INLINE void *infallibleAllocate(size_t thingSize) {
        JS_ASSERT(thingSize % CellSize == 0);
        checkSpan();
        uintptr_t thing = first;
        if (thing < last) {
            first = thing + thingSize;
        } else {
            JS_ASSERT(thing == last);
            *this = *reinterpret_cast<FreeSpan *>(thing);
        }
        checkSpan();
        return reinterpret_cast<void *>(thing);
    }

    





    MOZ_ALWAYS_INLINE void *allocateFromNewArena(uintptr_t arenaAddr, size_t firstThingOffset,
                                                size_t thingSize) {
        JS_ASSERT(!(arenaAddr & ArenaMask));
        uintptr_t thing = arenaAddr | firstThingOffset;
        first = thing + thingSize;
        last = arenaAddr | ArenaMask;
        checkSpan();
        return reinterpret_cast<void *>(thing);
    }

    void checkSpan() const {
#ifdef DEBUG
        
        JS_ASSERT(last != uintptr_t(-1));
        JS_ASSERT(first);
        JS_ASSERT(last);
        JS_ASSERT(first - 1 <= last);
        uintptr_t arenaAddr = arenaAddressUnchecked();
        if (last & 1) {
            
            JS_ASSERT((last & ArenaMask) == ArenaMask);

            if (first - 1 == last) {
                


                return;
            }
            size_t spanLength = last - first + 1;
            JS_ASSERT(spanLength % CellSize == 0);

            
            JS_ASSERT((first & ~ArenaMask) == arenaAddr);
            return;
        }

        
        JS_ASSERT(first <= last);
        size_t spanLengthWithoutOneThing = last - first;
        JS_ASSERT(spanLengthWithoutOneThing % CellSize == 0);

        JS_ASSERT((first & ~ArenaMask) == arenaAddr);

        




        size_t beforeTail = ArenaSize - (last & ArenaMask);
        JS_ASSERT(beforeTail >= sizeof(FreeSpan) + CellSize);

        FreeSpan *next = reinterpret_cast<FreeSpan *>(last);

        




        JS_ASSERT(last < next->first);
        JS_ASSERT(arenaAddr == next->arenaAddressUnchecked());

        if (next->first > next->last) {
            



            JS_ASSERT(next->first - 1 == next->last);
            JS_ASSERT(arenaAddr + ArenaSize == next->first);
        }
#endif
    }

};


struct ArenaHeader : public JS::shadow::ArenaHeader
{
    friend struct FreeLists;

    




    ArenaHeader     *next;

  private:
    




    size_t          firstFreeSpanOffsets;

    









    size_t       allocKind          : 8;

    






















  public:
    size_t       hasDelayedMarking  : 1;
    size_t       allocatedDuringIncremental : 1;
    size_t       markOverflow : 1;
    size_t       auxNextLink : JS_BITS_PER_WORD - 8 - 1 - 1 - 1;

    static void staticAsserts() {
        
        JS_STATIC_ASSERT(FINALIZE_LIMIT <= 255);

        



        JS_STATIC_ASSERT(ArenaShift >= 8 + 1 + 1 + 1);
    }

    inline uintptr_t address() const;
    inline Chunk *chunk() const;

    bool allocated() const {
        JS_ASSERT(allocKind <= size_t(FINALIZE_LIMIT));
        return allocKind < size_t(FINALIZE_LIMIT);
    }

    void init(Zone *zoneArg, AllocKind kind) {
        JS_ASSERT(!allocated());
        JS_ASSERT(!markOverflow);
        JS_ASSERT(!allocatedDuringIncremental);
        JS_ASSERT(!hasDelayedMarking);
        zone = zoneArg;

        JS_STATIC_ASSERT(FINALIZE_LIMIT <= 255);
        allocKind = size_t(kind);

        
        firstFreeSpanOffsets = FreeSpan::FullArenaOffsets;
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
        return firstFreeSpanOffsets != FreeSpan::FullArenaOffsets;
    }

    inline bool isEmpty() const;

    void setAsFullyUsed() {
        firstFreeSpanOffsets = FreeSpan::FullArenaOffsets;
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
        return address() | firstThingOffset(thingKind);
    }

    uintptr_t thingsEnd() {
        return address() + ArenaSize;
    }

    template <typename T>
    bool finalize(FreeOp *fop, AllocKind thingKind, size_t thingSize);
};

inline size_t
ArenaHeader::getThingSize() const
{
    JS_ASSERT(allocated());
    return Arena::thingSize(getAllocKind());
}


struct ChunkInfo
{
    Chunk           *next;
    Chunk           **prevp;

    
    ArenaHeader     *freeArenasHead;

#if JS_BITS_PER_WORD == 32
    



    char            padding[16];
#endif

    




    uint32_t        lastDecommittedArenaOffset;

    
    uint32_t        numArenasFree;

    
    uint32_t        numArenasFreeCommitted;

    
    uint32_t        age;

    
    JSRuntime       *runtime;
};






























const size_t BytesPerArenaWithHeader = ArenaSize + ArenaBitmapBytes;
const size_t ChunkDecommitBitmapBytes = ChunkSize / ArenaSize / JS_BITS_PER_BYTE;
const size_t ChunkBytesAvailable = ChunkSize - sizeof(ChunkInfo) - ChunkDecommitBitmapBytes;
const size_t ArenasPerChunk = ChunkBytesAvailable / BytesPerArenaWithHeader;


struct ChunkBitmap
{
    volatile uintptr_t bitmap[ArenaBitmapWords * ArenasPerChunk];

    MOZ_ALWAYS_INLINE void getMarkWordAndMask(const Cell *cell, uint32_t color,
                                              uintptr_t **wordp, uintptr_t *maskp)
    {
        GetGCThingMarkWordAndMask(cell, color, wordp, maskp);
    }

    MOZ_ALWAYS_INLINE bool isMarked(const Cell *cell, uint32_t color) {
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
        




        JS_STATIC_ASSERT(ArenaBitmapBits == ArenaBitmapWords * JS_BITS_PER_WORD);

        uintptr_t *word, unused;
        getMarkWordAndMask(reinterpret_cast<Cell *>(aheader->address()), BLACK, &word, &unused);
        return word;
    }
};

JS_STATIC_ASSERT(ArenaBitmapBytes * ArenasPerChunk == sizeof(ChunkBitmap));
JS_STATIC_ASSERT(js::gc::ChunkMarkBitmapBits == ArenaBitmapBits * ArenasPerChunk);

typedef BitArray<ArenasPerChunk> PerArenaBitmap;

const size_t ChunkPadSize = ChunkSize
                            - (sizeof(Arena) * ArenasPerChunk)
                            - sizeof(ChunkBitmap)
                            - sizeof(PerArenaBitmap)
                            - sizeof(ChunkInfo);
JS_STATIC_ASSERT(ChunkPadSize < BytesPerArenaWithHeader);





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

    inline void addToAvailableList(Zone *zone);
    inline void insertToAvailableList(Chunk **insertPoint);
    inline void removeFromAvailableList();

    ArenaHeader *allocateArena(JS::Zone *zone, AllocKind kind);

    void releaseArena(ArenaHeader *aheader);

    static Chunk *allocate(JSRuntime *rt);

    
    static inline void release(JSRuntime *rt, Chunk *chunk);
    static inline void releaseList(JSRuntime *rt, Chunk *chunkListHead);

    
    inline void prepareToBeFreed(JSRuntime *rt);

    



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

JS_STATIC_ASSERT(sizeof(Chunk) == ChunkSize);
JS_STATIC_ASSERT(js::gc::ChunkMarkBitmapOffset == offsetof(Chunk, bitmap));

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
    return firstFreeSpanOffsets == FreeSpan::encodeOffsets(firstThingOffset, ArenaMask);
}

FreeSpan
ArenaHeader::getFirstFreeSpan() const
{
#ifdef DEBUG
    checkSynchronizedWithFreeList();
#endif
    return FreeSpan::decodeOffsets(arenaAddress(), firstFreeSpanOffsets);
}

void
ArenaHeader::setFirstFreeSpan(const FreeSpan *span)
{
    JS_ASSERT(span->isWithinArena(arenaAddress()));
    firstFreeSpanOffsets = span->encodeAsOffsets();
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
    JS_ASSERT_IF(color, color < aheader->getThingSize() / CellSize);
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
Cell::runtime() const
{
    return chunk()->info.runtime;
}

AllocKind
Cell::tenuredGetAllocKind() const
{
    return arenaHeader()->getAllocKind();
}

bool
Cell::isMarked(uint32_t color ) const
{
    JS_ASSERT(isTenured());
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

Zone *
Cell::tenuredZone() const
{
    JS_ASSERT(isTenured());
    return arenaHeader()->zone;
}

#ifdef DEBUG
bool
Cell::isAligned() const
{
    return Arena::isAligned(address(), arenaHeader()->getThingSize());
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
    addr &= ~(ChunkSize - 1);
    return reinterpret_cast<Chunk *>(addr);
}

inline bool
InFreeList(ArenaHeader *aheader, void *thing)
{
    if (!aheader->hasFreeThings())
        return false;

    FreeSpan firstSpan(aheader->getFirstFreeSpan());
    uintptr_t addr = reinterpret_cast<uintptr_t>(thing);

    for (const FreeSpan *span = &firstSpan;;) {
        
        if (addr < span->first)
            return false;

        




        if (addr <= span->last)
            return true;

        



        span = span->nextSpan();
    }
}

} 
} 

#endif 
