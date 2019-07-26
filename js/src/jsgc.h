







#ifndef jsgc_h
#define jsgc_h

#include "mozilla/DebugOnly.h"
#include "mozilla/MemoryReporting.h"

#include "jslock.h"
#include "jsobj.h"

#include "js/GCAPI.h"
#include "js/SliceBudget.h"
#include "js/Vector.h"

class JSAtom;
struct JSCompartment;
class JSFlatString;
class JSLinearString;

namespace js {

class ArgumentsObject;
class ArrayBufferObject;
class ArrayBufferViewObject;
class SharedArrayBufferObject;
class BaseShape;
class DebugScopeObject;
class GCHelperThread;
class GlobalObject;
class LazyScript;
class Nursery;
class PropertyName;
class ScopeObject;
class Shape;
class UnownedBaseShape;

unsigned GetCPUCount();

enum HeapState {
    Idle,             
    Tracing,          
    MajorCollecting,  
    MinorCollecting   
};

struct ExtraTracer {
    JSTraceDataOp op;
    void *data;

    ExtraTracer()
      : op(nullptr), data(nullptr)
        {}
    ExtraTracer(JSTraceDataOp op, void *data)
      : op(op), data(data)
        {}
};

namespace jit {
    class JitCode;
}

namespace gc {

enum State {
    NO_INCREMENTAL,
    MARK_ROOTS,
    MARK,
    SWEEP,
    INVALID
};

class ChunkPool {
    Chunk   *emptyChunkListHead;
    size_t  emptyCount;

  public:
    ChunkPool()
      : emptyChunkListHead(nullptr),
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
        JSTRACE_BASE_SHAPE, 
        JSTRACE_TYPE_OBJECT,
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_JITCODE,    
    };
    JS_STATIC_ASSERT(JS_ARRAY_LENGTH(map) == FINALIZE_LIMIT);
    return map[kind];
}

template <typename T> struct MapTypeToTraceKind {};
template <> struct MapTypeToTraceKind<ObjectImpl>       { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<JSObject>         { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<JSFunction>       { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<ArgumentsObject>  { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<ArrayBufferObject>{ static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<ArrayBufferViewObject>{ static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<SharedArrayBufferObject>{ static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<DebugScopeObject> { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<GlobalObject>     { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<ScopeObject>      { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<JSScript>         { static const JSGCTraceKind kind = JSTRACE_SCRIPT; };
template <> struct MapTypeToTraceKind<LazyScript>       { static const JSGCTraceKind kind = JSTRACE_LAZY_SCRIPT; };
template <> struct MapTypeToTraceKind<Shape>            { static const JSGCTraceKind kind = JSTRACE_SHAPE; };
template <> struct MapTypeToTraceKind<BaseShape>        { static const JSGCTraceKind kind = JSTRACE_BASE_SHAPE; };
template <> struct MapTypeToTraceKind<UnownedBaseShape> { static const JSGCTraceKind kind = JSTRACE_BASE_SHAPE; };
template <> struct MapTypeToTraceKind<types::TypeObject>{ static const JSGCTraceKind kind = JSTRACE_TYPE_OBJECT; };
template <> struct MapTypeToTraceKind<JSAtom>           { static const JSGCTraceKind kind = JSTRACE_STRING; };
template <> struct MapTypeToTraceKind<JSString>         { static const JSGCTraceKind kind = JSTRACE_STRING; };
template <> struct MapTypeToTraceKind<JSFlatString>     { static const JSGCTraceKind kind = JSTRACE_STRING; };
template <> struct MapTypeToTraceKind<JSLinearString>   { static const JSGCTraceKind kind = JSTRACE_STRING; };
template <> struct MapTypeToTraceKind<PropertyName>     { static const JSGCTraceKind kind = JSTRACE_STRING; };
template <> struct MapTypeToTraceKind<jit::JitCode>     { static const JSGCTraceKind kind = JSTRACE_JITCODE; };


template <typename T> struct MapTypeToFinalizeKind {};
template <> struct MapTypeToFinalizeKind<JSScript>          { static const AllocKind kind = FINALIZE_SCRIPT; };
template <> struct MapTypeToFinalizeKind<LazyScript>        { static const AllocKind kind = FINALIZE_LAZY_SCRIPT; };
template <> struct MapTypeToFinalizeKind<Shape>             { static const AllocKind kind = FINALIZE_SHAPE; };
template <> struct MapTypeToFinalizeKind<BaseShape>         { static const AllocKind kind = FINALIZE_BASE_SHAPE; };
template <> struct MapTypeToFinalizeKind<types::TypeObject> { static const AllocKind kind = FINALIZE_TYPE_OBJECT; };
template <> struct MapTypeToFinalizeKind<JSFatInlineString> { static const AllocKind kind = FINALIZE_FAT_INLINE_STRING; };
template <> struct MapTypeToFinalizeKind<JSString>          { static const AllocKind kind = FINALIZE_STRING; };
template <> struct MapTypeToFinalizeKind<JSExternalString>  { static const AllocKind kind = FINALIZE_EXTERNAL_STRING; };
template <> struct MapTypeToFinalizeKind<jit::JitCode>      { static const AllocKind kind = FINALIZE_JITCODE; };

#if defined(JSGC_GENERATIONAL) || defined(DEBUG)
static inline bool
IsNurseryAllocable(AllocKind kind)
{
    JS_ASSERT(kind >= 0 && unsigned(kind) < FINALIZE_LIMIT);
    static const bool map[] = {
        false,     
        true,      
        false,     
        true,      
        false,     
        true,      
        false,     
        true,      
        false,     
        true,      
        false,     
        true,      
        false,     
        false,     
        false,     
        false,     
        false,     
        false,     
        false,     
        false,     
        false,     
    };
    JS_STATIC_ASSERT(JS_ARRAY_LENGTH(map) == FINALIZE_LIMIT);
    return map[kind];
}
#endif

static inline bool
IsBackgroundFinalized(AllocKind kind)
{
    JS_ASSERT(kind >= 0 && unsigned(kind) < FINALIZE_LIMIT);
    static const bool map[] = {
        false,     
        true,      
        false,     
        true,      
        false,     
        true,      
        false,     
        true,      
        false,     
        true,      
        false,     
        true,      
        false,     
        false,     
        true,      
        true,      
        true,      
        true,      
        true,      
        false,     
        false,     
    };
    JS_STATIC_ASSERT(JS_ARRAY_LENGTH(map) == FINALIZE_LIMIT);
    return map[kind];
}

static inline bool
CanBeFinalizedInBackground(gc::AllocKind kind, const Class *clasp)
{
    JS_ASSERT(kind <= gc::FINALIZE_OBJECT_LAST);
    






    return (!gc::IsBackgroundFinalized(kind) &&
            (!clasp->finalize || (clasp->flags & JSCLASS_BACKGROUND_FINALIZE)));
}

inline JSGCTraceKind
GetGCThingTraceKind(const void *thing);


const size_t SLOTS_TO_THING_KIND_LIMIT = 17;

extern const AllocKind slotsToThingKind[];


static inline AllocKind
GetGCObjectKind(size_t numSlots)
{
    if (numSlots >= SLOTS_TO_THING_KIND_LIMIT)
        return FINALIZE_OBJECT16;
    return slotsToThingKind[numSlots];
}


static inline AllocKind
GetGCArrayKind(size_t numSlots)
{
    





    JS_STATIC_ASSERT(ObjectElements::VALUES_PER_HEADER == 2);
    if (numSlots > JSObject::NELEMENTS_LIMIT || numSlots + 2 >= SLOTS_TO_THING_KIND_LIMIT)
        return FINALIZE_OBJECT2;
    return slotsToThingKind[numSlots + 2];
}

static inline AllocKind
GetGCObjectFixedSlotsKind(size_t numFixedSlots)
{
    JS_ASSERT(numFixedSlots < SLOTS_TO_THING_KIND_LIMIT);
    return slotsToThingKind[numFixedSlots];
}

static inline AllocKind
GetBackgroundAllocKind(AllocKind kind)
{
    JS_ASSERT(!IsBackgroundFinalized(kind));
    JS_ASSERT(kind <= FINALIZE_OBJECT_LAST);
    return (AllocKind) (kind + 1);
}





static inline bool
TryIncrementAllocKind(AllocKind *kindp)
{
    size_t next = size_t(*kindp) + 2;
    if (next >= size_t(FINALIZE_OBJECT_LIMIT))
        return false;
    *kindp = AllocKind(next);
    return true;
}


static inline size_t
GetGCKindSlots(AllocKind thingKind)
{
    
    switch (thingKind) {
      case FINALIZE_OBJECT0:
      case FINALIZE_OBJECT0_BACKGROUND:
        return 0;
      case FINALIZE_OBJECT2:
      case FINALIZE_OBJECT2_BACKGROUND:
        return 2;
      case FINALIZE_OBJECT4:
      case FINALIZE_OBJECT4_BACKGROUND:
        return 4;
      case FINALIZE_OBJECT8:
      case FINALIZE_OBJECT8_BACKGROUND:
        return 8;
      case FINALIZE_OBJECT12:
      case FINALIZE_OBJECT12_BACKGROUND:
        return 12;
      case FINALIZE_OBJECT16:
      case FINALIZE_OBJECT16_BACKGROUND:
        return 16;
      default:
        MOZ_ASSUME_UNREACHABLE("Bad object finalize kind");
    }
}

static inline size_t
GetGCKindSlots(AllocKind thingKind, const Class *clasp)
{
    size_t nslots = GetGCKindSlots(thingKind);

    
    if (clasp->flags & JSCLASS_HAS_PRIVATE) {
        JS_ASSERT(nslots > 0);
        nslots--;
    }

    



    if (clasp == FunctionClassPtr)
        nslots = 0;

    return nslots;
}

















class ArenaList {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    ArenaHeader     *head_;
    ArenaHeader     **cursorp_;

  public:
    ArenaList() {
        clear();
    }

    
    void check() const {
#ifdef DEBUG
        
        JS_ASSERT_IF(!head_, cursorp_ == &head_);

        
        ArenaHeader *cursor = *cursorp_;
        JS_ASSERT_IF(cursor, cursor->hasFreeThings());
#endif
    }

    
    void deepCheck() const {
#ifdef DEBUG
        check();
        
        
        
        
        
        
        








#endif
    }

    void clear() {
        head_ = nullptr;
        cursorp_ = &head_;
        check();
    }

    bool isEmpty() const {
        check();
        return !head_;
    }

    
    ArenaHeader *head() const {
        check();
        return head_;
    }

    bool isCursorAtEnd() const {
        check();
        return !*cursorp_;
    }

    
    ArenaHeader *arenaAfterCursor() const {
        check();
        return *cursorp_;
    }

    
    
    void moveCursorPast(ArenaHeader *aheader) {
        cursorp_ = &aheader->next;
        check();
    }

    
    
    
    
    
    void insertAtCursor(ArenaHeader *a) {
        check();
        a->next = *cursorp_;
        *cursorp_ = a;
        
        
        if (!a->hasFreeThings())
            cursorp_ = &a->next;
        check();
    }

    
    
    void insertAtStart(ArenaHeader *a) {
        check();
        a->next = head_;
        if (isEmpty())
            cursorp_ = &a->next;        
        head_ = a;
        check();
    }

    
    void appendToListWithCursorAtEnd(ArenaList &other) {
        JS_ASSERT(isCursorAtEnd());
        deepCheck();
        other.deepCheck();
        if (!other.isEmpty()) {
            
            
            *cursorp_ = other.head_;

            
            
            if (other.cursorp_ != &other.head_)
                cursorp_ = other.cursorp_;
        }
        deepCheck();
    }
};

class ArenaLists
{
    








    FreeList       freeLists[FINALIZE_LIMIT];

    ArenaList      arenaLists[FINALIZE_LIMIT];

    















    enum BackgroundFinalizeState {
        BFS_DONE,
        BFS_RUN,
        BFS_JUST_FINISHED
    };

    volatile uintptr_t backgroundFinalizeState[FINALIZE_LIMIT];

  public:
    
    ArenaHeader *arenaListsToSweep[FINALIZE_LIMIT];

    
    ArenaHeader *gcShapeArenasToSweep;

  public:
    ArenaLists() {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            freeLists[i].initAsEmpty();
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            backgroundFinalizeState[i] = BFS_DONE;
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            arenaListsToSweep[i] = nullptr;
        gcShapeArenasToSweep = nullptr;
    }

    ~ArenaLists() {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i) {
            



            JS_ASSERT(backgroundFinalizeState[i] == BFS_DONE);
            ArenaHeader *next;
            for (ArenaHeader *aheader = arenaLists[i].head(); aheader; aheader = next) {
                
                next = aheader->next;
                aheader->chunk()->releaseArena(aheader);
            }
        }
    }

    static uintptr_t getFreeListOffset(AllocKind thingKind) {
        uintptr_t offset = offsetof(ArenaLists, freeLists);
        return offset + thingKind * sizeof(FreeList);
    }

    const FreeList *getFreeList(AllocKind thingKind) const {
        return &freeLists[thingKind];
    }

    ArenaHeader *getFirstArena(AllocKind thingKind) const {
        return arenaLists[thingKind].head();
    }

    ArenaHeader *getFirstArenaToSweep(AllocKind thingKind) const {
        return arenaListsToSweep[thingKind];
    }

    bool arenaListsAreEmpty() const {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i) {
            



            if (backgroundFinalizeState[i] != BFS_DONE)
                return false;
            if (!arenaLists[i].isEmpty())
                return false;
        }
        return true;
    }

    void unmarkAll() {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i) {
            
            JS_ASSERT(backgroundFinalizeState[i] == BFS_DONE ||
                      backgroundFinalizeState[i] == BFS_JUST_FINISHED);
            for (ArenaHeader *aheader = arenaLists[i].head(); aheader; aheader = aheader->next) {
                uintptr_t *word = aheader->chunk()->bitmap.arenaBits(aheader);
                memset(word, 0, ArenaBitmapWords * sizeof(uintptr_t));
            }
        }
    }

    bool doneBackgroundFinalize(AllocKind kind) const {
        return backgroundFinalizeState[kind] == BFS_DONE ||
               backgroundFinalizeState[kind] == BFS_JUST_FINISHED;
    }

    bool needBackgroundFinalizeWait(AllocKind kind) const {
        return backgroundFinalizeState[kind] != BFS_DONE;
    }

    



    void purge() {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            purge(AllocKind(i));
    }

    void purge(AllocKind i) {
        FreeList *freeList = &freeLists[i];
        if (!freeList->isEmpty()) {
            ArenaHeader *aheader = freeList->arenaHeader();
            aheader->setFirstFreeSpan(freeList->getHead());
            freeList->initAsEmpty();
        }
    }

    inline void prepareForIncrementalGC(JSRuntime *rt);

    




    void copyFreeListsToArenas() {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            copyFreeListToArena(AllocKind(i));
    }

    void copyFreeListToArena(AllocKind thingKind) {
        FreeList *freeList = &freeLists[thingKind];
        if (!freeList->isEmpty()) {
            ArenaHeader *aheader = freeList->arenaHeader();
            JS_ASSERT(!aheader->hasFreeThings());
            aheader->setFirstFreeSpan(freeList->getHead());
        }
    }

    



    void clearFreeListsInArenas() {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            clearFreeListInArena(AllocKind(i));
    }


    void clearFreeListInArena(AllocKind kind) {
        FreeList *freeList = &freeLists[kind];
        if (!freeList->isEmpty()) {
            ArenaHeader *aheader = freeList->arenaHeader();
            JS_ASSERT(freeList->isSameNonEmptySpan(aheader->getFirstFreeSpan()));
            aheader->setAsFullyUsed();
        }
    }

    



    bool isSynchronizedFreeList(AllocKind kind) {
        FreeList *freeList = &freeLists[kind];
        if (freeList->isEmpty())
            return true;
        ArenaHeader *aheader = freeList->arenaHeader();
        if (aheader->hasFreeThings()) {
            



            JS_ASSERT(freeList->isSameNonEmptySpan(aheader->getFirstFreeSpan()));
            return true;
        }
        return false;
    }

    MOZ_ALWAYS_INLINE void *allocateFromFreeList(AllocKind thingKind, size_t thingSize) {
        return freeLists[thingKind].allocate(thingSize);
    }

    template <AllowGC allowGC>
    static void *refillFreeList(ThreadSafeContext *cx, AllocKind thingKind);

    






    void adoptArenas(JSRuntime *runtime, ArenaLists *fromArenaLists);

    
    bool containsArena(JSRuntime *runtime, ArenaHeader *arenaHeader);

    void checkEmptyFreeLists() {
#ifdef DEBUG
        for (size_t i = 0; i < mozilla::ArrayLength(freeLists); ++i)
            JS_ASSERT(freeLists[i].isEmpty());
#endif
    }

    void checkEmptyFreeList(AllocKind kind) {
        JS_ASSERT(freeLists[kind].isEmpty());
    }

    void queueObjectsForSweep(FreeOp *fop);
    void queueStringsForSweep(FreeOp *fop);
    void queueShapesForSweep(FreeOp *fop);
    void queueScriptsForSweep(FreeOp *fop);
    void queueJitCodeForSweep(FreeOp *fop);

    bool foregroundFinalize(FreeOp *fop, AllocKind thingKind, SliceBudget &sliceBudget);
    static void backgroundFinalize(FreeOp *fop, ArenaHeader *listHead, bool onBackgroundThread);

    void wipeDuringParallelExecution(JSRuntime *rt);

  private:
    inline void finalizeNow(FreeOp *fop, AllocKind thingKind);
    inline void forceFinalizeNow(FreeOp *fop, AllocKind thingKind);
    inline void queueForForegroundSweep(FreeOp *fop, AllocKind thingKind);
    inline void queueForBackgroundSweep(FreeOp *fop, AllocKind thingKind);

    void *allocateFromArena(JS::Zone *zone, AllocKind thingKind);
    inline void *allocateFromArenaInline(JS::Zone *zone, AllocKind thingKind);

    inline void normalizeBackgroundFinalizeState(AllocKind thingKind);

    friend class js::Nursery;
};






const size_t INITIAL_CHUNK_CAPACITY = 16 * 1024 * 1024 / ChunkSize;


const size_t MAX_EMPTY_CHUNK_AGE = 4;

} 

typedef enum JSGCRootType {
    JS_GC_ROOT_VALUE_PTR,
    JS_GC_ROOT_STRING_PTR,
    JS_GC_ROOT_OBJECT_PTR,
    JS_GC_ROOT_SCRIPT_PTR
} JSGCRootType;

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

extern bool
AddValueRoot(JSContext *cx, js::Value *vp, const char *name);

extern bool
AddValueRootRT(JSRuntime *rt, js::Value *vp, const char *name);

extern bool
AddStringRoot(JSContext *cx, JSString **rp, const char *name);

extern bool
AddObjectRoot(JSContext *cx, JSObject **rp, const char *name);

extern bool
AddObjectRoot(JSRuntime *rt, JSObject **rp, const char *name);

extern bool
AddScriptRoot(JSContext *cx, JSScript **rp, const char *name);

extern void
RemoveRoot(JSRuntime *rt, void *rp);

} 

extern bool
js_InitGC(JSRuntime *rt, uint32_t maxbytes);

extern void
js_FinishGC(JSRuntime *rt);

namespace js {

class InterpreterFrame;

extern void
MarkCompartmentActive(js::InterpreterFrame *fp);

extern void
TraceRuntime(JSTracer *trc);


extern bool
TriggerGC(JSRuntime *rt, JS::gcreason::Reason reason);


extern bool
TriggerZoneGC(Zone *zone, JS::gcreason::Reason reason);

extern void
MaybeGC(JSContext *cx);

extern void
ReleaseAllJITCode(FreeOp *op);




typedef enum JSGCInvocationKind {
    
    GC_NORMAL           = 0,

    
    GC_SHRINK             = 1
} JSGCInvocationKind;

extern void
GC(JSRuntime *rt, JSGCInvocationKind gckind, JS::gcreason::Reason reason);

extern void
GCSlice(JSRuntime *rt, JSGCInvocationKind gckind, JS::gcreason::Reason reason, int64_t millis = 0);

extern void
GCFinalSlice(JSRuntime *rt, JSGCInvocationKind gckind, JS::gcreason::Reason reason);

extern void
GCDebugSlice(JSRuntime *rt, bool limit, int64_t objCount);

extern void
PrepareForDebugGC(JSRuntime *rt);

extern void
MinorGC(JSRuntime *rt, JS::gcreason::Reason reason);

extern void
MinorGC(JSContext *cx, JS::gcreason::Reason reason);

#ifdef JS_GC_ZEAL
extern void
SetGCZeal(JSRuntime *rt, uint8_t zeal, uint32_t frequency);
#endif



extern void
DelayCrossCompartmentGrayMarking(JSObject *src);

extern void
NotifyGCNukeWrapper(JSObject *o);

extern unsigned
NotifyGCPreSwap(JSObject *a, JSObject *b);

extern void
NotifyGCPostSwap(JSObject *a, JSObject *b, unsigned preResult);









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

    void wait(PRCondVar *which);

    bool              sweepFlag;
    bool              shrinkFlag;

    Vector<void **, 16, js::SystemAllocPolicy> freeVector;
    void            **freeCursor;
    void            **freeCursorEnd;

    bool              backgroundAllocation;

    friend class js::gc::ArenaLists;

    void
    replenishAndFreeLater(void *ptr);

    static void freeElementsAndArray(void **array, void **end) {
        JS_ASSERT(array <= end);
        for (void **p = array; p != end; ++p)
            js_free(*p);
        js_free(array);
    }

    static void threadMain(void* arg);
    void threadLoop();

    
    void doSweep();

  public:
    GCHelperThread(JSRuntime *rt)
      : rt(rt),
        thread(nullptr),
        wakeup(nullptr),
        done(nullptr),
        state(IDLE),
        sweepFlag(false),
        shrinkFlag(false),
        freeCursor(nullptr),
        freeCursorEnd(nullptr),
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

    bool onBackgroundThread();

    



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

void
MarkStackRangeConservatively(JSTracer *trc, Value *begin, Value *end);

typedef void (*IterateChunkCallback)(JSRuntime *rt, void *data, gc::Chunk *chunk);
typedef void (*IterateZoneCallback)(JSRuntime *rt, void *data, JS::Zone *zone);
typedef void (*IterateArenaCallback)(JSRuntime *rt, void *data, gc::Arena *arena,
                                     JSGCTraceKind traceKind, size_t thingSize);
typedef void (*IterateCellCallback)(JSRuntime *rt, void *data, void *thing,
                                    JSGCTraceKind traceKind, size_t thingSize);






extern void
IterateZonesCompartmentsArenasCells(JSRuntime *rt, void *data,
                                    IterateZoneCallback zoneCallback,
                                    JSIterateCompartmentCallback compartmentCallback,
                                    IterateArenaCallback arenaCallback,
                                    IterateCellCallback cellCallback);





extern void
IterateZoneCompartmentsArenasCells(JSRuntime *rt, Zone *zone, void *data,
                                   IterateZoneCallback zoneCallback,
                                   JSIterateCompartmentCallback compartmentCallback,
                                   IterateArenaCallback arenaCallback,
                                   IterateCellCallback cellCallback);




extern void
IterateChunks(JSRuntime *rt, void *data, IterateChunkCallback chunkCallback);

typedef void (*IterateScriptCallback)(JSRuntime *rt, void *data, JSScript *script);





extern void
IterateScripts(JSRuntime *rt, JSCompartment *compartment,
               void *data, IterateScriptCallback scriptCallback);

} 

extern void
js_FinalizeStringRT(JSRuntime *rt, JSString *str);

namespace js {

JSCompartment *
NewCompartment(JSContext *cx, JS::Zone *zone, JSPrincipals *principals,
               const JS::CompartmentOptions &options);

namespace gc {

extern void
GCIfNeeded(JSContext *cx);


void
RunDebugGC(JSContext *cx);

void
SetDeterministicGC(JSContext *cx, bool enabled);

void
SetValidateGC(JSContext *cx, bool enabled);

void
SetFullCompartmentChecks(JSContext *cx, bool enabled);


void
FinishBackgroundFinalize(JSRuntime *rt);





void
MergeCompartments(JSCompartment *source, JSCompartment *target);

const int ZealPokeValue = 1;
const int ZealAllocValue = 2;
const int ZealFrameGCValue = 3;
const int ZealVerifierPreValue = 4;
const int ZealFrameVerifierPreValue = 5;
const int ZealStackRootingValue = 6;
const int ZealGenerationalGCValue = 7;
const int ZealIncrementalRootsThenFinish = 8;
const int ZealIncrementalMarkAllThenFinish = 9;
const int ZealIncrementalMultipleSlices = 10;
const int ZealVerifierPostValue = 11;
const int ZealFrameVerifierPostValue = 12;
const int ZealCheckHashTablesOnMinorGC = 13;
const int ZealLimit = 13;

enum VerifierType {
    PreBarrierVerifier,
    PostBarrierVerifier
};

#ifdef JS_GC_ZEAL


void
VerifyBarriers(JSRuntime *rt, VerifierType type);

void
MaybeVerifyBarriers(JSContext *cx, bool always = false);

#else

static inline void
VerifyBarriers(JSRuntime *rt, VerifierType type)
{
}

static inline void
MaybeVerifyBarriers(JSContext *cx, bool always = false)
{
}

#endif







class AutoSuppressGC
{
    int32_t &suppressGC_;

  public:
    AutoSuppressGC(ExclusiveContext *cx);
    AutoSuppressGC(JSCompartment *comp);
    AutoSuppressGC(JSRuntime *rt);

    ~AutoSuppressGC()
    {
        suppressGC_--;
    }
};

#ifdef DEBUG

class AutoEnterOOMUnsafeRegion
{
    uint32_t saved_;

  public:
    AutoEnterOOMUnsafeRegion() : saved_(OOM_maxAllocations) {
        OOM_maxAllocations = UINT32_MAX;
    }
    ~AutoEnterOOMUnsafeRegion() {
        OOM_maxAllocations = saved_;
    }
};
#else
class AutoEnterOOMUnsafeRegion {};
#endif 

} 

#ifdef DEBUG

class AutoDisableProxyCheck
{
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER;
    uintptr_t &count;

  public:
    AutoDisableProxyCheck(JSRuntime *rt
                          MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    ~AutoDisableProxyCheck() {
        count--;
    }
};
#else
struct AutoDisableProxyCheck
{
    AutoDisableProxyCheck(JSRuntime *rt) {}
};
#endif

void
PurgeJITCaches(JS::Zone *zone);


bool
UninlinedIsInsideNursery(JSRuntime *rt, const void *thing);

} 

#endif 
