







#ifndef jsgc_h
#define jsgc_h

#include "mozilla/Atomics.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/TypeTraits.h"

#include "jslock.h"

#include "js/GCAPI.h"
#include "js/SliceBudget.h"
#include "js/Vector.h"

#include "vm/NativeObject.h"

namespace js {

class AutoLockGC;

namespace gc {
class ForkJoinNursery;
}

unsigned GetCPUCount();

enum HeapState {
    Idle,             
    Tracing,          
    MajorCollecting,  
    MinorCollecting   
};

namespace jit {
    class JitCode;
}

namespace gc {

struct FinalizePhase;

enum State {
    NO_INCREMENTAL,
    MARK_ROOTS,
    MARK,
    SWEEP,
#ifdef JSGC_COMPACTING
    COMPACT
#endif
};


const char *
TraceKindAsAscii(JSGCTraceKind kind);


template <typename T> struct MapTypeToFinalizeKind {};
template <> struct MapTypeToFinalizeKind<JSScript>          { static const AllocKind kind = FINALIZE_SCRIPT; };
template <> struct MapTypeToFinalizeKind<LazyScript>        { static const AllocKind kind = FINALIZE_LAZY_SCRIPT; };
template <> struct MapTypeToFinalizeKind<Shape>             { static const AllocKind kind = FINALIZE_SHAPE; };
template <> struct MapTypeToFinalizeKind<AccessorShape>     { static const AllocKind kind = FINALIZE_ACCESSOR_SHAPE; };
template <> struct MapTypeToFinalizeKind<BaseShape>         { static const AllocKind kind = FINALIZE_BASE_SHAPE; };
template <> struct MapTypeToFinalizeKind<types::TypeObject> { static const AllocKind kind = FINALIZE_TYPE_OBJECT; };
template <> struct MapTypeToFinalizeKind<JSFatInlineString> { static const AllocKind kind = FINALIZE_FAT_INLINE_STRING; };
template <> struct MapTypeToFinalizeKind<JSString>          { static const AllocKind kind = FINALIZE_STRING; };
template <> struct MapTypeToFinalizeKind<JSExternalString>  { static const AllocKind kind = FINALIZE_EXTERNAL_STRING; };
template <> struct MapTypeToFinalizeKind<JS::Symbol>        { static const AllocKind kind = FINALIZE_SYMBOL; };
template <> struct MapTypeToFinalizeKind<jit::JitCode>      { static const AllocKind kind = FINALIZE_JITCODE; };

#if defined(JSGC_GENERATIONAL) || defined(DEBUG)
static inline bool
IsNurseryAllocable(AllocKind kind)
{
    MOZ_ASSERT(kind >= 0 && unsigned(kind) < FINALIZE_LIMIT);
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
        false,     
        false,     
    };
    JS_STATIC_ASSERT(JS_ARRAY_LENGTH(map) == FINALIZE_LIMIT);
    return map[kind];
}
#endif

#if defined(JSGC_FJGENERATIONAL)



static inline bool
IsFJNurseryAllocable(AllocKind kind)
{
    MOZ_ASSERT(kind >= 0 && unsigned(kind) < FINALIZE_LIMIT);
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
    MOZ_ASSERT(kind >= 0 && unsigned(kind) < FINALIZE_LIMIT);
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
        true,      
        false,     
        true,      
        false,     
    };
    JS_STATIC_ASSERT(JS_ARRAY_LENGTH(map) == FINALIZE_LIMIT);
    return map[kind];
}

static inline bool
CanBeFinalizedInBackground(gc::AllocKind kind, const Class *clasp)
{
    MOZ_ASSERT(kind <= gc::FINALIZE_OBJECT_LAST);
    






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
    if (numSlots > NativeObject::NELEMENTS_LIMIT || numSlots + 2 >= SLOTS_TO_THING_KIND_LIMIT)
        return FINALIZE_OBJECT2;
    return slotsToThingKind[numSlots + 2];
}

static inline AllocKind
GetGCObjectFixedSlotsKind(size_t numFixedSlots)
{
    MOZ_ASSERT(numFixedSlots < SLOTS_TO_THING_KIND_LIMIT);
    return slotsToThingKind[numFixedSlots];
}

static inline AllocKind
GetBackgroundAllocKind(AllocKind kind)
{
    MOZ_ASSERT(!IsBackgroundFinalized(kind));
    MOZ_ASSERT(kind <= FINALIZE_OBJECT_LAST);
    return (AllocKind) (kind + 1);
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
        MOZ_CRASH("Bad object finalize kind");
    }
}

static inline size_t
GetGCKindSlots(AllocKind thingKind, const Class *clasp)
{
    size_t nslots = GetGCKindSlots(thingKind);

    
    if (clasp->flags & JSCLASS_HAS_PRIVATE) {
        MOZ_ASSERT(nslots > 0);
        nslots--;
    }

    



    if (clasp == FunctionClassPtr)
        nslots = 0;

    return nslots;
}





class AutoMaybeStartBackgroundAllocation;





struct SortedArenaListSegment
{
    ArenaHeader *head;
    ArenaHeader **tailp;

    void clear() {
        head = nullptr;
        tailp = &head;
    }

    bool isEmpty() const {
        return tailp == &head;
    }

    
    void append(ArenaHeader *aheader) {
        MOZ_ASSERT(aheader);
        MOZ_ASSERT_IF(head, head->getAllocKind() == aheader->getAllocKind());
        *tailp = aheader;
        tailp = &aheader->next;
    }

    
    
    
    
    
    void linkTo(ArenaHeader *aheader) {
        *tailp = aheader;
    }
};

















class ArenaList {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    ArenaHeader     *head_;
    ArenaHeader     **cursorp_;

    void copy(const ArenaList &other) {
        other.check();
        head_ = other.head_;
        cursorp_ = other.isCursorAtHead() ? &head_ : other.cursorp_;
        check();
    }

  public:
    ArenaList() {
        clear();
    }

    ArenaList(const ArenaList &other) {
        copy(other);
    }

    ArenaList &operator=(const ArenaList &other) {
        copy(other);
        return *this;
    }

    explicit ArenaList(const SortedArenaListSegment &segment) {
        head_ = segment.head;
        cursorp_ = segment.isEmpty() ? &head_ : segment.tailp;
        check();
    }

    
    void check() const {
#ifdef DEBUG
        
        MOZ_ASSERT_IF(!head_, cursorp_ == &head_);

        
        ArenaHeader *cursor = *cursorp_;
        MOZ_ASSERT_IF(cursor, cursor->hasFreeThings());
#endif
    }

    void clear() {
        head_ = nullptr;
        cursorp_ = &head_;
        check();
    }

    ArenaList copyAndClear() {
        ArenaList result = *this;
        clear();
        return result;
    }

    bool isEmpty() const {
        check();
        return !head_;
    }

    
    ArenaHeader *head() const {
        check();
        return head_;
    }

    bool isCursorAtHead() const {
        check();
        return cursorp_ == &head_;
    }

    bool isCursorAtEnd() const {
        check();
        return !*cursorp_;
    }

    
    ArenaHeader *arenaAfterCursor() const {
        check();
        return *cursorp_;
    }

    
    ArenaHeader *takeNextArena() {
        check();
        ArenaHeader *aheader = *cursorp_;
        if (!aheader)
            return nullptr;
        cursorp_ = &aheader->next;
        check();
        return aheader;
    }

    
    
    
    
    
    void insertAtCursor(ArenaHeader *a) {
        check();
        a->next = *cursorp_;
        *cursorp_ = a;
        
        
        if (!a->hasFreeThings())
            cursorp_ = &a->next;
        check();
    }

    
    ArenaList &insertListWithCursorAtEnd(const ArenaList &other) {
        check();
        other.check();
        MOZ_ASSERT(other.isCursorAtEnd());
        if (other.isCursorAtHead())
            return *this;
        
        *other.cursorp_ = *cursorp_;
        *cursorp_ = other.head_;
        cursorp_ = other.cursorp_;
        check();
        return *this;
    }

#ifdef JSGC_COMPACTING
    size_t countUsedCells();
    ArenaHeader *removeRemainingArenas(ArenaHeader **arenap, const AutoLockGC &lock);
    ArenaHeader *pickArenasToRelocate(JSRuntime *runtime);
    ArenaHeader *relocateArenas(ArenaHeader *toRelocate, ArenaHeader *relocated);
#endif
};






class SortedArenaList
{
  public:
    
    static const size_t MinThingSize = 16;

    static_assert(ArenaSize <= 4096, "When increasing the Arena size, please consider how"\
                                     " this will affect the size of a SortedArenaList.");

    static_assert(MinThingSize >= 16, "When decreasing the minimum thing size, please consider"\
                                      " how this will affect the size of a SortedArenaList.");

  private:
    
    static const size_t MaxThingsPerArena = (ArenaSize - sizeof(ArenaHeader)) / MinThingSize;

    size_t thingsPerArena_;
    SortedArenaListSegment segments[MaxThingsPerArena + 1];

    
    ArenaHeader *headAt(size_t n) { return segments[n].head; }
    ArenaHeader **tailAt(size_t n) { return segments[n].tailp; }

  public:
    explicit SortedArenaList(size_t thingsPerArena = MaxThingsPerArena) {
        reset(thingsPerArena);
    }

    void setThingsPerArena(size_t thingsPerArena) {
        MOZ_ASSERT(thingsPerArena && thingsPerArena <= MaxThingsPerArena);
        thingsPerArena_ = thingsPerArena;
    }

    
    void reset(size_t thingsPerArena = MaxThingsPerArena) {
        setThingsPerArena(thingsPerArena);
        
        for (size_t i = 0; i <= thingsPerArena; ++i)
            segments[i].clear();
    }

    
    void insertAt(ArenaHeader *aheader, size_t nfree) {
        MOZ_ASSERT(nfree <= thingsPerArena_);
        segments[nfree].append(aheader);
    }

    
    void extractEmpty(ArenaHeader **empty) {
        SortedArenaListSegment &segment = segments[thingsPerArena_];
        if (segment.head) {
            *segment.tailp = *empty;
            *empty = segment.head;
            segment.clear();
        }
    }

    
    
    
    
    
    
    
    ArenaList toArenaList() {
        
        size_t tailIndex = 0;
        for (size_t headIndex = 1; headIndex <= thingsPerArena_; ++headIndex) {
            if (headAt(headIndex)) {
                segments[tailIndex].linkTo(headAt(headIndex));
                tailIndex = headIndex;
            }
        }
        
        
        segments[tailIndex].linkTo(nullptr);
        
        
        return ArenaList(segments[0]);
    }
};

class ArenaLists
{
    JSRuntime *runtime_;

    








    FreeList       freeLists[FINALIZE_LIMIT];

    ArenaList      arenaLists[FINALIZE_LIMIT];

    enum BackgroundFinalizeStateEnum { BFS_DONE, BFS_RUN };

    typedef mozilla::Atomic<BackgroundFinalizeStateEnum, mozilla::ReleaseAcquire>
        BackgroundFinalizeState;

    
    BackgroundFinalizeState backgroundFinalizeState[FINALIZE_LIMIT];

  public:
    
    ArenaHeader *arenaListsToSweep[FINALIZE_LIMIT];

    
    unsigned incrementalSweptArenaKind;
    ArenaList incrementalSweptArenas;

    
    
    ArenaHeader *gcShapeArenasToUpdate;
    ArenaHeader *gcAccessorShapeArenasToUpdate;
    ArenaHeader *gcScriptArenasToUpdate;
    ArenaHeader *gcTypeObjectArenasToUpdate;

    
    
    
    
    ArenaList savedObjectArenas[FINALIZE_OBJECT_LIMIT];
    ArenaHeader *savedEmptyObjectArenas;

  public:
    ArenaLists(JSRuntime *rt) : runtime_(rt) {
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            freeLists[i].initAsEmpty();
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            backgroundFinalizeState[i] = BFS_DONE;
        for (size_t i = 0; i != FINALIZE_LIMIT; ++i)
            arenaListsToSweep[i] = nullptr;
        incrementalSweptArenaKind = FINALIZE_LIMIT;
        gcShapeArenasToUpdate = nullptr;
        gcAccessorShapeArenasToUpdate = nullptr;
        gcScriptArenasToUpdate = nullptr;
        gcTypeObjectArenasToUpdate = nullptr;
        savedEmptyObjectArenas = nullptr;
    }

    ~ArenaLists();

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

    ArenaHeader *getFirstSweptArena(AllocKind thingKind) const {
        if (thingKind != incrementalSweptArenaKind)
            return nullptr;
        return incrementalSweptArenas.head();
    }

    ArenaHeader *getArenaAfterCursor(AllocKind thingKind) const {
        return arenaLists[thingKind].arenaAfterCursor();
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
            
            MOZ_ASSERT(backgroundFinalizeState[i] == BFS_DONE);
            for (ArenaHeader *aheader = arenaLists[i].head(); aheader; aheader = aheader->next)
                aheader->unmarkAll();
        }
    }

    bool doneBackgroundFinalize(AllocKind kind) const {
        return backgroundFinalizeState[kind] == BFS_DONE;
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
            MOZ_ASSERT(!aheader->hasFreeThings());
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
            MOZ_ASSERT(freeList->isSameNonEmptySpan(aheader->getFirstFreeSpan()));
            aheader->setAsFullyUsed();
        }
    }

    



    bool isSynchronizedFreeList(AllocKind kind) {
        FreeList *freeList = &freeLists[kind];
        if (freeList->isEmpty())
            return true;
        ArenaHeader *aheader = freeList->arenaHeader();
        if (aheader->hasFreeThings()) {
            



            MOZ_ASSERT(freeList->isSameNonEmptySpan(aheader->getFirstFreeSpan()));
            return true;
        }
        return false;
    }

    
    bool arenaIsInUse(ArenaHeader *aheader, AllocKind kind) const {
        MOZ_ASSERT(aheader);
        const FreeList &freeList = freeLists[kind];
        if (freeList.isEmpty())
            return false;
        return aheader == freeList.arenaHeader();
    }

    MOZ_ALWAYS_INLINE TenuredCell *allocateFromFreeList(AllocKind thingKind, size_t thingSize) {
        return freeLists[thingKind].allocate(thingSize);
    }

    
    
    
    TenuredCell *allocateFromArena(JS::Zone *zone, AllocKind thingKind);

    






    void adoptArenas(JSRuntime *runtime, ArenaLists *fromArenaLists);

    
    bool containsArena(JSRuntime *runtime, ArenaHeader *arenaHeader);

    void checkEmptyFreeLists() {
#ifdef DEBUG
        for (size_t i = 0; i < mozilla::ArrayLength(freeLists); ++i)
            MOZ_ASSERT(freeLists[i].isEmpty());
#endif
    }

    void checkEmptyFreeList(AllocKind kind) {
        MOZ_ASSERT(freeLists[kind].isEmpty());
    }

#ifdef JSGC_COMPACTING
    ArenaHeader *relocateArenas(ArenaHeader *relocatedList);
#endif

    void queueForegroundObjectsForSweep(FreeOp *fop);
    void queueForegroundThingsForSweep(FreeOp *fop);

    void mergeForegroundSweptObjectArenas();

    bool foregroundFinalize(FreeOp *fop, AllocKind thingKind, SliceBudget &sliceBudget,
                            SortedArenaList &sweepList);
    static void backgroundFinalize(FreeOp *fop, ArenaHeader *listHead);

    void wipeDuringParallelExecution(JSRuntime *rt);

    
    
    enum KeepArenasEnum {
        RELEASE_ARENAS,
        KEEP_ARENAS
    };

  private:
    inline void finalizeNow(FreeOp *fop, const FinalizePhase& phase);
    inline void queueForForegroundSweep(FreeOp *fop, const FinalizePhase& phase);
    inline void queueForBackgroundSweep(FreeOp *fop, const FinalizePhase& phase);

    inline void finalizeNow(FreeOp *fop, AllocKind thingKind,
                            KeepArenasEnum keepArenas, ArenaHeader **empty = nullptr);
    inline void forceFinalizeNow(FreeOp *fop, AllocKind thingKind,
                                 KeepArenasEnum keepArenas, ArenaHeader **empty = nullptr);
    inline void queueForForegroundSweep(FreeOp *fop, AllocKind thingKind);
    inline void queueForBackgroundSweep(FreeOp *fop, AllocKind thingKind);
    inline void mergeSweptArenas(AllocKind thingKind);

    TenuredCell *allocateFromArena(JS::Zone *zone, AllocKind thingKind,
                                   AutoMaybeStartBackgroundAllocation &maybeStartBGAlloc);

    enum ArenaAllocMode { HasFreeThings = true, IsEmpty = false };
    template <ArenaAllocMode hasFreeThings>
    inline TenuredCell *allocateFromArenaInner(JS::Zone *zone, ArenaHeader *aheader,
                                               AllocKind thingKind);

    inline void normalizeBackgroundFinalizeState(AllocKind thingKind);

    friend class GCRuntime;
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

extern void
ReleaseAllJITCode(FreeOp *op);




typedef enum JSGCInvocationKind {
    
    GC_NORMAL           = 0,

    
    GC_SHRINK             = 1
} JSGCInvocationKind;

extern void
PrepareForDebugGC(JSRuntime *rt);



extern void
DelayCrossCompartmentGrayMarking(JSObject *src);

extern void
NotifyGCNukeWrapper(JSObject *o);

extern unsigned
NotifyGCPreSwap(JSObject *a, JSObject *b);

extern void
NotifyGCPostSwap(JSObject *a, JSObject *b, unsigned preResult);









class GCHelperState
{
    enum State {
        IDLE,
        SWEEPING
    };

    
    JSRuntime *const rt;

    
    
    
    PRCondVar *done;

    
    State state_;

    
    PRThread *thread;

    void startBackgroundThread(State newState);
    void waitForBackgroundThread();

    State state();
    void setState(State state);

    bool              sweepFlag;
    bool              shrinkFlag;

    friend class js::gc::ArenaLists;

    static void freeElementsAndArray(void **array, void **end) {
        MOZ_ASSERT(array <= end);
        for (void **p = array; p != end; ++p)
            js_free(*p);
        js_free(array);
    }

    void doSweep(AutoLockGC &lock);

  public:
    explicit GCHelperState(JSRuntime *rt)
      : rt(rt),
        done(nullptr),
        state_(IDLE),
        thread(nullptr),
        sweepFlag(false),
        shrinkFlag(false)
    { }

    bool init();
    void finish();

    void work();

    
    void startBackgroundSweep();

    
    void startBackgroundShrink();

    
    void waitBackgroundSweepEnd();

    bool onBackgroundThread();

    



    bool isBackgroundSweeping() const {
        return state_ == SWEEPING;
    }

    bool shouldShrink() const {
        MOZ_ASSERT(isBackgroundSweeping());
        return shrinkFlag;
    }
};




class GCParallelTask
{
    
    enum TaskState {
        NotStarted,
        Dispatched,
        Finished,
    } state;

    
    uint64_t duration_;

  protected:
    
    mozilla::Atomic<bool> cancel_;

    virtual void run() = 0;

  public:
    GCParallelTask() : state(NotStarted), duration_(0) {}

    
    int64_t duration() const { return duration_; }

    
    bool start();
    void join();

    
    
    bool startWithLockHeld();
    void joinWithLockHeld();

    
    void runFromMainThread(JSRuntime *rt);

    
    enum CancelMode { CancelNoWait, CancelAndWait};
    void cancel(CancelMode mode = CancelNoWait) {
        cancel_ = true;
        if (mode == CancelAndWait)
            join();
    }

    
    bool isRunning() const;

    
    
  public:
    void runFromHelperThread();
};

struct GCChunkHasher {
    typedef gc::Chunk *Lookup;

    



    static HashNumber hash(gc::Chunk *chunk) {
        MOZ_ASSERT(!(uintptr_t(chunk) & gc::ChunkMask));
        return HashNumber(uintptr_t(chunk) >> gc::ChunkShift);
    }

    static bool match(gc::Chunk *k, gc::Chunk *l) {
        MOZ_ASSERT(!(uintptr_t(k) & gc::ChunkMask));
        MOZ_ASSERT(!(uintptr_t(l) & gc::ChunkMask));
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





void
MergeCompartments(JSCompartment *source, JSCompartment *target);

#if defined(JSGC_GENERATIONAL) || defined(JSGC_COMPACTING)





class RelocationOverlay
{
    friend class MinorCollectionTracer;
    friend class ForkJoinNursery;

    
    static const uintptr_t Relocated = uintptr_t(0xbad0bad1);

    
    

    
    Cell *newLocation_;

    
    uintptr_t magic_;

    
    RelocationOverlay *next_;

  public:
    static RelocationOverlay *fromCell(Cell *cell) {
        return reinterpret_cast<RelocationOverlay *>(cell);
    }

    bool isForwarded() const {
        return magic_ == Relocated;
    }

    Cell *forwardingAddress() const {
        MOZ_ASSERT(isForwarded());
        return newLocation_;
    }

    void forwardTo(Cell *cell) {
        MOZ_ASSERT(!isForwarded());
        MOZ_ASSERT(JSObject::offsetOfShape() == offsetof(RelocationOverlay, newLocation_));
        newLocation_ = cell;
        magic_ = Relocated;
        next_ = nullptr;
    }

    RelocationOverlay *next() const {
        return next_;
    }
};



template <typename T>
inline bool
IsForwarded(T *t)
{
    RelocationOverlay *overlay = RelocationOverlay::fromCell(t);
    return overlay->isForwarded();
}

inline bool
IsForwarded(const JS::Value &value)
{
    if (value.isObject())
        return IsForwarded(&value.toObject());

    if (value.isString())
        return IsForwarded(value.toString());

    if (value.isSymbol())
        return IsForwarded(value.toSymbol());

    MOZ_ASSERT(!value.isGCThing());
    return false;
}

template <typename T>
inline T *
Forwarded(T *t)
{
    RelocationOverlay *overlay = RelocationOverlay::fromCell(t);
    MOZ_ASSERT(overlay->isForwarded());
    return reinterpret_cast<T*>(overlay->forwardingAddress());
}

inline Value
Forwarded(const JS::Value &value)
{
    if (value.isObject())
        return ObjectValue(*Forwarded(&value.toObject()));
    else if (value.isString())
        return StringValue(Forwarded(value.toString()));
    else if (value.isSymbol())
        return SymbolValue(Forwarded(value.toSymbol()));

    MOZ_ASSERT(!value.isGCThing());
    return value;
}

template <typename T>
inline T
MaybeForwarded(T t)
{
    return IsForwarded(t) ? Forwarded(t) : t;
}

#else

template <typename T> inline bool IsForwarded(T t) { return false; }
template <typename T> inline T Forwarded(T t) { return t; }
template <typename T> inline T MaybeForwarded(T t) { return t; }

#endif 

#ifdef JSGC_HASH_TABLE_CHECKS

template <typename T>
inline void
CheckGCThingAfterMovingGC(T *t)
{
    MOZ_ASSERT_IF(t, !IsInsideNursery(t));
#ifdef JSGC_COMPACTING
    MOZ_ASSERT_IF(t, !IsForwarded(t));
#endif
}

inline void
CheckValueAfterMovingGC(const JS::Value& value)
{
    if (value.isObject())
        return CheckGCThingAfterMovingGC(&value.toObject());
    else if (value.isString())
        return CheckGCThingAfterMovingGC(value.toString());
    else if (value.isSymbol())
        return CheckGCThingAfterMovingGC(value.toSymbol());
}

#endif 

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
const int ZealCompactValue = 14;
const int ZealLimit = 14;

extern const char *ZealModeHelpText;

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
    explicit AutoSuppressGC(ExclusiveContext *cx);
    explicit AutoSuppressGC(JSCompartment *comp);
    explicit AutoSuppressGC(JSRuntime *rt);

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




bool
IsInsideGGCNursery(const gc::Cell *cell);

} 

#ifdef DEBUG

class AutoDisableProxyCheck
{
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER;
    gc::GCRuntime &gc;

  public:
    explicit AutoDisableProxyCheck(JSRuntime *rt
                                   MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
    ~AutoDisableProxyCheck();
};
#else
struct AutoDisableProxyCheck
{
    explicit AutoDisableProxyCheck(JSRuntime *rt) {}
};
#endif

struct AutoDisableCompactingGC
{
#ifdef JSGC_COMPACTING
    explicit AutoDisableCompactingGC(JSRuntime *rt);
    ~AutoDisableCompactingGC();

  private:
    gc::GCRuntime &gc;
#else
    explicit AutoDisableCompactingGC(JSRuntime *rt) {}
    ~AutoDisableCompactingGC() {}
#endif
};

void
PurgeJITCaches(JS::Zone *zone);


bool
UninlinedIsInsideNursery(const gc::Cell *cell);

} 

#endif 
