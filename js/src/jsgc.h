






































#ifndef jsgc_h___
#define jsgc_h___




#include <setjmp.h>

#include "mozilla/Util.h"

#include "jsalloc.h"
#include "jstypes.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsdhash.h"
#include "jslock.h"
#include "jsutil.h"
#include "jsversion.h"
#include "jscell.h"

#include "ds/BitArray.h"
#include "gc/Statistics.h"
#include "js/HashTable.h"
#include "js/Vector.h"
#include "js/TemplateLib.h"

struct JSCompartment;

extern "C" void
js_TraceXML(JSTracer *trc, JSXML* thing);

#if JS_STACK_GROWTH_DIRECTION > 0
# define JS_CHECK_STACK_SIZE(limit, lval)  ((uintptr_t)(lval) < limit)
#else
# define JS_CHECK_STACK_SIZE(limit, lval)  ((uintptr_t)(lval) > limit)
#endif

namespace js {

class GCHelperThread;
struct Shape;

namespace ion {
    class IonCode;
}

namespace gc {

enum State {
    NO_INCREMENTAL,
    MARK_ROOTS,
    MARK,
    SWEEP,
    INVALID
};

struct Arena;





const size_t MAX_BACKGROUND_FINALIZE_KINDS = FINALIZE_LIMIT - FINALIZE_OBJECT_LIMIT / 2;







#if defined(SOLARIS) && (defined(__sparc) || defined(__sparcv9))
const size_t PageShift = 13;
#else
const size_t PageShift = 12;
#endif
const size_t PageSize = size_t(1) << PageShift;

const size_t ChunkShift = 20;
const size_t ChunkSize = size_t(1) << ChunkShift;
const size_t ChunkMask = ChunkSize - 1;

const size_t ArenaShift = PageShift;
const size_t ArenaSize = PageSize;
const size_t ArenaMask = ArenaSize - 1;





const static uint32_t FreeCommittedArenasThreshold = (32 << 20) / ArenaSize;







const size_t ArenaCellCount = size_t(1) << (ArenaShift - Cell::CellShift);
const size_t ArenaBitmapBits = ArenaCellCount;
const size_t ArenaBitmapBytes = ArenaBitmapBits / 8;
const size_t ArenaBitmapWords = ArenaBitmapBits / JS_BITS_PER_WORD;



























struct FreeSpan {
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

    
    JS_ALWAYS_INLINE void *allocate(size_t thingSize) {
        JS_ASSERT(thingSize % Cell::CellSize == 0);
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

    
    JS_ALWAYS_INLINE void *infallibleAllocate(size_t thingSize) {
        JS_ASSERT(thingSize % Cell::CellSize == 0);
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

    





    JS_ALWAYS_INLINE void *allocateFromNewArena(uintptr_t arenaAddr, size_t firstThingOffset,
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
            JS_ASSERT(spanLength % Cell::CellSize == 0);

            
            JS_ASSERT((first & ~ArenaMask) == arenaAddr);
            return;
        }

        
        JS_ASSERT(first <= last);
        size_t spanLengthWithoutOneThing = last - first;
        JS_ASSERT(spanLengthWithoutOneThing % Cell::CellSize == 0);

        JS_ASSERT((first & ~ArenaMask) == arenaAddr);

        




        size_t beforeTail = ArenaSize - (last & ArenaMask);
        JS_ASSERT(beforeTail >= sizeof(FreeSpan) + Cell::CellSize);

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


struct ArenaHeader {
    friend struct FreeLists;

    JSCompartment   *compartment;

    




    ArenaHeader     *next;

  private:
    




    size_t          firstFreeSpanOffsets;

    






    size_t       allocKind          : 8;

    
















  public:
    size_t       hasDelayedMarking  : 1;
    size_t       allocatedDuringIncremental : 1;
    size_t       markOverflow : 1;
    size_t       nextDelayedMarking : JS_BITS_PER_WORD - 8 - 1 - 1 - 1;

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

    void init(JSCompartment *comp, AllocKind kind) {
        JS_ASSERT(!allocated());
        JS_ASSERT(!markOverflow);
        JS_ASSERT(!allocatedDuringIncremental);
        JS_ASSERT(!hasDelayedMarking);
        compartment = comp;

        JS_STATIC_ASSERT(FINALIZE_LIMIT <= 255);
        allocKind = size_t(kind);

        
        firstFreeSpanOffsets = FreeSpan::FullArenaOffsets;
    }

    void setAsNotAllocated() {
        allocKind = size_t(FINALIZE_LIMIT);
        markOverflow = 0;
        allocatedDuringIncremental = 0;
        hasDelayedMarking = 0;
        nextDelayedMarking = 0;
    }

    uintptr_t arenaAddress() const {
        return address();
    }

    Arena *getArena() {
        return reinterpret_cast<Arena *>(arenaAddress());
    }

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

    FreeSpan getFirstFreeSpan() const {
#ifdef DEBUG
        checkSynchronizedWithFreeList();
#endif
        return FreeSpan::decodeOffsets(arenaAddress(), firstFreeSpanOffsets);
    }

    void setFirstFreeSpan(const FreeSpan *span) {
        JS_ASSERT(span->isWithinArena(arenaAddress()));
        firstFreeSpanOffsets = span->encodeAsOffsets();
    }

#ifdef DEBUG
    void checkSynchronizedWithFreeList() const;
#endif

    inline ArenaHeader *getNextDelayedMarking() const;
    inline void setNextDelayedMarking(ArenaHeader *aheader);
};

struct Arena {
    













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
        JS_ASSERT(thingSize % Cell::CellSize == 0);

        
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
    bool finalize(JSContext *cx, AllocKind thingKind, size_t thingSize, bool background);
};


struct ChunkInfo {
    Chunk           *next;
    Chunk           **prevp;

    
    ArenaHeader     *freeArenasHead;

    




    uint32_t        lastDecommittedArenaOffset;

    
    uint32_t        numArenasFree;

    
    uint32_t        numArenasFreeCommitted;

    
    uint32_t        age;
};






























const size_t BytesPerArenaWithHeader = ArenaSize + ArenaBitmapBytes;
const size_t ChunkDecommitBitmapBytes = ChunkSize / ArenaSize / JS_BITS_PER_BYTE;
const size_t ChunkBytesAvailable = ChunkSize - sizeof(ChunkInfo) - ChunkDecommitBitmapBytes;
const size_t ArenasPerChunk = ChunkBytesAvailable / BytesPerArenaWithHeader;


struct ChunkBitmap {
    uintptr_t bitmap[ArenaBitmapWords * ArenasPerChunk];

    JS_ALWAYS_INLINE void getMarkWordAndMask(const Cell *cell, uint32_t color,
                                             uintptr_t **wordp, uintptr_t *maskp);

    JS_ALWAYS_INLINE bool isMarked(const Cell *cell, uint32_t color) {
        uintptr_t *word, mask;
        getMarkWordAndMask(cell, color, &word, &mask);
        return *word & mask;
    }

    JS_ALWAYS_INLINE bool markIfUnmarked(const Cell *cell, uint32_t color) {
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

    JS_ALWAYS_INLINE void unmark(const Cell *cell, uint32_t color) {
        uintptr_t *word, mask;
        getMarkWordAndMask(cell, color, &word, &mask);
        *word &= ~mask;
    }

    void clear() {
        PodArrayZero(bitmap);
    }

#ifdef DEBUG
    bool noBitsSet(ArenaHeader *aheader) {
        




        JS_STATIC_ASSERT(ArenaBitmapBits == ArenaBitmapWords * JS_BITS_PER_WORD);

        uintptr_t *word, unused;
        getMarkWordAndMask(reinterpret_cast<Cell *>(aheader->address()), BLACK, &word, &unused);
        for (size_t i = 0; i != ArenaBitmapWords; i++) {
            if (word[i])
                return false;
        }
        return true;
    }
#endif
};

JS_STATIC_ASSERT(ArenaBitmapBytes * ArenasPerChunk == sizeof(ChunkBitmap));

typedef BitArray<ArenasPerChunk> PerArenaBitmap;

const size_t ChunkPadSize = ChunkSize
                            - (sizeof(Arena) * ArenasPerChunk)
                            - sizeof(ChunkBitmap)
                            - sizeof(PerArenaBitmap)
                            - sizeof(ChunkInfo);
JS_STATIC_ASSERT(ChunkPadSize < BytesPerArenaWithHeader);





struct Chunk {
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

    inline void addToAvailableList(JSCompartment *compartment);
    inline void insertToAvailableList(Chunk **insertPoint);
    inline void removeFromAvailableList();

    ArenaHeader *allocateArena(JSCompartment *comp, AllocKind kind);

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
    inline void init();

    
    jsuint findDecommittedArenaOffset();
    ArenaHeader* fetchNextDecommittedArena();

  public:
    
    inline ArenaHeader* fetchNextFreeArena(JSRuntime *rt);

    inline void addArenaToFreeList(JSRuntime *rt, ArenaHeader *aheader);
};

JS_STATIC_ASSERT(sizeof(Chunk) == ChunkSize);

class ChunkPool {
    Chunk   *emptyChunkListHead;
    size_t  emptyCount;

  public:
    ChunkPool()
      : emptyChunkListHead(NULL),
        emptyCount(0) { }

    size_t getEmptyCount() const {
        return emptyCount;
    }

    inline bool wantBackgroundAllocation(JSRuntime *rt) const;

    
    inline Chunk *get(JSRuntime *rt);

    
    inline void put(Chunk *chunk);

    



    Chunk *expire(JSRuntime *rt, bool releaseAll);

    
    void expireAndFree(JSRuntime *rt, bool releaseAll);

    
    JS_FRIEND_API(int64_t) countCleanDecommittedArenas(JSRuntime *rt);
};

inline uintptr_t
Cell::address() const
{
    uintptr_t addr = uintptr_t(this);
    JS_ASSERT(addr % Cell::CellSize == 0);
    JS_ASSERT(Chunk::withinArenasRange(addr));
    return addr;
}

inline ArenaHeader *
Cell::arenaHeader() const
{
    uintptr_t addr = address();
    addr &= ~ArenaMask;
    return reinterpret_cast<ArenaHeader *>(addr);
}

Chunk *
Cell::chunk() const
{
    uintptr_t addr = uintptr_t(this);
    JS_ASSERT(addr % Cell::CellSize == 0);
    addr &= ~(ChunkSize - 1);
    return reinterpret_cast<Chunk *>(addr);
}

AllocKind
Cell::getAllocKind() const
{
    return arenaHeader()->getAllocKind();
}

#ifdef DEBUG
inline bool
Cell::isAligned() const
{
    return Arena::isAligned(address(), arenaHeader()->getThingSize());
}
#endif

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

inline bool
ArenaHeader::isEmpty() const
{
    
    JS_ASSERT(allocated());
    size_t firstThingOffset = Arena::firstThingOffset(getAllocKind());
    return firstFreeSpanOffsets == FreeSpan::encodeOffsets(firstThingOffset, ArenaMask);
}

inline size_t
ArenaHeader::getThingSize() const
{
    JS_ASSERT(allocated());
    return Arena::thingSize(getAllocKind());
}

inline ArenaHeader *
ArenaHeader::getNextDelayedMarking() const
{
    return &reinterpret_cast<Arena *>(nextDelayedMarking << ArenaShift)->aheader;
}

inline void
ArenaHeader::setNextDelayedMarking(ArenaHeader *aheader)
{
    JS_ASSERT(!(uintptr_t(aheader) & ArenaMask));
    hasDelayedMarking = 1;
    nextDelayedMarking = aheader->arenaAddress() >> ArenaShift;
}

JS_ALWAYS_INLINE void
ChunkBitmap::getMarkWordAndMask(const Cell *cell, uint32_t color,
                                uintptr_t **wordp, uintptr_t *maskp)
{
    size_t bit = (cell->address() & ChunkMask) / Cell::CellSize + color;
    JS_ASSERT(bit < ArenaBitmapBits * ArenasPerChunk);
    *maskp = uintptr_t(1) << (bit % JS_BITS_PER_WORD);
    *wordp = &bitmap[bit / JS_BITS_PER_WORD];
}

static void
AssertValidColor(const void *thing, uint32_t color)
{
#ifdef DEBUG
    ArenaHeader *aheader = reinterpret_cast<const js::gc::Cell *>(thing)->arenaHeader();
    JS_ASSERT_IF(color, color < aheader->getThingSize() / Cell::CellSize);
#endif
}

inline bool
Cell::isMarked(uint32_t color) const
{
    AssertValidColor(this, color);
    return chunk()->bitmap.isMarked(this, color);
}

bool
Cell::markIfUnmarked(uint32_t color) const
{
    AssertValidColor(this, color);
    return chunk()->bitmap.markIfUnmarked(this, color);
}

void
Cell::unmark(uint32_t color) const
{
    JS_ASSERT(color != BLACK);
    AssertValidColor(this, color);
    chunk()->bitmap.unmark(this, color);
}

JSCompartment *
Cell::compartment() const
{
    return arenaHeader()->compartment;
}

static inline JSGCTraceKind
MapAllocToTraceKind(AllocKind thingKind)
{
    static const JSGCTraceKind map[FINALIZE_LIMIT] = {
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
        JSTRACE_SHAPE,      
        JSTRACE_BASE_SHAPE, 
        JSTRACE_TYPE_OBJECT,
#if JS_HAS_XML_SUPPORT      
        JSTRACE_XML,
#endif
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_IONCODE,    
    };
    return map[thingKind];
}

inline JSGCTraceKind
GetGCThingTraceKind(const void *thing);

struct ArenaLists {

    










    struct ArenaList {
        ArenaHeader     *head;
        ArenaHeader     **cursor;

        ArenaList() {
            clear();
        }

        void clear() {
            head = NULL;
            cursor = &head;
        }
    };

  private:
    








    FreeSpan       freeLists[FINALIZE_LIMIT];

    ArenaList      arenaLists[FINALIZE_LIMIT];

#ifdef JS_THREADSAFE
    















    enum BackgroundFinalizeState {
        BFS_DONE,
        BFS_RUN,
        BFS_JUST_FINISHED
    };

    volatile uintptr_t backgroundFinalizeState[FINALIZE_LIMIT];
#endif

  public:
    ArenaLists() {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            freeLists[i].initAsEmpty();
#ifdef JS_THREADSAFE
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            backgroundFinalizeState[i] = BFS_DONE;
#endif
    }

    ~ArenaLists() {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i) {
#ifdef JS_THREADSAFE
            



            JS_ASSERT(backgroundFinalizeState[i] == BFS_DONE);
#endif
            ArenaHeader **headp = &arenaLists[i].head;
            while (ArenaHeader *aheader = *headp) {
                *headp = aheader->next;
                aheader->chunk()->releaseArena(aheader);
            }
        }
    }

    const FreeSpan *getFreeList(AllocKind thingKind) const {
        return &freeLists[thingKind];
    }

    ArenaHeader *getFirstArena(AllocKind thingKind) const {
        return arenaLists[thingKind].head;
    }

    bool arenaListsAreEmpty() const {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i) {
#ifdef JS_THREADSAFE
            



            if (backgroundFinalizeState[i] != BFS_DONE)
                return false;
#endif
            if (arenaLists[i].head)
                return false;
        }
        return true;
    }

#ifdef DEBUG
    bool checkArenaListAllUnmarked() const {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i) {
# ifdef JS_THREADSAFE
            
            JS_ASSERT(backgroundFinalizeState[i] == BFS_DONE ||
                      backgroundFinalizeState[i] == BFS_JUST_FINISHED);
# endif
            for (ArenaHeader *aheader = arenaLists[i].head; aheader; aheader = aheader->next) {
                if (!aheader->chunk()->bitmap.noBitsSet(aheader))
                    return false;
            }
        }
        return true;
    }
#endif

#ifdef JS_THREADSAFE
    bool doneBackgroundFinalize(AllocKind kind) const {
        return backgroundFinalizeState[kind] == BFS_DONE;
    }
#endif

    



    void purge() {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i) {
            FreeSpan *headSpan = &freeLists[i];
            if (!headSpan->isEmpty()) {
                ArenaHeader *aheader = headSpan->arenaHeader();
                aheader->setFirstFreeSpan(headSpan);
                headSpan->initAsEmpty();
            }
        }
    }

    inline void prepareForIncrementalGC(JSRuntime *rt);

    




    void copyFreeListsToArenas() {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            copyFreeListToArena(AllocKind(i));
    }

    void copyFreeListToArena(AllocKind thingKind) {
        FreeSpan *headSpan = &freeLists[thingKind];
        if (!headSpan->isEmpty()) {
            ArenaHeader *aheader = headSpan->arenaHeader();
            JS_ASSERT(!aheader->hasFreeThings());
            aheader->setFirstFreeSpan(headSpan);
        }
    }

    



    void clearFreeListsInArenas() {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            clearFreeListInArena(AllocKind(i));
    }


    void clearFreeListInArena(AllocKind kind) {
        FreeSpan *headSpan = &freeLists[kind];
        if (!headSpan->isEmpty()) {
            ArenaHeader *aheader = headSpan->arenaHeader();
            JS_ASSERT(aheader->getFirstFreeSpan().isSameNonEmptySpan(headSpan));
            aheader->setAsFullyUsed();
        }
    }

    



    bool isSynchronizedFreeList(AllocKind kind) {
        FreeSpan *headSpan = &freeLists[kind];
        if (headSpan->isEmpty())
            return true;
        ArenaHeader *aheader = headSpan->arenaHeader();
        if (aheader->hasFreeThings()) {
            



            JS_ASSERT(aheader->getFirstFreeSpan().isSameNonEmptySpan(headSpan));
            return true;
        }
        return false;
    }

    JS_ALWAYS_INLINE void *allocateFromFreeList(AllocKind thingKind, size_t thingSize) {
        return freeLists[thingKind].allocate(thingSize);
    }

    static void *refillFreeList(JSContext *cx, AllocKind thingKind);

    void checkEmptyFreeLists() {
#ifdef DEBUG
        for (size_t i = 0; i < mozilla::ArrayLength(freeLists); ++i)
            JS_ASSERT(freeLists[i].isEmpty());
#endif
    }

    void checkEmptyFreeList(AllocKind kind) {
        JS_ASSERT(freeLists[kind].isEmpty());
    }

    void finalizeObjects(JSContext *cx);
    void finalizeStrings(JSContext *cx);
    void finalizeShapes(JSContext *cx);
    void finalizeScripts(JSContext *cx);
    void finalizeIonCode(JSContext *cx);

#ifdef JS_THREADSAFE
    static void backgroundFinalize(JSContext *cx, ArenaHeader *listHead);
#endif

  private:
    inline void finalizeNow(JSContext *cx, AllocKind thingKind);
    inline void finalizeLater(JSContext *cx, AllocKind thingKind);

    inline void *allocateFromArena(JSCompartment *comp, AllocKind thingKind);
};






const size_t INITIAL_CHUNK_CAPACITY = 16 * 1024 * 1024 / ChunkSize;


const size_t MAX_EMPTY_CHUNK_AGE = 4;

inline Cell *
AsCell(JSObject *obj)
{
    return reinterpret_cast<Cell *>(obj);
}

} 

struct GCPtrHasher
{
    typedef void *Lookup;

    static HashNumber hash(void *key) {
        return HashNumber(uintptr_t(key) >> JS_GCTHING_ZEROBITS);
    }

    static bool match(void *l, void *k) { return l == k; }
};

typedef HashMap<void *, uint32_t, GCPtrHasher, SystemAllocPolicy> GCLocks;

struct RootInfo {
    RootInfo() {}
    RootInfo(const char *name, JSGCRootType type) : name(name), type(type) {}
    const char *name;
    JSGCRootType type;
};

typedef js::HashMap<void *,
                    RootInfo,
                    js::DefaultHasher<void *>,
                    js::SystemAllocPolicy> RootedValueMap;

} 

extern JS_FRIEND_API(JSGCTraceKind)
js_GetGCThingTraceKind(void *thing);

extern JSBool
js_InitGC(JSRuntime *rt, uint32_t maxbytes);

extern void
js_FinishGC(JSRuntime *rt);

extern JSBool
js_AddRoot(JSContext *cx, js::Value *vp, const char *name);

extern JSBool
js_AddGCThingRoot(JSContext *cx, void **rp, const char *name);

#ifdef DEBUG
extern void
js_DumpNamedRoots(JSRuntime *rt,
                  void (*dump)(const char *name, void *rp, JSGCRootType type, void *data),
                  void *data);
#endif

extern uint32_t
js_MapGCRoots(JSRuntime *rt, JSGCRootMapFun map, void *data);


typedef struct JSPtrTable {
    size_t      count;
    void        **array;
} JSPtrTable;

extern JSBool
js_LockGCThingRT(JSRuntime *rt, void *thing);

extern void
js_UnlockGCThingRT(JSRuntime *rt, void *thing);

extern JS_FRIEND_API(bool)
IsAboutToBeFinalized(const js::gc::Cell *thing);

extern bool
IsAboutToBeFinalized(const js::Value &value);

extern bool
js_IsAddressableGCThing(JSRuntime *rt, uintptr_t w, js::gc::AllocKind *thingKind, void **thing);

namespace js {

extern void
MarkCompartmentActive(js::StackFrame *fp);

extern void
TraceRuntime(JSTracer *trc);

extern JS_FRIEND_API(void)
MarkContext(JSTracer *trc, JSContext *acx);


extern void
TriggerGC(JSRuntime *rt, js::gcreason::Reason reason);


extern void
TriggerCompartmentGC(JSCompartment *comp, js::gcreason::Reason reason);

extern void
MaybeGC(JSContext *cx);

extern void
ShrinkGCBuffers(JSRuntime *rt);




typedef enum JSGCInvocationKind {
    
    GC_NORMAL           = 0,

    
    GC_SHRINK             = 1
} JSGCInvocationKind;


extern void
GC(JSContext *cx, JSCompartment *comp, JSGCInvocationKind gckind, js::gcreason::Reason reason);

extern void
GCSlice(JSContext *cx, JSCompartment *comp, JSGCInvocationKind gckind, js::gcreason::Reason reason);

extern void
GCDebugSlice(JSContext *cx, int64_t objCount);

} 

namespace js {

void
InitTracer(JSTracer *trc, JSRuntime *rt, JSTraceCallback callback);

#ifdef JS_THREADSAFE

class GCHelperThread {
    enum State {
        IDLE,
        SWEEPING,
        ALLOCATING,
        CANCEL_ALLOCATION,
        SHUTDOWN
    };

    









    static const size_t FREE_ARRAY_SIZE = size_t(1) << 16;
    static const size_t FREE_ARRAY_LENGTH = FREE_ARRAY_SIZE / sizeof(void *);

    JSRuntime         *const rt;
    PRThread          *thread;
    PRCondVar         *wakeup;
    PRCondVar         *done;
    volatile State    state;

    JSContext         *finalizationContext;
    bool              shrinkFlag;

    Vector<void **, 16, js::SystemAllocPolicy> freeVector;
    void            **freeCursor;
    void            **freeCursorEnd;

    Vector<js::gc::ArenaHeader *, 64, js::SystemAllocPolicy> finalizeVector;

    bool    backgroundAllocation;

    friend struct js::gc::ArenaLists;

    JS_FRIEND_API(void)
    replenishAndFreeLater(void *ptr);

    static void freeElementsAndArray(void **array, void **end) {
        JS_ASSERT(array <= end);
        for (void **p = array; p != end; ++p)
            js::Foreground::free_(*p);
        js::Foreground::free_(array);
    }

    static void threadMain(void* arg);
    void threadLoop();

    
    void doSweep();

  public:
    GCHelperThread(JSRuntime *rt)
      : rt(rt),
        thread(NULL),
        wakeup(NULL),
        done(NULL),
        state(IDLE),
        finalizationContext(NULL),
        shrinkFlag(false),
        freeCursor(NULL),
        freeCursorEnd(NULL),
        backgroundAllocation(true)
    { }

    bool init();
    void finish();

    
    void startBackgroundSweep(JSContext *cx, bool shouldShrink);

    
    void startBackgroundShrink();

    
    void waitBackgroundSweepEnd();

    
    void waitBackgroundSweepOrAllocEnd();

    
    inline void startBackgroundAllocationIfIdle();

    bool canBackgroundAllocate() const {
        return backgroundAllocation;
    }

    void disableBackgroundAllocation() {
        backgroundAllocation = false;
    }

    PRThread *getThread() const {
        return thread;
    }

    



    bool sweeping() const {
        return state == SWEEPING;
    }

    bool shouldShrink() const {
        JS_ASSERT(sweeping());
        return shrinkFlag;
    }

    void freeLater(void *ptr) {
        JS_ASSERT(!sweeping());
        if (freeCursor != freeCursorEnd)
            *freeCursor++ = ptr;
        else
            replenishAndFreeLater(ptr);
    }

    
    bool prepareForBackgroundSweep();
};

#endif 

struct GCChunkHasher {
    typedef gc::Chunk *Lookup;

    



    static HashNumber hash(gc::Chunk *chunk) {
        JS_ASSERT(!(uintptr_t(chunk) & gc::ChunkMask));
        return HashNumber(uintptr_t(chunk) >> gc::ChunkShift);
    }

    static bool match(gc::Chunk *k, gc::Chunk *l) {
        JS_ASSERT(!(uintptr_t(k) & gc::ChunkMask));
        JS_ASSERT(!(uintptr_t(l) & gc::ChunkMask));
        return k == l;
    }
};

typedef HashSet<js::gc::Chunk *, GCChunkHasher, SystemAllocPolicy> GCChunkSet;

template<class T>
struct MarkStack {
    T *stack;
    T *tos;
    T *limit;

    T *ballast;
    T *ballastLimit;

    size_t sizeLimit;

    MarkStack(size_t sizeLimit)
      : stack(NULL),
        tos(NULL),
        limit(NULL),
        ballast(NULL),
        ballastLimit(NULL),
        sizeLimit(sizeLimit) { }

    ~MarkStack() {
        if (stack != ballast)
            js_free(stack);
        js_free(ballast);
    }

    bool init(size_t ballastcap) {
        JS_ASSERT(!stack);

        if (ballastcap == 0)
            return true;

        ballast = (T *)js_malloc(sizeof(T) * ballastcap);
        if (!ballast)
            return false;
        ballastLimit = ballast + ballastcap;
        initFromBallast();
        return true;
    }

    void initFromBallast() {
        stack = ballast;
        limit = ballastLimit;
        if (size_t(limit - stack) > sizeLimit)
            limit = stack + sizeLimit;
        tos = stack;
    }

    void setSizeLimit(size_t size) {
        JS_ASSERT(isEmpty());

        sizeLimit = size;
        reset();
    }

    bool push(T item) {
        if (tos == limit) {
            if (!enlarge())
                return false;
        }
        JS_ASSERT(tos < limit);
        *tos++ = item;
        return true;
    }

    bool push(T item1, T item2, T item3) {
        T *nextTos = tos + 3;
        if (nextTos > limit) {
            if (!enlarge())
                return false;
            nextTos = tos + 3;
        }
        JS_ASSERT(nextTos <= limit);
        tos[0] = item1;
        tos[1] = item2;
        tos[2] = item3;
        tos = nextTos;
        return true;
    }

    bool isEmpty() const {
        return tos == stack;
    }

    T pop() {
        JS_ASSERT(!isEmpty());
        return *--tos;
    }

    ptrdiff_t position() const {
        return tos - stack;
    }

    void reset() {
        if (stack != ballast)
            js_free(stack);
        initFromBallast();
        JS_ASSERT(stack == ballast);
    }

    bool enlarge() {
        size_t tosIndex = tos - stack;
        size_t cap = limit - stack;
        if (cap == sizeLimit)
            return false;
        size_t newcap = cap * 2;
        if (newcap == 0)
            newcap = 32;
        if (newcap > sizeLimit)
            newcap = sizeLimit;

        T *newStack;
        if (stack == ballast) {
            newStack = (T *)js_malloc(sizeof(T) * newcap);
            if (!newStack)
                return false;
            for (T *src = stack, *dst = newStack; src < tos; )
                *dst++ = *src++;
        } else {
            newStack = (T *)js_realloc(stack, sizeof(T) * newcap);
            if (!newStack)
                return false;
        }
        stack = newStack;
        tos = stack + tosIndex;
        limit = newStack + newcap;
        return true;
    }

    size_t sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        size_t n = 0;
        if (stack != ballast)
            n += mallocSizeOf(stack);
        n += mallocSizeOf(ballast);
        return n;
    }
};







struct SliceBudget {
    int64_t deadline; 
    intptr_t counter;

    static const intptr_t CounterReset = 1000;

    static const int64_t Unlimited = 0;
    static int64_t TimeBudget(int64_t millis);
    static int64_t WorkBudget(int64_t work);

    
    SliceBudget();

    
    SliceBudget(int64_t budget);

    void reset() {
        deadline = INT64_MAX;
        counter = INTPTR_MAX;
    }

    void step() {
        counter--;
    }

    bool checkOverBudget();

    bool isOverBudget() {
        if (counter > 0)
            return false;
        return checkOverBudget();
    }
};

static const size_t MARK_STACK_LENGTH = 32768;

struct GCMarker : public JSTracer {
  private:
    




    enum StackTag {
        ValueArrayTag,
        ObjectTag,
        TypeTag,
        XmlTag,
        SavedValueArrayTag,
        IonCodeTag,
        LastTag = IonCodeTag
    };

    static const uintptr_t StackTagMask = 7;

    static void staticAsserts() {
        JS_STATIC_ASSERT(StackTagMask >= uintptr_t(LastTag));
        JS_STATIC_ASSERT(StackTagMask <= gc::Cell::CellMask);
    }

  public:
    explicit GCMarker();
    bool init();

    void setSizeLimit(size_t size) { stack.setSizeLimit(size); }
    size_t sizeLimit() const { return stack.sizeLimit; }

    void start(JSRuntime *rt);
    void stop();
    void reset();

    void pushObject(JSObject *obj) {
        pushTaggedPtr(ObjectTag, obj);
    }

    void pushType(types::TypeObject *type) {
        pushTaggedPtr(TypeTag, type);
    }

    void pushXML(JSXML *xml) {
        pushTaggedPtr(XmlTag, xml);
    }
	
    void pushIonCode(ion::IonCode *code) {
        pushTaggedPtr(IonCodeTag, code);
    }

    uint32_t getMarkColor() const {
        return color;
    }

    







    void setMarkColorGray() {
        JS_ASSERT(isDrained());
        JS_ASSERT(color == gc::BLACK);
        color = gc::GRAY;
    }

    inline void delayMarkingArena(gc::ArenaHeader *aheader);
    void delayMarkingChildren(const void *thing);
    void markDelayedChildren(gc::ArenaHeader *aheader);
    bool markDelayedChildren(SliceBudget &budget);
    bool hasDelayedChildren() const {
        return !!unmarkedArenaStackTop;
    }

    bool isDrained() {
        return isMarkStackEmpty() && !unmarkedArenaStackTop;
    }

    bool drainMarkStack(SliceBudget &budget);

    







    bool hasBufferedGrayRoots() const;
    void startBufferingGrayRoots();
    void endBufferingGrayRoots();
    void markBufferedGrayRoots();

    static void GrayCallback(JSTracer *trc, void **thing, JSGCTraceKind kind);

    size_t sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf) const;

    MarkStack<uintptr_t> stack;

  private:
#ifdef DEBUG
    void checkCompartment(void *p);
#else
    void checkCompartment(void *p) {}
#endif

    void pushTaggedPtr(StackTag tag, void *ptr) {
        checkCompartment(ptr);
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        JS_ASSERT(!(addr & StackTagMask));
        if (!stack.push(addr | uintptr_t(tag)))
            delayMarkingChildren(ptr);
    }

    void pushValueArray(JSObject *obj, void *start, void *end) {
        checkCompartment(obj);

        if (start == end)
            return;

        JS_ASSERT(start <= end);
        uintptr_t tagged = reinterpret_cast<uintptr_t>(obj) | GCMarker::ValueArrayTag;
        uintptr_t startAddr = reinterpret_cast<uintptr_t>(start);
        uintptr_t endAddr = reinterpret_cast<uintptr_t>(end);

        



        if (!stack.push(endAddr, startAddr, tagged))
            delayMarkingChildren(obj);
    }

    bool isMarkStackEmpty() {
        return stack.isEmpty();
    }

    bool restoreValueArray(JSObject *obj, void **vpp, void **endp);
    void saveValueRanges();
    inline void processMarkStackTop(SliceBudget &budget);

    void appendGrayRoot(void *thing, JSGCTraceKind kind);

    
    uint32_t color;

    DebugOnly<bool> started;

    
    js::gc::ArenaHeader *unmarkedArenaStackTop;
    
    DebugOnly<size_t> markLaterArenas;

    struct GrayRoot {
        void *thing;
        JSGCTraceKind kind;
#ifdef DEBUG
        JSTraceNamePrinter debugPrinter;
        const void *debugPrintArg;
        size_t debugPrintIndex;
#endif

        GrayRoot(void *thing, JSGCTraceKind kind)
          : thing(thing), kind(kind) {}
    };

    bool grayFailed;
    Vector<GrayRoot, 0, SystemAllocPolicy> grayRoots;
};

void
SetMarkStackLimit(JSRuntime *rt, size_t limit);

void
MarkStackRangeConservatively(JSTracer *trc, Value *begin, Value *end);

typedef void (*IterateChunkCallback)(JSRuntime *rt, void *data, gc::Chunk *chunk);
typedef void (*IterateArenaCallback)(JSRuntime *rt, void *data, gc::Arena *arena,
                                     JSGCTraceKind traceKind, size_t thingSize);
typedef void (*IterateCellCallback)(JSRuntime *rt, void *data, void *thing,
                                    JSGCTraceKind traceKind, size_t thingSize);






extern JS_FRIEND_API(void)
IterateCompartmentsArenasCells(JSRuntime *rt, void *data,
                               JSIterateCompartmentCallback compartmentCallback,
                               IterateArenaCallback arenaCallback,
                               IterateCellCallback cellCallback);




extern JS_FRIEND_API(void)
IterateChunks(JSRuntime *rt, void *data, IterateChunkCallback chunkCallback);





extern JS_FRIEND_API(void)
IterateCells(JSRuntime *rt, JSCompartment *compartment, gc::AllocKind thingKind,
             void *data, IterateCellCallback cellCallback);

} 

extern void
js_FinalizeStringRT(JSRuntime *rt, JSString *str);




#define IS_GC_MARKING_TRACER(trc) \
    ((trc)->callback == NULL || (trc)->callback == GCMarker::GrayCallback)

namespace js {
namespace gc {

JSCompartment *
NewCompartment(JSContext *cx, JSPrincipals *principals);


void
RunDebugGC(JSContext *cx);

#if defined(JSGC_ROOT_ANALYSIS) && defined(DEBUG) && !defined(JS_THREADSAFE)

void CheckStackRoots(JSContext *cx);

inline void MaybeCheckStackRoots(JSContext *cx) { CheckStackRoots(cx); }
#else
inline void MaybeCheckStackRoots(JSContext *cx) {}
#endif

const int ZealPokeValue = 1;
const int ZealAllocValue = 2;
const int ZealFrameGCValue = 3;
const int ZealVerifierValue = 4;
const int ZealFrameVerifierValue = 5;

#ifdef JS_GC_ZEAL


void
VerifyBarriers(JSContext *cx);

void
MaybeVerifyBarriers(JSContext *cx, bool always = false);

#else

static inline void
VerifyBarriers(JSContext *cx)
{
}

static inline void
MaybeVerifyBarriers(JSContext *cx, bool always = false)
{
}

#endif

} 

static inline JSCompartment *
GetObjectCompartment(JSObject *obj) { return reinterpret_cast<js::gc::Cell *>(obj)->compartment(); }

void
ReleaseAllJITCode(JSContext *cx, JSCompartment *c, bool resetUseCounts);

void
ReleaseAllJITCode(JSContext *cx, bool resetUseCounts);

} 

#endif 
