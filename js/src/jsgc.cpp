






















































































































































































#include "jsgcinlines.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/MacroForEach.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"

#include <ctype.h>
#include <string.h>
#ifndef XP_WIN
# include <sys/mman.h>
# include <unistd.h>
#endif

#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsobj.h"
#include "jsprf.h"
#include "jsscript.h"
#include "jstypes.h"
#include "jsutil.h"
#include "jswatchpoint.h"
#include "jsweakmap.h"
#ifdef XP_WIN
# include "jswin.h"
#endif
#include "prmjtime.h"

#include "gc/FindSCCs.h"
#include "gc/GCInternals.h"
#include "gc/GCTrace.h"
#include "gc/Marking.h"
#include "gc/Memory.h"
#include "jit/BaselineJIT.h"
#include "jit/IonCode.h"
#include "js/SliceBudget.h"
#include "proxy/DeadObjectProxy.h"
#include "vm/Debugger.h"
#include "vm/ProxyObject.h"
#include "vm/Shape.h"
#include "vm/String.h"
#include "vm/Symbol.h"
#include "vm/TraceLogging.h"
#include "vm/WrapperObject.h"

#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "vm/Stack-inl.h"
#include "vm/String-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::ArrayLength;
using mozilla::Maybe;
using mozilla::Swap;

using JS::AutoGCRooter;


static const uint64_t GC_IDLE_FULL_SPAN = 20 * 1000 * 1000;


static const int IGC_MARK_SLICE_MULTIPLIER = 2;

const AllocKind gc::slotsToThingKind[] = {
      FINALIZE_OBJECT0,  FINALIZE_OBJECT2,  FINALIZE_OBJECT2,  FINALIZE_OBJECT4,
      FINALIZE_OBJECT4,  FINALIZE_OBJECT8,  FINALIZE_OBJECT8,  FINALIZE_OBJECT8,
      FINALIZE_OBJECT8,  FINALIZE_OBJECT12, FINALIZE_OBJECT12, FINALIZE_OBJECT12,
     FINALIZE_OBJECT12, FINALIZE_OBJECT16, FINALIZE_OBJECT16, FINALIZE_OBJECT16,
     FINALIZE_OBJECT16
};

static_assert(JS_ARRAY_LENGTH(slotsToThingKind) == SLOTS_TO_THING_KIND_LIMIT,
              "We have defined a slot count for each kind.");


#define CHECK_MIN_THING_SIZE_INNER(x_)                                         \
    static_assert(x_ >= SortedArenaList::MinThingSize,                         \
    #x_ " is less than SortedArenaList::MinThingSize!");
#define CHECK_MIN_THING_SIZE(...) { __VA_ARGS__ }; /* Define the array. */     \
    MOZ_FOR_EACH(CHECK_MIN_THING_SIZE_INNER, (), (__VA_ARGS__ UINT32_MAX))

const uint32_t Arena::ThingSizes[] = CHECK_MIN_THING_SIZE(
    sizeof(JSObject_Slots0),    
    sizeof(JSObject_Slots0),    
    sizeof(JSObject_Slots2),    
    sizeof(JSObject_Slots2),    
    sizeof(JSObject_Slots4),    
    sizeof(JSObject_Slots4),    
    sizeof(JSObject_Slots8),    
    sizeof(JSObject_Slots8),    
    sizeof(JSObject_Slots12),   
    sizeof(JSObject_Slots12),   
    sizeof(JSObject_Slots16),   
    sizeof(JSObject_Slots16),   
    sizeof(JSScript),           
    sizeof(LazyScript),         
    sizeof(Shape),              
    sizeof(AccessorShape),      
    sizeof(BaseShape),          
    sizeof(types::TypeObject),  
    sizeof(JSFatInlineString),  
    sizeof(JSString),           
    sizeof(JSExternalString),   
    sizeof(JS::Symbol),         
    sizeof(jit::JitCode),       
);

#undef CHECK_MIN_THING_SIZE_INNER
#undef CHECK_MIN_THING_SIZE

#define OFFSET(type) uint32_t(sizeof(ArenaHeader) + (ArenaSize - sizeof(ArenaHeader)) % sizeof(type))

const uint32_t Arena::FirstThingOffsets[] = {
    OFFSET(JSObject_Slots0),    
    OFFSET(JSObject_Slots0),    
    OFFSET(JSObject_Slots2),    
    OFFSET(JSObject_Slots2),    
    OFFSET(JSObject_Slots4),    
    OFFSET(JSObject_Slots4),    
    OFFSET(JSObject_Slots8),    
    OFFSET(JSObject_Slots8),    
    OFFSET(JSObject_Slots12),   
    OFFSET(JSObject_Slots12),   
    OFFSET(JSObject_Slots16),   
    OFFSET(JSObject_Slots16),   
    OFFSET(JSScript),           
    OFFSET(LazyScript),         
    OFFSET(Shape),              
    OFFSET(AccessorShape),      
    OFFSET(BaseShape),          
    OFFSET(types::TypeObject),  
    OFFSET(JSFatInlineString),  
    OFFSET(JSString),           
    OFFSET(JSExternalString),   
    OFFSET(JS::Symbol),         
    OFFSET(jit::JitCode),       
};

#undef OFFSET

struct js::gc::FinalizePhase
{
    size_t length;
    const AllocKind *kinds;
    gcstats::Phase statsPhase;
};

#define PHASE(x, p) { ArrayLength(x), x, p }





static const AllocKind IncrementalPhaseStrings[] = {
    FINALIZE_EXTERNAL_STRING
};

static const AllocKind IncrementalPhaseScripts[] = {
    FINALIZE_SCRIPT,
    FINALIZE_LAZY_SCRIPT
};

static const AllocKind IncrementalPhaseJitCode[] = {
    FINALIZE_JITCODE
};

static const FinalizePhase IncrementalFinalizePhases[] = {
    PHASE(IncrementalPhaseStrings, gcstats::PHASE_SWEEP_STRING),
    PHASE(IncrementalPhaseScripts, gcstats::PHASE_SWEEP_SCRIPT),
    PHASE(IncrementalPhaseJitCode, gcstats::PHASE_SWEEP_JITCODE)
};





static const AllocKind BackgroundPhaseObjects[] = {
    FINALIZE_OBJECT0_BACKGROUND,
    FINALIZE_OBJECT2_BACKGROUND,
    FINALIZE_OBJECT4_BACKGROUND,
    FINALIZE_OBJECT8_BACKGROUND,
    FINALIZE_OBJECT12_BACKGROUND,
    FINALIZE_OBJECT16_BACKGROUND
};

static const AllocKind BackgroundPhaseStringsAndSymbols[] = {
    FINALIZE_FAT_INLINE_STRING,
    FINALIZE_STRING,
    FINALIZE_SYMBOL
};

static const AllocKind BackgroundPhaseShapes[] = {
    FINALIZE_SHAPE,
    FINALIZE_ACCESSOR_SHAPE,
    FINALIZE_BASE_SHAPE,
    FINALIZE_TYPE_OBJECT
};

static const FinalizePhase BackgroundFinalizePhases[] = {
    PHASE(BackgroundPhaseObjects, gcstats::PHASE_SWEEP_OBJECT),
    PHASE(BackgroundPhaseStringsAndSymbols, gcstats::PHASE_SWEEP_STRING),
    PHASE(BackgroundPhaseShapes, gcstats::PHASE_SWEEP_SHAPE)
};

#undef PHASE

template<>
JSObject *
ArenaCellIterImpl::get<JSObject>() const
{
    MOZ_ASSERT(!done());
    return reinterpret_cast<JSObject *>(getCell());
}

#ifdef DEBUG
void
ArenaHeader::checkSynchronizedWithFreeList() const
{
    



    MOZ_ASSERT(allocated());

    




    if (IsBackgroundFinalized(getAllocKind()) && zone->runtimeFromAnyThread()->gc.onBackgroundThread())
        return;

    FreeSpan firstSpan = firstFreeSpan.decompact(arenaAddress());
    if (firstSpan.isEmpty())
        return;
    const FreeList *freeList = zone->arenas.getFreeList(getAllocKind());
    if (freeList->isEmpty() || firstSpan.arenaAddress() != freeList->arenaAddress())
        return;

    



    MOZ_ASSERT(freeList->isSameNonEmptySpan(firstSpan));
}
#endif

void
ArenaHeader::unmarkAll()
{
    uintptr_t *word = chunk()->bitmap.arenaBits(this);
    memset(word, 0, ArenaBitmapWords * sizeof(uintptr_t));
}

 void
Arena::staticAsserts()
{
    static_assert(JS_ARRAY_LENGTH(ThingSizes) == FINALIZE_LIMIT, "We have defined all thing sizes.");
    static_assert(JS_ARRAY_LENGTH(FirstThingOffsets) == FINALIZE_LIMIT, "We have defined all offsets.");
}

void
Arena::setAsFullyUnused(AllocKind thingKind)
{
    FreeSpan fullSpan;
    size_t thingSize = Arena::thingSize(thingKind);
    fullSpan.initFinal(thingsStart(thingKind), thingsEnd() - thingSize, thingSize);
    aheader.setFirstFreeSpan(&fullSpan);
}

template<typename T>
inline size_t
Arena::finalize(FreeOp *fop, AllocKind thingKind, size_t thingSize)
{
    
    MOZ_ASSERT(thingSize % CellSize == 0);
    MOZ_ASSERT(thingSize <= 255);

    MOZ_ASSERT(aheader.allocated());
    MOZ_ASSERT(thingKind == aheader.getAllocKind());
    MOZ_ASSERT(thingSize == aheader.getThingSize());
    MOZ_ASSERT(!aheader.hasDelayedMarking);
    MOZ_ASSERT(!aheader.markOverflow);
    MOZ_ASSERT(!aheader.allocatedDuringIncremental);

    uintptr_t firstThing = thingsStart(thingKind);
    uintptr_t firstThingOrSuccessorOfLastMarkedThing = firstThing;
    uintptr_t lastThing = thingsEnd() - thingSize;

    FreeSpan newListHead;
    FreeSpan *newListTail = &newListHead;
    size_t nmarked = 0;

    for (ArenaCellIterUnderFinalize i(&aheader); !i.done(); i.next()) {
        T *t = i.get<T>();
        if (t->asTenured().isMarked()) {
            uintptr_t thing = reinterpret_cast<uintptr_t>(t);
            if (thing != firstThingOrSuccessorOfLastMarkedThing) {
                
                
                newListTail->initBoundsUnchecked(firstThingOrSuccessorOfLastMarkedThing,
                                                 thing - thingSize);
                newListTail = newListTail->nextSpanUnchecked();
            }
            firstThingOrSuccessorOfLastMarkedThing = thing + thingSize;
            nmarked++;
        } else {
            t->finalize(fop);
            JS_POISON(t, JS_SWEPT_TENURED_PATTERN, thingSize);
            TraceTenuredFinalize(t);
        }
    }

    if (nmarked == 0) {
        
        MOZ_ASSERT(newListTail == &newListHead);
        JS_EXTRA_POISON(data, JS_SWEPT_TENURED_PATTERN, sizeof(data));
        return nmarked;
    }

    MOZ_ASSERT(firstThingOrSuccessorOfLastMarkedThing != firstThing);
    uintptr_t lastMarkedThing = firstThingOrSuccessorOfLastMarkedThing - thingSize;
    if (lastThing == lastMarkedThing) {
        
        
        newListTail->initAsEmpty();
    } else {
        
        newListTail->initFinal(firstThingOrSuccessorOfLastMarkedThing, lastThing, thingSize);
    }

#ifdef DEBUG
    size_t nfree = 0;
    for (const FreeSpan *span = &newListHead; !span->isEmpty(); span = span->nextSpan())
        nfree += span->length(thingSize);
    MOZ_ASSERT(nfree + nmarked == thingsPerArena(thingSize));
#endif
    aheader.setFirstFreeSpan(&newListHead);
    return nmarked;
}




template<typename T>
static inline bool
FinalizeTypedArenas(FreeOp *fop,
                    ArenaHeader **src,
                    SortedArenaList &dest,
                    AllocKind thingKind,
                    SliceBudget &budget,
                    ArenaLists::KeepArenasEnum keepArenas)
{
    
    Maybe<AutoLockGC> maybeLock;
    if (!fop->onBackgroundThread())
        maybeLock.emplace(fop->runtime());

    
    
    MOZ_ASSERT_IF(fop->onBackgroundThread(), keepArenas == ArenaLists::KEEP_ARENAS);

    size_t thingSize = Arena::thingSize(thingKind);
    size_t thingsPerArena = Arena::thingsPerArena(thingSize);

    while (ArenaHeader *aheader = *src) {
        *src = aheader->next;
        size_t nmarked = aheader->getArena()->finalize<T>(fop, thingKind, thingSize);
        size_t nfree = thingsPerArena - nmarked;

        if (nmarked)
            dest.insertAt(aheader, nfree);
        else if (keepArenas == ArenaLists::KEEP_ARENAS)
            aheader->chunk()->recycleArena(aheader, dest, thingKind, thingsPerArena);
        else
            fop->runtime()->gc.releaseArena(aheader, maybeLock.ref());

        budget.step(thingsPerArena);
        if (budget.isOverBudget())
            return false;
    }

    return true;
}





static bool
FinalizeArenas(FreeOp *fop,
               ArenaHeader **src,
               SortedArenaList &dest,
               AllocKind thingKind,
               SliceBudget &budget,
               ArenaLists::KeepArenasEnum keepArenas)
{
    switch (thingKind) {
      case FINALIZE_OBJECT0:
      case FINALIZE_OBJECT0_BACKGROUND:
      case FINALIZE_OBJECT2:
      case FINALIZE_OBJECT2_BACKGROUND:
      case FINALIZE_OBJECT4:
      case FINALIZE_OBJECT4_BACKGROUND:
      case FINALIZE_OBJECT8:
      case FINALIZE_OBJECT8_BACKGROUND:
      case FINALIZE_OBJECT12:
      case FINALIZE_OBJECT12_BACKGROUND:
      case FINALIZE_OBJECT16:
      case FINALIZE_OBJECT16_BACKGROUND:
        return FinalizeTypedArenas<JSObject>(fop, src, dest, thingKind, budget, keepArenas);
      case FINALIZE_SCRIPT:
        return FinalizeTypedArenas<JSScript>(fop, src, dest, thingKind, budget, keepArenas);
      case FINALIZE_LAZY_SCRIPT:
        return FinalizeTypedArenas<LazyScript>(fop, src, dest, thingKind, budget, keepArenas);
      case FINALIZE_SHAPE:
        return FinalizeTypedArenas<Shape>(fop, src, dest, thingKind, budget, keepArenas);
      case FINALIZE_ACCESSOR_SHAPE:
        return FinalizeTypedArenas<AccessorShape>(fop, src, dest, thingKind, budget, keepArenas);
      case FINALIZE_BASE_SHAPE:
        return FinalizeTypedArenas<BaseShape>(fop, src, dest, thingKind, budget, keepArenas);
      case FINALIZE_TYPE_OBJECT:
        return FinalizeTypedArenas<types::TypeObject>(fop, src, dest, thingKind, budget, keepArenas);
      case FINALIZE_STRING:
        return FinalizeTypedArenas<JSString>(fop, src, dest, thingKind, budget, keepArenas);
      case FINALIZE_FAT_INLINE_STRING:
        return FinalizeTypedArenas<JSFatInlineString>(fop, src, dest, thingKind, budget, keepArenas);
      case FINALIZE_EXTERNAL_STRING:
        return FinalizeTypedArenas<JSExternalString>(fop, src, dest, thingKind, budget, keepArenas);
      case FINALIZE_SYMBOL:
        return FinalizeTypedArenas<JS::Symbol>(fop, src, dest, thingKind, budget, keepArenas);
      case FINALIZE_JITCODE:
        return FinalizeTypedArenas<jit::JitCode>(fop, src, dest, thingKind, budget, keepArenas);
      default:
        MOZ_CRASH("Invalid alloc kind");
    }
}

Chunk *
ChunkPool::pop()
{
    MOZ_ASSERT(bool(head_) == bool(count_));
    if (!count_)
        return nullptr;
    return remove(head_);
}

void
ChunkPool::push(Chunk *chunk)
{
    MOZ_ASSERT(!chunk->info.next);
    MOZ_ASSERT(!chunk->info.prev);

    chunk->info.age = 0;
    chunk->info.next = head_;
    if (head_)
        head_->info.prev = chunk;
    head_ = chunk;
    ++count_;

    MOZ_ASSERT(verify());
}

Chunk *
ChunkPool::remove(Chunk *chunk)
{
    MOZ_ASSERT(count_ > 0);
    MOZ_ASSERT(contains(chunk));

    if (head_ == chunk)
        head_ = chunk->info.next;
    if (chunk->info.prev)
        chunk->info.prev->info.next = chunk->info.next;
    if (chunk->info.next)
        chunk->info.next->info.prev = chunk->info.prev;
    chunk->info.next = chunk->info.prev = nullptr;
    --count_;

    MOZ_ASSERT(verify());
    return chunk;
}

#ifdef DEBUG
bool
ChunkPool::contains(Chunk *chunk) const
{
    verify();
    for (Chunk *cursor = head_; cursor; cursor = cursor->info.next) {
        if (cursor == chunk)
            return true;
    }
    return false;
}

bool
ChunkPool::verify() const
{
    MOZ_ASSERT(bool(head_) == bool(count_));
    uint32_t count = 0;
    for (Chunk *cursor = head_; cursor; cursor = cursor->info.next, ++count) {
        MOZ_ASSERT_IF(cursor->info.prev, cursor->info.prev->info.next == cursor);
        MOZ_ASSERT_IF(cursor->info.next, cursor->info.next->info.prev == cursor);
    }
    MOZ_ASSERT(count_ == count);
    return true;
}
#endif

void
ChunkPool::Iter::next()
{
    MOZ_ASSERT(!done());
    current_ = current_->info.next;
}

ChunkPool
GCRuntime::expireEmptyChunkPool(bool shrinkBuffers, const AutoLockGC &lock)
{
    





    MOZ_ASSERT(emptyChunks(lock).verify());
    ChunkPool expired;
    unsigned freeChunkCount = 0;
    for (ChunkPool::Iter iter(emptyChunks(lock)); !iter.done();) {
        Chunk *chunk = iter.get();
        iter.next();

        MOZ_ASSERT(chunk->unused());
        MOZ_ASSERT(!fullChunks(lock).contains(chunk));
        MOZ_ASSERT(!availableChunks(lock).contains(chunk));
        if (freeChunkCount >= tunables.maxEmptyChunkCount() ||
            (freeChunkCount >= tunables.minEmptyChunkCount() &&
             (shrinkBuffers || chunk->info.age == MAX_EMPTY_CHUNK_AGE)))
        {
            emptyChunks(lock).remove(chunk);
            prepareToFreeChunk(chunk->info);
            expired.push(chunk);
        } else {
            
            ++freeChunkCount;
            ++chunk->info.age;
        }
    }
    MOZ_ASSERT(expired.verify());
    MOZ_ASSERT(emptyChunks(lock).verify());
    MOZ_ASSERT(emptyChunks(lock).count() <= tunables.maxEmptyChunkCount());
    MOZ_ASSERT_IF(shrinkBuffers, emptyChunks(lock).count() <= tunables.minEmptyChunkCount());
    return expired;
}

static void
FreeChunkPool(JSRuntime *rt, ChunkPool &pool)
{
    for (ChunkPool::Iter iter(pool); !iter.done();) {
        Chunk *chunk = iter.get();
        iter.next();
        pool.remove(chunk);
        MOZ_ASSERT(!chunk->info.numArenasFreeCommitted);
        UnmapPages(static_cast<void *>(chunk), ChunkSize);
    }
    MOZ_ASSERT(pool.count() == 0);
}

void
GCRuntime::freeEmptyChunks(JSRuntime *rt, const AutoLockGC &lock)
{
    FreeChunkPool(rt, emptyChunks(lock));
}

 Chunk *
Chunk::allocate(JSRuntime *rt)
{
    Chunk *chunk = static_cast<Chunk *>(MapAlignedPages(ChunkSize, ChunkSize));
    if (!chunk)
        return nullptr;
    chunk->init(rt);
    rt->gc.stats.count(gcstats::STAT_NEW_CHUNK);
    return chunk;
}

inline void
GCRuntime::prepareToFreeChunk(ChunkInfo &info)
{
    MOZ_ASSERT(numArenasFreeCommitted >= info.numArenasFreeCommitted);
    numArenasFreeCommitted -= info.numArenasFreeCommitted;
    stats.count(gcstats::STAT_DESTROY_CHUNK);
#ifdef DEBUG
    



    info.numArenasFreeCommitted = 0;
#endif
}

void Chunk::decommitAllArenas(JSRuntime *rt)
{
    decommittedArenas.clear(true);
    MarkPagesUnused(&arenas[0], ArenasPerChunk * ArenaSize);

    info.freeArenasHead = nullptr;
    info.lastDecommittedArenaOffset = 0;
    info.numArenasFree = ArenasPerChunk;
    info.numArenasFreeCommitted = 0;
}

void
Chunk::init(JSRuntime *rt)
{
    JS_POISON(this, JS_FRESH_TENURED_PATTERN, ChunkSize);

    



    bitmap.clear();

    



    decommitAllArenas(rt);

    
    info.init();
    info.trailer.storeBuffer = nullptr;
    info.trailer.location = ChunkLocationBitTenuredHeap;
    info.trailer.runtime = rt;

    
}







uint32_t
Chunk::findDecommittedArenaOffset()
{
    
    for (unsigned i = info.lastDecommittedArenaOffset; i < ArenasPerChunk; i++)
        if (decommittedArenas.get(i))
            return i;
    for (unsigned i = 0; i < info.lastDecommittedArenaOffset; i++)
        if (decommittedArenas.get(i))
            return i;
    MOZ_CRASH("No decommitted arenas found.");
}

ArenaHeader *
Chunk::fetchNextDecommittedArena()
{
    MOZ_ASSERT(info.numArenasFreeCommitted == 0);
    MOZ_ASSERT(info.numArenasFree > 0);

    unsigned offset = findDecommittedArenaOffset();
    info.lastDecommittedArenaOffset = offset + 1;
    --info.numArenasFree;
    decommittedArenas.unset(offset);

    Arena *arena = &arenas[offset];
    MarkPagesInUse(arena, ArenaSize);
    arena->aheader.setAsNotAllocated();

    return &arena->aheader;
}

inline void
GCRuntime::updateOnFreeArenaAlloc(const ChunkInfo &info)
{
    MOZ_ASSERT(info.numArenasFreeCommitted <= numArenasFreeCommitted);
    --numArenasFreeCommitted;
}

inline ArenaHeader *
Chunk::fetchNextFreeArena(JSRuntime *rt)
{
    MOZ_ASSERT(info.numArenasFreeCommitted > 0);
    MOZ_ASSERT(info.numArenasFreeCommitted <= info.numArenasFree);

    ArenaHeader *aheader = info.freeArenasHead;
    info.freeArenasHead = aheader->next;
    --info.numArenasFreeCommitted;
    --info.numArenasFree;
    rt->gc.updateOnFreeArenaAlloc(info);

    return aheader;
}

ArenaHeader *
Chunk::allocateArena(JSRuntime *rt, Zone *zone, AllocKind thingKind, const AutoLockGC &lock)
{
    ArenaHeader *aheader = info.numArenasFreeCommitted > 0
                           ? fetchNextFreeArena(rt)
                           : fetchNextDecommittedArena();
    aheader->init(zone, thingKind);
    if (MOZ_UNLIKELY(!hasAvailableArenas())) {
        rt->gc.availableChunks(lock).remove(this);
        rt->gc.fullChunks(lock).push(this);
    }
    return aheader;
}

inline void
GCRuntime::updateOnArenaFree(const ChunkInfo &info)
{
    ++numArenasFreeCommitted;
}

void
Chunk::addArenaToFreeList(JSRuntime *rt, ArenaHeader *aheader)
{
    MOZ_ASSERT(!aheader->allocated());
    aheader->next = info.freeArenasHead;
    info.freeArenasHead = aheader;
    ++info.numArenasFreeCommitted;
    ++info.numArenasFree;
    rt->gc.updateOnArenaFree(info);
}

void
Chunk::addArenaToDecommittedList(JSRuntime *rt, const ArenaHeader *aheader)
{
    ++info.numArenasFree;
    decommittedArenas.set(Chunk::arenaIndex(aheader->arenaAddress()));
}

void
Chunk::recycleArena(ArenaHeader *aheader, SortedArenaList &dest, AllocKind thingKind,
                    size_t thingsPerArena)
{
    aheader->getArena()->setAsFullyUnused(thingKind);
    dest.insertAt(aheader, thingsPerArena);
}

void
Chunk::releaseArena(JSRuntime *rt, ArenaHeader *aheader, const AutoLockGC &lock,
                    ArenaDecommitState state )
{
    MOZ_ASSERT(aheader->allocated());
    MOZ_ASSERT(!aheader->hasDelayedMarking);

    if (state == IsCommitted) {
        aheader->setAsNotAllocated();
        addArenaToFreeList(rt, aheader);
    } else {
        addArenaToDecommittedList(rt, aheader);
    }

    if (info.numArenasFree == 1) {
        rt->gc.fullChunks(lock).remove(this);
        rt->gc.availableChunks(lock).push(this);
    } else if (!unused()) {
        MOZ_ASSERT(!rt->gc.fullChunks(lock).contains(this));
        MOZ_ASSERT(rt->gc.availableChunks(lock).contains(this));
        MOZ_ASSERT(!rt->gc.emptyChunks(lock).contains(this));
    } else {
        MOZ_ASSERT(unused());
        rt->gc.availableChunks(lock).remove(this);
        decommitAllArenas(rt);
        rt->gc.emptyChunks(lock).push(this);
    }
}

inline bool
GCRuntime::wantBackgroundAllocation(const AutoLockGC &lock) const
{
    
    
    
    return allocTask.enabled() &&
           emptyChunks(lock).count() < tunables.minEmptyChunkCount() &&
           (fullChunks(lock).count() + availableChunks(lock).count()) >= 4;
}

void
GCRuntime::startBackgroundAllocTaskIfIdle()
{
    AutoLockHelperThreadState helperLock;
    if (allocTask.isRunning())
        return;

    
    
    allocTask.joinWithLockHeld();
    allocTask.startWithLockHeld();
}

class js::gc::AutoMaybeStartBackgroundAllocation
{
  private:
    JSRuntime *runtime;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoMaybeStartBackgroundAllocation(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
      : runtime(nullptr)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    void tryToStartBackgroundAllocation(JSRuntime *rt) {
        runtime = rt;
    }

    ~AutoMaybeStartBackgroundAllocation() {
        if (runtime)
            runtime->gc.startBackgroundAllocTaskIfIdle();
    }
};

Chunk *
GCRuntime::pickChunk(const AutoLockGC &lock,
                     AutoMaybeStartBackgroundAllocation &maybeStartBackgroundAllocation)
{
    if (availableChunks(lock).count())
        return availableChunks(lock).head();

    Chunk *chunk = emptyChunks(lock).pop();
    if (!chunk) {
        chunk = Chunk::allocate(rt);
        if (!chunk)
            return nullptr;
        MOZ_ASSERT(chunk->info.numArenasFreeCommitted == 0);
    }

    MOZ_ASSERT(chunk->unused());
    MOZ_ASSERT(!fullChunks(lock).contains(chunk));

    if (wantBackgroundAllocation(lock))
        maybeStartBackgroundAllocation.tryToStartBackgroundAllocation(rt);

    chunkAllocationSinceLastGC = true;

    availableChunks(lock).push(chunk);

    return chunk;
}

ArenaHeader *
GCRuntime::allocateArena(Chunk *chunk, Zone *zone, AllocKind thingKind, const AutoLockGC &lock)
{
    MOZ_ASSERT(chunk->hasAvailableArenas());

    
    if (!isHeapMinorCollecting() &&
        !isHeapCompacting() &&
        usage.gcBytes() >= tunables.gcMaxBytes())
    {
        return nullptr;
    }

    ArenaHeader *aheader = chunk->allocateArena(rt, zone, thingKind, lock);
    zone->usage.addGCArena();

    
    if (!isHeapMinorCollecting() && !isHeapCompacting())
        maybeAllocTriggerZoneGC(zone, lock);

    return aheader;
}

void
GCRuntime::releaseArena(ArenaHeader *aheader, const AutoLockGC &lock)
{
    aheader->zone->usage.removeGCArena();
    if (isBackgroundSweeping())
        aheader->zone->threshold.updateForRemovedArena(tunables);
    return aheader->chunk()->releaseArena(rt, aheader, lock);
}

void
GCRuntime::decommitArena(ArenaHeader *aheader, AutoLockGC &lock)
{
    aheader->zone->usage.removeGCArena();
    if (isBackgroundSweeping())
        aheader->zone->threshold.updateForRemovedArena(tunables);

    bool ok;
    {
        AutoUnlockGC unlock(lock);
        ok = MarkPagesUnused(aheader, ArenaSize);
    }
    return aheader->chunk()->releaseArena(rt, aheader, lock, Chunk::ArenaDecommitState(ok));
}

GCRuntime::GCRuntime(JSRuntime *rt) :
    rt(rt),
    systemZone(nullptr),
    nursery(rt),
    storeBuffer(rt, nursery),
    stats(rt),
    marker(rt),
    usage(nullptr),
    maxMallocBytes(0),
    numArenasFreeCommitted(0),
    verifyPreData(nullptr),
    verifyPostData(nullptr),
    chunkAllocationSinceLastGC(false),
    nextFullGCTime(0),
    lastGCTime(PRMJ_Now()),
    mode(JSGC_MODE_INCREMENTAL),
    decommitThreshold(32 * 1024 * 1024),
    cleanUpEverything(false),
    grayBitsValid(false),
    majorGCRequested(0),
    majorGCTriggerReason(JS::gcreason::NO_REASON),
    minorGCRequested(false),
    minorGCTriggerReason(JS::gcreason::NO_REASON),
    majorGCNumber(0),
    jitReleaseNumber(0),
    number(0),
    startNumber(0),
    isFull(false),
#ifdef DEBUG
    disableStrictProxyCheckingCount(0),
#endif
    incrementalState(gc::NO_INCREMENTAL),
    lastMarkSlice(false),
    sweepOnBackgroundThread(false),
    foundBlackGrayEdges(false),
    freeLifoAlloc(JSRuntime::TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    zoneGroupIndex(0),
    zoneGroups(nullptr),
    currentZoneGroup(nullptr),
    sweepZone(nullptr),
    sweepKindIndex(0),
    abortSweepAfterCurrentGroup(false),
    arenasAllocatedDuringSweep(nullptr),
#ifdef JS_GC_MARKING_VALIDATION
    markingValidator(nullptr),
#endif
    interFrameGC(false),
    sliceBudget(SliceBudget::Unlimited),
    incrementalAllowed(true),
    generationalDisabled(0),
    compactingDisabled(0),
    manipulatingDeadZones(false),
    objectsMarkedInDeadZones(0),
    poked(false),
    heapState(Idle),
#ifdef JS_GC_ZEAL
    zealMode(0),
    zealFrequency(0),
    nextScheduled(0),
    deterministicOnly(false),
    incrementalLimit(0),
#endif
    validate(true),
    fullCompartmentChecks(false),
    mallocBytes(0),
    mallocGCTriggered(false),
    alwaysPreserveCode(false),
#ifdef DEBUG
    inUnsafeRegion(0),
    noGCOrAllocationCheck(0),
    relocatedArenasToRelease(nullptr),
#endif
    lock(nullptr),
    lockOwner(nullptr),
    allocTask(rt, emptyChunks_),
    decommitTask(rt),
    helperState(rt)
{
    setGCMode(JSGC_MODE_GLOBAL);
}

#ifdef JS_GC_ZEAL

void
GCRuntime::getZeal(uint8_t *zeal, uint32_t *frequency, uint32_t *scheduled)
{
    *zeal = zealMode;
    *frequency = zealFrequency;
    *scheduled = nextScheduled;
}

const char *gc::ZealModeHelpText =
    "  Specifies how zealous the garbage collector should be. Values for level:\n"
    "    0: Normal amount of collection\n"
    "    1: Collect when roots are added or removed\n"
    "    2: Collect when every N allocations (default: 100)\n"
    "    3: Collect when the window paints (browser only)\n"
    "    4: Verify pre write barriers between instructions\n"
    "    5: Verify pre write barriers between paints\n"
    "    6: Verify stack rooting\n"
    "    7: Collect the nursery every N nursery allocations\n"
    "    8: Incremental GC in two slices: 1) mark roots 2) finish collection\n"
    "    9: Incremental GC in two slices: 1) mark all 2) new marking and finish\n"
    "   10: Incremental GC in multiple slices\n"
    "   11: Verify post write barriers between instructions\n"
    "   12: Verify post write barriers between paints\n"
    "   13: Check internal hashtables on minor GC\n"
    "   14: Perform a shrinking collection every N allocations\n";

void
GCRuntime::setZeal(uint8_t zeal, uint32_t frequency)
{
    if (verifyPreData)
        VerifyBarriers(rt, PreBarrierVerifier);
    if (verifyPostData)
        VerifyBarriers(rt, PostBarrierVerifier);

    if (zealMode == ZealGenerationalGCValue) {
        evictNursery(JS::gcreason::DEBUG_GC);
        nursery.leaveZealMode();
    }

    if (zeal == ZealGenerationalGCValue)
        nursery.enterZealMode();

    bool schedule = zeal >= js::gc::ZealAllocValue;
    zealMode = zeal;
    zealFrequency = frequency;
    nextScheduled = schedule ? frequency : 0;
}

void
GCRuntime::setNextScheduled(uint32_t count)
{
    nextScheduled = count;
}

bool
GCRuntime::parseAndSetZeal(const char *str)
{
    int zeal = -1;
    int frequency = -1;

    if (isdigit(str[0])) {
        zeal = atoi(str);

        const char *p = strchr(str, ',');
        if (!p)
            frequency = JS_DEFAULT_ZEAL_FREQ;
        else
            frequency = atoi(p + 1);
    }

    if (zeal < 0 || zeal > ZealLimit || frequency <= 0) {
        fprintf(stderr, "Format: JS_GC_ZEAL=level[,N]\n");
        fputs(ZealModeHelpText, stderr);
        return false;
    }

    setZeal(zeal, frequency);
    return true;
}

#endif





static const uint64_t JIT_SCRIPT_RELEASE_TYPES_PERIOD = 20;

bool
GCRuntime::init(uint32_t maxbytes, uint32_t maxNurseryBytes)
{
    InitMemorySubsystem();

    lock = PR_NewLock();
    if (!lock)
        return false;

    if (!rootsHash.init(256))
        return false;

    if (!helperState.init())
        return false;

    



    tunables.setParameter(JSGC_MAX_BYTES, maxbytes);
    setMaxMallocBytes(maxbytes);

    jitReleaseNumber = majorGCNumber + JIT_SCRIPT_RELEASE_TYPES_PERIOD;

    if (!nursery.init(maxNurseryBytes))
        return false;

    if (!nursery.isEnabled()) {
        MOZ_ASSERT(nursery.nurserySize() == 0);
        ++rt->gc.generationalDisabled;
    } else {
        MOZ_ASSERT(nursery.nurserySize() > 0);
        if (!storeBuffer.enable())
            return false;
    }

#ifdef JS_GC_ZEAL
    const char *zealSpec = getenv("JS_GC_ZEAL");
    if (zealSpec && zealSpec[0] && !parseAndSetZeal(zealSpec))
        return false;
#endif

    if (!InitTrace(*this))
        return false;

    if (!marker.init(mode))
        return false;

    return true;
}

void
GCRuntime::finish()
{
    



    helperState.finish();

#ifdef JS_GC_ZEAL
    
    finishVerifier();
#endif

    
    if (rt->gcInitialized) {
        for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
            for (CompartmentsInZoneIter comp(zone); !comp.done(); comp.next())
                js_delete(comp.get());
            js_delete(zone.get());
        }
    }

    zones.clear();

    FreeChunkPool(rt, fullChunks_);
    FreeChunkPool(rt, availableChunks_);
    FreeChunkPool(rt, emptyChunks_);

    if (rootsHash.initialized())
        rootsHash.clear();

    FinishPersistentRootedChains(rt);

    if (lock) {
        PR_DestroyLock(lock);
        lock = nullptr;
    }

    FinishTrace();
}

void
js::gc::FinishPersistentRootedChains(JSRuntime *rt)
{
    
    rt->functionPersistentRooteds.clear();
    rt->idPersistentRooteds.clear();
    rt->objectPersistentRooteds.clear();
    rt->scriptPersistentRooteds.clear();
    rt->stringPersistentRooteds.clear();
    rt->valuePersistentRooteds.clear();
}

void
GCRuntime::setParameter(JSGCParamKey key, uint32_t value)
{
    switch (key) {
      case JSGC_MAX_MALLOC_BYTES:
        setMaxMallocBytes(value);
        break;
      case JSGC_SLICE_TIME_BUDGET:
        sliceBudget = value ? value : SliceBudget::Unlimited;
        break;
      case JSGC_MARK_STACK_LIMIT:
        setMarkStackLimit(value);
        break;
      case JSGC_DECOMMIT_THRESHOLD:
        decommitThreshold = value * 1024 * 1024;
        break;
      case JSGC_MODE:
        mode = JSGCMode(value);
        MOZ_ASSERT(mode == JSGC_MODE_GLOBAL ||
                   mode == JSGC_MODE_COMPARTMENT ||
                   mode == JSGC_MODE_INCREMENTAL);
        break;
      default:
        tunables.setParameter(key, value);
    }
}

void
GCSchedulingTunables::setParameter(JSGCParamKey key, uint32_t value)
{
    switch(key) {
      case JSGC_MAX_BYTES:
        gcMaxBytes_ = value;
        break;
      case JSGC_HIGH_FREQUENCY_TIME_LIMIT:
        highFrequencyThresholdUsec_ = value * PRMJ_USEC_PER_MSEC;
        break;
      case JSGC_HIGH_FREQUENCY_LOW_LIMIT:
        highFrequencyLowLimitBytes_ = value * 1024 * 1024;
        if (highFrequencyLowLimitBytes_ >= highFrequencyHighLimitBytes_)
            highFrequencyHighLimitBytes_ = highFrequencyLowLimitBytes_ + 1;
        MOZ_ASSERT(highFrequencyHighLimitBytes_ > highFrequencyLowLimitBytes_);
        break;
      case JSGC_HIGH_FREQUENCY_HIGH_LIMIT:
        MOZ_ASSERT(value > 0);
        highFrequencyHighLimitBytes_ = value * 1024 * 1024;
        if (highFrequencyHighLimitBytes_ <= highFrequencyLowLimitBytes_)
            highFrequencyLowLimitBytes_ = highFrequencyHighLimitBytes_ - 1;
        MOZ_ASSERT(highFrequencyHighLimitBytes_ > highFrequencyLowLimitBytes_);
        break;
      case JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MAX:
        highFrequencyHeapGrowthMax_ = value / 100.0;
        MOZ_ASSERT(highFrequencyHeapGrowthMax_ / 0.85 > 1.0);
        break;
      case JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MIN:
        highFrequencyHeapGrowthMin_ = value / 100.0;
        MOZ_ASSERT(highFrequencyHeapGrowthMin_ / 0.85 > 1.0);
        break;
      case JSGC_LOW_FREQUENCY_HEAP_GROWTH:
        lowFrequencyHeapGrowth_ = value / 100.0;
        MOZ_ASSERT(lowFrequencyHeapGrowth_ / 0.9 > 1.0);
        break;
      case JSGC_DYNAMIC_HEAP_GROWTH:
        dynamicHeapGrowthEnabled_ = value;
        break;
      case JSGC_DYNAMIC_MARK_SLICE:
        dynamicMarkSliceEnabled_ = value;
        break;
      case JSGC_ALLOCATION_THRESHOLD:
        gcZoneAllocThresholdBase_ = value * 1024 * 1024;
        break;
      case JSGC_MIN_EMPTY_CHUNK_COUNT:
        minEmptyChunkCount_ = value;
        if (minEmptyChunkCount_ > maxEmptyChunkCount_)
            maxEmptyChunkCount_ = minEmptyChunkCount_;
        MOZ_ASSERT(maxEmptyChunkCount_ >= minEmptyChunkCount_);
        break;
      case JSGC_MAX_EMPTY_CHUNK_COUNT:
        maxEmptyChunkCount_ = value;
        if (minEmptyChunkCount_ > maxEmptyChunkCount_)
            minEmptyChunkCount_ = maxEmptyChunkCount_;
        MOZ_ASSERT(maxEmptyChunkCount_ >= minEmptyChunkCount_);
        break;
      default:
        MOZ_CRASH("Unknown GC parameter.");
    }
}

uint32_t
GCRuntime::getParameter(JSGCParamKey key, const AutoLockGC &lock)
{
    switch (key) {
      case JSGC_MAX_BYTES:
        return uint32_t(tunables.gcMaxBytes());
      case JSGC_MAX_MALLOC_BYTES:
        return maxMallocBytes;
      case JSGC_BYTES:
        return uint32_t(usage.gcBytes());
      case JSGC_MODE:
        return uint32_t(mode);
      case JSGC_UNUSED_CHUNKS:
        return uint32_t(emptyChunks(lock).count());
      case JSGC_TOTAL_CHUNKS:
        return uint32_t(fullChunks(lock).count() +
                        availableChunks(lock).count() +
                        emptyChunks(lock).count());
      case JSGC_SLICE_TIME_BUDGET:
        return uint32_t(sliceBudget > 0 ? sliceBudget : 0);
      case JSGC_MARK_STACK_LIMIT:
        return marker.maxCapacity();
      case JSGC_HIGH_FREQUENCY_TIME_LIMIT:
        return tunables.highFrequencyThresholdUsec();
      case JSGC_HIGH_FREQUENCY_LOW_LIMIT:
        return tunables.highFrequencyLowLimitBytes() / 1024 / 1024;
      case JSGC_HIGH_FREQUENCY_HIGH_LIMIT:
        return tunables.highFrequencyHighLimitBytes() / 1024 / 1024;
      case JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MAX:
        return uint32_t(tunables.highFrequencyHeapGrowthMax() * 100);
      case JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MIN:
        return uint32_t(tunables.highFrequencyHeapGrowthMin() * 100);
      case JSGC_LOW_FREQUENCY_HEAP_GROWTH:
        return uint32_t(tunables.lowFrequencyHeapGrowth() * 100);
      case JSGC_DYNAMIC_HEAP_GROWTH:
        return tunables.isDynamicHeapGrowthEnabled();
      case JSGC_DYNAMIC_MARK_SLICE:
        return tunables.isDynamicMarkSliceEnabled();
      case JSGC_ALLOCATION_THRESHOLD:
        return tunables.gcZoneAllocThresholdBase() / 1024 / 1024;
      case JSGC_MIN_EMPTY_CHUNK_COUNT:
        return tunables.minEmptyChunkCount();
      case JSGC_MAX_EMPTY_CHUNK_COUNT:
        return tunables.maxEmptyChunkCount();
      default:
        MOZ_ASSERT(key == JSGC_NUMBER);
        return uint32_t(number);
    }
}

void
GCRuntime::setMarkStackLimit(size_t limit)
{
    MOZ_ASSERT(!isHeapBusy());
    AutoStopVerifyingBarriers pauseVerification(rt, false);
    marker.setMaxCapacity(limit);
}

template <typename T> struct BarrierOwner {};
template <typename T> struct BarrierOwner<T *> { typedef T result; };
template <> struct BarrierOwner<Value> { typedef HeapValue result; };

bool
GCRuntime::addBlackRootsTracer(JSTraceDataOp traceOp, void *data)
{
    AssertHeapIsIdle(rt);
    return !!blackRootTracers.append(Callback<JSTraceDataOp>(traceOp, data));
}

void
GCRuntime::removeBlackRootsTracer(JSTraceDataOp traceOp, void *data)
{
    
    for (size_t i = 0; i < blackRootTracers.length(); i++) {
        Callback<JSTraceDataOp> *e = &blackRootTracers[i];
        if (e->op == traceOp && e->data == data) {
            blackRootTracers.erase(e);
        }
    }
}

void
GCRuntime::setGrayRootsTracer(JSTraceDataOp traceOp, void *data)
{
    AssertHeapIsIdle(rt);
    grayRootTracer.op = traceOp;
    grayRootTracer.data = data;
}

void
GCRuntime::setGCCallback(JSGCCallback callback, void *data)
{
    gcCallback.op = callback;
    gcCallback.data = data;
}

bool
GCRuntime::addFinalizeCallback(JSFinalizeCallback callback, void *data)
{
    return finalizeCallbacks.append(Callback<JSFinalizeCallback>(callback, data));
}

void
GCRuntime::removeFinalizeCallback(JSFinalizeCallback callback)
{
    for (Callback<JSFinalizeCallback> *p = finalizeCallbacks.begin();
         p < finalizeCallbacks.end(); p++)
    {
        if (p->op == callback) {
            finalizeCallbacks.erase(p);
            break;
        }
    }
}

void
GCRuntime::callFinalizeCallbacks(FreeOp *fop, JSFinalizeStatus status) const
{
    for (const Callback<JSFinalizeCallback> *p = finalizeCallbacks.begin();
         p < finalizeCallbacks.end(); p++)
    {
        p->op(fop, status, !isFull, p->data);
    }
}

bool
GCRuntime::addWeakPointerCallback(JSWeakPointerCallback callback, void *data)
{
    return updateWeakPointerCallbacks.append(Callback<JSWeakPointerCallback>(callback, data));
}

void
GCRuntime::removeWeakPointerCallback(JSWeakPointerCallback callback)
{
    for (Callback<JSWeakPointerCallback> *p = updateWeakPointerCallbacks.begin();
         p < updateWeakPointerCallbacks.end(); p++)
    {
        if (p->op == callback) {
            updateWeakPointerCallbacks.erase(p);
            break;
        }
    }
}

void
GCRuntime::callWeakPointerCallbacks() const
{
    for (const Callback<JSWeakPointerCallback> *p = updateWeakPointerCallbacks.begin();
         p < updateWeakPointerCallbacks.end(); p++)
    {
        p->op(rt, p->data);
    }
}

JS::GCSliceCallback
GCRuntime::setSliceCallback(JS::GCSliceCallback callback) {
    return stats.setSliceCallback(callback);
}

template <typename T>
bool
GCRuntime::addRoot(T *rp, const char *name, JSGCRootType rootType)
{
    





    if (isIncrementalGCInProgress())
        BarrierOwner<T>::result::writeBarrierPre(*rp);

    return rootsHash.put((void *)rp, RootInfo(name, rootType));
}

void
GCRuntime::removeRoot(void *rp)
{
    rootsHash.remove(rp);
    poke();
}

template <typename T>
static bool
AddRoot(JSRuntime *rt, T *rp, const char *name, JSGCRootType rootType)
{
    return rt->gc.addRoot(rp, name, rootType);
}

template <typename T>
static bool
AddRoot(JSContext *cx, T *rp, const char *name, JSGCRootType rootType)
{
    bool ok = cx->runtime()->gc.addRoot(rp, name, rootType);
    if (!ok)
        JS_ReportOutOfMemory(cx);
    return ok;
}

bool
js::AddValueRoot(JSContext *cx, Value *vp, const char *name)
{
    return AddRoot(cx, vp, name, JS_GC_ROOT_VALUE_PTR);
}

extern bool
js::AddValueRootRT(JSRuntime *rt, js::Value *vp, const char *name)
{
    return AddRoot(rt, vp, name, JS_GC_ROOT_VALUE_PTR);
}

extern bool
js::AddStringRoot(JSContext *cx, JSString **rp, const char *name)
{
    return AddRoot(cx, rp, name, JS_GC_ROOT_STRING_PTR);
}

extern bool
js::AddObjectRoot(JSContext *cx, JSObject **rp, const char *name)
{
    return AddRoot(cx, rp, name, JS_GC_ROOT_OBJECT_PTR);
}

extern bool
js::AddObjectRoot(JSRuntime *rt, JSObject **rp, const char *name)
{
    return AddRoot(rt, rp, name, JS_GC_ROOT_OBJECT_PTR);
}

extern bool
js::AddScriptRoot(JSContext *cx, JSScript **rp, const char *name)
{
    return AddRoot(cx, rp, name, JS_GC_ROOT_SCRIPT_PTR);
}

extern JS_FRIEND_API(bool)
js::AddRawValueRoot(JSContext *cx, Value *vp, const char *name)
{
    return AddRoot(cx, vp, name, JS_GC_ROOT_VALUE_PTR);
}

extern JS_FRIEND_API(void)
js::RemoveRawValueRoot(JSContext *cx, Value *vp)
{
    RemoveRoot(cx->runtime(), vp);
}

void
js::RemoveRoot(JSRuntime *rt, void *rp)
{
    rt->gc.removeRoot(rp);
}

void
GCRuntime::setMaxMallocBytes(size_t value)
{
    



    maxMallocBytes = (ptrdiff_t(value) >= 0) ? value : size_t(-1) >> 1;
    resetMallocBytes();
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next())
        zone->setGCMaxMallocBytes(value);
}

void
GCRuntime::resetMallocBytes()
{
    mallocBytes = ptrdiff_t(maxMallocBytes);
    mallocGCTriggered = false;
}

void
GCRuntime::updateMallocCounter(JS::Zone *zone, size_t nbytes)
{
    mallocBytes -= ptrdiff_t(nbytes);
    if (MOZ_UNLIKELY(isTooMuchMalloc()))
        onTooMuchMalloc();
    else if (zone)
        zone->updateMallocCounter(nbytes);
}

void
GCRuntime::onTooMuchMalloc()
{
    if (!mallocGCTriggered)
        mallocGCTriggered = triggerGC(JS::gcreason::TOO_MUCH_MALLOC);
}

bool
ZoneHeapThreshold::isCloseToAllocTrigger(const js::gc::HeapUsage& usage, bool highFrequencyGC) const
{
    double factor = highFrequencyGC ? 0.85 : 0.9;
    return usage.gcBytes() >= factor * gcTriggerBytes();
}

 double
ZoneHeapThreshold::computeZoneHeapGrowthFactorForHeapSize(size_t lastBytes,
                                                          const GCSchedulingTunables &tunables,
                                                          const GCSchedulingState &state)
{
    if (!tunables.isDynamicHeapGrowthEnabled())
        return 3.0;

    
    
    if (lastBytes < 1 * 1024 * 1024)
        return tunables.lowFrequencyHeapGrowth();

    
    
    if (!state.inHighFrequencyGCMode())
        return tunables.lowFrequencyHeapGrowth();

    
    
    
    
    
    
    

    
    double minRatio = tunables.highFrequencyHeapGrowthMin();
    double maxRatio = tunables.highFrequencyHeapGrowthMax();
    double lowLimit = tunables.highFrequencyLowLimitBytes();
    double highLimit = tunables.highFrequencyHighLimitBytes();

    if (lastBytes <= lowLimit)
        return maxRatio;

    if (lastBytes >= highLimit)
        return minRatio;

    double factor = maxRatio - ((maxRatio - minRatio) * ((lastBytes - lowLimit) /
                                                         (highLimit - lowLimit)));
    MOZ_ASSERT(factor >= minRatio);
    MOZ_ASSERT(factor <= maxRatio);
    return factor;
}

 size_t
ZoneHeapThreshold::computeZoneTriggerBytes(double growthFactor, size_t lastBytes,
                                           JSGCInvocationKind gckind,
                                           const GCSchedulingTunables &tunables)
{
    size_t base = gckind == GC_SHRINK
                ? lastBytes
                : Max(lastBytes, tunables.gcZoneAllocThresholdBase());
    double trigger = double(base) * growthFactor;
    return size_t(Min(double(tunables.gcMaxBytes()), trigger));
}

void
ZoneHeapThreshold::updateAfterGC(size_t lastBytes, JSGCInvocationKind gckind,
                                 const GCSchedulingTunables &tunables,
                                 const GCSchedulingState &state)
{
    gcHeapGrowthFactor_ = computeZoneHeapGrowthFactorForHeapSize(lastBytes, tunables, state);
    gcTriggerBytes_ = computeZoneTriggerBytes(gcHeapGrowthFactor_, lastBytes, gckind, tunables);
}

void
ZoneHeapThreshold::updateForRemovedArena(const GCSchedulingTunables &tunables)
{
    size_t amount = ArenaSize * gcHeapGrowthFactor_;

    MOZ_ASSERT(amount > 0);
    MOZ_ASSERT(gcTriggerBytes_ >= amount);

    if (gcTriggerBytes_ - amount < tunables.gcZoneAllocThresholdBase() * gcHeapGrowthFactor_)
        return;

    gcTriggerBytes_ -= amount;
}

inline void
GCMarker::delayMarkingArena(ArenaHeader *aheader)
{
    if (aheader->hasDelayedMarking) {
        
        return;
    }
    aheader->setNextDelayedMarking(unmarkedArenaStackTop);
    unmarkedArenaStackTop = aheader;
    markLaterArenas++;
}

void
GCMarker::delayMarkingChildren(const void *thing)
{
    const TenuredCell *cell = TenuredCell::fromPointer(thing);
    cell->arenaHeader()->markOverflow = 1;
    delayMarkingArena(cell->arenaHeader());
}

inline void
ArenaLists::prepareForIncrementalGC(JSRuntime *rt)
{
    for (size_t i = 0; i != FINALIZE_LIMIT; ++i) {
        FreeList *freeList = &freeLists[i];
        if (!freeList->isEmpty()) {
            ArenaHeader *aheader = freeList->arenaHeader();
            aheader->allocatedDuringIncremental = true;
            rt->gc.marker.delayMarkingArena(aheader);
        }
    }
}

inline void
GCRuntime::arenaAllocatedDuringGC(JS::Zone *zone, ArenaHeader *arena)
{
    if (zone->needsIncrementalBarrier()) {
        arena->allocatedDuringIncremental = true;
        marker.delayMarkingArena(arena);
    } else if (zone->isGCSweeping()) {
        arena->setNextAllocDuringSweep(arenasAllocatedDuringSweep);
        arenasAllocatedDuringSweep = arena;
    }
}

TenuredCell *
ArenaLists::allocateFromArena(JS::Zone *zone, AllocKind thingKind)
{
    AutoMaybeStartBackgroundAllocation maybeStartBackgroundAllocation;
    return allocateFromArena(zone, thingKind, maybeStartBackgroundAllocation);
}

TenuredCell *
ArenaLists::allocateFromArena(JS::Zone *zone, AllocKind thingKind,
                              AutoMaybeStartBackgroundAllocation &maybeStartBGAlloc)
{
    JSRuntime *rt = zone->runtimeFromAnyThread();
    Maybe<AutoLockGC> maybeLock;

    
    if (backgroundFinalizeState[thingKind] != BFS_DONE)
        maybeLock.emplace(rt);

    ArenaList &al = arenaLists[thingKind];
    ArenaHeader *aheader = al.takeNextArena();
    if (aheader) {
        
        MOZ_ASSERT(!aheader->isEmpty());

        return allocateFromArenaInner<HasFreeThings>(zone, aheader, thingKind);
    }

    
    
    if (maybeLock.isNothing())
        maybeLock.emplace(rt);

    Chunk *chunk = rt->gc.pickChunk(maybeLock.ref(), maybeStartBGAlloc);
    if (!chunk)
        return nullptr;

    
    
    aheader = rt->gc.allocateArena(chunk, zone, thingKind, maybeLock.ref());
    if (!aheader)
        return nullptr;

    MOZ_ASSERT(!maybeLock->wasUnlocked());
    MOZ_ASSERT(al.isCursorAtEnd());
    al.insertAtCursor(aheader);

    return allocateFromArenaInner<IsEmpty>(zone, aheader, thingKind);
}

template <ArenaLists::ArenaAllocMode hasFreeThings>
inline TenuredCell *
ArenaLists::allocateFromArenaInner(JS::Zone *zone, ArenaHeader *aheader, AllocKind thingKind)
{
    size_t thingSize = Arena::thingSize(thingKind);

    FreeSpan span;
    if (hasFreeThings) {
        MOZ_ASSERT(aheader->hasFreeThings());
        span = aheader->getFirstFreeSpan();
        aheader->setAsFullyUsed();
    } else {
        MOZ_ASSERT(!aheader->hasFreeThings());
        Arena *arena = aheader->getArena();
        span.initFinal(arena->thingsStart(thingKind), arena->thingsEnd() - thingSize, thingSize);
    }
    freeLists[thingKind].setHead(&span);

    if (MOZ_UNLIKELY(zone->wasGCStarted()))
        zone->runtimeFromAnyThread()->gc.arenaAllocatedDuringGC(zone, aheader);
    TenuredCell *thing = freeLists[thingKind].allocate(thingSize);
    MOZ_ASSERT(thing); 
    return thing;
}



bool
GCRuntime::shouldCompact()
{
    return invocationKind == GC_SHRINK && isCompactingGCEnabled();
}

void
GCRuntime::disableCompactingGC()
{
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(rt));
    ++rt->gc.compactingDisabled;
}

void
GCRuntime::enableCompactingGC()
{
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(rt));
    MOZ_ASSERT(compactingDisabled > 0);
    --compactingDisabled;
}

bool
GCRuntime::isCompactingGCEnabled()
{
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(rt));
    return rt->gc.compactingDisabled == 0;
}

AutoDisableCompactingGC::AutoDisableCompactingGC(JSRuntime *rt)
  : gc(rt->gc)
{
    gc.disableCompactingGC();
}

AutoDisableCompactingGC::~AutoDisableCompactingGC()
{
    gc.enableCompactingGC();
}

static bool
CanRelocateZone(JSRuntime *rt, Zone *zone)
{
    return !rt->isAtomsZone(zone) && !rt->isSelfHostingZone(zone);
}

static bool
CanRelocateAllocKind(AllocKind kind)
{
    return kind <= FINALIZE_OBJECT_LAST;
}


size_t ArenaHeader::countFreeCells()
{
    size_t count = 0;
    size_t thingSize = getThingSize();
    FreeSpan firstSpan(getFirstFreeSpan());
    for (const FreeSpan *span = &firstSpan; !span->isEmpty(); span = span->nextSpan())
        count += span->length(thingSize);
    return count;
}

size_t ArenaHeader::countUsedCells()
{
    return Arena::thingsPerArena(getThingSize()) - countFreeCells();
}

ArenaHeader *
ArenaList::removeRemainingArenas(ArenaHeader **arenap, const AutoLockGC &lock)
{
    
    
#ifdef DEBUG
    for (ArenaHeader *arena = *arenap; arena; arena = arena->next)
        MOZ_ASSERT(cursorp_ != &arena->next);
#endif
    ArenaHeader *remainingArenas = *arenap;
    *arenap = nullptr;
    check();
    return remainingArenas;
}





ArenaHeader *
ArenaList::pickArenasToRelocate(JSRuntime *runtime)
{
    AutoLockGC lock(runtime);

    check();
    if (isEmpty())
        return nullptr;

    
    
    
    bool relocateAll = runtime->gc.zeal() == ZealCompactValue;
#if defined(DEBUG) && defined(JS_PUNBOX64)
    relocateAll = true;
#endif
    if (relocateAll) {
        ArenaHeader *allArenas = head();
        clear();
        return allArenas;
    }

    
    
    
    
    
    
    
    
    
    

    if (isCursorAtEnd())
        return nullptr;

    ArenaHeader **arenap = cursorp_;               
    size_t previousFreeCells = 0;                  

    
    size_t followingUsedCells = 0;
    for (ArenaHeader *arena = *arenap; arena; arena = arena->next)
        followingUsedCells += arena->countUsedCells();

    mozilla::DebugOnly<size_t> lastFreeCells(0);
    size_t cellsPerArena = Arena::thingsPerArena((*arenap)->getThingSize());

    while (*arenap) {
        ArenaHeader *arena = *arenap;
        if (followingUsedCells <= previousFreeCells)
            return removeRemainingArenas(arenap, lock);
        size_t freeCells = arena->countFreeCells();
        size_t usedCells = cellsPerArena - freeCells;
        followingUsedCells -= usedCells;
#ifdef DEBUG
        MOZ_ASSERT(freeCells >= lastFreeCells);
        lastFreeCells = freeCells;
#endif
        previousFreeCells += freeCells;
        arenap = &arena->next;
    }

    check();
    return nullptr;
}

#ifdef DEBUG
inline bool
PtrIsInRange(const void *ptr, const void *start, size_t length)
{
    return uintptr_t(ptr) - uintptr_t(start) < length;
}
#endif

static bool
RelocateCell(Zone *zone, TenuredCell *src, AllocKind thingKind, size_t thingSize)
{
    JS::AutoSuppressGCAnalysis nogc(zone->runtimeFromMainThread());

    
    MOZ_ASSERT(zone == src->zone());
    void *dstAlloc = zone->arenas.allocateFromFreeList(thingKind, thingSize);
    if (!dstAlloc)
        dstAlloc = GCRuntime::refillFreeListInGC(zone, thingKind);
    if (!dstAlloc)
        return false;
    TenuredCell *dst = TenuredCell::fromPointer(dstAlloc);

    
    memcpy(dst, src, thingSize);

    if (thingKind <= FINALIZE_OBJECT_LAST) {
        JSObject *srcObj = static_cast<JSObject *>(static_cast<Cell *>(src));
        JSObject *dstObj = static_cast<JSObject *>(static_cast<Cell *>(dst));

        if (srcObj->isNative()) {
            NativeObject *srcNative = &srcObj->as<NativeObject>();
            NativeObject *dstNative = &dstObj->as<NativeObject>();

            
            if (srcNative->hasFixedElements())
                dstNative->setFixedElements();

            
            
            if (srcNative->hasDynamicElements() && srcNative->denseElementsAreCopyOnWrite()) {
                HeapPtrNativeObject &owner = srcNative->getElementsHeader()->ownerObject();
                if (owner == srcNative)
                    owner = dstNative;
            }
        }

        
        if (JSObjectMovedOp op = srcObj->getClass()->ext.objectMovedOp)
            op(dstObj, srcObj);

        MOZ_ASSERT_IF(dstObj->isNative(),
                      !PtrIsInRange((const Value*)dstObj->as<NativeObject>().getDenseElements(),
                                    src, thingSize));
    }

    
    dst->copyMarkBitsFrom(src);

    
    RelocationOverlay* overlay = RelocationOverlay::fromCell(src);
    overlay->forwardTo(dst);

    return true;
}

static void
RelocateArena(ArenaHeader *aheader)
{
    MOZ_ASSERT(aheader->allocated());
    MOZ_ASSERT(!aheader->hasDelayedMarking);
    MOZ_ASSERT(!aheader->markOverflow);
    MOZ_ASSERT(!aheader->allocatedDuringIncremental);

    Zone *zone = aheader->zone;

    AllocKind thingKind = aheader->getAllocKind();
    size_t thingSize = aheader->getThingSize();

    for (ArenaCellIterUnderFinalize i(aheader); !i.done(); i.next()) {
        if (!RelocateCell(zone, i.getCell(), thingKind, thingSize)) {
            
            
            
            CrashAtUnhandlableOOM("Could not allocate new arena while compacting");
        }
    }
}





ArenaHeader *
ArenaList::relocateArenas(ArenaHeader *toRelocate, ArenaHeader *relocated)
{
    check();

    while (ArenaHeader *arena = toRelocate) {
        toRelocate = arena->next;
        RelocateArena(arena);
        
        arena->next = relocated;
        relocated = arena;
    }

    check();

    return relocated;
}

ArenaHeader *
ArenaLists::relocateArenas(ArenaHeader *relocatedList)
{
    
    purge();
    checkEmptyFreeLists();

    for (size_t i = 0; i < FINALIZE_LIMIT; i++) {
        if (CanRelocateAllocKind(AllocKind(i))) {
            ArenaList &al = arenaLists[i];
            ArenaHeader *toRelocate = al.pickArenasToRelocate(runtime_);
            if (toRelocate)
                relocatedList = al.relocateArenas(toRelocate, relocatedList);
        }
    }

    





    purge();
    checkEmptyFreeLists();

    return relocatedList;
}

ArenaHeader *
GCRuntime::relocateArenas()
{
    gcstats::AutoPhase ap(stats, gcstats::PHASE_COMPACT_MOVE);

    ArenaHeader *relocatedList = nullptr;
    for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
        MOZ_ASSERT(zone->isGCFinished());
        MOZ_ASSERT(!zone->isPreservingCode());

        if (CanRelocateZone(rt, zone)) {
            zone->setGCState(Zone::Compact);
            StopAllOffThreadCompilations(zone);
            relocatedList = zone->arenas.relocateArenas(relocatedList);
        }
    }

    return relocatedList;
}

void
MovingTracer::Visit(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    TenuredCell *thing = TenuredCell::fromPointer(*thingp);
    Zone *zone = thing->zoneFromAnyThread();
    if (!zone->isGCCompacting()) {
        MOZ_ASSERT(!IsForwarded(thing));
        return;
    }

    if (IsForwarded(thing)) {
        Cell *dst = Forwarded(thing);
        *thingp = dst;
    }
}

void
GCRuntime::sweepTypesAfterCompacting(Zone *zone)
{
    FreeOp *fop = rt->defaultFreeOp();
    zone->beginSweepTypes(fop, rt->gc.releaseObservedTypes && !zone->isPreservingCode());

    types::AutoClearTypeInferenceStateOnOOM oom(zone);

    for (ZoneCellIterUnderGC i(zone, FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        script->maybeSweepTypes(&oom);
    }

    for (ZoneCellIterUnderGC i(zone, FINALIZE_TYPE_OBJECT); !i.done(); i.next()) {
        types::TypeObject *object = i.get<types::TypeObject>();
        object->maybeSweep(&oom);
    }

    zone->types.endSweep(rt);
}

void
GCRuntime::sweepZoneAfterCompacting(Zone *zone)
{
    MOZ_ASSERT(zone->isCollecting());
    FreeOp *fop = rt->defaultFreeOp();
    zone->discardJitCode(fop);
    sweepTypesAfterCompacting(zone);
    zone->sweepBreakpoints(fop);

    for (CompartmentsInZoneIter c(zone); !c.done(); c.next()) {
        c->sweepInnerViews();
        c->sweepCrossCompartmentWrappers();
        c->sweepBaseShapeTable();
        c->sweepInitialShapeTable();
        c->sweepTypeObjectTables();
        c->sweepRegExps();
        c->sweepCallsiteClones();
        c->sweepSavedStacks();
        c->sweepGlobalObject(fop);
        c->sweepSelfHostingScriptSource();
        c->sweepDebugScopes();
        c->sweepJitCompartment(fop);
        c->sweepWeakMaps();
        c->sweepNativeIterators();
    }
}

template <typename T>
static void
UpdateCellPointersTyped(MovingTracer *trc, ArenaHeader *arena, JSGCTraceKind traceKind)
{
    for (ArenaCellIterUnderGC i(arena); !i.done(); i.next()) {
        T *cell = reinterpret_cast<T*>(i.getCell());
        cell->fixupAfterMovingGC();
        TraceChildren(trc, cell, traceKind);
    }
}




static void
UpdateCellPointers(MovingTracer *trc, ArenaHeader *arena)
{
    AllocKind kind = arena->getAllocKind();
    JSGCTraceKind traceKind = MapAllocToTraceKind(kind);

    switch (kind) {
      case FINALIZE_OBJECT0:
      case FINALIZE_OBJECT0_BACKGROUND:
      case FINALIZE_OBJECT2:
      case FINALIZE_OBJECT2_BACKGROUND:
      case FINALIZE_OBJECT4:
      case FINALIZE_OBJECT4_BACKGROUND:
      case FINALIZE_OBJECT8:
      case FINALIZE_OBJECT8_BACKGROUND:
      case FINALIZE_OBJECT12:
      case FINALIZE_OBJECT12_BACKGROUND:
      case FINALIZE_OBJECT16:
      case FINALIZE_OBJECT16_BACKGROUND:
        UpdateCellPointersTyped<JSObject>(trc, arena, traceKind);
        return;
      case FINALIZE_SCRIPT:
        UpdateCellPointersTyped<JSScript>(trc, arena, traceKind);
        return;
      case FINALIZE_LAZY_SCRIPT:
        UpdateCellPointersTyped<LazyScript>(trc, arena, traceKind);
        return;
      case FINALIZE_SHAPE:
        UpdateCellPointersTyped<Shape>(trc, arena, traceKind);
        return;
      case FINALIZE_ACCESSOR_SHAPE:
        UpdateCellPointersTyped<AccessorShape>(trc, arena, traceKind);
        return;
      case FINALIZE_BASE_SHAPE:
        UpdateCellPointersTyped<BaseShape>(trc, arena, traceKind);
        return;
      case FINALIZE_TYPE_OBJECT:
        UpdateCellPointersTyped<types::TypeObject>(trc, arena, traceKind);
        return;
      case FINALIZE_JITCODE:
        UpdateCellPointersTyped<jit::JitCode>(trc, arena, traceKind);
        return;
      default:
        MOZ_CRASH("Invalid alloc kind for UpdateCellPointers");
    }
}

namespace js {
namespace gc {

struct ArenasToUpdate
{
    explicit ArenasToUpdate(JSRuntime *rt);
    bool done() { return initialized && arena == nullptr; }
    ArenaHeader* next();
    ArenaHeader *getArenasToUpdate(AutoLockHelperThreadState& lock, unsigned max);

  private:
    bool initialized;
    GCZonesIter zone;    
    unsigned kind;       
    ArenaHeader *arena;  

    bool shouldProcessKind(unsigned kind);
};

bool ArenasToUpdate::shouldProcessKind(unsigned kind)
{
    MOZ_ASSERT(kind < FINALIZE_LIMIT);
    return
        kind != FINALIZE_FAT_INLINE_STRING &&
        kind != FINALIZE_STRING &&
        kind != FINALIZE_EXTERNAL_STRING &&
        kind != FINALIZE_SYMBOL;
}

ArenasToUpdate::ArenasToUpdate(JSRuntime *rt)
  : initialized(false), zone(rt, SkipAtoms)
{}

ArenaHeader *
ArenasToUpdate::next()
{
    
    
    
    
    
    
    
    

    if (initialized) {
        MOZ_ASSERT(arena);
        MOZ_ASSERT(shouldProcessKind(kind));
        MOZ_ASSERT(!zone.done());
        goto resumePoint;
    }

    initialized = true;
    for (; !zone.done(); zone.next()) {
        for (kind = 0; kind < FINALIZE_LIMIT; ++kind) {
            if (shouldProcessKind(kind)) {
                for (arena = zone.get()->arenas.getFirstArena(AllocKind(kind));
                     arena;
                     arena = arena->next)
                {
                    return arena;
                    resumePoint:;
                }
            }
        }
    }
    return nullptr;
}

ArenaHeader *
ArenasToUpdate::getArenasToUpdate(AutoLockHelperThreadState& lock, unsigned count)
{
    if (zone.done())
        return nullptr;

    ArenaHeader *head = nullptr;
    ArenaHeader *tail = nullptr;

    for (unsigned i = 0; i < count; ++i) {
        ArenaHeader *arena = next();
        if (!arena)
            break;

        if (tail)
            tail->setNextArenaToUpdate(arena);
        else
            head = arena;
        tail = arena;
    }

    return head;
}

struct UpdateCellPointersTask : public GCParallelTask
{
    
#ifdef DEBUG
    static const unsigned ArenasToProcess = 16;
#else
    static const unsigned ArenasToProcess = 256;
#endif

    UpdateCellPointersTask() : rt_(nullptr), source_(nullptr), arenaList_(nullptr) {}
    void init(JSRuntime *rt, ArenasToUpdate *source, AutoLockHelperThreadState& lock);

  private:
    JSRuntime *rt_;
    ArenasToUpdate *source_;
    ArenaHeader *arenaList_;

    virtual void run() MOZ_OVERRIDE;
    void getArenasToUpdate(AutoLockHelperThreadState& lock);
    void updateArenas();
};

void
UpdateCellPointersTask::init(JSRuntime *rt, ArenasToUpdate *source, AutoLockHelperThreadState& lock)
{
    rt_ = rt;
    source_ = source;
    getArenasToUpdate(lock);
}

void
UpdateCellPointersTask::getArenasToUpdate(AutoLockHelperThreadState& lock)
{
    arenaList_ = source_->getArenasToUpdate(lock, ArenasToProcess);
}

void
UpdateCellPointersTask::updateArenas()
{
    MovingTracer trc(rt_);
    for (ArenaHeader *arena = arenaList_;
         arena;
         arena = arena->getNextArenaToUpdateAndUnlink())
    {
        UpdateCellPointers(&trc, arena);
    }
    arenaList_ = nullptr;
}

 void
UpdateCellPointersTask::run()
{
    while (arenaList_) {
        updateArenas();
        {
            AutoLockHelperThreadState lock;
            getArenasToUpdate(lock);
        }
    }
}

} 
} 

void
GCRuntime::updateAllCellPointersParallel(ArenasToUpdate &source)
{
    AutoDisableProxyCheck noProxyCheck(rt); 

    const size_t minTasks = 2;
    const size_t maxTasks = 8;
    unsigned taskCount = Min(Max(HelperThreadState().cpuCount / 2, minTasks),
                             maxTasks);
    UpdateCellPointersTask updateTasks[maxTasks];

    AutoLockHelperThreadState lock;
    unsigned i;
    for (i = 0; i < taskCount && !source.done(); ++i) {
        updateTasks[i].init(rt, &source, lock);
        startTask(updateTasks[i], gcstats::PHASE_COMPACT_UPDATE_CELLS);
    }
    unsigned tasksStarted = i;

    for (i = 0; i < tasksStarted; ++i)
        joinTask(updateTasks[i], gcstats::PHASE_COMPACT_UPDATE_CELLS);
}

void
GCRuntime::updateAllCellPointersSerial(MovingTracer *trc, ArenasToUpdate &source)
{
    while (ArenaHeader *arena = source.next())
        UpdateCellPointers(trc, arena);
}







void
GCRuntime::updatePointersToRelocatedCells()
{
    MOZ_ASSERT(rt->currentThreadHasExclusiveAccess());

    gcstats::AutoPhase ap(stats, gcstats::PHASE_COMPACT_UPDATE);
    MovingTracer trc(rt);

    
    for (GCCompartmentsIter comp(rt); !comp.done(); comp.next())
        comp->fixupAfterMovingGC();

    
    for (CompartmentsIter comp(rt, SkipAtoms); !comp.done(); comp.next())
        comp->sweepCrossCompartmentWrappers();

    
    
    
    ArenasToUpdate source(rt);
    if (CanUseExtraThreads())
        updateAllCellPointersParallel(source);
    else
        updateAllCellPointersSerial(&trc, source);

    
    {
        markRuntime(&trc, MarkRuntime);

        gcstats::AutoPhase ap(stats, gcstats::PHASE_MARK_ROOTS);
        Debugger::markAll(&trc);
        Debugger::markAllCrossCompartmentEdges(&trc);

        for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
            WeakMapBase::markAll(c, &trc);
            if (c->watchpointMap)
                c->watchpointMap->markAll(&trc);
        }

        
        
        if (JSTraceDataOp op = grayRootTracer.op)
            (*op)(&trc, grayRootTracer.data);
    }

    
    WatchpointMap::sweepAll(rt);
    Debugger::sweepAll(rt->defaultFreeOp());
    for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
        if (CanRelocateZone(rt, zone))
            rt->gc.sweepZoneAfterCompacting(zone);
    }

    
    freeLifoAlloc.freeAll();

    
    
    rt->newObjectCache.purge();
    rt->nativeIterCache.purge();

    
    callWeakPointerCallbacks();
}

#ifdef DEBUG
void
GCRuntime::protectRelocatedArenas(ArenaHeader *relocatedList)
{
    for (ArenaHeader* arena = relocatedList, *next; arena; arena = next) {
        next = arena->next;
#if defined(XP_WIN)
        DWORD oldProtect;
        if (!VirtualProtect(arena, ArenaSize, PAGE_NOACCESS, &oldProtect))
            MOZ_CRASH();
#else  
        if (mprotect(arena, ArenaSize, PROT_NONE))
            MOZ_CRASH();
#endif
    }
}

void
GCRuntime::unprotectRelocatedArenas(ArenaHeader *relocatedList)
{
    for (ArenaHeader* arena = relocatedList; arena; arena = arena->next) {
#if defined(XP_WIN)
        DWORD oldProtect;
        if (!VirtualProtect(arena, ArenaSize, PAGE_READWRITE, &oldProtect))
            MOZ_CRASH();
#else  
        if (mprotect(arena, ArenaSize, PROT_READ | PROT_WRITE))
            MOZ_CRASH();
#endif
    }
}
#endif

void
GCRuntime::releaseRelocatedArenas(ArenaHeader *relocatedList)
{
    AutoLockGC lock(rt);
    releaseRelocatedArenasWithoutUnlocking(relocatedList, lock);
    expireChunksAndArenas(true, lock);
}

void
GCRuntime::releaseRelocatedArenasWithoutUnlocking(ArenaHeader *relocatedList, const AutoLockGC &lock)
{
    
    unsigned count = 0;
    while (relocatedList) {
        ArenaHeader *aheader = relocatedList;
        relocatedList = relocatedList->next;

        
        aheader->unmarkAll();

        
        AllocKind thingKind = aheader->getAllocKind();
        size_t thingSize = aheader->getThingSize();
        Arena *arena = aheader->getArena();
        FreeSpan fullSpan;
        fullSpan.initFinal(arena->thingsStart(thingKind), arena->thingsEnd() - thingSize, thingSize);
        aheader->setFirstFreeSpan(&fullSpan);

#if defined(JS_CRASH_DIAGNOSTICS) || defined(JS_GC_ZEAL)
        JS_POISON(reinterpret_cast<void *>(arena->thingsStart(thingKind)),
                  JS_MOVED_TENURED_PATTERN, Arena::thingsSpan(thingSize));
#endif

        releaseArena(aheader, lock);
        ++count;
    }
}

void
GCRuntime::releaseHeldRelocatedArenas()
{
#ifdef DEBUG
    
    
    
    unprotectRelocatedArenas(relocatedArenasToRelease);
    releaseRelocatedArenas(relocatedArenasToRelease);
    relocatedArenasToRelease = nullptr;
#endif
}

void
ReleaseArenaList(JSRuntime *rt, ArenaHeader *aheader, const AutoLockGC &lock)
{
    ArenaHeader *next;
    for (; aheader; aheader = next) {
        next = aheader->next;
        rt->gc.releaseArena(aheader, lock);
    }
}

void
DecommitArenaList(JSRuntime *rt, ArenaHeader *aheader, AutoLockGC &lock)
{
    ArenaHeader *next;
    for (; aheader; aheader = next) {
        next = aheader->next;
        rt->gc.decommitArena(aheader, lock);
    }
}

ArenaLists::~ArenaLists()
{
    AutoLockGC lock(runtime_);

    for (size_t i = 0; i != FINALIZE_LIMIT; ++i) {
        



        MOZ_ASSERT(backgroundFinalizeState[i] == BFS_DONE);
        ReleaseArenaList(runtime_, arenaLists[i].head(), lock);
    }
    ReleaseArenaList(runtime_, incrementalSweptArenas.head(), lock);

    for (size_t i = 0; i < FINALIZE_OBJECT_LIMIT; i++)
        ReleaseArenaList(runtime_, savedObjectArenas[i].head(), lock);
    ReleaseArenaList(runtime_, savedEmptyObjectArenas, lock);
}

void
ArenaLists::finalizeNow(FreeOp *fop, const FinalizePhase& phase)
{
    gcstats::AutoPhase ap(fop->runtime()->gc.stats, phase.statsPhase);
    for (unsigned i = 0; i < phase.length; ++i)
        finalizeNow(fop, phase.kinds[i], RELEASE_ARENAS, nullptr);
}

void
ArenaLists::finalizeNow(FreeOp *fop, AllocKind thingKind, KeepArenasEnum keepArenas, ArenaHeader **empty)
{
    MOZ_ASSERT(!IsBackgroundFinalized(thingKind));
    forceFinalizeNow(fop, thingKind, keepArenas, empty);
}

void
ArenaLists::forceFinalizeNow(FreeOp *fop, AllocKind thingKind, KeepArenasEnum keepArenas, ArenaHeader **empty)
{
    MOZ_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE);

    ArenaHeader *arenas = arenaLists[thingKind].head();
    if (!arenas)
        return;
    arenaLists[thingKind].clear();

    const size_t thingsPerArena = Arena::thingsPerArena(Arena::thingSize(thingKind));
    SortedArenaList finalizedSorted(thingsPerArena);

    SliceBudget budget;
    FinalizeArenas(fop, &arenas, finalizedSorted, thingKind, budget, keepArenas);
    MOZ_ASSERT(!arenas);

    if (empty) {
        MOZ_ASSERT(keepArenas == KEEP_ARENAS);
        finalizedSorted.extractEmpty(empty);
    }

    arenaLists[thingKind] = finalizedSorted.toArenaList();
}

void
ArenaLists::queueForForegroundSweep(FreeOp *fop, const FinalizePhase& phase)
{
    gcstats::AutoPhase ap(fop->runtime()->gc.stats, phase.statsPhase);
    for (unsigned i = 0; i < phase.length; ++i)
        queueForForegroundSweep(fop, phase.kinds[i]);
}

void
ArenaLists::queueForForegroundSweep(FreeOp *fop, AllocKind thingKind)
{
    MOZ_ASSERT(!IsBackgroundFinalized(thingKind));
    MOZ_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE);
    MOZ_ASSERT(!arenaListsToSweep[thingKind]);

    arenaListsToSweep[thingKind] = arenaLists[thingKind].head();
    arenaLists[thingKind].clear();
}

void
ArenaLists::queueForBackgroundSweep(FreeOp *fop, const FinalizePhase& phase)
{
    gcstats::AutoPhase ap(fop->runtime()->gc.stats, phase.statsPhase);
    for (unsigned i = 0; i < phase.length; ++i)
        queueForBackgroundSweep(fop, phase.kinds[i]);
}

inline void
ArenaLists::queueForBackgroundSweep(FreeOp *fop, AllocKind thingKind)
{
    MOZ_ASSERT(IsBackgroundFinalized(thingKind));

    ArenaList *al = &arenaLists[thingKind];
    if (al->isEmpty()) {
        MOZ_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE);
        return;
    }

    MOZ_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE);

    arenaListsToSweep[thingKind] = al->head();
    al->clear();
    backgroundFinalizeState[thingKind] = BFS_RUN;
}

 void
ArenaLists::backgroundFinalize(FreeOp *fop, ArenaHeader *listHead, ArenaHeader **empty)
{
    MOZ_ASSERT(listHead);
    MOZ_ASSERT(empty);

    AllocKind thingKind = listHead->getAllocKind();
    Zone *zone = listHead->zone;

    const size_t thingsPerArena = Arena::thingsPerArena(Arena::thingSize(thingKind));
    SortedArenaList finalizedSorted(thingsPerArena);

    SliceBudget budget;
    FinalizeArenas(fop, &listHead, finalizedSorted, thingKind, budget, KEEP_ARENAS);
    MOZ_ASSERT(!listHead);

    finalizedSorted.extractEmpty(empty);

    
    
    
    
    ArenaLists *lists = &zone->arenas;
    ArenaList *al = &lists->arenaLists[thingKind];

    
    ArenaList finalized = finalizedSorted.toArenaList();

    
    
    
    
    
    {
        AutoLockGC lock(fop->runtime());
        MOZ_ASSERT(lists->backgroundFinalizeState[thingKind] == BFS_RUN);

        
        *al = finalized.insertListWithCursorAtEnd(*al);

        lists->arenaListsToSweep[thingKind] = nullptr;
    }

    lists->backgroundFinalizeState[thingKind] = BFS_DONE;
}

void
ArenaLists::queueForegroundObjectsForSweep(FreeOp *fop)
{
    gcstats::AutoPhase ap(fop->runtime()->gc.stats, gcstats::PHASE_SWEEP_OBJECT);

#ifdef DEBUG
    for (size_t i = 0; i < FINALIZE_OBJECT_LIMIT; i++)
        MOZ_ASSERT(savedObjectArenas[i].isEmpty());
    MOZ_ASSERT(savedEmptyObjectArenas == nullptr);
#endif

    
    
    
    
    finalizeNow(fop, FINALIZE_OBJECT0, KEEP_ARENAS, &savedEmptyObjectArenas);
    finalizeNow(fop, FINALIZE_OBJECT2, KEEP_ARENAS, &savedEmptyObjectArenas);
    finalizeNow(fop, FINALIZE_OBJECT4, KEEP_ARENAS, &savedEmptyObjectArenas);
    finalizeNow(fop, FINALIZE_OBJECT8, KEEP_ARENAS, &savedEmptyObjectArenas);
    finalizeNow(fop, FINALIZE_OBJECT12, KEEP_ARENAS, &savedEmptyObjectArenas);
    finalizeNow(fop, FINALIZE_OBJECT16, KEEP_ARENAS, &savedEmptyObjectArenas);

    
    
    
    savedObjectArenas[FINALIZE_OBJECT0] = arenaLists[FINALIZE_OBJECT0].copyAndClear();
    savedObjectArenas[FINALIZE_OBJECT2] = arenaLists[FINALIZE_OBJECT2].copyAndClear();
    savedObjectArenas[FINALIZE_OBJECT4] = arenaLists[FINALIZE_OBJECT4].copyAndClear();
    savedObjectArenas[FINALIZE_OBJECT8] = arenaLists[FINALIZE_OBJECT8].copyAndClear();
    savedObjectArenas[FINALIZE_OBJECT12] = arenaLists[FINALIZE_OBJECT12].copyAndClear();
    savedObjectArenas[FINALIZE_OBJECT16] = arenaLists[FINALIZE_OBJECT16].copyAndClear();
}

void
ArenaLists::mergeForegroundSweptObjectArenas()
{
    AutoLockGC lock(runtime_);
    ReleaseArenaList(runtime_, savedEmptyObjectArenas, lock);
    savedEmptyObjectArenas = nullptr;

    mergeSweptArenas(FINALIZE_OBJECT0);
    mergeSweptArenas(FINALIZE_OBJECT2);
    mergeSweptArenas(FINALIZE_OBJECT4);
    mergeSweptArenas(FINALIZE_OBJECT8);
    mergeSweptArenas(FINALIZE_OBJECT12);
    mergeSweptArenas(FINALIZE_OBJECT16);
}

inline void
ArenaLists::mergeSweptArenas(AllocKind thingKind)
{
    ArenaList *al = &arenaLists[thingKind];
    ArenaList *saved = &savedObjectArenas[thingKind];

    *al = saved->insertListWithCursorAtEnd(*al);
    saved->clear();
}

void
ArenaLists::queueForegroundThingsForSweep(FreeOp *fop)
{
    gcShapeArenasToUpdate = arenaListsToSweep[FINALIZE_SHAPE];
    gcAccessorShapeArenasToUpdate = arenaListsToSweep[FINALIZE_ACCESSOR_SHAPE];
    gcTypeObjectArenasToUpdate = arenaListsToSweep[FINALIZE_TYPE_OBJECT];
    gcScriptArenasToUpdate = arenaListsToSweep[FINALIZE_SCRIPT];
}

static void *
RunLastDitchGC(JSContext *cx, JS::Zone *zone, AllocKind thingKind)
{
    PrepareZoneForGC(zone);

    JSRuntime *rt = cx->runtime();

    
    AutoKeepAtoms keepAtoms(cx->perThreadData);
    rt->gc.gc(GC_NORMAL, JS::gcreason::LAST_DITCH);

    




    size_t thingSize = Arena::thingSize(thingKind);
    return zone->arenas.allocateFromFreeList(thingKind, thingSize);
}

template <AllowGC allowGC>
 void *
GCRuntime::refillFreeListFromMainThread(JSContext *cx, AllocKind thingKind)
{
    JSRuntime *rt = cx->runtime();
    MOZ_ASSERT(!rt->isHeapBusy(), "allocating while under GC");
    MOZ_ASSERT_IF(allowGC, !rt->currentThreadHasExclusiveAccess());

    ArenaLists *arenas = cx->arenas();
    Zone *zone = cx->zone();

    
    
    
    const bool mustCollectNow = allowGC && rt->gc.isIncrementalGCInProgress() &&
                                zone->usage.gcBytes() > zone->threshold.gcTriggerBytes();

    bool outOfMemory = false;  
    bool ranGC = false;  
    do {
        if (MOZ_UNLIKELY(mustCollectNow || outOfMemory)) {
            
            
            if (!allowGC) {
                MOZ_ASSERT(!mustCollectNow);
                return nullptr;
            }

            if (void *thing = RunLastDitchGC(cx, zone, thingKind))
                return thing;
            ranGC = true;
        }

        AutoMaybeStartBackgroundAllocation maybeStartBGAlloc;
        void *thing = arenas->allocateFromArena(zone, thingKind, maybeStartBGAlloc);
        if (MOZ_LIKELY(thing))
            return thing;

        
        
        
        
        rt->gc.waitBackgroundSweepEnd();

        thing = arenas->allocateFromArena(zone, thingKind, maybeStartBGAlloc);
        if (MOZ_LIKELY(thing))
            return thing;

        
        outOfMemory = true;
    } while (!ranGC);

    MOZ_ASSERT(allowGC, "A fallible allocation must not report OOM on failure.");
    js_ReportOutOfMemory(cx);
    return nullptr;
}

 void *
GCRuntime::refillFreeListOffMainThread(ExclusiveContext *cx, AllocKind thingKind)
{
    ArenaLists *arenas = cx->arenas();
    Zone *zone = cx->zone();
    JSRuntime *rt = zone->runtimeFromAnyThread();

    AutoMaybeStartBackgroundAllocation maybeStartBGAlloc;

    
    
    
    AutoLockHelperThreadState lock;
    while (rt->isHeapBusy())
        HelperThreadState().wait(GlobalHelperThreadState::PRODUCER);

    void *thing = arenas->allocateFromArena(zone, thingKind, maybeStartBGAlloc);
    if (thing)
        return thing;

    js_ReportOutOfMemory(cx);
    return nullptr;
}

template <AllowGC allowGC>
 void *
GCRuntime::refillFreeListFromAnyThread(ExclusiveContext *cx, AllocKind thingKind)
{
    MOZ_ASSERT(cx->arenas()->freeLists[thingKind].isEmpty());

    if (cx->isJSContext())
        return refillFreeListFromMainThread<allowGC>(cx->asJSContext(), thingKind);

    return refillFreeListOffMainThread(cx, thingKind);
}

template void *
GCRuntime::refillFreeListFromAnyThread<NoGC>(ExclusiveContext *cx, AllocKind thingKind);

template void *
GCRuntime::refillFreeListFromAnyThread<CanGC>(ExclusiveContext *cx, AllocKind thingKind);

 void *
GCRuntime::refillFreeListInGC(Zone *zone, AllocKind thingKind)
{
    



    MOZ_ASSERT(zone->arenas.freeLists[thingKind].isEmpty());
    mozilla::DebugOnly<JSRuntime *> rt = zone->runtimeFromMainThread();
    MOZ_ASSERT(rt->isHeapMajorCollecting());
    MOZ_ASSERT(!rt->gc.isBackgroundSweeping());

    return zone->arenas.allocateFromArena(zone, thingKind);
}

SliceBudget::SliceBudget()
{
    makeUnlimited();
}

SliceBudget::SliceBudget(TimeBudget time)
{
    if (time.budget < 0) {
        makeUnlimited();
    } else {
        
        deadline = PRMJ_Now() + time.budget * PRMJ_USEC_PER_MSEC;
        counter = CounterReset;
    }
}

SliceBudget::SliceBudget(WorkBudget work)
{
    if (work.budget < 0) {
        makeUnlimited();
    } else {
        deadline = 0;
        counter = work.budget;
    }
}

bool
SliceBudget::checkOverBudget()
{
    bool over = PRMJ_Now() >= deadline;
    if (!over)
        counter = CounterReset;
    return over;
}

void
js::MarkCompartmentActive(InterpreterFrame *fp)
{
    fp->script()->compartment()->zone()->active = true;
}

void
GCRuntime::requestMajorGC(JS::gcreason::Reason reason)
{
    if (majorGCRequested)
        return;

    majorGCRequested = true;
    majorGCTriggerReason = reason;
    rt->requestInterrupt(JSRuntime::RequestInterruptUrgent);
}

void
GCRuntime::requestMinorGC(JS::gcreason::Reason reason)
{
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(rt));
    if (minorGCRequested)
        return;

    minorGCRequested = true;
    minorGCTriggerReason = reason;
    rt->requestInterrupt(JSRuntime::RequestInterruptUrgent);
}

bool
GCRuntime::triggerGC(JS::gcreason::Reason reason)
{
    



    if (!CurrentThreadCanAccessRuntime(rt))
        return false;

    
    if (rt->isHeapCollecting())
        return false;

    JS::PrepareForFullGC(rt);
    requestMajorGC(reason);
    return true;
}

void
GCRuntime::maybeAllocTriggerZoneGC(Zone *zone, const AutoLockGC &lock)
{
    size_t usedBytes = zone->usage.gcBytes();
    size_t thresholdBytes = zone->threshold.gcTriggerBytes();
    size_t igcThresholdBytes = thresholdBytes * tunables.zoneAllocThresholdFactor();

    if (usedBytes >= thresholdBytes) {
        
        
        triggerZoneGC(zone, JS::gcreason::ALLOC_TRIGGER);
    } else if (usedBytes >= igcThresholdBytes && interFrameGC) {
        
        if (zone->gcDelayBytes < ArenaSize)
            zone->gcDelayBytes = 0;
        else
            zone->gcDelayBytes -= ArenaSize;

        if (!zone->gcDelayBytes) {
            
            
            
            
            triggerZoneGC(zone, JS::gcreason::ALLOC_TRIGGER);

            
            
            zone->gcDelayBytes = tunables.zoneAllocDelayBytes();
        }
    }
}

bool
GCRuntime::triggerZoneGC(Zone *zone, JS::gcreason::Reason reason)
{
    
    if (zone->usedByExclusiveThread)
        return false;

    
    if (rt->isHeapCollecting())
        return false;

#ifdef JS_GC_ZEAL
    if (zealMode == ZealAllocValue) {
        triggerGC(reason);
        return true;
    }
#endif

    if (rt->isAtomsZone(zone)) {
        
        triggerGC(reason);
        return true;
    }

    PrepareZoneForGC(zone);
    requestMajorGC(reason);
    return true;
}

bool
GCRuntime::maybeGC(Zone *zone)
{
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(rt));

#ifdef JS_GC_ZEAL
    if (zealMode == ZealAllocValue || zealMode == ZealPokeValue) {
        JS::PrepareForFullGC(rt);
        gc(GC_NORMAL, JS::gcreason::MAYBEGC);
        return true;
    }
#endif

    if (gcIfNeeded())
        return true;

    if (zone->usage.gcBytes() > 1024 * 1024 &&
        zone->threshold.isCloseToAllocTrigger(zone->usage, schedulingState.inHighFrequencyGCMode()) &&
        !isIncrementalGCInProgress() &&
        !isBackgroundSweeping())
    {
        PrepareZoneForGC(zone);
        startGC(GC_NORMAL, JS::gcreason::MAYBEGC);
        return true;
    }

    return false;
}

void
GCRuntime::maybePeriodicFullGC()
{
    








#ifndef JS_MORE_DETERMINISTIC
    int64_t now = PRMJ_Now();
    if (nextFullGCTime && nextFullGCTime <= now && !isIncrementalGCInProgress()) {
        if (chunkAllocationSinceLastGC ||
            numArenasFreeCommitted > decommitThreshold)
        {
            JS::PrepareForFullGC(rt);
            startGC(GC_SHRINK, JS::gcreason::MAYBEGC);
        } else {
            nextFullGCTime = now + GC_IDLE_FULL_SPAN;
        }
    }
#endif
}



void
GCRuntime::decommitAllWithoutUnlocking(const AutoLockGC &lock)
{
    MOZ_ASSERT(emptyChunks(lock).count() == 0);
    for (ChunkPool::Iter chunk(availableChunks(lock)); !chunk.done(); chunk.next()) {
        for (size_t i = 0; i < ArenasPerChunk; ++i) {
            if (chunk->decommittedArenas.get(i) || chunk->arenas[i].aheader.allocated())
                continue;

            if (MarkPagesUnused(&chunk->arenas[i], ArenaSize)) {
                chunk->info.numArenasFreeCommitted--;
                chunk->decommittedArenas.set(i);
            }
        }
    }
    MOZ_ASSERT(availableChunks(lock).verify());
}

void
GCRuntime::startDecommit()
{
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(rt));
    MOZ_ASSERT(!decommitTask.isRunningOutsideLock());

    {
        AutoLockGC lock(rt);

        
        for (ChunkPool::Iter chunk(emptyChunks(lock)); !chunk.done(); chunk.next())
            MOZ_ASSERT(!chunk->info.numArenasFreeCommitted);

        
        MOZ_ASSERT(availableChunks(lock).verify());
        ChunkVector toDecommit;
        for (ChunkPool::Iter iter(availableChunks(lock)); !iter.done(); iter.next()) {
            if (!toDecommit.append(iter.get())) {
                
                
                return onOutOfMallocMemory(lock);
            }
        }
        if (!decommitTask.setChunksToScan(toDecommit))
            return onOutOfMallocMemory(lock);
    }

    if (sweepOnBackgroundThread)
        decommitTask.start();
    else
        decommitTask.runFromMainThread(rt);
}

bool
BackgroundDecommitTask::setChunksToScan(const ChunkVector &chunks)
{
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(runtime));
    MOZ_ASSERT(toDecommit_.empty());
    return toDecommit_.appendAll(chunks);
}

 void
BackgroundDecommitTask::run()
{
    AutoLockGC lock(runtime);

    
    
    for (size_t i = toDecommit_.length(); i > 1; --i) {
        Chunk *chunk = toDecommit_[i - 1];
        MOZ_ASSERT(chunk);

        
        
        while (chunk->info.numArenasFreeCommitted) {
            ArenaHeader *aheader = chunk->allocateArena(runtime, nullptr, FINALIZE_OBJECT0, lock);
            bool ok;
            {
                AutoUnlockGC unlock(lock);
                ok = MarkPagesUnused(aheader->getArena(), ArenaSize);
            }
            chunk->releaseArena(runtime, aheader, lock, Chunk::ArenaDecommitState(ok));

            
            
            if (cancel_ || !ok)
                break;
        }
    }
    toDecommit_.clearAndFree();
}

void
GCRuntime::expireChunksAndArenas(bool shouldShrink, AutoLockGC &lock)
{
    ChunkPool toFree = expireEmptyChunkPool(shouldShrink, lock);
    if (toFree.count()) {
        AutoUnlockGC unlock(lock);
        FreeChunkPool(rt, toFree);
    }
}

void
GCRuntime::sweepBackgroundThings(ZoneList &zones, LifoAlloc &freeBlocks, ThreadType threadType)
{
    freeBlocks.freeAll();

    if (zones.isEmpty())
        return;

    
    ArenaHeader *emptyArenas = nullptr;
    FreeOp fop(rt, threadType);
    for (unsigned phase = 0 ; phase < ArrayLength(BackgroundFinalizePhases) ; ++phase) {
        for (Zone *zone = zones.front(); zone; zone = zone->nextZone()) {
            for (unsigned index = 0 ; index < BackgroundFinalizePhases[phase].length ; ++index) {
                AllocKind kind = BackgroundFinalizePhases[phase].kinds[index];
                ArenaHeader *arenas = zone->arenas.arenaListsToSweep[kind];
                if (arenas)
                    ArenaLists::backgroundFinalize(&fop, arenas, &emptyArenas);
            }
        }
    }

    AutoLockGC lock(rt);
    DecommitArenaList(rt, emptyArenas, lock);
    while (!zones.isEmpty())
        zones.removeFront();
}

void
GCRuntime::assertBackgroundSweepingFinished()
{
#ifdef DEBUG
    MOZ_ASSERT(backgroundSweepZones.isEmpty());
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        MOZ_ASSERT(!zone->isOnList());
        for (unsigned i = 0; i < FINALIZE_LIMIT; ++i) {
            MOZ_ASSERT(!zone->arenas.arenaListsToSweep[i]);
            MOZ_ASSERT(zone->arenas.doneBackgroundFinalize(AllocKind(i)));
        }
    }
    MOZ_ASSERT(freeLifoAlloc.computedSizeOfExcludingThis() == 0);
#endif
}

unsigned
js::GetCPUCount()
{
    static unsigned ncpus = 0;
    if (ncpus == 0) {
# ifdef XP_WIN
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        ncpus = unsigned(sysinfo.dwNumberOfProcessors);
# else
        long n = sysconf(_SC_NPROCESSORS_ONLN);
        ncpus = (n > 0) ? unsigned(n) : 1;
# endif
    }
    return ncpus;
}

bool
GCHelperState::init()
{
    if (!(done = PR_NewCondVar(rt->gc.lock)))
        return false;

    return true;
}

void
GCHelperState::finish()
{
    if (!rt->gc.lock) {
        MOZ_ASSERT(state_ == IDLE);
        return;
    }

    
    waitBackgroundSweepEnd();

    if (done)
        PR_DestroyCondVar(done);
}

GCHelperState::State
GCHelperState::state()
{
    MOZ_ASSERT(rt->gc.currentThreadOwnsGCLock());
    return state_;
}

void
GCHelperState::setState(State state)
{
    MOZ_ASSERT(rt->gc.currentThreadOwnsGCLock());
    state_ = state;
}

void
GCHelperState::startBackgroundThread(State newState)
{
    MOZ_ASSERT(!thread && state() == IDLE && newState != IDLE);
    setState(newState);

    if (!HelperThreadState().gcHelperWorklist().append(this))
        CrashAtUnhandlableOOM("Could not add to pending GC helpers list");
    HelperThreadState().notifyAll(GlobalHelperThreadState::PRODUCER);
}

void
GCHelperState::waitForBackgroundThread()
{
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(rt));

#ifdef DEBUG
    rt->gc.lockOwner = nullptr;
#endif
    PR_WaitCondVar(done, PR_INTERVAL_NO_TIMEOUT);
#ifdef DEBUG
    rt->gc.lockOwner = PR_GetCurrentThread();
#endif
}

void
GCHelperState::work()
{
    MOZ_ASSERT(CanUseExtraThreads());

    AutoLockGC lock(rt);

    MOZ_ASSERT(!thread);
    thread = PR_GetCurrentThread();

    TraceLoggerThread *logger = TraceLoggerForCurrentThread();

    switch (state()) {

      case IDLE:
        MOZ_CRASH("GC helper triggered on idle state");
        break;

      case SWEEPING: {
        AutoTraceLog logSweeping(logger, TraceLogger_GCSweeping);
        doSweep(lock);
        MOZ_ASSERT(state() == SWEEPING);
        break;
      }

    }

    setState(IDLE);
    thread = nullptr;

    PR_NotifyAllCondVar(done);
}

BackgroundAllocTask::BackgroundAllocTask(JSRuntime *rt, ChunkPool &pool)
  : runtime(rt),
    chunkPool_(pool),
    enabled_(CanUseExtraThreads() && GetCPUCount() >= 2)
{
}

 void
BackgroundAllocTask::run()
{
    TraceLoggerThread *logger = TraceLoggerForCurrentThread();
    AutoTraceLog logAllocation(logger, TraceLogger_GCAllocation);

    AutoLockGC lock(runtime);
    while (!cancel_ && runtime->gc.wantBackgroundAllocation(lock)) {
        Chunk *chunk;
        {
            AutoUnlockGC unlock(lock);
            chunk = Chunk::allocate(runtime);
            if (!chunk)
                break;
        }
        chunkPool_.push(chunk);
    }
}

void
GCRuntime::queueZonesForBackgroundSweep(ZoneList &zones)
{
    AutoLockHelperThreadState helperLock;
    AutoLockGC lock(rt);
    backgroundSweepZones.transferFrom(zones);
    helperState.maybeStartBackgroundSweep(lock);
}

void
GCRuntime::freeUnusedLifoBlocksAfterSweeping(LifoAlloc *lifo)
{
    MOZ_ASSERT(isHeapBusy());
    AutoLockGC lock(rt);
    freeLifoAlloc.transferUnusedFrom(lifo);
}

void
GCRuntime::freeAllLifoBlocksAfterSweeping(LifoAlloc *lifo)
{
    MOZ_ASSERT(isHeapBusy());
    AutoLockGC lock(rt);
    freeLifoAlloc.transferFrom(lifo);
}

void
GCHelperState::maybeStartBackgroundSweep(const AutoLockGC &lock)
{
    MOZ_ASSERT(CanUseExtraThreads());

    if (state() == IDLE)
        startBackgroundThread(SWEEPING);
}

void
GCHelperState::startBackgroundShrink(const AutoLockGC &lock)
{
    MOZ_ASSERT(CanUseExtraThreads());
    switch (state()) {
      case IDLE:
        shrinkFlag = true;
        startBackgroundThread(SWEEPING);
        break;
      case SWEEPING:
        shrinkFlag = true;
        break;
      default:
        MOZ_CRASH("Invalid GC helper thread state.");
    }
}

void
GCHelperState::waitBackgroundSweepEnd()
{
    AutoLockGC lock(rt);
    while (state() == SWEEPING)
        waitForBackgroundThread();
    if (!rt->gc.isIncrementalGCInProgress())
        rt->gc.assertBackgroundSweepingFinished();
}

void
GCHelperState::doSweep(AutoLockGC &lock)
{
    
    
    

    do {
        while (!rt->gc.backgroundSweepZones.isEmpty()) {
            ZoneList zones;
            zones.transferFrom(rt->gc.backgroundSweepZones);
            LifoAlloc freeLifoAlloc(JSRuntime::TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE);
            freeLifoAlloc.transferFrom(&rt->gc.freeLifoAlloc);

            AutoUnlockGC unlock(lock);
            rt->gc.sweepBackgroundThings(zones, freeLifoAlloc, BackgroundThread);
        }

        bool shrinking = shrinkFlag;
        shrinkFlag = false;
        rt->gc.expireChunksAndArenas(shrinking, lock);
    } while (!rt->gc.backgroundSweepZones.isEmpty() || shrinkFlag);
}

bool
GCHelperState::onBackgroundThread()
{
    return PR_GetCurrentThread() == thread;
}

bool
GCRuntime::shouldReleaseObservedTypes()
{
    bool releaseTypes = false;

#ifdef JS_GC_ZEAL
    if (zealMode != 0)
        releaseTypes = true;
#endif

    
    if (majorGCNumber >= jitReleaseNumber)
        releaseTypes = true;

    if (releaseTypes)
        jitReleaseNumber = majorGCNumber + JIT_SCRIPT_RELEASE_TYPES_PERIOD;

    return releaseTypes;
}










void
Zone::sweepCompartments(FreeOp *fop, bool keepAtleastOne, bool lastGC)
{
    JSRuntime *rt = runtimeFromMainThread();
    JSDestroyCompartmentCallback callback = rt->destroyCompartmentCallback;

    JSCompartment **read = compartments.begin();
    JSCompartment **end = compartments.end();
    JSCompartment **write = read;
    bool foundOne = false;
    while (read < end) {
        JSCompartment *comp = *read++;
        MOZ_ASSERT(!rt->isAtomsCompartment(comp));

        



        bool dontDelete = read == end && !foundOne && keepAtleastOne;
        if ((!comp->marked && !dontDelete) || lastGC) {
            if (callback)
                callback(fop, comp);
            if (comp->principals)
                JS_DropPrincipals(rt, comp->principals);
            js_delete(comp);
        } else {
            *write++ = comp;
            foundOne = true;
        }
    }
    compartments.resize(write - compartments.begin());
    MOZ_ASSERT_IF(keepAtleastOne, !compartments.empty());
}

void
GCRuntime::sweepZones(FreeOp *fop, bool lastGC)
{
    AutoLockGC lock(rt); 

    JSZoneCallback callback = rt->destroyZoneCallback;

    
    Zone **read = zones.begin() + 1;
    Zone **end = zones.end();
    Zone **write = read;
    MOZ_ASSERT(zones.length() >= 1);
    MOZ_ASSERT(rt->isAtomsZone(zones[0]));

    while (read < end) {
        Zone *zone = *read++;

        if (zone->wasGCStarted()) {
            if ((!zone->isQueuedForBackgroundSweep() &&
                 zone->arenas.arenaListsAreEmpty() &&
                 !zone->hasMarkedCompartments()) || lastGC)
            {
                zone->arenas.checkEmptyFreeLists();
                AutoUnlockGC unlock(lock);

                if (callback)
                    callback(zone);
                zone->sweepCompartments(fop, false, lastGC);
                MOZ_ASSERT(zone->compartments.empty());
                fop->delete_(zone);
                continue;
            }
            zone->sweepCompartments(fop, true, lastGC);
        }
        *write++ = zone;
    }
    zones.resize(write - zones.begin());
}

void
GCRuntime::purgeRuntime()
{
    for (GCCompartmentsIter comp(rt); !comp.done(); comp.next())
        comp->purge();

    freeUnusedLifoBlocksAfterSweeping(&rt->tempLifoAlloc);

    rt->interpreterStack().purge(rt);
    rt->gsnCache.purge();
    rt->scopeCoordinateNameCache.purge();
    rt->newObjectCache.purge();
    rt->nativeIterCache.purge();
    rt->uncompressedSourceCache.purge();
    rt->evalCache.clear();

    if (!rt->hasActiveCompilations())
        rt->parseMapPool().purgeAll();
}

bool
GCRuntime::shouldPreserveJITCode(JSCompartment *comp, int64_t currentTime,
                                 JS::gcreason::Reason reason)
{
    if (cleanUpEverything)
        return false;

    if (alwaysPreserveCode)
        return true;
    if (comp->preserveJitCode())
        return true;
    if (comp->lastAnimationTime + PRMJ_USEC_PER_SEC >= currentTime)
        return true;
    if (reason == JS::gcreason::DEBUG_GC)
        return true;

    return false;
}

#ifdef DEBUG
class CompartmentCheckTracer : public JSTracer
{
  public:
    CompartmentCheckTracer(JSRuntime *rt, JSTraceCallback callback)
      : JSTracer(rt, callback)
    {}

    Cell *src;
    JSGCTraceKind srcKind;
    Zone *zone;
    JSCompartment *compartment;
};

static bool
InCrossCompartmentMap(JSObject *src, Cell *dst, JSGCTraceKind dstKind)
{
    JSCompartment *srccomp = src->compartment();

    if (dstKind == JSTRACE_OBJECT) {
        Value key = ObjectValue(*static_cast<JSObject *>(dst));
        if (WrapperMap::Ptr p = srccomp->lookupWrapper(key)) {
            if (*p->value().unsafeGet() == ObjectValue(*src))
                return true;
        }
    }

    



    for (JSCompartment::WrapperEnum e(srccomp); !e.empty(); e.popFront()) {
        if (e.front().key().wrapped == dst && ToMarkable(e.front().value()) == src)
            return true;
    }

    return false;
}

static void
CheckCompartment(CompartmentCheckTracer *trc, JSCompartment *thingCompartment,
                 Cell *thing, JSGCTraceKind kind)
{
    MOZ_ASSERT(thingCompartment == trc->compartment ||
               trc->runtime()->isAtomsCompartment(thingCompartment) ||
               (trc->srcKind == JSTRACE_OBJECT &&
                InCrossCompartmentMap((JSObject *)trc->src, thing, kind)));
}

static JSCompartment *
CompartmentOfCell(Cell *thing, JSGCTraceKind kind)
{
    if (kind == JSTRACE_OBJECT)
        return static_cast<JSObject *>(thing)->compartment();
    else if (kind == JSTRACE_SHAPE)
        return static_cast<Shape *>(thing)->compartment();
    else if (kind == JSTRACE_BASE_SHAPE)
        return static_cast<BaseShape *>(thing)->compartment();
    else if (kind == JSTRACE_SCRIPT)
        return static_cast<JSScript *>(thing)->compartment();
    else
        return nullptr;
}

static void
CheckCompartmentCallback(JSTracer *trcArg, void **thingp, JSGCTraceKind kind)
{
    CompartmentCheckTracer *trc = static_cast<CompartmentCheckTracer *>(trcArg);
    TenuredCell *thing = TenuredCell::fromPointer(*thingp);

    JSCompartment *comp = CompartmentOfCell(thing, kind);
    if (comp && trc->compartment) {
        CheckCompartment(trc, comp, thing, kind);
    } else {
        MOZ_ASSERT(thing->zone() == trc->zone ||
                   trc->runtime()->isAtomsZone(thing->zone()));
    }
}

void
GCRuntime::checkForCompartmentMismatches()
{
    if (disableStrictProxyCheckingCount)
        return;

    CompartmentCheckTracer trc(rt, CheckCompartmentCallback);
    for (ZonesIter zone(rt, SkipAtoms); !zone.done(); zone.next()) {
        trc.zone = zone;
        for (size_t thingKind = 0; thingKind < FINALIZE_LAST; thingKind++) {
            for (ZoneCellIterUnderGC i(zone, AllocKind(thingKind)); !i.done(); i.next()) {
                trc.src = i.getCell();
                trc.srcKind = MapAllocToTraceKind(AllocKind(thingKind));
                trc.compartment = CompartmentOfCell(trc.src, trc.srcKind);
                JS_TraceChildren(&trc, trc.src, trc.srcKind);
            }
        }
    }
}
#endif

bool
GCRuntime::beginMarkPhase(JS::gcreason::Reason reason)
{
    int64_t currentTime = PRMJ_Now();

#ifdef DEBUG
    if (fullCompartmentChecks)
        checkForCompartmentMismatches();
#endif

    isFull = true;
    bool any = false;

    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        
        MOZ_ASSERT(!zone->isCollecting());
        MOZ_ASSERT(!zone->compartments.empty());
        for (unsigned i = 0; i < FINALIZE_LIMIT; ++i)
            MOZ_ASSERT(!zone->arenas.arenaListsToSweep[i]);

        
        if (zone->isGCScheduled()) {
            if (!rt->isAtomsZone(zone)) {
                any = true;
                zone->setGCState(Zone::Mark);
            }
        } else {
            isFull = false;
        }

        zone->setPreservingCode(false);
    }

    for (CompartmentsIter c(rt, WithAtoms); !c.done(); c.next()) {
        c->marked = false;
        c->scheduledForDestruction = false;
        c->maybeAlive = false;
        if (shouldPreserveJITCode(c, currentTime, reason))
            c->zone()->setPreservingCode(true);
    }

    if (!rt->gc.cleanUpEverything) {
        if (JSCompartment *comp = jit::TopmostIonActivationCompartment(rt))
            comp->zone()->setPreservingCode(true);
    }

    









    if (isFull && !rt->keepAtoms()) {
        Zone *atomsZone = rt->atomsCompartment()->zone();
        if (atomsZone->isGCScheduled()) {
            MOZ_ASSERT(!atomsZone->isCollecting());
            atomsZone->setGCState(Zone::Mark);
            any = true;
        }
    }

    
    if (!any)
        return false;

    






    if (isIncremental) {
        for (GCZonesIter zone(rt); !zone.done(); zone.next())
            zone->arenas.purge();
    }

    marker.start();
    MOZ_ASSERT(!marker.callback);
    MOZ_ASSERT(IS_GC_MARKING_TRACER(&marker));

    
    if (isIncremental) {
        for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
            gcstats::AutoPhase ap(stats, gcstats::PHASE_MARK_DISCARD_CODE);
            zone->discardJitCode(rt->defaultFreeOp());
        }
    }

    GCMarker *gcmarker = &marker;

    startNumber = number;

    








    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_PURGE);
        purgeRuntime();
    }

    


    gcstats::AutoPhase ap1(stats, gcstats::PHASE_MARK);

    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_UNMARK);

        for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
            
            zone->arenas.unmarkAll();
        }

        for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
            
            WeakMapBase::unmarkCompartment(c);
        }

        if (isFull)
            UnmarkScriptData(rt);
    }

    markRuntime(gcmarker, MarkRuntime);

    gcstats::AutoPhase ap2(stats, gcstats::PHASE_MARK_ROOTS);

    if (isIncremental)
        bufferGrayRoots();

    























    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_MARK_COMPARTMENTS);

        
        for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
            for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
                const CrossCompartmentKey &key = e.front().key();
                JSCompartment *dest;
                switch (key.kind) {
                  case CrossCompartmentKey::ObjectWrapper:
                  case CrossCompartmentKey::DebuggerObject:
                  case CrossCompartmentKey::DebuggerSource:
                  case CrossCompartmentKey::DebuggerEnvironment:
                    dest = static_cast<JSObject *>(key.wrapped)->compartment();
                    break;
                  case CrossCompartmentKey::DebuggerScript:
                    dest = static_cast<JSScript *>(key.wrapped)->compartment();
                    break;
                  default:
                    dest = nullptr;
                    break;
                }
                if (dest)
                    dest->maybeAlive = true;
            }
        }
    }

    




    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        if (!c->maybeAlive && !rt->isAtomsCompartment(c))
            c->scheduledForDestruction = true;
    }
    foundBlackGrayEdges = false;

    return true;
}

template <class CompartmentIterT>
void
GCRuntime::markWeakReferences(gcstats::Phase phase)
{
    MOZ_ASSERT(marker.isDrained());

    gcstats::AutoPhase ap1(stats, phase);

    for (;;) {
        bool markedAny = false;
        for (CompartmentIterT c(rt); !c.done(); c.next()) {
            markedAny |= WatchpointMap::markCompartmentIteratively(c, &marker);
            markedAny |= WeakMapBase::markCompartmentIteratively(c, &marker);
        }
        markedAny |= Debugger::markAllIteratively(&marker);

        if (!markedAny)
            break;

        SliceBudget budget;
        marker.drainMarkStack(budget);
    }
    MOZ_ASSERT(marker.isDrained());
}

void
GCRuntime::markWeakReferencesInCurrentGroup(gcstats::Phase phase)
{
    markWeakReferences<GCCompartmentGroupIter>(phase);
}

template <class ZoneIterT, class CompartmentIterT>
void
GCRuntime::markGrayReferences(gcstats::Phase phase)
{
    gcstats::AutoPhase ap(stats, phase);
    if (marker.hasBufferedGrayRoots()) {
        for (ZoneIterT zone(rt); !zone.done(); zone.next())
            marker.markBufferedGrayRoots(zone);
    } else {
        MOZ_ASSERT(!isIncremental);
        if (JSTraceDataOp op = grayRootTracer.op)
            (*op)(&marker, grayRootTracer.data);
    }
    SliceBudget budget;
    marker.drainMarkStack(budget);
}

void
GCRuntime::markGrayReferencesInCurrentGroup(gcstats::Phase phase)
{
    markGrayReferences<GCZoneGroupIter, GCCompartmentGroupIter>(phase);
}

void
GCRuntime::markAllWeakReferences(gcstats::Phase phase)
{
    markWeakReferences<GCCompartmentsIter>(phase);
}

void
GCRuntime::markAllGrayReferences(gcstats::Phase phase)
{
    markGrayReferences<GCZonesIter, GCCompartmentsIter>(phase);
}

#ifdef DEBUG

class js::gc::MarkingValidator
{
  public:
    explicit MarkingValidator(GCRuntime *gc);
    ~MarkingValidator();
    void nonIncrementalMark();
    void validate();

  private:
    GCRuntime *gc;
    bool initialized;

    typedef HashMap<Chunk *, ChunkBitmap *, GCChunkHasher, SystemAllocPolicy> BitmapMap;
    BitmapMap map;
};

#endif 

#ifdef JS_GC_MARKING_VALIDATION

js::gc::MarkingValidator::MarkingValidator(GCRuntime *gc)
  : gc(gc),
    initialized(false)
{}

js::gc::MarkingValidator::~MarkingValidator()
{
    if (!map.initialized())
        return;

    for (BitmapMap::Range r(map.all()); !r.empty(); r.popFront())
        js_delete(r.front().value());
}

void
js::gc::MarkingValidator::nonIncrementalMark()
{
    






    if (!map.init())
        return;

    JSRuntime *runtime = gc->rt;
    GCMarker *gcmarker = &gc->marker;

    gc->waitBackgroundSweepEnd();

    
    for (auto chunk = gc->allNonEmptyChunks(); !chunk.done(); chunk.next()) {
        ChunkBitmap *bitmap = &chunk->bitmap;
	ChunkBitmap *entry = js_new<ChunkBitmap>();
        if (!entry)
            return;

        memcpy((void *)entry->bitmap, (void *)bitmap->bitmap, sizeof(bitmap->bitmap));
        if (!map.putNew(chunk, entry))
            return;
    }

    




    WeakMapSet markedWeakMaps;
    if (!markedWeakMaps.init())
        return;

    for (GCCompartmentsIter c(runtime); !c.done(); c.next()) {
        if (!WeakMapBase::saveCompartmentMarkedWeakMaps(c, markedWeakMaps))
            return;
    }

    



    initialized = true;

    
    js::gc::State state = gc->incrementalState;
    gc->incrementalState = MARK_ROOTS;

    {
        gcstats::AutoPhase ap(gc->stats, gcstats::PHASE_MARK);

        {
            gcstats::AutoPhase ap(gc->stats, gcstats::PHASE_UNMARK);

            for (GCCompartmentsIter c(runtime); !c.done(); c.next())
                WeakMapBase::unmarkCompartment(c);

            MOZ_ASSERT(gcmarker->isDrained());
            gcmarker->reset();

            for (auto chunk = gc->allNonEmptyChunks(); !chunk.done(); chunk.next())
                chunk->bitmap.clear();
        }

        gc->markRuntime(gcmarker, GCRuntime::MarkRuntime, GCRuntime::UseSavedRoots);

        SliceBudget budget;
        gc->incrementalState = MARK;
        gc->marker.drainMarkStack(budget);
    }

    gc->incrementalState = SWEEP;
    {
        gcstats::AutoPhase ap1(gc->stats, gcstats::PHASE_SWEEP);
        gcstats::AutoPhase ap2(gc->stats, gcstats::PHASE_SWEEP_MARK);
        gc->markAllWeakReferences(gcstats::PHASE_SWEEP_MARK_WEAK);

        
        for (GCZonesIter zone(runtime); !zone.done(); zone.next()) {
            MOZ_ASSERT(zone->isGCMarkingBlack());
            zone->setGCState(Zone::MarkGray);
        }
        gc->marker.setMarkColorGray();

        gc->markAllGrayReferences(gcstats::PHASE_SWEEP_MARK_GRAY);
        gc->markAllWeakReferences(gcstats::PHASE_SWEEP_MARK_GRAY_WEAK);

        
        for (GCZonesIter zone(runtime); !zone.done(); zone.next()) {
            MOZ_ASSERT(zone->isGCMarkingGray());
            zone->setGCState(Zone::Mark);
        }
        MOZ_ASSERT(gc->marker.isDrained());
        gc->marker.setMarkColorBlack();
    }

    
    for (auto chunk = gc->allNonEmptyChunks(); !chunk.done(); chunk.next()) {
        ChunkBitmap *bitmap = &chunk->bitmap;
        ChunkBitmap *entry = map.lookup(chunk)->value();
        Swap(*entry, *bitmap);
    }

    for (GCCompartmentsIter c(runtime); !c.done(); c.next())
        WeakMapBase::unmarkCompartment(c);
    WeakMapBase::restoreCompartmentMarkedWeakMaps(markedWeakMaps);

    gc->incrementalState = state;
}

void
js::gc::MarkingValidator::validate()
{
    




    if (!initialized)
        return;

    gc->waitBackgroundSweepEnd();

    for (auto chunk = gc->allNonEmptyChunks(); !chunk.done(); chunk.next()) {
        BitmapMap::Ptr ptr = map.lookup(chunk);
        if (!ptr)
            continue;  

        ChunkBitmap *bitmap = ptr->value();
        ChunkBitmap *incBitmap = &chunk->bitmap;

        for (size_t i = 0; i < ArenasPerChunk; i++) {
            if (chunk->decommittedArenas.get(i))
                continue;
            Arena *arena = &chunk->arenas[i];
            if (!arena->aheader.allocated())
                continue;
            if (!arena->aheader.zone->isGCSweeping())
                continue;
            if (arena->aheader.allocatedDuringIncremental)
                continue;

            AllocKind kind = arena->aheader.getAllocKind();
            uintptr_t thing = arena->thingsStart(kind);
            uintptr_t end = arena->thingsEnd();
            while (thing < end) {
                Cell *cell = (Cell *)thing;

                



                MOZ_ASSERT_IF(bitmap->isMarked(cell, BLACK), incBitmap->isMarked(cell, BLACK));

                




                MOZ_ASSERT_IF(!bitmap->isMarked(cell, GRAY), !incBitmap->isMarked(cell, GRAY));

                thing += Arena::thingSize(kind);
            }
        }
    }
}

#endif 

void
GCRuntime::computeNonIncrementalMarkingForValidation()
{
#ifdef JS_GC_MARKING_VALIDATION
    MOZ_ASSERT(!markingValidator);
    if (isIncremental && validate)
        markingValidator = js_new<MarkingValidator>(this);
    if (markingValidator)
        markingValidator->nonIncrementalMark();
#endif
}

void
GCRuntime::validateIncrementalMarking()
{
#ifdef JS_GC_MARKING_VALIDATION
    if (markingValidator)
        markingValidator->validate();
#endif
}

void
GCRuntime::finishMarkingValidation()
{
#ifdef JS_GC_MARKING_VALIDATION
    js_delete(markingValidator);
    markingValidator = nullptr;
#endif
}

static void
AssertNeedsBarrierFlagsConsistent(JSRuntime *rt)
{
#ifdef JS_GC_MARKING_VALIDATION
    bool anyNeedsBarrier = false;
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next())
        anyNeedsBarrier |= zone->needsIncrementalBarrier();
    MOZ_ASSERT(rt->needsIncrementalBarrier() == anyNeedsBarrier);
#endif
}

static void
DropStringWrappers(JSRuntime *rt)
{
    




    for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            if (e.front().key().kind == CrossCompartmentKey::StringWrapper)
                e.removeFront();
        }
    }
}
















void
JSCompartment::findOutgoingEdges(ComponentFinder<JS::Zone> &finder)
{
    for (js::WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront()) {
        CrossCompartmentKey::Kind kind = e.front().key().kind;
        MOZ_ASSERT(kind != CrossCompartmentKey::StringWrapper);
        TenuredCell &other = e.front().key().wrapped->asTenured();
        if (kind == CrossCompartmentKey::ObjectWrapper) {
            




            if (!other.isMarked(BLACK) || other.isMarked(GRAY)) {
                JS::Zone *w = other.zone();
                if (w->isGCMarking())
                    finder.addEdgeTo(w);
            }
        } else {
            MOZ_ASSERT(kind == CrossCompartmentKey::DebuggerScript ||
                       kind == CrossCompartmentKey::DebuggerSource ||
                       kind == CrossCompartmentKey::DebuggerObject ||
                       kind == CrossCompartmentKey::DebuggerEnvironment);
            




            JS::Zone *w = other.zone();
            if (w->isGCMarking())
                finder.addEdgeTo(w);
        }
    }

    Debugger::findCompartmentEdges(zone(), finder);
}

void
Zone::findOutgoingEdges(ComponentFinder<JS::Zone> &finder)
{
    



    JSRuntime *rt = runtimeFromMainThread();
    if (rt->atomsCompartment()->zone()->isGCMarking())
        finder.addEdgeTo(rt->atomsCompartment()->zone());

    for (CompartmentsInZoneIter comp(this); !comp.done(); comp.next())
        comp->findOutgoingEdges(finder);

    for (ZoneSet::Range r = gcZoneGroupEdges.all(); !r.empty(); r.popFront()) {
        if (r.front()->isGCMarking())
            finder.addEdgeTo(r.front());
    }
    gcZoneGroupEdges.clear();
}

bool
GCRuntime::findZoneEdgesForWeakMaps()
{
    









    for (GCCompartmentsIter comp(rt); !comp.done(); comp.next()) {
        if (!WeakMapBase::findZoneEdgesForCompartment(comp))
            return false;
    }

    return true;
}

void
GCRuntime::findZoneGroups()
{
    ComponentFinder<Zone> finder(rt->mainThread.nativeStackLimit[StackForSystemCode]);
    if (!isIncremental || !findZoneEdgesForWeakMaps())
        finder.useOneComponent();

    for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
        MOZ_ASSERT(zone->isGCMarking());
        finder.addNode(zone);
    }
    zoneGroups = finder.getResultsList();
    currentZoneGroup = zoneGroups;
    zoneGroupIndex = 0;

    for (Zone *head = currentZoneGroup; head; head = head->nextGroup()) {
        for (Zone *zone = head; zone; zone = zone->nextNodeInGroup())
            MOZ_ASSERT(zone->isGCMarking());
    }

    MOZ_ASSERT_IF(!isIncremental, !currentZoneGroup->nextGroup());
}

static void
ResetGrayList(JSCompartment* comp);

void
GCRuntime::getNextZoneGroup()
{
    currentZoneGroup = currentZoneGroup->nextGroup();
    ++zoneGroupIndex;
    if (!currentZoneGroup) {
        abortSweepAfterCurrentGroup = false;
        return;
    }

    for (Zone *zone = currentZoneGroup; zone; zone = zone->nextNodeInGroup()) {
        MOZ_ASSERT(zone->isGCMarking());
        MOZ_ASSERT(!zone->isQueuedForBackgroundSweep());
    }

    if (!isIncremental)
        ComponentFinder<Zone>::mergeGroups(currentZoneGroup);

    if (abortSweepAfterCurrentGroup) {
        MOZ_ASSERT(!isIncremental);
        for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
            MOZ_ASSERT(!zone->gcNextGraphComponent);
            MOZ_ASSERT(zone->isGCMarking());
            zone->setNeedsIncrementalBarrier(false, Zone::UpdateJit);
            zone->setGCState(Zone::NoGC);
            zone->gcGrayRoots.clearAndFree();
        }
        rt->setNeedsIncrementalBarrier(false);
        AssertNeedsBarrierFlagsConsistent(rt);

        for (GCCompartmentGroupIter comp(rt); !comp.done(); comp.next())
            ResetGrayList(comp);

        abortSweepAfterCurrentGroup = false;
        currentZoneGroup = nullptr;
    }
}



























static bool
IsGrayListObject(JSObject *obj)
{
    MOZ_ASSERT(obj);
    return obj->is<CrossCompartmentWrapperObject>() && !IsDeadProxyObject(obj);
}

 unsigned
ProxyObject::grayLinkExtraSlot(JSObject *obj)
{
    MOZ_ASSERT(IsGrayListObject(obj));
    return 1;
}

#ifdef DEBUG
static void
AssertNotOnGrayList(JSObject *obj)
{
    MOZ_ASSERT_IF(IsGrayListObject(obj),
                  GetProxyExtra(obj, ProxyObject::grayLinkExtraSlot(obj)).isUndefined());
}
#endif

static JSObject *
CrossCompartmentPointerReferent(JSObject *obj)
{
    MOZ_ASSERT(IsGrayListObject(obj));
    return &obj->as<ProxyObject>().private_().toObject();
}

static JSObject *
NextIncomingCrossCompartmentPointer(JSObject *prev, bool unlink)
{
    unsigned slot = ProxyObject::grayLinkExtraSlot(prev);
    JSObject *next = GetProxyExtra(prev, slot).toObjectOrNull();
    MOZ_ASSERT_IF(next, IsGrayListObject(next));

    if (unlink)
        SetProxyExtra(prev, slot, UndefinedValue());

    return next;
}

void
js::DelayCrossCompartmentGrayMarking(JSObject *src)
{
    MOZ_ASSERT(IsGrayListObject(src));

    
    unsigned slot = ProxyObject::grayLinkExtraSlot(src);
    JSObject *dest = CrossCompartmentPointerReferent(src);
    JSCompartment *comp = dest->compartment();

    if (GetProxyExtra(src, slot).isUndefined()) {
        SetProxyExtra(src, slot, ObjectOrNullValue(comp->gcIncomingGrayPointers));
        comp->gcIncomingGrayPointers = src;
    } else {
        MOZ_ASSERT(GetProxyExtra(src, slot).isObjectOrNull());
    }

#ifdef DEBUG
    



    JSObject *obj = comp->gcIncomingGrayPointers;
    bool found = false;
    while (obj) {
        if (obj == src)
            found = true;
        obj = NextIncomingCrossCompartmentPointer(obj, false);
    }
    MOZ_ASSERT(found);
#endif
}

static void
MarkIncomingCrossCompartmentPointers(JSRuntime *rt, const uint32_t color)
{
    MOZ_ASSERT(color == BLACK || color == GRAY);

    static const gcstats::Phase statsPhases[] = {
        gcstats::PHASE_SWEEP_MARK_INCOMING_BLACK,
        gcstats::PHASE_SWEEP_MARK_INCOMING_GRAY
    };
    gcstats::AutoPhase ap1(rt->gc.stats, statsPhases[color]);

    bool unlinkList = color == GRAY;

    for (GCCompartmentGroupIter c(rt); !c.done(); c.next()) {
        MOZ_ASSERT_IF(color == GRAY, c->zone()->isGCMarkingGray());
        MOZ_ASSERT_IF(color == BLACK, c->zone()->isGCMarkingBlack());
        MOZ_ASSERT_IF(c->gcIncomingGrayPointers, IsGrayListObject(c->gcIncomingGrayPointers));

        for (JSObject *src = c->gcIncomingGrayPointers;
             src;
             src = NextIncomingCrossCompartmentPointer(src, unlinkList))
        {
            JSObject *dst = CrossCompartmentPointerReferent(src);
            MOZ_ASSERT(dst->compartment() == c);

            if (color == GRAY) {
                if (IsObjectMarked(&src) && src->asTenured().isMarked(GRAY))
                    MarkGCThingUnbarriered(&rt->gc.marker, (void**)&dst,
                                           "cross-compartment gray pointer");
            } else {
                if (IsObjectMarked(&src) && !src->asTenured().isMarked(GRAY))
                    MarkGCThingUnbarriered(&rt->gc.marker, (void**)&dst,
                                           "cross-compartment black pointer");
            }
        }

        if (unlinkList)
            c->gcIncomingGrayPointers = nullptr;
    }

    SliceBudget budget;
    rt->gc.marker.drainMarkStack(budget);
}

static bool
RemoveFromGrayList(JSObject *wrapper)
{
    if (!IsGrayListObject(wrapper))
        return false;

    unsigned slot = ProxyObject::grayLinkExtraSlot(wrapper);
    if (GetProxyExtra(wrapper, slot).isUndefined())
        return false;  

    JSObject *tail = GetProxyExtra(wrapper, slot).toObjectOrNull();
    SetProxyExtra(wrapper, slot, UndefinedValue());

    JSCompartment *comp = CrossCompartmentPointerReferent(wrapper)->compartment();
    JSObject *obj = comp->gcIncomingGrayPointers;
    if (obj == wrapper) {
        comp->gcIncomingGrayPointers = tail;
        return true;
    }

    while (obj) {
        unsigned slot = ProxyObject::grayLinkExtraSlot(obj);
        JSObject *next = GetProxyExtra(obj, slot).toObjectOrNull();
        if (next == wrapper) {
            SetProxyExtra(obj, slot, ObjectOrNullValue(tail));
            return true;
        }
        obj = next;
    }

    MOZ_CRASH("object not found in gray link list");
}

static void
ResetGrayList(JSCompartment *comp)
{
    JSObject *src = comp->gcIncomingGrayPointers;
    while (src)
        src = NextIncomingCrossCompartmentPointer(src, true);
    comp->gcIncomingGrayPointers = nullptr;
}

void
js::NotifyGCNukeWrapper(JSObject *obj)
{
    



    RemoveFromGrayList(obj);
}

enum {
    JS_GC_SWAP_OBJECT_A_REMOVED = 1 << 0,
    JS_GC_SWAP_OBJECT_B_REMOVED = 1 << 1
};

unsigned
js::NotifyGCPreSwap(JSObject *a, JSObject *b)
{
    




    return (RemoveFromGrayList(a) ? JS_GC_SWAP_OBJECT_A_REMOVED : 0) |
           (RemoveFromGrayList(b) ? JS_GC_SWAP_OBJECT_B_REMOVED : 0);
}

void
js::NotifyGCPostSwap(JSObject *a, JSObject *b, unsigned removedFlags)
{
    



    if (removedFlags & JS_GC_SWAP_OBJECT_A_REMOVED)
        DelayCrossCompartmentGrayMarking(b);
    if (removedFlags & JS_GC_SWAP_OBJECT_B_REMOVED)
        DelayCrossCompartmentGrayMarking(a);
}

void
GCRuntime::endMarkingZoneGroup()
{
    gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP_MARK);

    




    MarkIncomingCrossCompartmentPointers(rt, BLACK);

    markWeakReferencesInCurrentGroup(gcstats::PHASE_SWEEP_MARK_WEAK);

    





    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        MOZ_ASSERT(zone->isGCMarkingBlack());
        zone->setGCState(Zone::MarkGray);
    }
    marker.setMarkColorGray();

    
    MarkIncomingCrossCompartmentPointers(rt, GRAY);

    
    markGrayReferencesInCurrentGroup(gcstats::PHASE_SWEEP_MARK_GRAY);
    markWeakReferencesInCurrentGroup(gcstats::PHASE_SWEEP_MARK_GRAY_WEAK);

    
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        MOZ_ASSERT(zone->isGCMarkingGray());
        zone->setGCState(Zone::Mark);
    }
    MOZ_ASSERT(marker.isDrained());
    marker.setMarkColorBlack();
}

#define MAKE_GC_PARALLEL_TASK(name) \
    class name : public GCParallelTask {\
        JSRuntime *runtime;\
        virtual void run() MOZ_OVERRIDE;\
      public:\
        explicit name (JSRuntime *rt) : runtime(rt) {}\
    }
MAKE_GC_PARALLEL_TASK(SweepAtomsTask);
MAKE_GC_PARALLEL_TASK(SweepInnerViewsTask);
MAKE_GC_PARALLEL_TASK(SweepCCWrappersTask);
MAKE_GC_PARALLEL_TASK(SweepBaseShapesTask);
MAKE_GC_PARALLEL_TASK(SweepInitialShapesTask);
MAKE_GC_PARALLEL_TASK(SweepTypeObjectsTask);
MAKE_GC_PARALLEL_TASK(SweepRegExpsTask);
MAKE_GC_PARALLEL_TASK(SweepMiscTask);

 void
SweepAtomsTask::run()
{
    runtime->sweepAtoms();
}

 void
SweepInnerViewsTask::run()
{
    for (GCCompartmentGroupIter c(runtime); !c.done(); c.next())
        c->sweepInnerViews();
}

 void
SweepCCWrappersTask::run()
{
    for (GCCompartmentGroupIter c(runtime); !c.done(); c.next())
        c->sweepCrossCompartmentWrappers();
}

 void
SweepBaseShapesTask::run()
{
    for (GCCompartmentGroupIter c(runtime); !c.done(); c.next())
        c->sweepBaseShapeTable();
}

 void
SweepInitialShapesTask::run()
{
    for (GCCompartmentGroupIter c(runtime); !c.done(); c.next())
        c->sweepInitialShapeTable();
}

 void
SweepTypeObjectsTask::run()
{
    for (GCCompartmentGroupIter c(runtime); !c.done(); c.next())
        c->sweepTypeObjectTables();
}

 void
SweepRegExpsTask::run()
{
    for (GCCompartmentGroupIter c(runtime); !c.done(); c.next())
        c->sweepRegExps();
}

 void
SweepMiscTask::run()
{
    for (GCCompartmentGroupIter c(runtime); !c.done(); c.next()) {
        c->sweepCallsiteClones();
        c->sweepSavedStacks();
        c->sweepSelfHostingScriptSource();
        c->sweepNativeIterators();
    }
}

void
GCRuntime::startTask(GCParallelTask &task, gcstats::Phase phase)
{
    if (!task.startWithLockHeld()) {
        gcstats::AutoPhase ap(stats, phase);
        task.runFromMainThread(rt);
    }
}

void
GCRuntime::joinTask(GCParallelTask &task, gcstats::Phase phase)
{
    gcstats::AutoPhase ap(stats, task, phase);
    task.joinWithLockHeld();
}

void
GCRuntime::beginSweepingZoneGroup()
{
    




    bool sweepingAtoms = false;
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        
        MOZ_ASSERT(zone->isGCMarking());
        zone->setGCState(Zone::Sweep);

        
        zone->arenas.purge();

        if (rt->isAtomsZone(zone))
            sweepingAtoms = true;

        if (rt->sweepZoneCallback)
            rt->sweepZoneCallback(zone);

        zone->gcLastZoneGroupIndex = zoneGroupIndex;
    }

    validateIncrementalMarking();

    FreeOp fop(rt);
    SweepAtomsTask sweepAtomsTask(rt);
    SweepInnerViewsTask sweepInnerViewsTask(rt);
    SweepCCWrappersTask sweepCCWrappersTask(rt);
    SweepBaseShapesTask sweepBaseShapesTask(rt);
    SweepInitialShapesTask sweepInitialShapesTask(rt);
    SweepTypeObjectsTask sweepTypeObjectsTask(rt);
    SweepRegExpsTask sweepRegExpsTask(rt);
    SweepMiscTask sweepMiscTask(rt);

    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_FINALIZE_START);
        callFinalizeCallbacks(&fop, JSFINALIZE_GROUP_START);
        callWeakPointerCallbacks();
    }

    if (sweepingAtoms) {
        AutoLockHelperThreadState helperLock;
        startTask(sweepAtomsTask, gcstats::PHASE_SWEEP_ATOMS);
    }

    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP_COMPARTMENTS);
        gcstats::AutoSCC scc(stats, zoneGroupIndex);

        {
            AutoLockHelperThreadState helperLock;
            startTask(sweepInnerViewsTask, gcstats::PHASE_SWEEP_INNER_VIEWS);
            startTask(sweepCCWrappersTask, gcstats::PHASE_SWEEP_CC_WRAPPER);
            startTask(sweepBaseShapesTask, gcstats::PHASE_SWEEP_BASE_SHAPE);
            startTask(sweepInitialShapesTask, gcstats::PHASE_SWEEP_INITIAL_SHAPE);
            startTask(sweepTypeObjectsTask, gcstats::PHASE_SWEEP_TYPE_OBJECT);
            startTask(sweepRegExpsTask, gcstats::PHASE_SWEEP_REGEXP);
            startTask(sweepMiscTask, gcstats::PHASE_SWEEP_MISC);
        }

        
        
        {
            gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP_MISC);

            for (GCCompartmentGroupIter c(rt); !c.done(); c.next()) {
                c->sweepGlobalObject(&fop);
                c->sweepDebugScopes();
                c->sweepJitCompartment(&fop);
                c->sweepWeakMaps();
            }

            
            

            
            WatchpointMap::sweepAll(rt);

            
            Debugger::sweepAll(&fop);
        }

        {
            gcstats::AutoPhase apdc(stats, gcstats::PHASE_SWEEP_DISCARD_CODE);
            for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
                zone->discardJitCode(&fop);
            }
        }

        {
            gcstats::AutoPhase ap1(stats, gcstats::PHASE_SWEEP_TYPES);
            gcstats::AutoPhase ap2(stats, gcstats::PHASE_SWEEP_TYPES_BEGIN);
            for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
                zone->beginSweepTypes(&fop, releaseObservedTypes && !zone->isPreservingCode());
            }
        }

        {
            gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP_BREAKPOINT);
            for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
                zone->sweepBreakpoints(&fop);
            }
        }
    }

    if (sweepingAtoms) {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP_SYMBOL_REGISTRY);
        rt->symbolRegistry().sweep();
    }

    
    if (sweepingAtoms) {
        AutoLockHelperThreadState helperLock;
        joinTask(sweepAtomsTask, gcstats::PHASE_SWEEP_ATOMS);
    }

    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP_COMPARTMENTS);
        gcstats::AutoSCC scc(stats, zoneGroupIndex);

        AutoLockHelperThreadState helperLock;
        joinTask(sweepInnerViewsTask, gcstats::PHASE_SWEEP_INNER_VIEWS);
        joinTask(sweepCCWrappersTask, gcstats::PHASE_SWEEP_CC_WRAPPER);
        joinTask(sweepBaseShapesTask, gcstats::PHASE_SWEEP_BASE_SHAPE);
        joinTask(sweepInitialShapesTask, gcstats::PHASE_SWEEP_INITIAL_SHAPE);
        joinTask(sweepTypeObjectsTask, gcstats::PHASE_SWEEP_TYPE_OBJECT);
        joinTask(sweepRegExpsTask, gcstats::PHASE_SWEEP_REGEXP);
        joinTask(sweepMiscTask, gcstats::PHASE_SWEEP_MISC);
    }

    








    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        gcstats::AutoSCC scc(stats, zoneGroupIndex);
        zone->arenas.queueForegroundObjectsForSweep(&fop);
    }
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        gcstats::AutoSCC scc(stats, zoneGroupIndex);
        for (unsigned i = 0; i < ArrayLength(IncrementalFinalizePhases); ++i)
            zone->arenas.queueForForegroundSweep(&fop, IncrementalFinalizePhases[i]);
    }
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        gcstats::AutoSCC scc(stats, zoneGroupIndex);
        for (unsigned i = 0; i < ArrayLength(BackgroundFinalizePhases); ++i)
            zone->arenas.queueForBackgroundSweep(&fop, BackgroundFinalizePhases[i]);
    }
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        gcstats::AutoSCC scc(stats, zoneGroupIndex);
        zone->arenas.queueForegroundThingsForSweep(&fop);
    }

    sweepingTypes = true;

    finalizePhase = 0;
    sweepZone = currentZoneGroup;
    sweepKindIndex = 0;

    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_FINALIZE_END);
        callFinalizeCallbacks(&fop, JSFINALIZE_GROUP_END);
    }
}

void
GCRuntime::endSweepingZoneGroup()
{
    
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        MOZ_ASSERT(zone->isGCSweeping());
        zone->setGCState(Zone::Finished);
    }

    
    ZoneList zones;
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next())
        zones.append(zone);
    if (sweepOnBackgroundThread)
        queueZonesForBackgroundSweep(zones);
    else
        sweepBackgroundThings(zones, freeLifoAlloc, MainThread);

    
    while (ArenaHeader *arena = arenasAllocatedDuringSweep) {
        arenasAllocatedDuringSweep = arena->getNextAllocDuringSweep();
        arena->unsetAllocDuringSweep();
    }
}

void
GCRuntime::beginSweepPhase(bool lastGC)
{
    







    MOZ_ASSERT(!abortSweepAfterCurrentGroup);

    releaseHeldRelocatedArenas();

    computeNonIncrementalMarkingForValidation();

    gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP);

    sweepOnBackgroundThread =
        !lastGC && !TraceEnabled() && CanUseExtraThreads();

    releaseObservedTypes = shouldReleaseObservedTypes();

#ifdef DEBUG
    for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
        MOZ_ASSERT(!c->gcIncomingGrayPointers);
        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            if (e.front().key().kind != CrossCompartmentKey::StringWrapper)
                AssertNotOnGrayList(&e.front().value().get().toObject());
        }
    }
#endif

    DropStringWrappers(rt);
    findZoneGroups();
    endMarkingZoneGroup();
    beginSweepingZoneGroup();
}

bool
ArenaLists::foregroundFinalize(FreeOp *fop, AllocKind thingKind, SliceBudget &sliceBudget,
                               SortedArenaList &sweepList)
{
    if (!arenaListsToSweep[thingKind] && incrementalSweptArenas.isEmpty())
        return true;

    if (!FinalizeArenas(fop, &arenaListsToSweep[thingKind], sweepList,
                        thingKind, sliceBudget, RELEASE_ARENAS))
    {
        incrementalSweptArenaKind = thingKind;
        incrementalSweptArenas = sweepList.toArenaList();
        return false;
    }

    
    incrementalSweptArenas.clear();

    
    ArenaList finalized = sweepList.toArenaList();
    arenaLists[thingKind] = finalized.insertListWithCursorAtEnd(arenaLists[thingKind]);

    return true;
}

bool
GCRuntime::drainMarkStack(SliceBudget &sliceBudget, gcstats::Phase phase)
{
    
    gcstats::AutoPhase ap(stats, phase);
    return marker.drainMarkStack(sliceBudget);
}



static bool
AdvanceArenaList(ArenaHeader **list, AllocKind kind, SliceBudget &sliceBudget)
{
    *list = (*list)->next;
    sliceBudget.step(Arena::thingsPerArena(Arena::thingSize(kind)));
    return !sliceBudget.isOverBudget();
}

static bool
SweepShapes(ArenaHeader **arenasToSweep, AllocKind kind, SliceBudget &sliceBudget)
{
    while (ArenaHeader *arena = *arenasToSweep) {
        for (ArenaCellIterUnderGC i(arena); !i.done(); i.next()) {
            Shape *shape = i.get<Shape>();
            if (!shape->isMarked())
                shape->sweep();
        }

        if (!AdvanceArenaList(arenasToSweep, kind, sliceBudget))
            return false;
    }

    return true;
}

bool
GCRuntime::sweepPhase(SliceBudget &sliceBudget)
{
    gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP);
    FreeOp fop(rt);

    bool finished = drainMarkStack(sliceBudget, gcstats::PHASE_SWEEP_MARK);
    if (!finished)
        return false;

    for (;;) {
        
        
        
        
        
        
        
        if (sweepingTypes) {
            gcstats::AutoPhase ap1(stats, gcstats::PHASE_SWEEP_COMPARTMENTS);
            gcstats::AutoPhase ap2(stats, gcstats::PHASE_SWEEP_TYPES);

            for (; sweepZone; sweepZone = sweepZone->nextNodeInGroup()) {
                ArenaLists &al = sweepZone->arenas;

                types::AutoClearTypeInferenceStateOnOOM oom(sweepZone);

                while (ArenaHeader *arena = al.gcScriptArenasToUpdate) {
                    for (ArenaCellIterUnderGC i(arena); !i.done(); i.next()) {
                        JSScript *script = i.get<JSScript>();
                        script->maybeSweepTypes(&oom);
                    }

                    if (!AdvanceArenaList(&al.gcScriptArenasToUpdate,
                                          FINALIZE_SCRIPT, sliceBudget))
                    {
                        return false;
                    }
                }

                while (ArenaHeader *arena = al.gcTypeObjectArenasToUpdate) {
                    for (ArenaCellIterUnderGC i(arena); !i.done(); i.next()) {
                        types::TypeObject *object = i.get<types::TypeObject>();
                        object->maybeSweep(&oom);
                    }

                    if (!AdvanceArenaList(&al.gcTypeObjectArenasToUpdate,
                                          FINALIZE_TYPE_OBJECT, sliceBudget))
                    {
                        return false;
                    }
                }

                
                {
                    gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP_TYPES_END);
                    sweepZone->types.endSweep(rt);
                }

                
                
                
                al.mergeForegroundSweptObjectArenas();
            }

            sweepZone = currentZoneGroup;
            sweepingTypes = false;
        }

        
        for (; finalizePhase < ArrayLength(IncrementalFinalizePhases) ; ++finalizePhase) {
            gcstats::AutoPhase ap(stats, IncrementalFinalizePhases[finalizePhase].statsPhase);

            for (; sweepZone; sweepZone = sweepZone->nextNodeInGroup()) {
                Zone *zone = sweepZone;

                while (sweepKindIndex < IncrementalFinalizePhases[finalizePhase].length) {
                    AllocKind kind = IncrementalFinalizePhases[finalizePhase].kinds[sweepKindIndex];

                    
                    size_t thingsPerArena = Arena::thingsPerArena(Arena::thingSize(kind));
                    incrementalSweepList.setThingsPerArena(thingsPerArena);

                    if (!zone->arenas.foregroundFinalize(&fop, kind, sliceBudget,
                                                         incrementalSweepList))
                        return false;  

                    
                    incrementalSweepList.reset(thingsPerArena);

                    ++sweepKindIndex;
                }
                sweepKindIndex = 0;
            }
            sweepZone = currentZoneGroup;
        }

        
        {
            gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP_SHAPE);

            for (; sweepZone; sweepZone = sweepZone->nextNodeInGroup()) {
                Zone *zone = sweepZone;
                if (!SweepShapes(&zone->arenas.gcShapeArenasToUpdate, FINALIZE_SHAPE, sliceBudget))
                    return false;  
                if (!SweepShapes(&zone->arenas.gcAccessorShapeArenasToUpdate,
                                 FINALIZE_ACCESSOR_SHAPE, sliceBudget))
                {
                    return false;  
                }
            }
        }

        endSweepingZoneGroup();
        getNextZoneGroup();
        if (!currentZoneGroup)
            return true;  
        endMarkingZoneGroup();
        beginSweepingZoneGroup();
    }
}

void
GCRuntime::endSweepPhase(bool lastGC)
{
    gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP);
    FreeOp fop(rt);

    MOZ_ASSERT_IF(lastGC, !sweepOnBackgroundThread);

    



    if (isFull) {
        for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
            if (!zone->isCollecting()) {
                isFull = false;
                break;
            }
        }
    }

    





    if (foundBlackGrayEdges) {
        for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
            if (!zone->isCollecting())
                zone->arenas.unmarkAll();
        }
    }

    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_DESTROY);

        





        if (isFull)
            SweepScriptData(rt);

        
        if (jit::ExecutableAllocator *execAlloc = rt->maybeExecAlloc())
            execAlloc->purge();

        if (rt->jitRuntime() && rt->jitRuntime()->hasIonAlloc())
            rt->jitRuntime()->ionAlloc(rt)->purge();

        



        if (!lastGC)
            sweepZones(&fop, lastGC);
    }

    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_FINALIZE_END);
        callFinalizeCallbacks(&fop, JSFINALIZE_COLLECTION_END);

        
        if (isFull)
            grayBitsValid = true;
    }

    




    startDecommit();

    
    if (!sweepOnBackgroundThread) {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_DESTROY);

        assertBackgroundSweepingFinished();

        





        {
            AutoLockGC lock(rt);
            expireChunksAndArenas(invocationKind == GC_SHRINK, lock);
        }

        
        if (lastGC)
            sweepZones(&fop, lastGC);
    }

    finishMarkingValidation();

#ifdef DEBUG
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        for (unsigned i = 0 ; i < FINALIZE_LIMIT ; ++i) {
            MOZ_ASSERT_IF(!IsBackgroundFinalized(AllocKind(i)) ||
                          !sweepOnBackgroundThread,
                          !zone->arenas.arenaListsToSweep[i]);
        }
    }

    for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
        MOZ_ASSERT(!c->gcIncomingGrayPointers);

        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            if (e.front().key().kind != CrossCompartmentKey::StringWrapper)
                AssertNotOnGrayList(&e.front().value().unbarrieredGet().toObject());
        }
    }
#endif
}

bool
GCRuntime::compactPhase(bool lastGC)
{
    gcstats::AutoPhase ap(stats, gcstats::PHASE_COMPACT);

    if (isIncremental) {
        
        AutoLockGC lock(rt);
        if (isBackgroundSweeping())
            return false;
    } else {
        waitBackgroundSweepEnd();
    }

    MOZ_ASSERT(rt->gc.nursery.isEmpty());
    assertBackgroundSweepingFinished();

    ArenaHeader *relocatedList = relocateArenas();
    updatePointersToRelocatedCells();

#ifdef DEBUG
    for (ArenaHeader *arena = relocatedList; arena; arena = arena->next) {
        for (ArenaCellIterUnderFinalize i(arena); !i.done(); i.next()) {
            TenuredCell *src = i.getCell();
            MOZ_ASSERT(IsForwarded(src));
            TenuredCell *dest = Forwarded(src);
            MOZ_ASSERT(src->isMarked(BLACK) == dest->isMarked(BLACK));
            MOZ_ASSERT(src->isMarked(GRAY) == dest->isMarked(GRAY));
        }
    }
#endif

    
    
#ifndef DEBUG
    releaseRelocatedArenas(relocatedList);
#else
    protectRelocatedArenas(relocatedList);
    MOZ_ASSERT(!relocatedArenasToRelease);
    if (!lastGC)
        relocatedArenasToRelease = relocatedList;
    else
        releaseRelocatedArenas(relocatedList);
#endif

#ifdef DEBUG
    CheckHashTablesAfterMovingGC(rt);
    for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
        if (CanRelocateZone(rt, zone)) {
            MOZ_ASSERT(!zone->isPreservingCode());
            zone->arenas.checkEmptyFreeLists();

            
            
            for (size_t i = 0; i < FINALIZE_LIMIT; i++) {
                size_t thingsPerArena = Arena::thingsPerArena(Arena::thingSize(AllocKind(i)));
                if (CanRelocateAllocKind(AllocKind(i))) {
                    ArenaList &al = zone->arenas.arenaLists[i];
                    size_t freeCells = 0;
                    for (ArenaHeader *arena = al.arenaAfterCursor(); arena; arena = arena->next)
                        freeCells += arena->countFreeCells();
                    MOZ_ASSERT(freeCells < thingsPerArena);
                }
            }
        }
    }
#endif
    return true;
}

void
GCRuntime::finishCollection()
{
    MOZ_ASSERT(marker.isDrained());
    marker.stop();

    uint64_t currentTime = PRMJ_Now();
    schedulingState.updateHighFrequencyMode(lastGCTime, currentTime, tunables);

    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        if (zone->isCollecting()) {
            MOZ_ASSERT(zone->isGCFinished() || zone->isGCCompacting());
            zone->threshold.updateAfterGC(zone->usage.gcBytes(), invocationKind, tunables,
                                          schedulingState);
            zone->setGCState(Zone::NoGC);
            zone->active = false;
        }

        MOZ_ASSERT(!zone->isCollecting());
        MOZ_ASSERT(!zone->wasGCStarted());
    }

    lastGCTime = currentTime;
}


AutoTraceSession::AutoTraceSession(JSRuntime *rt, js::HeapState heapState)
  : lock(rt),
    runtime(rt),
    prevState(rt->gc.heapState)
{
    MOZ_ASSERT(rt->gc.isAllocAllowed());
    MOZ_ASSERT(rt->gc.heapState == Idle);
    MOZ_ASSERT(heapState != Idle);
    MOZ_ASSERT_IF(heapState == MajorCollecting, rt->gc.nursery.isEmpty());

    
    
    
    
    MOZ_ASSERT(rt->currentThreadHasExclusiveAccess());

    if (rt->exclusiveThreadsPresent()) {
        
        
        AutoLockHelperThreadState lock;
        rt->gc.heapState = heapState;
    } else {
        rt->gc.heapState = heapState;
    }
}

AutoTraceSession::~AutoTraceSession()
{
    MOZ_ASSERT(runtime->isHeapBusy());

    if (runtime->exclusiveThreadsPresent()) {
        AutoLockHelperThreadState lock;
        runtime->gc.heapState = prevState;

        
        HelperThreadState().notifyAll(GlobalHelperThreadState::PRODUCER);
    } else {
        runtime->gc.heapState = prevState;
    }
}

AutoCopyFreeListToArenas::AutoCopyFreeListToArenas(JSRuntime *rt, ZoneSelector selector)
  : runtime(rt),
    selector(selector)
{
    for (ZonesIter zone(rt, selector); !zone.done(); zone.next())
        zone->arenas.copyFreeListsToArenas();
}

AutoCopyFreeListToArenas::~AutoCopyFreeListToArenas()
{
    for (ZonesIter zone(runtime, selector); !zone.done(); zone.next())
        zone->arenas.clearFreeListsInArenas();
}

class AutoCopyFreeListToArenasForGC
{
    JSRuntime *runtime;

  public:
    explicit AutoCopyFreeListToArenasForGC(JSRuntime *rt) : runtime(rt) {
        MOZ_ASSERT(rt->currentThreadHasExclusiveAccess());
        for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next())
            zone->arenas.copyFreeListsToArenas();
    }
    ~AutoCopyFreeListToArenasForGC() {
        for (ZonesIter zone(runtime, WithAtoms); !zone.done(); zone.next())
            zone->arenas.clearFreeListsInArenas();
    }
};

void
GCRuntime::resetIncrementalGC(const char *reason)
{
    switch (incrementalState) {
      case NO_INCREMENTAL:
        return;

      case MARK: {
        
        AutoCopyFreeListToArenasForGC copy(rt);

        marker.reset();
        marker.stop();

        for (GCCompartmentsIter c(rt); !c.done(); c.next())
            ResetGrayList(c);

        for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
            MOZ_ASSERT(zone->isGCMarking());
            zone->setNeedsIncrementalBarrier(false, Zone::UpdateJit);
            zone->setGCState(Zone::NoGC);
        }
        rt->setNeedsIncrementalBarrier(false);
        AssertNeedsBarrierFlagsConsistent(rt);

        freeLifoAlloc.freeAll();

        incrementalState = NO_INCREMENTAL;

        MOZ_ASSERT(!marker.shouldCheckCompartments());

        break;
      }

      case SWEEP: {
        marker.reset();

        for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next())
            c->scheduledForDestruction = false;

        
        abortSweepAfterCurrentGroup = true;

        
        JSGCInvocationKind oldInvocationKind = invocationKind;
        invocationKind = GC_NORMAL;

        SliceBudget budget;
        incrementalCollectSlice(budget, JS::gcreason::RESET);

        invocationKind = oldInvocationKind;

        {
            gcstats::AutoPhase ap(stats, gcstats::PHASE_WAIT_BACKGROUND_THREAD);
            rt->gc.waitBackgroundSweepOrAllocEnd();
        }
        break;
      }

      case COMPACT: {
        {
            gcstats::AutoPhase ap(stats, gcstats::PHASE_WAIT_BACKGROUND_THREAD);
            rt->gc.waitBackgroundSweepOrAllocEnd();
        }

        JSGCInvocationKind oldInvocationKind = invocationKind;
        invocationKind = GC_NORMAL;

        SliceBudget budget;
        incrementalCollectSlice(budget, JS::gcreason::RESET);

        invocationKind = oldInvocationKind;
        break;
      }

      default:
        MOZ_CRASH("Invalid incremental GC state");
    }

    stats.reset(reason);

#ifdef DEBUG
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        MOZ_ASSERT(!zone->needsIncrementalBarrier());
        for (unsigned i = 0; i < FINALIZE_LIMIT; ++i)
            MOZ_ASSERT(!zone->arenas.arenaListsToSweep[i]);
    }
#endif
}

namespace {

class AutoGCSlice {
  public:
    explicit AutoGCSlice(JSRuntime *rt);
    ~AutoGCSlice();

  private:
    JSRuntime *runtime;
};

} 

AutoGCSlice::AutoGCSlice(JSRuntime *rt)
  : runtime(rt)
{
    





    for (ActivationIterator iter(rt); !iter.done(); ++iter)
        iter->compartment()->zone()->active = true;

    for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
        





        if (zone->isGCMarking()) {
            MOZ_ASSERT(zone->needsIncrementalBarrier());
            zone->setNeedsIncrementalBarrier(false, Zone::DontUpdateJit);
        } else {
            MOZ_ASSERT(!zone->needsIncrementalBarrier());
        }
    }
    rt->setNeedsIncrementalBarrier(false);
    AssertNeedsBarrierFlagsConsistent(rt);
}

AutoGCSlice::~AutoGCSlice()
{
    
    bool haveBarriers = false;
    for (ZonesIter zone(runtime, WithAtoms); !zone.done(); zone.next()) {
        if (zone->isGCMarking()) {
            zone->setNeedsIncrementalBarrier(true, Zone::UpdateJit);
            zone->arenas.prepareForIncrementalGC(runtime);
            haveBarriers = true;
        } else {
            zone->setNeedsIncrementalBarrier(false, Zone::UpdateJit);
        }
    }
    runtime->setNeedsIncrementalBarrier(haveBarriers);
    AssertNeedsBarrierFlagsConsistent(runtime);
}

void
GCRuntime::pushZealSelectedObjects()
{
#ifdef JS_GC_ZEAL
    
    for (JSObject **obj = selectedForMarking.begin(); obj != selectedForMarking.end(); obj++)
        MarkObjectUnbarriered(&marker, obj, "selected obj");
#endif
}

void
GCRuntime::incrementalCollectSlice(SliceBudget &budget, JS::gcreason::Reason reason)
{
    MOZ_ASSERT(rt->currentThreadHasExclusiveAccess());

    AutoCopyFreeListToArenasForGC copy(rt);
    AutoGCSlice slice(rt);

    bool lastGC = (reason == JS::gcreason::DESTROY_RUNTIME);

    gc::State initialState = incrementalState;

    int zeal = 0;
#ifdef JS_GC_ZEAL
    if (reason == JS::gcreason::DEBUG_GC && !budget.isUnlimited()) {
        




        zeal = zealMode;
    }
#endif

    MOZ_ASSERT_IF(isIncrementalGCInProgress(), isIncremental);
    isIncremental = !budget.isUnlimited();

    if (zeal == ZealIncrementalRootsThenFinish || zeal == ZealIncrementalMarkAllThenFinish) {
        



        budget.makeUnlimited();
    }

    if (incrementalState == NO_INCREMENTAL) {
        incrementalState = MARK_ROOTS;
        lastMarkSlice = false;
    }

    if (incrementalState == MARK)
        AutoGCRooter::traceAllWrappers(&marker);

    switch (incrementalState) {

      case MARK_ROOTS:
        if (!beginMarkPhase(reason)) {
            incrementalState = NO_INCREMENTAL;
            return;
        }

        if (!lastGC)
            pushZealSelectedObjects();

        incrementalState = MARK;

        if (isIncremental && zeal == ZealIncrementalRootsThenFinish)
            break;

        

      case MARK: {
        
        if (!marker.hasBufferedGrayRoots()) {
            budget.makeUnlimited();
            isIncremental = false;
        }

        bool finished = drainMarkStack(budget, gcstats::PHASE_MARK);
        if (!finished)
            break;

        MOZ_ASSERT(marker.isDrained());

        if (!lastMarkSlice && isIncremental &&
            ((initialState == MARK && zeal != ZealIncrementalRootsThenFinish) ||
             zeal == ZealIncrementalMarkAllThenFinish))
        {
            




            lastMarkSlice = true;
            break;
        }

        incrementalState = SWEEP;

        



        beginSweepPhase(lastGC);
        if (budget.isOverBudget())
            break;

        



        if (isIncremental && zeal == ZealIncrementalMultipleSlices)
            break;

        
      }

      case SWEEP:
        {
            bool finished = sweepPhase(budget);
            if (!finished)
                break;
        }

        endSweepPhase(lastGC);

        incrementalState = COMPACT;

        
        if (shouldCompact() && isIncremental)
            break;

      case COMPACT:
        if (shouldCompact()) {
            bool finished = compactPhase(lastGC);
            if (!finished)
                break;
        }

        finishCollection();
        incrementalState = NO_INCREMENTAL;
        break;

      default:
        MOZ_ASSERT(false);
    }
}

IncrementalSafety
gc::IsIncrementalGCSafe(JSRuntime *rt)
{
    MOZ_ASSERT(!rt->mainThread.suppressGC);

    if (rt->keepAtoms())
        return IncrementalSafety::Unsafe("keepAtoms set");

    if (!rt->gc.isIncrementalGCAllowed())
        return IncrementalSafety::Unsafe("incremental permanently disabled");

    return IncrementalSafety::Safe();
}

void
GCRuntime::budgetIncrementalGC(SliceBudget &budget)
{
    IncrementalSafety safe = IsIncrementalGCSafe(rt);
    if (!safe) {
        resetIncrementalGC(safe.reason());
        budget.makeUnlimited();
        stats.nonincremental(safe.reason());
        return;
    }

    if (mode != JSGC_MODE_INCREMENTAL) {
        resetIncrementalGC("GC mode change");
        budget.makeUnlimited();
        stats.nonincremental("GC mode");
        return;
    }

    if (isTooMuchMalloc()) {
        budget.makeUnlimited();
        stats.nonincremental("malloc bytes trigger");
    }

    bool reset = false;
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        if (zone->usage.gcBytes() >= zone->threshold.gcTriggerBytes()) {
            budget.makeUnlimited();
            stats.nonincremental("allocation trigger");
        }

        if (isIncrementalGCInProgress() && zone->isGCScheduled() != zone->wasGCStarted())
            reset = true;

        if (zone->isTooMuchMalloc()) {
            budget.makeUnlimited();
            stats.nonincremental("malloc bytes trigger");
        }
    }

    if (reset)
        resetIncrementalGC("zone change");
}

namespace {

class AutoDisableStoreBuffer
{
    StoreBuffer &sb;
    bool prior;

  public:
    explicit AutoDisableStoreBuffer(GCRuntime *gc) : sb(gc->storeBuffer) {
        prior = sb.isEnabled();
        sb.disable();
    }
    ~AutoDisableStoreBuffer() {
        if (prior)
            sb.enable();
    }
};

} 










MOZ_NEVER_INLINE bool
GCRuntime::gcCycle(bool incremental, SliceBudget &budget, JS::gcreason::Reason reason)
{
    evictNursery(reason);

    



    AutoDisableStoreBuffer adsb(this);

    AutoTraceSession session(rt, MajorCollecting);

    majorGCRequested = false;
    interFrameGC = true;

    number++;
    if (!isIncrementalGCInProgress())
        majorGCNumber++;

    
    
    MOZ_ASSERT(!rt->mainThread.suppressGC);

    
    JS::AutoAssertOnGC::VerifyIsSafeToGC(rt);

    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_WAIT_BACKGROUND_THREAD);

        
        
        
        if (!isIncrementalGCInProgress()) {
            waitBackgroundSweepEnd();
            decommitTask.cancel(GCParallelTask::CancelAndWait);
        }

        
        
        
        
        allocTask.cancel(GCParallelTask::CancelAndWait);
    }

    State prevState = incrementalState;

    if (!incremental) {
        
        
        
        
        if (reason != JS::gcreason::ALLOC_TRIGGER)
            resetIncrementalGC("requested");

        stats.nonincremental("requested");
        budget.makeUnlimited();
    } else {
        budgetIncrementalGC(budget);
    }

    
    if (prevState != NO_INCREMENTAL && !isIncrementalGCInProgress())
        return true;

    TraceMajorGCStart();

    incrementalCollectSlice(budget, reason);

#ifndef JS_MORE_DETERMINISTIC
    nextFullGCTime = PRMJ_Now() + GC_IDLE_FULL_SPAN;
#endif

    chunkAllocationSinceLastGC = false;

#ifdef JS_GC_ZEAL
    
    clearSelectedForMarking();
#endif

    
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        zone->resetGCMallocBytes();
        zone->unscheduleGC();
    }

    resetMallocBytes();

    TraceMajorGCEnd();

    return false;
}

#ifdef JS_GC_ZEAL
static bool
IsDeterministicGCReason(JS::gcreason::Reason reason)
{
    if (reason > JS::gcreason::DEBUG_GC &&
        reason != JS::gcreason::CC_FORCED && reason != JS::gcreason::SHUTDOWN_CC)
    {
        return false;
    }

    if (reason == JS::gcreason::MAYBEGC)
        return false;

    return true;
}
#endif

static bool
ShouldCleanUpEverything(JS::gcreason::Reason reason, JSGCInvocationKind gckind)
{
    
    
    
    return reason == JS::gcreason::DESTROY_RUNTIME ||
           reason == JS::gcreason::SHUTDOWN_CC ||
           gckind == GC_SHRINK;
}

gcstats::ZoneGCStats
GCRuntime::scanZonesBeforeGC()
{
    gcstats::ZoneGCStats zoneStats;
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        if (mode == JSGC_MODE_GLOBAL)
            zone->scheduleGC();

        
        if (isIncrementalGCInProgress() && zone->needsIncrementalBarrier())
            zone->scheduleGC();

        
        if (zone->threshold.isCloseToAllocTrigger(zone->usage, schedulingState.inHighFrequencyGCMode()))
            zone->scheduleGC();

        zoneStats.zoneCount++;
        if (zone->isGCScheduled()) {
            zoneStats.collectedZoneCount++;
            zoneStats.collectedCompartmentCount += zone->compartments.length();
        }
    }

    for (CompartmentsIter c(rt, WithAtoms); !c.done(); c.next())
        zoneStats.compartmentCount++;

    return zoneStats;
}

void
GCRuntime::collect(bool incremental, SliceBudget budget, JS::gcreason::Reason reason)
{
    JS_AbortIfWrongThread(rt);

    
    MOZ_ALWAYS_TRUE(!rt->isHeapBusy());

    
    MOZ_ASSERT(!rt->currentThreadHasExclusiveAccess());

    if (rt->mainThread.suppressGC)
        return;

    TraceLoggerThread *logger = TraceLoggerForMainThread(rt);
    AutoTraceLog logGC(logger, TraceLogger_GC);

#ifdef JS_GC_ZEAL
    if (deterministicOnly && !IsDeterministicGCReason(reason))
        return;
#endif

    AutoStopVerifyingBarriers av(rt, reason == JS::gcreason::SHUTDOWN_CC ||
                                     reason == JS::gcreason::DESTROY_RUNTIME);

    gcstats::AutoGCSlice agc(stats, scanZonesBeforeGC(), invocationKind, reason);

    cleanUpEverything = ShouldCleanUpEverything(reason, invocationKind);

    bool repeat = false;
    do {
        



        if (!isIncrementalGCInProgress()) {
            gcstats::AutoPhase ap(stats, gcstats::PHASE_GC_BEGIN);
            if (gcCallback.op)
                gcCallback.op(rt, JSGC_BEGIN, gcCallback.data);
        }

        poked = false;
        bool wasReset = gcCycle(incremental, budget, reason);

        if (!isIncrementalGCInProgress()) {
            gcstats::AutoPhase ap(stats, gcstats::PHASE_GC_END);
            if (gcCallback.op)
                gcCallback.op(rt, JSGC_END, gcCallback.data);
        }

        
        if (poked && cleanUpEverything)
            JS::PrepareForFullGC(rt);

        




        bool repeatForDeadZone = false;
        if (incremental && !isIncrementalGCInProgress()) {
            for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
                if (c->scheduledForDestruction) {
                    incremental = false;
                    repeatForDeadZone = true;
                    reason = JS::gcreason::COMPARTMENT_REVIVED;
                    c->zone()->scheduleGC();
                }
            }
        }

        





        repeat = (poked && cleanUpEverything) || wasReset || repeatForDeadZone;
    } while (repeat);

    if (!isIncrementalGCInProgress())
        EnqueuePendingParseTasksAfterGC(rt);
}

SliceBudget
GCRuntime::defaultBudget(JS::gcreason::Reason reason, int64_t millis)
{
    if (millis == 0) {
        if (reason == JS::gcreason::ALLOC_TRIGGER)
            millis = sliceBudget;
        else if (schedulingState.inHighFrequencyGCMode() && tunables.isDynamicMarkSliceEnabled())
            millis = sliceBudget * IGC_MARK_SLICE_MULTIPLIER;
        else
            millis = sliceBudget;
    }

    return SliceBudget(TimeBudget(millis));
}

void
GCRuntime::gc(JSGCInvocationKind gckind, JS::gcreason::Reason reason)
{
    invocationKind = gckind;
    collect(false, SliceBudget(), reason);
}

void
GCRuntime::startGC(JSGCInvocationKind gckind, JS::gcreason::Reason reason, int64_t millis)
{
    MOZ_ASSERT(!isIncrementalGCInProgress());
    invocationKind = gckind;
    collect(true, defaultBudget(reason, millis), reason);
}

void
GCRuntime::gcSlice(JS::gcreason::Reason reason, int64_t millis)
{
    MOZ_ASSERT(isIncrementalGCInProgress());
    collect(true, defaultBudget(reason, millis), reason);
}

void
GCRuntime::finishGC(JS::gcreason::Reason reason)
{
    MOZ_ASSERT(isIncrementalGCInProgress());
    collect(true, SliceBudget(), reason);
}

void
GCRuntime::notifyDidPaint()
{
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(rt));

#ifdef JS_GC_ZEAL
    if (zealMode == ZealFrameVerifierPreValue) {
        verifyPreBarriers();
        return;
    }

    if (zealMode == ZealFrameVerifierPostValue) {
        verifyPostBarriers();
        return;
    }

    if (zealMode == ZealFrameGCValue) {
        JS::PrepareForFullGC(rt);
        gc(GC_NORMAL, JS::gcreason::REFRESH_FRAME);
        return;
    }
#endif

    if (isIncrementalGCInProgress() && !interFrameGC) {
        JS::PrepareForIncrementalGC(rt);
        gcSlice(JS::gcreason::REFRESH_FRAME);
    }

    interFrameGC = false;
}

static bool
ZonesSelected(JSRuntime *rt)
{
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        if (zone->isGCScheduled())
            return true;
    }
    return false;
}

void
GCRuntime::startDebugGC(JSGCInvocationKind gckind, SliceBudget &budget)
{
    MOZ_ASSERT(!isIncrementalGCInProgress());
    if (!ZonesSelected(rt))
        JS::PrepareForFullGC(rt);
    invocationKind = gckind;
    collect(true, budget, JS::gcreason::DEBUG_GC);
}

void
GCRuntime::debugGCSlice(SliceBudget &budget)
{
    MOZ_ASSERT(isIncrementalGCInProgress());
    if (!ZonesSelected(rt))
        JS::PrepareForIncrementalGC(rt);
    collect(true, budget, JS::gcreason::DEBUG_GC);
}


void
js::PrepareForDebugGC(JSRuntime *rt)
{
    if (!ZonesSelected(rt))
        JS::PrepareForFullGC(rt);
}

JS_PUBLIC_API(void)
JS::ShrinkGCBuffers(JSRuntime *rt)
{
    rt->gc.shrinkBuffers();
}

void
GCRuntime::shrinkBuffers()
{
    AutoLockHelperThreadState helperLock;
    AutoLockGC lock(rt);
    MOZ_ASSERT(!rt->isHeapBusy());

    if (CanUseExtraThreads())
        helperState.startBackgroundShrink(lock);
    else
        expireChunksAndArenas(true, lock);
}

void
GCRuntime::onOutOfMallocMemory()
{
    
    allocTask.cancel(GCParallelTask::CancelAndWait);

    AutoLockGC lock(rt);
    onOutOfMallocMemory(lock);
}

void
GCRuntime::onOutOfMallocMemory(const AutoLockGC &lock)
{
    
    
#ifdef DEBUG
    unprotectRelocatedArenas(relocatedArenasToRelease);
    releaseRelocatedArenasWithoutUnlocking(relocatedArenasToRelease, lock);
    relocatedArenasToRelease = nullptr;
#endif

    
    freeEmptyChunks(rt, lock);

    
    
    
    decommitAllWithoutUnlocking(lock);
}

void
GCRuntime::minorGCImpl(JS::gcreason::Reason reason, Nursery::TypeObjectList *pretenureTypes)
{
    minorGCRequested = false;
    TraceLoggerThread *logger = TraceLoggerForMainThread(rt);
    AutoTraceLog logMinorGC(logger, TraceLogger_MinorGC);
    nursery.collect(rt, reason, pretenureTypes);
    MOZ_ASSERT_IF(!rt->mainThread.suppressGC, nursery.isEmpty());
}



void
GCRuntime::minorGC(JSContext *cx, JS::gcreason::Reason reason)
{
    gcstats::AutoPhase ap(stats, gcstats::PHASE_MINOR_GC);

    Nursery::TypeObjectList pretenureTypes;
    minorGCImpl(reason, &pretenureTypes);
    for (size_t i = 0; i < pretenureTypes.length(); i++) {
        if (pretenureTypes[i]->canPreTenure())
            pretenureTypes[i]->setShouldPreTenure(cx);
    }
}

void
GCRuntime::disableGenerationalGC()
{
    if (isGenerationalGCEnabled()) {
        minorGC(JS::gcreason::API);
        nursery.disable();
        storeBuffer.disable();
    }
    ++rt->gc.generationalDisabled;
}

void
GCRuntime::enableGenerationalGC()
{
    MOZ_ASSERT(generationalDisabled > 0);
    --generationalDisabled;
    if (generationalDisabled == 0) {
        nursery.enable();
        storeBuffer.enable();
    }
}

bool
GCRuntime::gcIfNeeded(JSContext *cx )
{
    

    if (minorGCRequested) {
        if (cx)
            minorGC(cx, minorGCTriggerReason);
        else
            minorGC(minorGCTriggerReason);
    }

    if (majorGCRequested) {
        if (!isIncrementalGCInProgress())
            startGC(GC_NORMAL, majorGCTriggerReason);
        else
            gcSlice(majorGCTriggerReason);
        return true;
    }

    return false;
}

AutoFinishGC::AutoFinishGC(JSRuntime *rt)
{
    if (JS::IsIncrementalGCInProgress(rt)) {
        JS::PrepareForIncrementalGC(rt);
        JS::FinishIncrementalGC(rt, JS::gcreason::API);
    }

    rt->gc.waitBackgroundSweepEnd();
}

AutoPrepareForTracing::AutoPrepareForTracing(JSRuntime *rt, ZoneSelector selector)
  : finish(rt),
    session(rt),
    copy(rt, selector)
{
}

JSCompartment *
js::NewCompartment(JSContext *cx, Zone *zone, JSPrincipals *principals,
                   const JS::CompartmentOptions &options)
{
    JSRuntime *rt = cx->runtime();
    JS_AbortIfWrongThread(rt);

    ScopedJSDeletePtr<Zone> zoneHolder;
    if (!zone) {
        zone = cx->new_<Zone>(rt);
        if (!zone)
            return nullptr;

        zoneHolder.reset(zone);

        const JSPrincipals *trusted = rt->trustedPrincipals();
        bool isSystem = principals && principals == trusted;
        if (!zone->init(isSystem))
            return nullptr;
    }

    ScopedJSDeletePtr<JSCompartment> compartment(cx->new_<JSCompartment>(zone, options));
    if (!compartment || !compartment->init(cx))
        return nullptr;

    
    JS_SetCompartmentPrincipals(compartment, principals);

    AutoLockGC lock(rt);

    if (!zone->compartments.append(compartment.get())) {
        js_ReportOutOfMemory(cx);
        return nullptr;
    }

    if (zoneHolder && !rt->gc.zones.append(zone)) {
        js_ReportOutOfMemory(cx);
        return nullptr;
    }

    zoneHolder.forget();
    return compartment.forget();
}

void
gc::MergeCompartments(JSCompartment *source, JSCompartment *target)
{
    
    
    MOZ_ASSERT(source->options_.mergeable());

    MOZ_ASSERT(source->addonId == target->addonId);

    JSRuntime *rt = source->runtimeFromMainThread();

    AutoPrepareForTracing prepare(rt, SkipAtoms);

    
    

    source->clearTables();
    source->unsetIsDebuggee();

    
    
    rt->gc.releaseHeldRelocatedArenas();

    
    

    for (ZoneCellIter iter(source->zone(), FINALIZE_SCRIPT); !iter.done(); iter.next()) {
        JSScript *script = iter.get<JSScript>();
        MOZ_ASSERT(script->compartment() == source);
        script->compartment_ = target;
        script->setTypesGeneration(target->zone()->types.generation);
    }

    for (ZoneCellIter iter(source->zone(), FINALIZE_BASE_SHAPE); !iter.done(); iter.next()) {
        BaseShape *base = iter.get<BaseShape>();
        MOZ_ASSERT(base->compartment() == source);
        base->compartment_ = target;
    }

    for (ZoneCellIter iter(source->zone(), FINALIZE_TYPE_OBJECT); !iter.done(); iter.next()) {
        types::TypeObject *type = iter.get<types::TypeObject>();
        type->setGeneration(target->zone()->types.generation);
    }

    

    for (size_t thingKind = 0; thingKind != FINALIZE_LIMIT; thingKind++) {
        for (ArenaIter aiter(source->zone(), AllocKind(thingKind)); !aiter.done(); aiter.next()) {
            ArenaHeader *aheader = aiter.get();
            aheader->zone = target->zone();
        }
    }

    
    for (CompartmentsInZoneIter c(source->zone()); !c.done(); c.next())
        MOZ_ASSERT(c.get() == source);

    
    target->zone()->arenas.adoptArenas(rt, &source->zone()->arenas);
    target->zone()->usage.adopt(source->zone()->usage);

    
    target->zone()->types.typeLifoAlloc.transferFrom(&source->zone()->types.typeLifoAlloc);
}

void
GCRuntime::runDebugGC()
{
#ifdef JS_GC_ZEAL
    int type = zealMode;

    if (rt->mainThread.suppressGC)
        return;

    if (type == js::gc::ZealGenerationalGCValue)
        return minorGC(JS::gcreason::DEBUG_GC);

    PrepareForDebugGC(rt);

    SliceBudget budget;
    if (type == ZealIncrementalRootsThenFinish ||
        type == ZealIncrementalMarkAllThenFinish ||
        type == ZealIncrementalMultipleSlices)
    {
        js::gc::State initialState = incrementalState;
        if (type == ZealIncrementalMultipleSlices) {
            




            if (!isIncrementalGCInProgress())
                incrementalLimit = zealFrequency / 2;
            else
                incrementalLimit *= 2;
            budget = SliceBudget(WorkBudget(incrementalLimit));
        } else {
            
            budget = SliceBudget(WorkBudget(1));
        }

        if (!isIncrementalGCInProgress())
            invocationKind = GC_NORMAL;
        collect(true, budget, JS::gcreason::DEBUG_GC);

        



        if (type == ZealIncrementalMultipleSlices &&
            initialState == MARK && incrementalState == SWEEP)
        {
            incrementalLimit = zealFrequency / 2;
        }
    } else if (type == ZealCompactValue) {
        gc(GC_SHRINK, JS::gcreason::DEBUG_GC);
    } else {
        gc(GC_NORMAL, JS::gcreason::DEBUG_GC);
    }

#endif
}

void
GCRuntime::setValidate(bool enabled)
{
    MOZ_ASSERT(!isHeapMajorCollecting());
    validate = enabled;
}

void
GCRuntime::setFullCompartmentChecks(bool enabled)
{
    MOZ_ASSERT(!isHeapMajorCollecting());
    fullCompartmentChecks = enabled;
}

#ifdef JS_GC_ZEAL
bool
GCRuntime::selectForMarking(JSObject *object)
{
    MOZ_ASSERT(!isHeapMajorCollecting());
    return selectedForMarking.append(object);
}

void
GCRuntime::clearSelectedForMarking()
{
    selectedForMarking.clearAndFree();
}

void
GCRuntime::setDeterministic(bool enabled)
{
    MOZ_ASSERT(!isHeapMajorCollecting());
    deterministicOnly = enabled;
}
#endif

#ifdef DEBUG


void PreventGCDuringInteractiveDebug()
{
    TlsPerThreadData.get()->suppressGC++;
}

#endif

void
js::ReleaseAllJITCode(FreeOp *fop)
{
    



    fop->runtime()->gc.evictNursery();

    for (ZonesIter zone(fop->runtime(), SkipAtoms); !zone.done(); zone.next()) {
        if (!zone->jitZone())
            continue;

#ifdef DEBUG
        
        for (ZoneCellIter i(zone, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            MOZ_ASSERT_IF(script->hasBaselineScript(), !script->baselineScript()->active());
        }
#endif

        
        jit::MarkActiveBaselineScripts(zone);

        jit::InvalidateAll(fop, zone);

        for (ZoneCellIter i(zone, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            jit::FinishInvalidation(fop, script);

            



            jit::FinishDiscardBaselineScript(fop, script);
        }

        zone->jitZone()->optimizedStubSpace()->free();
    }
}

void
js::PurgeJITCaches(Zone *zone)
{
    for (ZoneCellIterUnderGC i(zone, FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();

        
        jit::PurgeCaches(script);
    }
}

void
ArenaLists::normalizeBackgroundFinalizeState(AllocKind thingKind)
{
    ArenaLists::BackgroundFinalizeState *bfs = &backgroundFinalizeState[thingKind];
    switch (*bfs) {
      case BFS_DONE:
        break;
      default:
        MOZ_ASSERT(!"Background finalization in progress, but it should not be.");
        break;
    }
}

void
ArenaLists::adoptArenas(JSRuntime *rt, ArenaLists *fromArenaLists)
{
    
    AutoLockGC lock(rt);

    fromArenaLists->purge();

    for (size_t thingKind = 0; thingKind != FINALIZE_LIMIT; thingKind++) {
        
        
        
        normalizeBackgroundFinalizeState(AllocKind(thingKind));
        fromArenaLists->normalizeBackgroundFinalizeState(AllocKind(thingKind));

        ArenaList *fromList = &fromArenaLists->arenaLists[thingKind];
        ArenaList *toList = &arenaLists[thingKind];
        fromList->check();
        toList->check();
        ArenaHeader *next;
        for (ArenaHeader *fromHeader = fromList->head(); fromHeader; fromHeader = next) {
            
            next = fromHeader->next;

            MOZ_ASSERT(!fromHeader->isEmpty());
            toList->insertAtCursor(fromHeader);
        }
        fromList->clear();
        toList->check();
    }
}

bool
ArenaLists::containsArena(JSRuntime *rt, ArenaHeader *needle)
{
    AutoLockGC lock(rt);
    size_t allocKind = needle->getAllocKind();
    for (ArenaHeader *aheader = arenaLists[allocKind].head(); aheader; aheader = aheader->next) {
        if (aheader == needle)
            return true;
    }
    return false;
}


AutoSuppressGC::AutoSuppressGC(ExclusiveContext *cx)
  : suppressGC_(cx->perThreadData->suppressGC)
{
    suppressGC_++;
}

AutoSuppressGC::AutoSuppressGC(JSCompartment *comp)
  : suppressGC_(comp->runtimeFromMainThread()->mainThread.suppressGC)
{
    suppressGC_++;
}

AutoSuppressGC::AutoSuppressGC(JSRuntime *rt)
  : suppressGC_(rt->mainThread.suppressGC)
{
    suppressGC_++;
}

bool
js::UninlinedIsInsideNursery(const gc::Cell *cell)
{
    return IsInsideNursery(cell);
}

#ifdef DEBUG
AutoDisableProxyCheck::AutoDisableProxyCheck(JSRuntime *rt
                                             MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : gc(rt->gc)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    gc.disableStrictProxyChecking();
}

AutoDisableProxyCheck::~AutoDisableProxyCheck()
{
    gc.enableStrictProxyChecking();
}

JS_FRIEND_API(void)
JS::AssertGCThingMustBeTenured(JSObject *obj)
{
    MOZ_ASSERT(obj->isTenured() &&
               (!IsNurseryAllocable(obj->asTenured().getAllocKind()) || obj->getClass()->finalize));
}

JS_FRIEND_API(void)
js::gc::AssertGCThingHasType(js::gc::Cell *cell, JSGCTraceKind kind)
{
    if (!cell)
        MOZ_ASSERT(kind == JSTRACE_NULL);
    else if (IsInsideNursery(cell))
        MOZ_ASSERT(kind == JSTRACE_OBJECT);
    else
        MOZ_ASSERT(MapAllocToTraceKind(cell->asTenured().getAllocKind()) == kind);
}

JS_PUBLIC_API(size_t)
JS::GetGCNumber()
{
    JSRuntime *rt = js::TlsPerThreadData.get()->runtimeFromMainThread();
    if (!rt)
        return 0;
    return rt->gc.gcNumber();
}
#endif

#ifdef DEBUG
JS::AutoAssertOnGC::AutoAssertOnGC()
  : gc(nullptr), gcNumber(0)
{
    js::PerThreadData *data = js::TlsPerThreadData.get();
    if (data) {
        





        JSRuntime *runtime = data->runtimeIfOnOwnerThread();
        if (runtime) {
            gc = &runtime->gc;
            gcNumber = gc->gcNumber();
            gc->enterUnsafeRegion();
        }
    }
}

JS::AutoAssertOnGC::AutoAssertOnGC(JSRuntime *rt)
  : gc(&rt->gc), gcNumber(rt->gc.gcNumber())
{
    gc->enterUnsafeRegion();
}

JS::AutoAssertOnGC::~AutoAssertOnGC()
{
    if (gc) {
        gc->leaveUnsafeRegion();

        



        MOZ_ASSERT(gcNumber == gc->gcNumber(), "GC ran inside an AutoAssertOnGC scope.");
    }
}

 void
JS::AutoAssertOnGC::VerifyIsSafeToGC(JSRuntime *rt)
{
    if (rt->gc.isInsideUnsafeRegion())
        MOZ_CRASH("[AutoAssertOnGC] possible GC in GC-unsafe region");
}

JS::AutoAssertNoAlloc::AutoAssertNoAlloc(JSRuntime *rt)
  : gc(nullptr)
{
    disallowAlloc(rt);
}

void JS::AutoAssertNoAlloc::disallowAlloc(JSRuntime *rt)
{
    MOZ_ASSERT(!gc);
    gc = &rt->gc;
    gc->disallowAlloc();
}

JS::AutoAssertNoAlloc::~AutoAssertNoAlloc()
{
    if (gc)
        gc->allowAlloc();
}
#endif

JS::AutoAssertGCCallback::AutoAssertGCCallback(JSObject *obj)
  : AutoSuppressGCAnalysis()
{
    MOZ_ASSERT(obj->runtimeFromMainThread()->isHeapCollecting());
}

JS_FRIEND_API(const char *)
JS::GCTraceKindToAscii(JSGCTraceKind kind)
{
    switch(kind) {
      case JSTRACE_OBJECT: return "Object";
      case JSTRACE_SCRIPT: return "Script";
      case JSTRACE_STRING: return "String";
      case JSTRACE_SYMBOL: return "Symbol";
      case JSTRACE_SHAPE: return "Shape";
      case JSTRACE_BASE_SHAPE: return "BaseShape";
      case JSTRACE_LAZY_SCRIPT: return "LazyScript";
      case JSTRACE_JITCODE: return "JitCode";
      case JSTRACE_TYPE_OBJECT: return "TypeObject";
      default: return "Invalid";
    }
}

JS::GCCellPtr::GCCellPtr(const Value &v)
  : ptr(0)
{
    if (v.isString())
        ptr = checkedCast(v.toString(), JSTRACE_STRING);
    else if (v.isObject())
        ptr = checkedCast(&v.toObject(), JSTRACE_OBJECT);
    else if (v.isSymbol())
        ptr = checkedCast(v.toSymbol(), JSTRACE_SYMBOL);
    else
        ptr = checkedCast(nullptr, JSTRACE_NULL);
}

JSGCTraceKind
JS::GCCellPtr::outOfLineKind() const
{
    MOZ_ASSERT(JSGCTraceKind(ptr & JSTRACE_OUTOFLINE) == JSTRACE_OUTOFLINE);
    MOZ_ASSERT(asCell()->isTenured());
    return MapAllocToTraceKind(asCell()->asTenured().getAllocKind());
}

#ifdef JSGC_HASH_TABLE_CHECKS
void
js::gc::CheckHashTablesAfterMovingGC(JSRuntime *rt)
{
    



    for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
        c->checkTypeObjectTablesAfterMovingGC();
        c->checkInitialShapesTableAfterMovingGC();
        c->checkWrapperMapAfterMovingGC();
        if (c->debugScopes)
            c->debugScopes->checkHashTablesAfterMovingGC(rt);
    }
}
#endif

JS_PUBLIC_API(void)
JS::PrepareZoneForGC(Zone *zone)
{
    zone->scheduleGC();
}

JS_PUBLIC_API(void)
JS::PrepareForFullGC(JSRuntime *rt)
{
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next())
        zone->scheduleGC();
}

JS_PUBLIC_API(void)
JS::PrepareForIncrementalGC(JSRuntime *rt)
{
    if (!JS::IsIncrementalGCInProgress(rt))
        return;

    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        if (zone->wasGCStarted())
            PrepareZoneForGC(zone);
    }
}

JS_PUBLIC_API(bool)
JS::IsGCScheduled(JSRuntime *rt)
{
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        if (zone->isGCScheduled())
            return true;
    }

    return false;
}

JS_PUBLIC_API(void)
JS::SkipZoneForGC(Zone *zone)
{
    zone->unscheduleGC();
}

JS_PUBLIC_API(void)
JS::GCForReason(JSRuntime *rt, JSGCInvocationKind gckind, gcreason::Reason reason)
{
    MOZ_ASSERT(gckind == GC_NORMAL || gckind == GC_SHRINK);
    rt->gc.gc(gckind, reason);
}

JS_PUBLIC_API(void)
JS::StartIncrementalGC(JSRuntime *rt, JSGCInvocationKind gckind, gcreason::Reason reason, int64_t millis)
{
    MOZ_ASSERT(gckind == GC_NORMAL || gckind == GC_SHRINK);
    rt->gc.startGC(gckind, reason, millis);
}

JS_PUBLIC_API(void)
JS::IncrementalGCSlice(JSRuntime *rt, gcreason::Reason reason, int64_t millis)
{
    rt->gc.gcSlice(reason, millis);
}

JS_PUBLIC_API(void)
JS::FinishIncrementalGC(JSRuntime *rt, gcreason::Reason reason)
{
    rt->gc.finishGC(reason);
}

char16_t *
JS::GCDescription::formatMessage(JSRuntime *rt) const
{
    return rt->gc.stats.formatMessage();
}

char16_t *
JS::GCDescription::formatJSON(JSRuntime *rt, uint64_t timestamp) const
{
    return rt->gc.stats.formatJSON(timestamp);
}

JS_PUBLIC_API(JS::GCSliceCallback)
JS::SetGCSliceCallback(JSRuntime *rt, GCSliceCallback callback)
{
    return rt->gc.setSliceCallback(callback);
}

JS_PUBLIC_API(void)
JS::DisableIncrementalGC(JSRuntime *rt)
{
    rt->gc.disallowIncrementalGC();
}

JS_PUBLIC_API(bool)
JS::IsIncrementalGCEnabled(JSRuntime *rt)
{
    return rt->gc.isIncrementalGCEnabled();
}

JS_PUBLIC_API(void)
JS::DisableCompactingGC(JSRuntime *rt)
{
    rt->gc.disableCompactingGC();
}

JS_PUBLIC_API(bool)
JS::IsCompactingGCEnabled(JSRuntime *rt)
{
    return rt->gc.isCompactingGCEnabled();
}

JS_PUBLIC_API(bool)
JS::IsIncrementalGCInProgress(JSRuntime *rt)
{
    return rt->gc.isIncrementalGCInProgress() && !rt->gc.isVerifyPreBarriersEnabled();
}

JS_PUBLIC_API(bool)
JS::IsIncrementalBarrierNeeded(JSRuntime *rt)
{
    return rt->gc.state() == gc::MARK && !rt->isHeapBusy();
}

JS_PUBLIC_API(bool)
JS::IsIncrementalBarrierNeeded(JSContext *cx)
{
    return IsIncrementalBarrierNeeded(cx->runtime());
}

JS_PUBLIC_API(void)
JS::IncrementalReferenceBarrier(GCCellPtr thing)
{
    if (!thing)
        return;

    if (thing.isString() && StringIsPermanentAtom(thing.toString()))
        return;

#ifdef DEBUG
    Zone *zone = thing.isObject()
                 ? thing.toObject()->zone()
                 : thing.asCell()->asTenured().zone();
    MOZ_ASSERT(!zone->runtimeFromMainThread()->isHeapMajorCollecting());
#endif

    switch(thing.kind()) {
      case JSTRACE_OBJECT: return JSObject::writeBarrierPre(thing.toObject());
      case JSTRACE_STRING: return JSString::writeBarrierPre(thing.toString());
      case JSTRACE_SCRIPT: return JSScript::writeBarrierPre(thing.toScript());
      case JSTRACE_SYMBOL: return JS::Symbol::writeBarrierPre(thing.toSymbol());
      case JSTRACE_LAZY_SCRIPT:
        return LazyScript::writeBarrierPre(static_cast<LazyScript*>(thing.asCell()));
      case JSTRACE_JITCODE:
        return jit::JitCode::writeBarrierPre(static_cast<jit::JitCode*>(thing.asCell()));
      case JSTRACE_SHAPE:
        return Shape::writeBarrierPre(static_cast<Shape*>(thing.asCell()));
      case JSTRACE_BASE_SHAPE:
        return BaseShape::writeBarrierPre(static_cast<BaseShape*>(thing.asCell()));
      case JSTRACE_TYPE_OBJECT:
        return types::TypeObject::writeBarrierPre(static_cast<types::TypeObject *>(thing.asCell()));
      default:
        MOZ_CRASH("Invalid trace kind in IncrementalReferenceBarrier.");
    }
}

JS_PUBLIC_API(void)
JS::IncrementalValueBarrier(const Value &v)
{
    js::HeapValue::writeBarrierPre(v);
}

JS_PUBLIC_API(void)
JS::IncrementalObjectBarrier(JSObject *obj)
{
    if (!obj)
        return;

    MOZ_ASSERT(!obj->zone()->runtimeFromMainThread()->isHeapMajorCollecting());

    JSObject::writeBarrierPre(obj);
}

JS_PUBLIC_API(bool)
JS::WasIncrementalGC(JSRuntime *rt)
{
    return rt->gc.isIncrementalGc();
}

JS::AutoDisableGenerationalGC::AutoDisableGenerationalGC(JSRuntime *rt)
  : gc(&rt->gc)
#ifdef JS_GC_ZEAL
  , restartVerifier(false)
#endif
{
#ifdef JS_GC_ZEAL
    restartVerifier = gc->endVerifyPostBarriers();
#endif
    gc->disableGenerationalGC();
}

JS::AutoDisableGenerationalGC::~AutoDisableGenerationalGC()
{
    gc->enableGenerationalGC();
#ifdef JS_GC_ZEAL
    if (restartVerifier) {
        MOZ_ASSERT(gc->isGenerationalGCEnabled());
        gc->startVerifyPostBarriers();
    }
#endif
}

JS_PUBLIC_API(bool)
JS::IsGenerationalGCEnabled(JSRuntime *rt)
{
    return rt->gc.isGenerationalGCEnabled();
}
