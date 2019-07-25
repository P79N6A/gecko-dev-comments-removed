






































#ifndef jsgc_h___
#define jsgc_h___




#include <setjmp.h>

#include "jstypes.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsdhash.h"
#include "jsbit.h"
#include "jsgcchunk.h"
#include "jsutil.h"
#include "jsvector.h"
#include "jsversion.h"
#include "jsobj.h"
#include "jsfun.h"
#include "jsgcstats.h"
#include "jscell.h"

struct JSCompartment;

extern "C" void
js_TraceXML(JSTracer *trc, JSXML* thing);

#if JS_STACK_GROWTH_DIRECTION > 0
# define JS_CHECK_STACK_SIZE(limit, lval)  ((jsuword)(lval) < limit)
#else
# define JS_CHECK_STACK_SIZE(limit, lval)  ((jsuword)(lval) > limit)
#endif

namespace js {

class GCHelperThread;
struct Shape;

namespace ion {
    class IonCode;
}

namespace gc {

struct Arena;
struct MarkingDelay;


enum FinalizeKind {
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
    FINALIZE_FUNCTION,
    FINALIZE_FUNCTION_AND_OBJECT_LAST = FINALIZE_FUNCTION,
    FINALIZE_SHAPE,
    FINALIZE_TYPE_OBJECT,
#if JS_HAS_XML_SUPPORT
    FINALIZE_XML,
#endif
    FINALIZE_SHORT_STRING,
    FINALIZE_STRING,
    FINALIZE_EXTERNAL_STRING,
    FINALIZE_IONCODE,
    FINALIZE_LIMIT
};

extern JS_FRIEND_DATA(const uint8) GCThingSizeMap[];

const size_t ArenaShift = 12;
const size_t ArenaSize = size_t(1) << ArenaShift;
const size_t ArenaMask = ArenaSize - 1;







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

    



    static size_t encodeOffsets(size_t firstOffset, size_t lastOffset = ArenaSize - 1) {
        
        JS_STATIC_ASSERT(ArenaShift < 16);
        JS_ASSERT(firstOffset <= ArenaSize);
        JS_ASSERT(lastOffset < ArenaSize);
        JS_ASSERT(firstOffset <= ((lastOffset + 1) & ~size_t(1)));
        return firstOffset | (lastOffset << 16);
    }

    static const size_t EmptyOffsets = ArenaSize | ((ArenaSize - 1) << 16);

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
    JSCompartment   *compartment;
    ArenaHeader     *next;

  private:
    




    size_t          firstFreeSpanOffsets;

    






    unsigned        thingKind;

    friend struct FreeLists;

  public:
    inline uintptr_t address() const;
    inline Chunk *chunk() const;

    void setAsNotAllocated() {
        thingKind = FINALIZE_LIMIT;
    }

    bool allocated() const {
        return thingKind < FINALIZE_LIMIT;
    }

    inline void init(JSCompartment *comp, unsigned thingKind, size_t thingSize);

    uintptr_t arenaAddress() const {
        return address();
    }

    Arena *getArena() {
        return reinterpret_cast<Arena *>(arenaAddress());
    }

    unsigned getThingKind() const {
        JS_ASSERT(allocated());
        return thingKind;
    }

    bool hasFreeThings() const {
        return firstFreeSpanOffsets != FreeSpan::EmptyOffsets;
    }

    void setAsFullyUsed() {
        firstFreeSpanOffsets = FreeSpan::EmptyOffsets;
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

    inline MarkingDelay *getMarkingDelay() const;

    size_t getThingSize() const {
        return GCThingSizeMap[getThingKind()];
    }

#ifdef DEBUG
    void checkSynchronizedWithFreeList() const;
#endif
};

struct Arena {
    













    ArenaHeader aheader;
    uint8_t     data[ArenaSize - sizeof(ArenaHeader)];

    static void staticAsserts() {
        JS_STATIC_ASSERT(sizeof(Arena) == ArenaSize);
    }

    static size_t thingsPerArena(size_t thingSize) {
        JS_ASSERT(thingSize % Cell::CellSize == 0);

        
        JS_ASSERT(thingSize >= sizeof(FreeSpan));

        
        JS_ASSERT(thingSize < 256);

        return (ArenaSize - sizeof(ArenaHeader)) / thingSize;
    }

    static size_t thingsSpan(size_t thingSize) {
        return thingsPerArena(thingSize) * thingSize;
    }

    static size_t thingsStartOffset(size_t thingSize) {
        return ArenaSize - thingsSpan(thingSize);
    }

    static bool isAligned(uintptr_t thing, size_t thingSize) {
        
        uintptr_t tailOffset = (ArenaSize - thing) & ArenaMask;
        return tailOffset % thingSize == 0;
    }

    uintptr_t address() const {
        return aheader.address();
    }

    uintptr_t thingsStart(size_t thingSize) {
        return address() | thingsStartOffset(thingSize);
    }

    uintptr_t thingsEnd() {
        return address() + ArenaSize;
    }

    template <typename T>
    bool finalize(JSContext *cx);
};






struct MarkingDelay {
    ArenaHeader *link;

    void init() {
        link = NULL;
    }

    





    static ArenaHeader *stackBottom() {
        return reinterpret_cast<ArenaHeader *>(ArenaSize);
    }
};


struct ChunkInfo {
    JSRuntime       *runtime;
    Chunk           *next;
    Chunk           **prevp;
    ArenaHeader     *emptyArenaListHead;
    size_t          age;
    size_t          numFree;
};

const size_t BytesPerArena = ArenaSize + ArenaBitmapBytes + sizeof(MarkingDelay);
const size_t ArenasPerChunk = (GC_CHUNK_SIZE - sizeof(ChunkInfo)) / BytesPerArena;


struct ChunkBitmap {
    uintptr_t bitmap[ArenaBitmapWords * ArenasPerChunk];

    JS_ALWAYS_INLINE void getMarkWordAndMask(const Cell *cell, uint32 color,
                                             uintptr_t **wordp, uintptr_t *maskp);

    JS_ALWAYS_INLINE bool isMarked(const Cell *cell, uint32 color) {
        uintptr_t *word, mask;
        getMarkWordAndMask(cell, color, &word, &mask);
        return *word & mask;
    }

    JS_ALWAYS_INLINE bool markIfUnmarked(const Cell *cell, uint32 color) {
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

    JS_ALWAYS_INLINE void unmark(const Cell *cell, uint32 color) {
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





struct Chunk {
    Arena           arenas[ArenasPerChunk];
    ChunkBitmap     bitmap;
    MarkingDelay    markingDelay[ArenasPerChunk];
    ChunkInfo       info;

    static Chunk *fromAddress(uintptr_t addr) {
        addr &= ~GC_CHUNK_MASK;
        return reinterpret_cast<Chunk *>(addr);
    }

    static bool withinArenasRange(uintptr_t addr) {
        uintptr_t offset = addr & GC_CHUNK_MASK;
        return offset < ArenasPerChunk * ArenaSize;
    }

    static size_t arenaIndex(uintptr_t addr) {
        JS_ASSERT(withinArenasRange(addr));
        return (addr & GC_CHUNK_MASK) >> ArenaShift;
    }

    uintptr_t address() const {
        uintptr_t addr = reinterpret_cast<uintptr_t>(this);
        JS_ASSERT(!(addr & GC_CHUNK_MASK));
        return addr;
    }

    void init(JSRuntime *rt);

    bool unused() const {
        return info.numFree == ArenasPerChunk;
    }

    bool hasAvailableArenas() const {
        return info.numFree > 0;
    }

    inline void addToAvailableList(JSCompartment *compartment);
    inline void removeFromAvailableList();

    template <size_t thingSize>
    ArenaHeader *allocateArena(JSContext *cx, unsigned thingKind);

    void releaseArena(ArenaHeader *aheader);
};

JS_STATIC_ASSERT(sizeof(Chunk) <= GC_CHUNK_SIZE);
JS_STATIC_ASSERT(sizeof(Chunk) + BytesPerArena > GC_CHUNK_SIZE);

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
    addr &= ~(GC_CHUNK_SIZE - 1);
    return reinterpret_cast<Chunk *>(addr);
}

#ifdef DEBUG
inline bool
Cell::isAligned() const
{
    return Arena::isAligned(address(), arenaHeader()->getThingSize());
}
#endif

inline void
ArenaHeader::init(JSCompartment *comp, unsigned kind, size_t thingSize)
{
    JS_ASSERT(!allocated());
    JS_ASSERT(!getMarkingDelay()->link);
    compartment = comp;
    thingKind = kind;
    firstFreeSpanOffsets = FreeSpan::encodeOffsets(Arena::thingsStartOffset(thingSize));
}

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

JS_ALWAYS_INLINE void
ChunkBitmap::getMarkWordAndMask(const Cell *cell, uint32 color,
                                uintptr_t **wordp, uintptr_t *maskp)
{
    JS_ASSERT(cell->chunk() == Chunk::fromAddress(reinterpret_cast<uintptr_t>(this)));
    size_t bit = (cell->address() & GC_CHUNK_MASK) / Cell::CellSize + color;
    JS_ASSERT(bit < ArenaBitmapBits * ArenasPerChunk);
    *maskp = uintptr_t(1) << (bit % JS_BITS_PER_WORD);
    *wordp = &bitmap[bit / JS_BITS_PER_WORD];
}

inline MarkingDelay *
ArenaHeader::getMarkingDelay() const
{
    return &chunk()->markingDelay[Chunk::arenaIndex(address())];
}

static void
AssertValidColor(const void *thing, uint32 color)
{
#ifdef DEBUG
    ArenaHeader *aheader = reinterpret_cast<const js::gc::Cell *>(thing)->arenaHeader();
    JS_ASSERT_IF(color, color < aheader->getThingSize() / Cell::CellSize);
#endif
}

inline bool
Cell::isMarked(uint32 color) const
{
    AssertValidColor(this, color);
    return chunk()->bitmap.isMarked(this, color);
}

bool
Cell::markIfUnmarked(uint32 color) const
{
    AssertValidColor(this, color);
    return chunk()->bitmap.markIfUnmarked(this, color);
}

void
Cell::unmark(uint32 color) const
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

#define JSTRACE_TYPE_OBJECT 3
#define JSTRACE_IONCODE     4
#define JSTRACE_XML         5




#define JSTRACE_LIMIT       6




const size_t GC_ALLOCATION_THRESHOLD = 30 * 1024 * 1024;






const float GC_HEAP_GROWTH_FACTOR = 3.0f;


static const int64 GC_IDLE_FULL_SPAN = 20 * 1000 * 1000;

static inline size_t
GetFinalizableTraceKind(size_t thingKind)
{
    JS_STATIC_ASSERT(JSExternalString::TYPE_LIMIT == 8);

    static const uint8 map[FINALIZE_LIMIT] = {
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
        JSTRACE_OBJECT,     
        JSTRACE_SHAPE,      
        JSTRACE_TYPE_OBJECT,
#if JS_HAS_XML_SUPPORT      
        JSTRACE_XML,
#endif
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_IONCODE,    
    };

    JS_ASSERT(thingKind < FINALIZE_LIMIT);
    return map[thingKind];
}

inline uint32
GetGCThingTraceKind(const void *thing);

static inline JSRuntime *
GetGCThingRuntime(void *thing)
{
    return reinterpret_cast<Cell *>(thing)->chunk()->info.runtime;
}


class ArenaList {
  private:
    ArenaHeader     *head;      
    ArenaHeader     **cursor;   

#ifdef JS_THREADSAFE
    















    enum BackgroundFinalizeState {
        BFS_DONE,
        BFS_RUN,
        BFS_JUST_FINISHED
    };

    volatile BackgroundFinalizeState backgroundFinalizeState;
#endif

  public:
    void init() {
        head = NULL;
        cursor = &head;
#ifdef JS_THREADSAFE
        backgroundFinalizeState = BFS_DONE;
#endif
    }

    ArenaHeader *getHead() { return head; }

    inline ArenaHeader *searchForFreeArena();

    template <size_t thingSize>
    inline ArenaHeader *getArenaWithFreeList(JSContext *cx, unsigned thingKind);

    template<typename T>
    void finalizeNow(JSContext *cx);

#ifdef JS_THREADSAFE
    template<typename T>
    inline void finalizeLater(JSContext *cx);

    static void backgroundFinalize(JSContext *cx, ArenaHeader *listHead);

    bool willBeFinalizedLater() const {
        return backgroundFinalizeState == BFS_RUN;
    }
#endif

#ifdef DEBUG
    bool markedThingsInArenaList() {
# ifdef JS_THREADSAFE
        
        JS_ASSERT(backgroundFinalizeState == BFS_DONE ||
                  backgroundFinalizeState == BFS_JUST_FINISHED);
# endif
        for (ArenaHeader *aheader = head; aheader; aheader = aheader->next) {
            if (!aheader->chunk()->bitmap.noBitsSet(aheader))
                return true;
        }
        return false;
    }
#endif 

    void releaseAll(unsigned thingKind) {
# ifdef JS_THREADSAFE
        



        JS_ASSERT(backgroundFinalizeState == BFS_DONE);
# endif
        while (ArenaHeader *aheader = head) {
            head = aheader->next;
            aheader->chunk()->releaseArena(aheader);
        }
        cursor = &head;
    }

    bool isEmpty() const {
#ifdef JS_THREADSAFE
        



        if (backgroundFinalizeState != BFS_DONE)
            return false;
#endif
        return !head;
    }
};

struct FreeLists {
    









    FreeSpan       lists[FINALIZE_LIMIT];

    void init() {
        for (size_t i = 0; i != JS_ARRAY_LENGTH(lists); ++i)
            lists[i].initAsEmpty();
    }

    



    void purge() {
        for (size_t i = 0; i != size_t(FINALIZE_LIMIT); ++i) {
            FreeSpan *list = &lists[i];
            if (!list->isEmpty()) {
                ArenaHeader *aheader = list->arenaHeader();
                JS_ASSERT(!aheader->hasFreeThings());
                aheader->setFirstFreeSpan(list);
                list->initAsEmpty();
            }
        }
    }

    




    void copyToArenas() {
        for (size_t i = 0; i != size_t(FINALIZE_LIMIT); ++i) {
            FreeSpan *list = &lists[i];
            if (!list->isEmpty()) {
                ArenaHeader *aheader = list->arenaHeader();
                JS_ASSERT(!aheader->hasFreeThings());
                aheader->setFirstFreeSpan(list);
            }
        }
    }

    



    void clearInArenas() {
        for (size_t i = 0; i != size_t(FINALIZE_LIMIT); ++i) {
            FreeSpan *list = &lists[i];
            if (!list->isEmpty()) {
                ArenaHeader *aheader = list->arenaHeader();
                JS_ASSERT(aheader->getFirstFreeSpan().isSameNonEmptySpan(list));
                aheader->setAsFullyUsed();
            }
        }
    }

    JS_ALWAYS_INLINE void *getNext(unsigned thingKind, size_t thingSize) {
        return lists[thingKind].allocate(thingSize);
    }

    void *populate(ArenaHeader *aheader, unsigned thingKind, size_t thingSize) {
        FreeSpan *list = &lists[thingKind];
        *list = aheader->getFirstFreeSpan();
        aheader->setAsFullyUsed();
        void *t = list->allocate(thingSize);
        JS_ASSERT(t);
        return t;
    }

    void checkEmpty() {
#ifdef DEBUG
        for (size_t i = 0; i != JS_ARRAY_LENGTH(lists); ++i)
            JS_ASSERT(lists[i].isEmpty());
#endif
    }
};

extern void *
RefillFinalizableFreeList(JSContext *cx, unsigned thingKind);






const size_t INITIAL_CHUNK_CAPACITY = 16 * 1024 * 1024 / GC_CHUNK_SIZE;


const size_t MAX_EMPTY_CHUNK_AGE = 4;

} 

struct GCPtrHasher
{
    typedef void *Lookup;

    static HashNumber hash(void *key) {
        return HashNumber(uintptr_t(key) >> JS_GCTHING_ZEROBITS);
    }

    static bool match(void *l, void *k) { return l == k; }
};

typedef HashMap<void *, uint32, GCPtrHasher, SystemAllocPolicy> GCLocks;

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


JS_STATIC_ASSERT(sizeof(HashNumber) == 4);

struct WrapperHasher
{
    typedef Value Lookup;

    static HashNumber hash(Value key) {
        uint64 bits = JSVAL_BITS(Jsvalify(key));
        return (uint32)bits ^ (uint32)(bits >> 32);
    }

    static bool match(const Value &l, const Value &k) { return l == k; }
};

typedef HashMap<Value, Value, WrapperHasher, SystemAllocPolicy> WrapperMap;

class AutoValueVector;
class AutoIdVector;

} 

#ifdef DEBUG
extern bool
CheckAllocation(JSContext *cx);
#endif

extern JS_FRIEND_API(uint32)
js_GetGCThingTraceKind(void *thing);

extern JSBool
js_InitGC(JSRuntime *rt, uint32 maxbytes);

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

extern uint32
js_MapGCRoots(JSRuntime *rt, JSGCRootMapFun map, void *data);


typedef struct JSPtrTable {
    size_t      count;
    void        **array;
} JSPtrTable;

extern JSBool
js_RegisterCloseableIterator(JSContext *cx, JSObject *obj);

#ifdef JS_TRACER
extern JSBool
js_ReserveObjects(JSContext *cx, size_t nobjects);
#endif

extern JSBool
js_LockGCThingRT(JSRuntime *rt, void *thing);

extern void
js_UnlockGCThingRT(JSRuntime *rt, void *thing);

extern JS_FRIEND_API(bool)
IsAboutToBeFinalized(JSContext *cx, const void *thing);

extern JS_FRIEND_API(bool)
js_GCThingIsMarked(void *thing, uintN color);

extern void
js_TraceStackFrame(JSTracer *trc, js::StackFrame *fp);

namespace js {

extern JS_REQUIRES_STACK void
MarkRuntime(JSTracer *trc);

extern void
TraceRuntime(JSTracer *trc);

extern JS_REQUIRES_STACK JS_FRIEND_API(void)
MarkContext(JSTracer *trc, JSContext *acx);


extern void
TriggerGC(JSRuntime *rt);


extern void
TriggerCompartmentGC(JSCompartment *comp);

extern void
MaybeGC(JSContext *cx);

} 




typedef enum JSGCInvocationKind {
    
    GC_NORMAL           = 0,

    



    GC_LAST_CONTEXT     = 1,

    
    GC_SHRINK             = 2
} JSGCInvocationKind;


extern void
js_GC(JSContext *cx, JSCompartment *comp, JSGCInvocationKind gckind);

#ifdef JS_THREADSAFE






extern void
js_WaitForGC(JSRuntime *rt);

#else 

# define js_WaitForGC(rt)    ((void) 0)

#endif

extern void
js_DestroyScriptsToGC(JSContext *cx, JSCompartment *comp);


namespace js {

#ifdef JS_THREADSAFE











class GCHelperThread {
    static const size_t FREE_ARRAY_SIZE = size_t(1) << 16;
    static const size_t FREE_ARRAY_LENGTH = FREE_ARRAY_SIZE / sizeof(void *);

    JSContext         *cx;
    PRThread*         thread;
    PRCondVar*        wakeup;
    PRCondVar*        sweepingDone;
    bool              shutdown;
    JSGCInvocationKind lastGCKind;

    Vector<void **, 16, js::SystemAllocPolicy> freeVector;
    void            **freeCursor;
    void            **freeCursorEnd;

    Vector<js::gc::ArenaHeader *, 64, js::SystemAllocPolicy> finalizeVector;

    friend class js::gc::ArenaList;

    JS_FRIEND_API(void)
    replenishAndFreeLater(void *ptr);

    static void freeElementsAndArray(void **array, void **end) {
        JS_ASSERT(array <= end);
        for (void **p = array; p != end; ++p)
            js::Foreground::free_(*p);
        js::Foreground::free_(array);
    }

    static void threadMain(void* arg);

    void threadLoop(JSRuntime *rt);
    void doSweep();

  public:
    GCHelperThread()
      : thread(NULL),
        wakeup(NULL),
        sweepingDone(NULL),
        shutdown(false),
        freeCursor(NULL),
        freeCursorEnd(NULL),
        sweeping(false) { }

    volatile bool     sweeping;
    bool init(JSRuntime *rt);
    void finish(JSRuntime *rt);

    
    void startBackgroundSweep(JSRuntime *rt, JSGCInvocationKind gckind);

    void waitBackgroundSweepEnd(JSRuntime *rt, bool gcUnlocked = true);

    void freeLater(void *ptr) {
        JS_ASSERT(!sweeping);
        if (freeCursor != freeCursorEnd)
            *freeCursor++ = ptr;
        else
            replenishAndFreeLater(ptr);
    }

    void setContext(JSContext *context) { cx = context; }
};

#endif 

struct GCChunkHasher {
    typedef gc::Chunk *Lookup;

    



    static HashNumber hash(gc::Chunk *chunk) {
        JS_ASSERT(!(jsuword(chunk) & GC_CHUNK_MASK));
        return HashNumber(jsuword(chunk) >> GC_CHUNK_SHIFT);
    }

    static bool match(gc::Chunk *k, gc::Chunk *l) {
        JS_ASSERT(!(jsuword(k) & GC_CHUNK_MASK));
        JS_ASSERT(!(jsuword(l) & GC_CHUNK_MASK));
        return k == l;
    }
};

typedef HashSet<js::gc::Chunk *, GCChunkHasher, SystemAllocPolicy> GCChunkSet;

struct ConservativeGCThreadData {

    



    jsuword             *nativeStackTop;

    union {
        jmp_buf         jmpbuf;
        jsuword         words[JS_HOWMANY(sizeof(jmp_buf), sizeof(jsuword))];
    } registerSnapshot;

    




    unsigned requestThreshold;

    ConservativeGCThreadData()
      : nativeStackTop(NULL), requestThreshold(0)
    {
    }

    ~ConservativeGCThreadData() {
#ifdef JS_THREADSAFE
        



        JS_ASSERT(!hasStackToScan());
#endif
    }

    JS_NEVER_INLINE void recordStackTop();

#ifdef JS_THREADSAFE
    void updateForRequestEnd(unsigned suspendCount) {
        if (suspendCount)
            recordStackTop();
        else
            nativeStackTop = NULL;
    }
#endif

    bool hasStackToScan() const {
        return !!nativeStackTop;
    }
};

template<class T>
struct MarkStack {
    T *stack;
    uintN tos, limit;

    bool push(T item) {
        if (tos == limit)
            return false;
        stack[tos++] = item;
        return true;
    }

    bool isEmpty() { return tos == 0; }

    T pop() {
        JS_ASSERT(!isEmpty());
        return stack[--tos];
    }

    T &peek() {
        JS_ASSERT(!isEmpty());
        return stack[tos-1];
    }

    MarkStack(void **buffer, size_t size)
    {
        tos = 0;
        limit = size / sizeof(T) - 1;
        stack = (T *)buffer;
    }
};

struct LargeMarkItem
{
    JSObject *obj;
    uintN markpos;

    LargeMarkItem(JSObject *obj) : obj(obj), markpos(0) {}
};

static const size_t OBJECT_MARK_STACK_SIZE = 32768 * sizeof(JSObject *);
static const size_t ROPES_MARK_STACK_SIZE = 1024 * sizeof(JSString *);
static const size_t XML_MARK_STACK_SIZE = 1024 * sizeof(JSXML *);
static const size_t TYPE_MARK_STACK_SIZE = 1024 * sizeof(types::TypeObject *);
static const size_t LARGE_MARK_STACK_SIZE = 64 * sizeof(LargeMarkItem);
static const size_t IONCODE_MARK_STACK_SIZE = 1024 * sizeof(ion::IonCode *);

struct GCMarker : public JSTracer {
  private:
    
    uint32 color;

  public:
    
    js::gc::ArenaHeader *unmarkedArenaStackTop;
#ifdef DEBUG
    size_t              markLaterArenas;
#endif

#ifdef JS_DUMP_CONSERVATIVE_GC_ROOTS
    js::gc::ConservativeGCStats conservativeStats;
    Vector<void *, 0, SystemAllocPolicy> conservativeRoots;
    const char *conservativeDumpFileName;
    void dumpConservativeRoots();
#endif

    MarkStack<JSObject *> objStack;
    MarkStack<JSRope *> ropeStack;
    MarkStack<types::TypeObject *> typeStack;
    MarkStack<JSXML *> xmlStack;
    MarkStack<LargeMarkItem> largeStack;
    MarkStack<ion::IonCode *> ionCodeStack;

  public:
    explicit GCMarker(JSContext *cx);
    ~GCMarker();

    uint32 getMarkColor() const {
        return color;
    }

    void setMarkColor(uint32 newColor) {
        
        drainMarkStack();
        color = newColor;
    }

    void delayMarkingChildren(const void *thing);

    void markDelayedChildren();

    bool isMarkStackEmpty() {
        return objStack.isEmpty() &&
               ropeStack.isEmpty() &&
               typeStack.isEmpty() &&
               xmlStack.isEmpty() &&
               largeStack.isEmpty() &&
               ionCodeStack.isEmpty();
    }

    JS_FRIEND_API(void) drainMarkStack();

    void pushObject(JSObject *obj) {
        if (!objStack.push(obj))
            delayMarkingChildren(obj);
    }

    void pushRope(JSRope *rope) {
        if (!ropeStack.push(rope))
            delayMarkingChildren(rope);
    }

    void pushType(types::TypeObject *type) {
        if (!typeStack.push(type))
            delayMarkingChildren(type);
    }

    void pushXML(JSXML *xml) {
        if (!xmlStack.push(xml))
            delayMarkingChildren(xml);
    }

    void pushIonCode(ion::IonCode *code) {
        if (!ionCodeStack.push(code))
            delayMarkingChildren(code);
    }
};

void
MarkStackRangeConservatively(JSTracer *trc, Value *begin, Value *end);

typedef void (*IterateCompartmentCallback)(JSContext *cx, void *data, JSCompartment *compartment);
typedef void (*IterateArenaCallback)(JSContext *cx, void *data, gc::Arena *arena, size_t traceKind,
                                     size_t thingSize);
typedef void (*IterateCellCallback)(JSContext *cx, void *data, void *thing, size_t traceKind,
                                    size_t thingSize);






extern JS_FRIEND_API(void)
IterateCompartmentsArenasCells(JSContext *cx, void *data,
                               IterateCompartmentCallback compartmentCallback,
                               IterateArenaCallback arenaCallback,
                               IterateCellCallback cellCallback);


void
IterateCells(JSContext *cx, JSCompartment *compartment, gc::FinalizeKind thingKind,
             void *data, IterateCellCallback cellCallback);

} 

extern void
js_FinalizeStringRT(JSRuntime *rt, JSString *str);




#define IS_GC_MARKING_TRACER(trc) ((trc)->callback == NULL)

#if JS_HAS_XML_SUPPORT
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) < JSTRACE_LIMIT)
#else
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) <= JSTRACE_IONCODE)
#endif

namespace js {
namespace gc {

JSCompartment *
NewCompartment(JSContext *cx, JSPrincipals *principals);


void
RunDebugGC(JSContext *cx);

} 
} 

inline JSCompartment *
JSObject::getCompartment() const
{
    return compartment();
}

#endif 
