






































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
#if JS_HAS_XML_SUPPORT
    FINALIZE_XML,
#endif
    FINALIZE_SHORT_STRING,
    FINALIZE_STRING,
    FINALIZE_EXTERNAL_STRING,
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
    uintptr_t   start;
    uintptr_t   end;

  public:
    FreeSpan() { }

    FreeSpan(uintptr_t start, uintptr_t end)
      : start(start), end(end) {
        checkSpan();
    }

    bool isEmpty() const {
        checkSpan();
        return !(start & ArenaMask);
    }

    bool hasNext() const {
        checkSpan();
        return !!(end & ArenaMask);
    }

    FreeSpan *nextSpan() const {
        JS_ASSERT(hasNext());
        return reinterpret_cast<FreeSpan *>(end);
    }

    FreeSpan *nextSpanUnchecked() const {
        JS_ASSERT(end & ArenaMask);
        return reinterpret_cast<FreeSpan *>(end);
    }

    uintptr_t arenaAddress() const {
        JS_ASSERT(!isEmpty());
        return start & ~ArenaMask;
    }

    void checkSpan() const {
#ifdef DEBUG
        JS_ASSERT(start <= end);
        JS_ASSERT(end - start <= ArenaSize);
        if (!(start & ArenaMask)) {
            
            JS_ASSERT(start == end);
            return;
        }

        JS_ASSERT(start);
        JS_ASSERT(end);
        uintptr_t arena = start & ~ArenaMask;
        if (!(end & ArenaMask)) {
            
            JS_ASSERT(arena + ArenaSize == end);
            return;
        }

        
        JS_ASSERT(arena == (end & ~ArenaMask));
        FreeSpan *next = reinterpret_cast<FreeSpan *>(end);

        




        JS_ASSERT(end < next->start);

        if (!(next->start & ArenaMask)) {
            



            JS_ASSERT(next->start == next->end);
            JS_ASSERT(arena + ArenaSize == next->start);
        } else {
            
            JS_ASSERT(arena == (next->start & ~ArenaMask));
        }
#endif
    }
};


struct ArenaHeader {
    JSCompartment   *compartment;
    ArenaHeader     *next;

  private:
    






    uint16_t        firstFreeSpanStart;
    uint16_t        firstFreeSpanEnd;

    unsigned        thingKind;

    friend struct FreeLists;

  public:
    inline uintptr_t address() const;
    inline Chunk *chunk() const;

    inline void init(JSCompartment *comp, unsigned thingKind, size_t thingSize);

    Arena *getArena() {
        return reinterpret_cast<Arena *>(address());
    }

    unsigned getThingKind() const {
        return thingKind;
    }

    bool hasFreeThings() const {
        return firstFreeSpanStart != ArenaSize;
    }

    void setAsFullyUsed() {
        firstFreeSpanStart = firstFreeSpanEnd = ArenaSize;
    }

    FreeSpan getFirstFreeSpan() const {
#ifdef DEBUG
        checkSynchronizedWithFreeList();
#endif
        return FreeSpan(address() + firstFreeSpanStart, address() + firstFreeSpanEnd);
    }

    void setFirstFreeSpan(const FreeSpan *span) {
        span->checkSpan();
        JS_ASSERT(span->start - address() <= ArenaSize);
        JS_ASSERT(span->end - address() <= ArenaSize);
        firstFreeSpanStart = uint16_t(span->start - address());
        firstFreeSpanEnd = uint16_t(span->end - address());
    }

    inline MarkingDelay *getMarkingDelay() const;

    size_t getThingSize() const {
        return GCThingSizeMap[getThingKind()];
    }

#ifdef DEBUG
    void checkSynchronizedWithFreeList() const;
#endif

#if defined DEBUG || defined JS_GCMETER
    static size_t CountListLength(const ArenaHeader *aheader) {
        size_t n = 0;
        for (; aheader; aheader = aheader->next)
            ++n;
        return n;
    }
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
    Chunk           *link;
    JSRuntime       *runtime;
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

    void init(JSRuntime *rt);
    bool unused();
    bool hasAvailableArenas();
    bool withinArenasRange(Cell *cell);

    template <size_t thingSize>
    ArenaHeader *allocateArena(JSContext *cx, unsigned thingKind);

    void releaseArena(ArenaHeader *aheader);

    JSRuntime *getRuntime();
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
    JS_ASSERT(!compartment);
    JS_ASSERT(!getMarkingDelay()->link);
    compartment = comp;
    thingKind = kind;
    firstFreeSpanStart = Arena::thingsStartOffset(thingSize);
    firstFreeSpanEnd = ArenaSize;
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

#define JSTRACE_XML         3




#define JSTRACE_LIMIT       4




const size_t GC_ARENA_ALLOCATION_TRIGGER = 30 * js::GC_CHUNK_SIZE;







const float GC_HEAP_GROWTH_FACTOR = 3.0f;

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
#if JS_HAS_XML_SUPPORT      
        JSTRACE_XML,
#endif
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
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
#ifdef JS_GCMETER
    JSGCArenaStats  stats;
#endif

    void init() {
        head = NULL;
        cursor = &head;
#ifdef JS_THREADSAFE
        backgroundFinalizeState = BFS_DONE;
#endif
#ifdef JS_GCMETER
        PodZero(&stats);
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
            lists[i].start = lists[i].end = 0;
    }

    



    void purge() {
        for (size_t i = 0; i != size_t(FINALIZE_LIMIT); ++i) {
            FreeSpan *list = &lists[i];
            if (!list->isEmpty()) {
                ArenaHeader *aheader = reinterpret_cast<Cell *>(list->start)->arenaHeader();
                JS_ASSERT(!aheader->hasFreeThings());
                aheader->setFirstFreeSpan(list);
                list->start = list->end = 0;
            }
        }
    }

    




    void copyToArenas() {
        for (size_t i = 0; i != size_t(FINALIZE_LIMIT); ++i) {
            FreeSpan *list = &lists[i];
            if (!list->isEmpty()) {
                ArenaHeader *aheader = reinterpret_cast<Cell *>(list->start)->arenaHeader();
                JS_ASSERT(!aheader->hasFreeThings());
                aheader->setFirstFreeSpan(list);
            }
        }
    }

    



    void clearInArenas() {
        for (size_t i = 0; i != size_t(FINALIZE_LIMIT); ++i) {
            FreeSpan *list = &lists[i];
            if (!list->isEmpty()) {
                ArenaHeader *aheader = reinterpret_cast<Cell *>(list->start)->arenaHeader();
#ifdef DEBUG
                FreeSpan span(aheader->getFirstFreeSpan());
                JS_ASSERT(span.start == list->start);
                JS_ASSERT(span.end == list->end);
#endif
                aheader->setAsFullyUsed();
            }
        }
    }

    JS_ALWAYS_INLINE Cell *getNext(unsigned thingKind, size_t thingSize) {
        FreeSpan *list = &lists[thingKind];
        list->checkSpan();
        uintptr_t thing = list->start;
        if (thing != list->end) {
            





            list->start += thingSize;
            JS_ASSERT(list->start <= list->end);
        } else if (thing & ArenaMask) {
            




            *list = *list->nextSpan();
        } else {
            return NULL;
        }
        return reinterpret_cast<Cell *>(thing);
    }

    Cell *populate(ArenaHeader *aheader, unsigned thingKind, size_t thingSize) {
        lists[thingKind] = aheader->getFirstFreeSpan();
        aheader->setAsFullyUsed();
        Cell *t = getNext(thingKind, thingSize);
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

extern Cell *
RefillFinalizableFreeList(JSContext *cx, unsigned thingKind);

} 

typedef Vector<gc::Chunk *, 32, SystemAllocPolicy> GCChunks;

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

    



    GC_LAST_CONTEXT     = 1
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

    
    void startBackgroundSweep(JSRuntime *rt);

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
static const size_t LARGE_MARK_STACK_SIZE = 64 * sizeof(LargeMarkItem);

struct GCMarker : public JSTracer {
  private:
    
    uint32 color;

  public:
    
    js::gc::ArenaHeader *unmarkedArenaStackTop;
#ifdef DEBUG
    size_t              markLaterArenas;
#endif

#if defined(JS_DUMP_CONSERVATIVE_GC_ROOTS) || defined(JS_GCMETER)
    js::gc::ConservativeGCStats conservativeStats;
#endif

#ifdef JS_DUMP_CONSERVATIVE_GC_ROOTS
    Vector<void *, 0, SystemAllocPolicy> conservativeRoots;
    const char *conservativeDumpFileName;

    void dumpConservativeRoots();
#endif

    MarkStack<JSObject *> objStack;
    MarkStack<JSRope *> ropeStack;
    MarkStack<JSXML *> xmlStack;
    MarkStack<LargeMarkItem> largeStack;

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
               xmlStack.isEmpty() &&
               largeStack.isEmpty();
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

    void pushXML(JSXML *xml) {
        if (!xmlStack.push(xml))
            delayMarkingChildren(xml);
    }
};

JS_FRIEND_API(void)
MarkWeakReferences(GCMarker *trc);

void
MarkStackRangeConservatively(JSTracer *trc, Value *begin, Value *end);

static inline uint64
TraceKindMask(unsigned kind)
{
    return uint64(1) << kind;
}

static inline bool
TraceKindInMask(unsigned kind, uint64 mask)
{
    return !!(mask & TraceKindMask(kind));
}

typedef void (*IterateCallback)(JSContext *cx, void *data, size_t traceKind, void *obj);








extern JS_FRIEND_API(void)
IterateCells(JSContext *cx, JSCompartment *comp, uint64 traceKindMask,
             void *data, IterateCallback callback);

} 

extern void
js_FinalizeStringRT(JSRuntime *rt, JSString *str);





extern void
js_MarkTraps(JSTracer *trc);




#define IS_GC_MARKING_TRACER(trc) ((trc)->callback == NULL)

#if JS_HAS_XML_SUPPORT
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) < JSTRACE_LIMIT)
#else
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) <= JSTRACE_SHAPE)
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
