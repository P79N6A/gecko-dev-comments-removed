






































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
namespace gc {


template <typename T>
struct ArenaHeader {
    JSCompartment   *compartment;
    Arena<T>        *next;
    FreeCell        *freeList;
    unsigned        thingKind;
    bool            isUsed;
    size_t          thingSize;
#ifdef DEBUG
    bool            hasFreeThings;
#endif
};

template <typename T>
union ThingOrCell {
    T               t;
    FreeCell        cell;
};

template <typename T, size_t N, size_t R>
struct Things {
    ThingOrCell<T>  things[N];
    char            filler[R];
};

template <typename T, size_t N>
struct Things<T, N, 0> {
    ThingOrCell<T>  things[N];
};

template <typename T>
struct Arena {
    static const size_t ArenaSize = 4096;

    struct AlignedArenaHeader {
        T align[(sizeof(ArenaHeader<T>) + sizeof(T) - 1) / sizeof(T)];
    };

    
    union {
        ArenaHeader<T> aheader;
        AlignedArenaHeader align;
    };

    static const size_t ThingsPerArena = (ArenaSize - sizeof(AlignedArenaHeader)) / sizeof(T);
    static const size_t FillerSize = ArenaSize - sizeof(AlignedArenaHeader) - sizeof(T) * ThingsPerArena;
    Things<T, ThingsPerArena, FillerSize> t;

    inline Chunk *chunk() const;
    inline size_t arenaIndex() const;

    inline ArenaHeader<T> *header() { return &aheader; };

    inline MarkingDelay *getMarkingDelay() const;
    inline ArenaBitmap *bitmap() const;

    inline ConservativeGCTest mark(T *thing, JSTracer *trc);
    void markDelayedChildren(JSTracer *trc);
    inline bool inFreeList(void *thing) const;
    inline T *getAlignedThing(T *thing);
#ifdef DEBUG
    bool assureThingIsAligned(T *thing);
#endif

    void init(JSCompartment *compartment, unsigned thingKind);
};
JS_STATIC_ASSERT(sizeof(Arena<FreeCell>) == 4096);





static const uint32 BLACK = 0;


struct ArenaBitmap {
    static const size_t BitCount = Arena<FreeCell>::ArenaSize / Cell::CellSize;
    static const size_t BitWords = BitCount / JS_BITS_PER_WORD;

    uintptr_t bitmap[BitWords];

    JS_ALWAYS_INLINE bool isMarked(size_t bit, uint32 color) {
        bit += color;
        JS_ASSERT(bit < BitCount);
        uintptr_t *word = &bitmap[bit / JS_BITS_PER_WORD];
        return *word & (uintptr_t(1) << (bit % JS_BITS_PER_WORD));
    }

    JS_ALWAYS_INLINE bool markIfUnmarked(size_t bit, uint32 color) {
        JS_ASSERT(bit + color < BitCount);
        uintptr_t *word = &bitmap[bit / JS_BITS_PER_WORD];
        uintptr_t mask = (uintptr_t(1) << (bit % JS_BITS_PER_WORD));
        if (*word & mask)
            return false;
        *word |= mask;
        if (color != BLACK) {
            bit += color;
            word = &bitmap[bit / JS_BITS_PER_WORD];
            mask = (uintptr_t(1) << (bit % JS_BITS_PER_WORD));
            if (*word & mask)
                return false;
            *word |= mask;
        }
        return true;
    }
};


JS_STATIC_ASSERT(Arena<FreeCell>::ArenaSize % Cell::CellSize == 0);
JS_STATIC_ASSERT(ArenaBitmap::BitCount % JS_BITS_PER_WORD == 0);


struct MarkingDelay {
    Arena<Cell> *link;
    uintptr_t   unmarkedChildren;
    jsuword     start;

    void init()
    {
        link = NULL;
        unmarkedChildren = 0;
    }
};

struct EmptyArenaLists {
    Arena<FreeCell>      *cellFreeList;
    Arena<JSObject>      *objectFreeList;
    Arena<JSString>      *stringFreeList;
    Arena<JSShortString> *shortStringFreeList;
    Arena<JSFunction>    *functionFreeList;

    void init() {
        cellFreeList        = NULL;
        objectFreeList      = NULL;
        stringFreeList      = NULL;
        shortStringFreeList = NULL;
        functionFreeList    = NULL;
    }

    Arena<FreeCell> *getOtherArena() {
        Arena<FreeCell> *arena = NULL;
        if ((arena = (Arena<FreeCell> *)cellFreeList)) {
            cellFreeList = cellFreeList->header()->next;
            return arena;
        } else if ((arena = (Arena<FreeCell> *)objectFreeList)) {
            objectFreeList = objectFreeList->header()->next;
            return arena;
        } else if ((arena = (Arena<FreeCell> *)stringFreeList)) {
            stringFreeList = stringFreeList->header()->next;
            return arena;
        } else if ((arena = (Arena<FreeCell> *)shortStringFreeList)) {
            shortStringFreeList = shortStringFreeList->header()->next;
            return arena;
        } else {
            JS_ASSERT(functionFreeList);
            arena = (Arena<FreeCell> *)functionFreeList;
            functionFreeList = functionFreeList->header()->next;
            return arena;
        }
    }

    template <typename T>
    Arena<T> *getTypedFreeList();

    template <typename T>
    Arena<T> *getNext(JSCompartment *comp, unsigned thingKind);

    template <typename T>
    void insert(Arena<T> *arena);
};

template<typename T>
Arena<T> *EmptyArenaLists::getNext(JSCompartment *comp, unsigned thingKind) {
    Arena<T> *arena = getTypedFreeList<T>();
    if (arena) {
        JS_ASSERT(arena->header()->isUsed == false);
        JS_ASSERT(arena->header()->thingSize == sizeof(T));
        arena->header()->isUsed = true;
        arena->header()->thingKind = thingKind;
        arena->header()->compartment = comp;
        return arena;
    }
    arena = (Arena<T> *)getOtherArena();
    JS_ASSERT(arena->header()->isUsed == false);
    arena->init(comp, thingKind);
    return arena;
}


struct ChunkInfo {
    Chunk           *link;
    JSRuntime       *runtime;
    EmptyArenaLists emptyArenaLists;
    size_t          age;
    size_t          numFree;
};


struct Chunk {
    static const size_t BytesPerArena = sizeof(Arena<FreeCell>) +
                                        sizeof(ArenaBitmap) +
                                        sizeof(MarkingDelay);

    static const size_t ArenasPerChunk = (GC_CHUNK_SIZE - sizeof(ChunkInfo)) / BytesPerArena;
    static const size_t MaxAge = 3;

    Arena<FreeCell> arenas[ArenasPerChunk];
    ArenaBitmap     bitmaps[ArenasPerChunk];
    MarkingDelay    markingDelay[ArenasPerChunk];

    ChunkInfo       info;

    void clearMarkBitmap();
    void init(JSRuntime *rt);

    bool unused();
    bool hasAvailableArenas();
    bool withinArenasRange(Cell *cell);

    template <typename T>
    Arena<T> *allocateArena(JSCompartment *comp, unsigned thingKind);

    template <typename T>
    void releaseArena(Arena<T> *a);

    JSRuntime *getRuntime();
    bool expire();
};
JS_STATIC_ASSERT(sizeof(Chunk) <= GC_CHUNK_SIZE);
JS_STATIC_ASSERT(sizeof(Chunk) + Chunk::BytesPerArena > GC_CHUNK_SIZE);

Arena<Cell> *
Cell::arena() const
{
    uintptr_t addr = uintptr_t(this);
    JS_ASSERT(addr % sizeof(FreeCell) == 0);
    addr &= ~(Arena<FreeCell>::ArenaSize - 1);
    return reinterpret_cast<Arena<Cell> *>(addr);
}

Chunk *
Cell::chunk() const
{
    uintptr_t addr = uintptr_t(this);
    JS_ASSERT(addr % sizeof(FreeCell) == 0);
    addr &= ~(GC_CHUNK_SIZE - 1);
    return reinterpret_cast<Chunk *>(addr);
}

ArenaBitmap *
Cell::bitmap() const
{
    return &chunk()->bitmaps[arena()->arenaIndex()];
}

size_t
Cell::cellIndex() const
{
    return reinterpret_cast<const FreeCell *>(this) - reinterpret_cast<FreeCell *>(&arena()->t);
}

template <typename T>
Chunk *
Arena<T>::chunk() const
{
    uintptr_t addr = uintptr_t(this);
    JS_ASSERT(addr % sizeof(FreeCell) == 0);
    addr &= ~(GC_CHUNK_SIZE - 1);
    return reinterpret_cast<Chunk *>(addr);
}

template <typename T>
size_t
Arena<T>::arenaIndex() const
{
    return reinterpret_cast<const Arena<FreeCell> *>(this) - chunk()->arenas;
}

template <typename T>
MarkingDelay *
Arena<T>::getMarkingDelay() const
{
    return &chunk()->markingDelay[arenaIndex()];
}

template <typename T>
ArenaBitmap *
Arena<T>::bitmap() const
{
    return &chunk()->bitmaps[arenaIndex()];
}

static void
AssertValidColor(const void *thing, uint32 color)
{
    JS_ASSERT_IF(color, color < reinterpret_cast<const js::gc::FreeCell *>(thing)->arena()->header()->thingSize / sizeof(FreeCell));
}

inline bool
Cell::isMarked(uint32 color = BLACK) const
{
    AssertValidColor(this, color);
    return bitmap()->isMarked(cellIndex(), color);
}

bool
Cell::markIfUnmarked(uint32 color = BLACK) const
{
    AssertValidColor(this, color);
    return bitmap()->markIfUnmarked(cellIndex(), color);
}

JSCompartment *
Cell::compartment() const
{
    return arena()->header()->compartment;
}

template <typename T>
static inline
Arena<T> *
GetArena(Cell *cell)
{
    return reinterpret_cast<Arena<T> *>(cell->arena());
}





enum JSFinalizeGCThingKind {
    FINALIZE_OBJECT,
    FINALIZE_FUNCTION,
#if JS_HAS_XML_SUPPORT
    FINALIZE_XML,
#endif
    FINALIZE_SHORT_STRING,
    FINALIZE_STRING,
    FINALIZE_EXTERNAL_STRING0,
    FINALIZE_EXTERNAL_STRING1,
    FINALIZE_EXTERNAL_STRING2,
    FINALIZE_EXTERNAL_STRING3,
    FINALIZE_EXTERNAL_STRING4,
    FINALIZE_EXTERNAL_STRING5,
    FINALIZE_EXTERNAL_STRING6,
    FINALIZE_EXTERNAL_STRING7,
    FINALIZE_EXTERNAL_STRING_LAST = FINALIZE_EXTERNAL_STRING7,
    FINALIZE_LIMIT
};

#define JSTRACE_XML         2




#define JSTRACE_LIMIT       3




const size_t GC_ARENA_ALLOCATION_TRIGGER = 30 * js::GC_CHUNK_SIZE;






const float GC_HEAP_GROWTH_FACTOR = 3;

const uintN JS_EXTERNAL_STRING_LIMIT = 8;

static inline size_t
GetFinalizableTraceKind(size_t thingKind)
{
    JS_STATIC_ASSERT(JS_EXTERNAL_STRING_LIMIT == 8);

    static const uint8 map[FINALIZE_LIMIT] = {
        JSTRACE_OBJECT,     
        JSTRACE_OBJECT,     
#if JS_HAS_XML_SUPPORT      
        JSTRACE_XML,
#endif
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
    };

    JS_ASSERT(thingKind < FINALIZE_LIMIT);
    return map[thingKind];
}

static inline bool
IsFinalizableStringKind(unsigned thingKind)
{
    return unsigned(FINALIZE_SHORT_STRING) <= thingKind &&
           thingKind <= unsigned(FINALIZE_EXTERNAL_STRING_LAST);
}





static inline intN
GetExternalStringGCType(JSString *str)
{
    JS_STATIC_ASSERT(FINALIZE_STRING + 1 == FINALIZE_EXTERNAL_STRING0);
    JS_ASSERT(!JSString::isStatic(str));

    unsigned thingKind = GetArena<JSString>((Cell *)str)->header()->thingKind;
    JS_ASSERT(IsFinalizableStringKind(thingKind));
    return intN(thingKind) - intN(FINALIZE_EXTERNAL_STRING0);
}

static inline uint32
GetGCThingTraceKind(void *thing)
{
    JS_ASSERT(thing);
    if (JSString::isStatic(thing))
        return JSTRACE_STRING;
    Cell *cell = reinterpret_cast<Cell *>(thing);
    return GetFinalizableTraceKind(cell->arena()->header()->thingKind);
}

static inline JSRuntime *
GetGCThingRuntime(void *thing)
{
    return reinterpret_cast<FreeCell *>(thing)->chunk()->info.runtime;
}

#ifdef DEBUG
extern bool
checkArenaListsForThing(JSCompartment *comp, jsuword thing);
#endif

template <typename T>
struct ArenaList {
    Arena<T>       *head;          
    Arena<T>       *cursor;        

    inline void init() {
        head = NULL;
        cursor = NULL;
    }

    inline Arena<T> *getNextWithFreeList() {
        Arena<T> *a;
        while (cursor != NULL) {
            ArenaHeader<T> *aheader = cursor->header();
            a = cursor;
            cursor = (Arena<T> *)aheader->next;
            if (aheader->freeList)
                return a;
        }
        return NULL;
    }

#ifdef DEBUG
    bool arenasContainThing(void *thing) {
        for (Arena<T> *a = head; a; a = (Arena<T> *)a->header()->next) {
            JS_ASSERT(a->header()->isUsed);
            if (thing >= &a->t.things[0] && thing < &a->t.things[a->ThingsPerArena])
                return true;
        }
        return false;
    }
#endif

    inline void insert(Arena<T> *a) {
        a->header()->next = head;
        head = a;
    }

    void releaseAll() {
        while (head) {
            Arena<T> *next = head->header()->next;
            head->chunk()->releaseArena(head);
            head = next;
        }
        head = NULL;
        cursor = NULL;
    }

    inline bool isEmpty() const {
        return (head == NULL);
    }
};

struct FreeLists {
    FreeCell       **finalizables[FINALIZE_LIMIT];

    void purge();

    inline FreeCell *getNext(uint32 kind) {
        FreeCell *top = NULL;
        if (finalizables[kind]) {
            top = *finalizables[kind];
            if (top) {
                *finalizables[kind] = top->link;
            } else {
                finalizables[kind] = NULL;
            }
#ifdef DEBUG
            if (top && !top->link)
                top->arena()->header()->hasFreeThings = false;
#endif
        }
        return top;
    }

    template <typename T>
    inline void populate(Arena<T> *a, uint32 thingKind) {
        finalizables[thingKind] = &a->header()->freeList;
    }

#ifdef DEBUG
    bool isEmpty() const {
        for (size_t i = 0; i != JS_ARRAY_LENGTH(finalizables); ++i) {
            if (finalizables[i])
                return false;
        }
        return true;
    }
#endif
};
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

static inline void
CheckGCFreeListLink(js::gc::FreeCell *cell)
{
    



    JS_ASSERT_IF(cell->link,
                 cell->arena() ==
                 cell->link->arena());
    JS_ASSERT_IF(cell->link, cell < cell->link);
}

template <typename T>
extern bool
RefillFinalizableFreeList(JSContext *cx, unsigned thingKind);

#ifdef DEBUG
extern bool
CheckAllocation(JSContext *cx);
#endif





extern intN
js_GetExternalStringGCType(JSString *str);

extern JS_FRIEND_API(uint32)
js_GetGCThingTraceKind(void *thing);

#if 1





#define GC_POKE(cx, oldval) ((cx)->runtime->gcPoke = JS_TRUE)
#else
#define GC_POKE(cx, oldval) ((cx)->runtime->gcPoke = JSVAL_IS_GCTHING(oldval))
#endif

extern JSBool
js_InitGC(JSRuntime *rt, uint32 maxbytes);

extern void
js_FinishGC(JSRuntime *rt);

extern intN
js_ChangeExternalStringFinalizer(JSStringFinalizeOp oldop,
                                 JSStringFinalizeOp newop);

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
IsAboutToBeFinalized(void *thing);

extern JS_FRIEND_API(bool)
js_GCThingIsMarked(void *thing, uint32 color);

extern void
js_TraceStackFrame(JSTracer *trc, JSStackFrame *fp);

namespace js {

extern JS_REQUIRES_STACK void
MarkRuntime(JSTracer *trc);

extern void
TraceRuntime(JSTracer *trc);

extern JS_REQUIRES_STACK JS_FRIEND_API(void)
MarkContext(JSTracer *trc, JSContext *acx);


extern void
TriggerGC(JSRuntime *rt);

} 




typedef enum JSGCInvocationKind {
    
    GC_NORMAL           = 0,

    



    GC_LAST_CONTEXT     = 1,

    


    GC_LOCK_HELD        = 0x10
} JSGCInvocationKind;

extern void
js_GC(JSContext *cx, JSGCInvocationKind gckind);

#ifdef JS_THREADSAFE






extern void
js_WaitForGC(JSRuntime *rt);

#else 

# define js_WaitForGC(rt)    ((void) 0)

#endif

extern void
js_DestroyScriptsToGC(JSContext *cx, JSThreadData *data);

namespace js {

#ifdef JS_THREADSAFE











class GCHelperThread {
    static const size_t FREE_ARRAY_SIZE = size_t(1) << 16;
    static const size_t FREE_ARRAY_LENGTH = FREE_ARRAY_SIZE / sizeof(void *);

    PRThread*         thread;
    PRCondVar*        wakeup;
    PRCondVar*        sweepingDone;
    bool              shutdown;
    bool              sweeping;

    Vector<void **, 16, js::SystemAllocPolicy> freeVector;
    void            **freeCursor;
    void            **freeCursorEnd;

    JS_FRIEND_API(void)
    replenishAndFreeLater(void *ptr);

    static void freeElementsAndArray(void **array, void **end) {
        JS_ASSERT(array <= end);
        for (void **p = array; p != end; ++p)
            js_free(*p);
        js_free(array);
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
        sweeping(false),
        freeCursor(NULL),
        freeCursorEnd(NULL) { }
    
    bool init(JSRuntime *rt);
    void finish(JSRuntime *rt);
    
    
    void startBackgroundSweep(JSRuntime *rt);
    
    
    void waitBackgroundSweepEnd(JSRuntime *rt);
    
    void freeLater(void *ptr) {
        JS_ASSERT(!sweeping);
        if (freeCursor != freeCursorEnd)
            *freeCursor++ = ptr;
        else
            replenishAndFreeLater(ptr);
    }
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

struct GCMarker : public JSTracer {
  private:
    
    uint32 color;
  public:
    jsuword stackLimit;
    
    js::gc::Arena<js::gc::Cell> *unmarkedArenaStackTop;
#ifdef DEBUG
    size_t              markLaterCount;
#endif

#if defined(JS_DUMP_CONSERVATIVE_GC_ROOTS) || defined(JS_GCMETER)
    js::gc::ConservativeGCStats conservativeStats;
#endif

#ifdef JS_DUMP_CONSERVATIVE_GC_ROOTS
    struct ConservativeRoot { void *thing; uint32 traceKind; };
    Vector<ConservativeRoot, 0, SystemAllocPolicy> conservativeRoots;
    const char *conservativeDumpFileName;

    void dumpConservativeRoots();
#endif

    js::Vector<JSObject *, 0, js::SystemAllocPolicy> arraysToSlowify;

  public:
    explicit GCMarker(JSContext *cx);
    ~GCMarker();

    uint32 getMarkColor() const {
        return color;
    }

    void setMarkColor(uint32 newColor) {
        



        markDelayedChildren();
        color = newColor;
    }

    void delayMarkingChildren(void *thing);

    JS_FRIEND_API(void) markDelayedChildren();

    void slowifyArrays();
};

void
MarkStackRangeConservatively(JSTracer *trc, Value *begin, Value *end);

} 

extern void
js_FinalizeStringRT(JSRuntime *rt, JSString *str);





extern void
js_MarkTraps(JSTracer *trc);

namespace js {
namespace gc {





#define IS_GC_MARKING_TRACER(trc) ((trc)->callback == NULL)

#if JS_HAS_XML_SUPPORT
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) < JSTRACE_LIMIT)
#else
# define JS_IS_VALID_TRACE_KIND(kind) ((uint32)(kind) <= JSTRACE_STRING)
#endif








extern bool
SetProtoCheckingForCycles(JSContext *cx, JSObject *obj, JSObject *proto);

JSCompartment *
NewCompartment(JSContext *cx, JSPrincipals *principals);

} 
} 

#endif 
