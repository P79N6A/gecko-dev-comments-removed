





#ifndef jsgc_h___
#define jsgc_h___




#include <setjmp.h>

#include "mozilla/Util.h"

#include "jsalloc.h"
#include "jstypes.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jslock.h"
#include "jsutil.h"
#include "jsversion.h"

#include "ds/BitArray.h"
#include "gc/Heap.h"
#include "gc/Statistics.h"
#include "js/HashTable.h"
#include "js/Vector.h"
#include "js/TemplateLib.h"

struct JSCompartment;

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
    INVALID
};

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
};

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

    















    enum BackgroundFinalizeState {
        BFS_DONE,
        BFS_RUN,
        BFS_JUST_FINISHED
    };

    volatile uintptr_t backgroundFinalizeState[FINALIZE_LIMIT];

  public:
    ArenaLists() {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            freeLists[i].initAsEmpty();
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            backgroundFinalizeState[i] = BFS_DONE;
    }

    ~ArenaLists() {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i) {
            



            JS_ASSERT(backgroundFinalizeState[i] == BFS_DONE);
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
            



            if (backgroundFinalizeState[i] != BFS_DONE)
                return false;
            if (arenaLists[i].head)
                return false;
        }
        return true;
    }

    void unmarkAll() {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i) {
            
            JS_ASSERT(backgroundFinalizeState[i] == BFS_DONE ||
                      backgroundFinalizeState[i] == BFS_JUST_FINISHED);
            for (ArenaHeader *aheader = arenaLists[i].head; aheader; aheader = aheader->next) {
                uintptr_t *word = aheader->chunk()->bitmap.arenaBits(aheader);
                memset(word, 0, ArenaBitmapWords * sizeof(uintptr_t));
            }
        }
    }

    bool doneBackgroundFinalize(AllocKind kind) const {
        return backgroundFinalizeState[kind] == BFS_DONE;
    }

    



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

    void finalizeObjects(FreeOp *fop);
    void finalizeStrings(FreeOp *fop);
    void finalizeShapes(FreeOp *fop);
    void finalizeScripts(FreeOp *fop);
    void finalizeIonCode(FreeOp *fop);

    static void backgroundFinalize(FreeOp *fop, ArenaHeader *listHead);

  private:
    inline void finalizeNow(FreeOp *fop, AllocKind thingKind);
    inline void finalizeLater(FreeOp *fop, AllocKind thingKind);

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

extern JS_FRIEND_API(void)
PrepareForFullGC(JSRuntime *rt);




typedef enum JSGCInvocationKind {
    
    GC_NORMAL           = 0,

    
    GC_SHRINK             = 1
} JSGCInvocationKind;

extern void
GC(JSRuntime *rt, JSGCInvocationKind gckind, js::gcreason::Reason reason);

extern void
GCSlice(JSRuntime *rt, JSGCInvocationKind gckind, js::gcreason::Reason reason);

extern void
GCDebugSlice(JSRuntime *rt, bool limit, int64_t objCount);

extern void
PrepareForDebugGC(JSRuntime *rt);

} 

namespace js {

void
InitTracer(JSTracer *trc, JSRuntime *rt, JSTraceCallback callback);









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

    bool              sweepFlag;
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
        sweepFlag(false),
        shrinkFlag(false),
        freeCursor(NULL),
        freeCursorEnd(NULL),
        backgroundAllocation(true)
    { }

    bool init();
    void finish();

    
    void startBackgroundSweep(bool shouldShrink);

    
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

    void step(intptr_t amt = 1) {
        counter -= amt;
    }

    bool checkOverBudget();

    bool isOverBudget() {
        if (counter >= 0)
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
        ArenaTag,
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

    void pushArenaList(gc::ArenaHeader *firstArena) {
        pushTaggedPtr(ArenaTag, firstArena);
    }

    void pushType(types::TypeObject *type) {
        pushTaggedPtr(TypeTag, type);
    }

#if JS_HAS_XML_SUPPORT
    void pushXML(JSXML *xml) {
        pushTaggedPtr(XmlTag, xml);
    }

#endif

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
    void processMarkStackOther(SliceBudget &budget, uintptr_t tag, uintptr_t addr);

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

void
SetDeterministicGC(JSContext *cx, bool enabled);

const int ZealPokeValue = 1;
const int ZealAllocValue = 2;
const int ZealFrameGCValue = 3;
const int ZealVerifierValue = 4;
const int ZealFrameVerifierValue = 5;
const int ZealStackRootingSafeValue = 6;
const int ZealStackRootingValue = 7;
const int ZealIncrementalRootsThenFinish = 8;
const int ZealIncrementalMarkAllThenFinish = 9;
const int ZealIncrementalMultipleSlices = 10;

#ifdef JS_GC_ZEAL


void
VerifyBarriers(JSRuntime *rt);

void
MaybeVerifyBarriers(JSContext *cx, bool always = false);

#else

static inline void
VerifyBarriers(JSRuntime *rt)
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
ReleaseAllJITCode(FreeOp *fop, JSCompartment *c, bool resetUseCounts);

void
ReleaseAllJITCode(FreeOp *fop, bool resetUseCounts);

} 

#endif 
