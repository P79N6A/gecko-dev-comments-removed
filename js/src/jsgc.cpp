












































































































































































#include "jsgcinlines.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"

#include <string.h>     
#ifndef XP_WIN
# include <unistd.h>
#endif

#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsobj.h"
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
#include "gc/Marking.h"
#include "gc/Memory.h"
#ifdef JS_ION
# include "jit/BaselineJIT.h"
#endif
#include "jit/IonCode.h"
#include "js/SliceBudget.h"
#include "vm/Debugger.h"
#include "vm/ForkJoin.h"
#include "vm/ProxyObject.h"
#include "vm/Shape.h"
#include "vm/String.h"
#include "vm/TraceLogging.h"
#include "vm/WrapperObject.h"

#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "vm/Stack-inl.h"
#include "vm/String-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::ArrayEnd;
using mozilla::DebugOnly;
using mozilla::Maybe;
using mozilla::Swap;


static const uint64_t GC_IDLE_FULL_SPAN = 20 * 1000 * 1000;


static const int IGC_MARK_SLICE_MULTIPLIER = 2;

#if defined(ANDROID) || defined(MOZ_B2G)
static const int MAX_EMPTY_CHUNK_COUNT = 2;
#else
static const int MAX_EMPTY_CHUNK_COUNT = 30;
#endif


const AllocKind gc::slotsToThingKind[] = {
      FINALIZE_OBJECT0,  FINALIZE_OBJECT2,  FINALIZE_OBJECT2,  FINALIZE_OBJECT4,
      FINALIZE_OBJECT4,  FINALIZE_OBJECT8,  FINALIZE_OBJECT8,  FINALIZE_OBJECT8,
      FINALIZE_OBJECT8,  FINALIZE_OBJECT12, FINALIZE_OBJECT12, FINALIZE_OBJECT12,
     FINALIZE_OBJECT12, FINALIZE_OBJECT16, FINALIZE_OBJECT16, FINALIZE_OBJECT16,
     FINALIZE_OBJECT16
};

static_assert(JS_ARRAY_LENGTH(slotsToThingKind) == SLOTS_TO_THING_KIND_LIMIT,
              "We have defined a slot count for each kind.");

const uint32_t Arena::ThingSizes[] = {
    sizeof(JSObject),           
    sizeof(JSObject),           
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
    sizeof(BaseShape),          
    sizeof(types::TypeObject),  
    sizeof(JSFatInlineString),  
    sizeof(JSString),           
    sizeof(JSExternalString),   
    sizeof(jit::JitCode),       
};

#define OFFSET(type) uint32_t(sizeof(ArenaHeader) + (ArenaSize - sizeof(ArenaHeader)) % sizeof(type))

const uint32_t Arena::FirstThingOffsets[] = {
    OFFSET(JSObject),           
    OFFSET(JSObject),           
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
    OFFSET(BaseShape),          
    OFFSET(types::TypeObject),  
    OFFSET(JSFatInlineString),  
    OFFSET(JSString),           
    OFFSET(JSExternalString),   
    OFFSET(jit::JitCode),       
};

#undef OFFSET

const char *
js::gc::TraceKindAsAscii(JSGCTraceKind kind)
{
    switch(kind) {
      case JSTRACE_OBJECT: return "JSTRACE_OBJECT";
      case JSTRACE_STRING: return "JSTRACE_STRING";
      case JSTRACE_SCRIPT: return "JSTRACE_SCRIPT";
      case JSTRACE_LAZY_SCRIPT: return "JSTRACE_SCRIPT";
      case JSTRACE_JITCODE: return "JSTRACE_JITCODE";
      case JSTRACE_SHAPE: return "JSTRACE_SHAPE";
      case JSTRACE_BASE_SHAPE: return "JSTRACE_BASE_SHAPE";
      case JSTRACE_TYPE_OBJECT: return "JSTRACE_TYPE_OBJECT";
      default: return "INVALID";
    }
}





static const AllocKind FinalizePhaseStrings[] = {
    FINALIZE_EXTERNAL_STRING
};

static const AllocKind FinalizePhaseScripts[] = {
    FINALIZE_SCRIPT,
    FINALIZE_LAZY_SCRIPT
};

static const AllocKind FinalizePhaseJitCode[] = {
    FINALIZE_JITCODE
};

static const AllocKind * const FinalizePhases[] = {
    FinalizePhaseStrings,
    FinalizePhaseScripts,
    FinalizePhaseJitCode
};
static const int FinalizePhaseCount = sizeof(FinalizePhases) / sizeof(AllocKind*);

static const int FinalizePhaseLength[] = {
    sizeof(FinalizePhaseStrings) / sizeof(AllocKind),
    sizeof(FinalizePhaseScripts) / sizeof(AllocKind),
    sizeof(FinalizePhaseJitCode) / sizeof(AllocKind)
};

static const gcstats::Phase FinalizePhaseStatsPhase[] = {
    gcstats::PHASE_SWEEP_STRING,
    gcstats::PHASE_SWEEP_SCRIPT,
    gcstats::PHASE_SWEEP_JITCODE
};





static const AllocKind BackgroundPhaseObjects[] = {
    FINALIZE_OBJECT0_BACKGROUND,
    FINALIZE_OBJECT2_BACKGROUND,
    FINALIZE_OBJECT4_BACKGROUND,
    FINALIZE_OBJECT8_BACKGROUND,
    FINALIZE_OBJECT12_BACKGROUND,
    FINALIZE_OBJECT16_BACKGROUND
};

static const AllocKind BackgroundPhaseStrings[] = {
    FINALIZE_FAT_INLINE_STRING,
    FINALIZE_STRING
};

static const AllocKind BackgroundPhaseShapes[] = {
    FINALIZE_SHAPE,
    FINALIZE_BASE_SHAPE,
    FINALIZE_TYPE_OBJECT
};

static const AllocKind * const BackgroundPhases[] = {
    BackgroundPhaseObjects,
    BackgroundPhaseStrings,
    BackgroundPhaseShapes
};
static const int BackgroundPhaseCount = sizeof(BackgroundPhases) / sizeof(AllocKind*);

static const int BackgroundPhaseLength[] = {
    sizeof(BackgroundPhaseObjects) / sizeof(AllocKind),
    sizeof(BackgroundPhaseStrings) / sizeof(AllocKind),
    sizeof(BackgroundPhaseShapes) / sizeof(AllocKind)
};

#ifdef DEBUG
void
ArenaHeader::checkSynchronizedWithFreeList() const
{
    



    JS_ASSERT(allocated());

    




    if (IsBackgroundFinalized(getAllocKind()) && zone->runtimeFromAnyThread()->gc.onBackgroundThread())
        return;

    FreeSpan firstSpan = firstFreeSpan.decompact(arenaAddress());
    if (firstSpan.isEmpty())
        return;
    const FreeList *freeList = zone->allocator.arenas.getFreeList(getAllocKind());
    if (freeList->isEmpty() || firstSpan.arenaAddress() != freeList->arenaAddress())
        return;

    



    JS_ASSERT(freeList->isSameNonEmptySpan(firstSpan));
}
#endif

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
inline bool
Arena::finalize(FreeOp *fop, AllocKind thingKind, size_t thingSize)
{
    
    JS_ASSERT(thingSize % CellSize == 0);
    JS_ASSERT(thingSize <= 255);

    JS_ASSERT(aheader.allocated());
    JS_ASSERT(thingKind == aheader.getAllocKind());
    JS_ASSERT(thingSize == aheader.getThingSize());
    JS_ASSERT(!aheader.hasDelayedMarking);
    JS_ASSERT(!aheader.markOverflow);
    JS_ASSERT(!aheader.allocatedDuringIncremental);

    uintptr_t firstThing = thingsStart(thingKind);
    uintptr_t firstThingOrSuccessorOfLastMarkedThing = firstThing;
    uintptr_t lastThing = thingsEnd() - thingSize;

    FreeSpan newListHead;
    FreeSpan *newListTail = &newListHead;
    size_t nmarked = 0;

    for (ArenaCellIterUnderFinalize i(&aheader); !i.done(); i.next()) {
        T *t = i.get<T>();
        if (t->isMarked()) {
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
        }
    }

    if (nmarked == 0) {
        
        JS_ASSERT(newListTail == &newListHead);
        JS_EXTRA_POISON(data, JS_SWEPT_TENURED_PATTERN, sizeof(data));
        return true;
    }

    JS_ASSERT(firstThingOrSuccessorOfLastMarkedThing != firstThing);
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
    JS_ASSERT(nfree + nmarked == thingsPerArena(thingSize));
#endif
    aheader.setFirstFreeSpan(&newListHead);
    return false;
}

template<typename T>
static inline bool
FinalizeTypedArenas(FreeOp *fop,
                    ArenaHeader **src,
                    ArenaList &dest,
                    AllocKind thingKind,
                    SliceBudget &budget)
{
    




    




    bool releaseArenas = !InParallelSection();

    size_t thingSize = Arena::thingSize(thingKind);

    while (ArenaHeader *aheader = *src) {
        *src = aheader->next;
        bool allClear = aheader->getArena()->finalize<T>(fop, thingKind, thingSize);
        if (!allClear)
            dest.insertAtCursor(aheader);
        else if (releaseArenas)
            aheader->chunk()->releaseArena(aheader);
        else
            aheader->chunk()->recycleArena(aheader, dest, thingKind);

        budget.step(Arena::thingsPerArena(thingSize));
        if (budget.isOverBudget())
            return false;
    }
    dest.deepCheck();

    return true;
}





static bool
FinalizeArenas(FreeOp *fop,
               ArenaHeader **src,
               ArenaList &dest,
               AllocKind thingKind,
               SliceBudget &budget)
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
        return FinalizeTypedArenas<JSObject>(fop, src, dest, thingKind, budget);
      case FINALIZE_SCRIPT:
        return FinalizeTypedArenas<JSScript>(fop, src, dest, thingKind, budget);
      case FINALIZE_LAZY_SCRIPT:
        return FinalizeTypedArenas<LazyScript>(fop, src, dest, thingKind, budget);
      case FINALIZE_SHAPE:
        return FinalizeTypedArenas<Shape>(fop, src, dest, thingKind, budget);
      case FINALIZE_BASE_SHAPE:
        return FinalizeTypedArenas<BaseShape>(fop, src, dest, thingKind, budget);
      case FINALIZE_TYPE_OBJECT:
        return FinalizeTypedArenas<types::TypeObject>(fop, src, dest, thingKind, budget);
      case FINALIZE_STRING:
        return FinalizeTypedArenas<JSString>(fop, src, dest, thingKind, budget);
      case FINALIZE_FAT_INLINE_STRING:
        return FinalizeTypedArenas<JSFatInlineString>(fop, src, dest, thingKind, budget);
      case FINALIZE_EXTERNAL_STRING:
        return FinalizeTypedArenas<JSExternalString>(fop, src, dest, thingKind, budget);
      case FINALIZE_JITCODE:
#ifdef JS_ION
      {
        
        
        JSRuntime::AutoLockForInterrupt lock(fop->runtime());
        return FinalizeTypedArenas<jit::JitCode>(fop, src, dest, thingKind, budget);
      }
#endif
      default:
        MOZ_ASSUME_UNREACHABLE("Invalid alloc kind");
    }
}

static inline Chunk *
AllocChunk(JSRuntime *rt)
{
    return static_cast<Chunk *>(rt->gc.pageAllocator.mapAlignedPages(ChunkSize, ChunkSize));
}

static inline void
FreeChunk(JSRuntime *rt, Chunk *p)
{
    rt->gc.pageAllocator.unmapPages(static_cast<void *>(p), ChunkSize);
}


inline Chunk *
ChunkPool::get(JSRuntime *rt)
{
    Chunk *chunk = emptyChunkListHead;
    if (!chunk) {
        JS_ASSERT(!emptyCount);
        return nullptr;
    }

    JS_ASSERT(emptyCount);
    emptyChunkListHead = chunk->info.next;
    --emptyCount;
    return chunk;
}


inline void
ChunkPool::put(Chunk *chunk)
{
    chunk->info.age = 0;
    chunk->info.next = emptyChunkListHead;
    emptyChunkListHead = chunk;
    emptyCount++;
}


Chunk *
ChunkPool::expire(JSRuntime *rt, bool releaseAll)
{
    JS_ASSERT(this == &rt->gc.chunkPool);

    





    Chunk *freeList = nullptr;
    int freeChunkCount = 0;
    for (Chunk **chunkp = &emptyChunkListHead; *chunkp; ) {
        JS_ASSERT(emptyCount);
        Chunk *chunk = *chunkp;
        JS_ASSERT(chunk->unused());
        JS_ASSERT(!rt->gc.chunkSet.has(chunk));
        JS_ASSERT(chunk->info.age <= MAX_EMPTY_CHUNK_AGE);
        if (releaseAll || chunk->info.age == MAX_EMPTY_CHUNK_AGE ||
            freeChunkCount++ > MAX_EMPTY_CHUNK_COUNT)
        {
            *chunkp = chunk->info.next;
            --emptyCount;
            chunk->prepareToBeFreed(rt);
            chunk->info.next = freeList;
            freeList = chunk;
        } else {
            
            ++chunk->info.age;
            chunkp = &chunk->info.next;
        }
    }
    JS_ASSERT_IF(releaseAll, !emptyCount);
    return freeList;
}

static void
FreeChunkList(JSRuntime *rt, Chunk *chunkListHead)
{
    while (Chunk *chunk = chunkListHead) {
        JS_ASSERT(!chunk->info.numArenasFreeCommitted);
        chunkListHead = chunk->info.next;
        FreeChunk(rt, chunk);
    }
}

void
ChunkPool::expireAndFree(JSRuntime *rt, bool releaseAll)
{
    FreeChunkList(rt, expire(rt, releaseAll));
}

 Chunk *
Chunk::allocate(JSRuntime *rt)
{
    Chunk *chunk = AllocChunk(rt);
    if (!chunk)
        return nullptr;
    chunk->init(rt);
    rt->gc.stats.count(gcstats::STAT_NEW_CHUNK);
    return chunk;
}


 inline void
Chunk::release(JSRuntime *rt, Chunk *chunk)
{
    JS_ASSERT(chunk);
    chunk->prepareToBeFreed(rt);
    FreeChunk(rt, chunk);
}

inline void
Chunk::prepareToBeFreed(JSRuntime *rt)
{
    JS_ASSERT(rt->gc.numArenasFreeCommitted >= info.numArenasFreeCommitted);
    rt->gc.numArenasFreeCommitted -= info.numArenasFreeCommitted;
    rt->gc.stats.count(gcstats::STAT_DESTROY_CHUNK);

#ifdef DEBUG
    



    info.numArenasFreeCommitted = 0;
#endif
}

void Chunk::decommitAllArenas(JSRuntime *rt)
{
    decommittedArenas.clear(true);
    rt->gc.pageAllocator.markPagesUnused(&arenas[0], ArenasPerChunk * ArenaSize);

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

    
    info.age = 0;
    info.trailer.storeBuffer = nullptr;
    info.trailer.location = ChunkLocationTenuredHeap;
    info.trailer.runtime = rt;

    
}

static inline Chunk **
GetAvailableChunkList(Zone *zone)
{
    JSRuntime *rt = zone->runtimeFromAnyThread();
    return zone->isSystem
           ? &rt->gc.systemAvailableChunkListHead
           : &rt->gc.userAvailableChunkListHead;
}

inline void
Chunk::addToAvailableList(Zone *zone)
{
    insertToAvailableList(GetAvailableChunkList(zone));
}

inline void
Chunk::insertToAvailableList(Chunk **insertPoint)
{
    JS_ASSERT(hasAvailableArenas());
    JS_ASSERT(!info.prevp);
    JS_ASSERT(!info.next);
    info.prevp = insertPoint;
    Chunk *insertBefore = *insertPoint;
    if (insertBefore) {
        JS_ASSERT(insertBefore->info.prevp == insertPoint);
        insertBefore->info.prevp = &info.next;
    }
    info.next = insertBefore;
    *insertPoint = this;
}

inline void
Chunk::removeFromAvailableList()
{
    JS_ASSERT(info.prevp);
    *info.prevp = info.next;
    if (info.next) {
        JS_ASSERT(info.next->info.prevp == &info.next);
        info.next->info.prevp = info.prevp;
    }
    info.prevp = nullptr;
    info.next = nullptr;
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
    MOZ_ASSUME_UNREACHABLE("No decommitted arenas found.");
}

ArenaHeader *
Chunk::fetchNextDecommittedArena()
{
    JS_ASSERT(info.numArenasFreeCommitted == 0);
    JS_ASSERT(info.numArenasFree > 0);

    unsigned offset = findDecommittedArenaOffset();
    info.lastDecommittedArenaOffset = offset + 1;
    --info.numArenasFree;
    decommittedArenas.unset(offset);

    Arena *arena = &arenas[offset];
    info.trailer.runtime->gc.pageAllocator.markPagesInUse(arena, ArenaSize);
    arena->aheader.setAsNotAllocated();

    return &arena->aheader;
}

inline ArenaHeader *
Chunk::fetchNextFreeArena(JSRuntime *rt)
{
    JS_ASSERT(info.numArenasFreeCommitted > 0);
    JS_ASSERT(info.numArenasFreeCommitted <= info.numArenasFree);
    JS_ASSERT(info.numArenasFreeCommitted <= rt->gc.numArenasFreeCommitted);

    ArenaHeader *aheader = info.freeArenasHead;
    info.freeArenasHead = aheader->next;
    --info.numArenasFreeCommitted;
    --info.numArenasFree;
    --rt->gc.numArenasFreeCommitted;

    return aheader;
}

ArenaHeader *
Chunk::allocateArena(Zone *zone, AllocKind thingKind)
{
    JS_ASSERT(hasAvailableArenas());

    JSRuntime *rt = zone->runtimeFromAnyThread();
    if (!rt->isHeapMinorCollecting() && rt->gc.bytes >= rt->gc.maxBytes)
        return nullptr;

    ArenaHeader *aheader = MOZ_LIKELY(info.numArenasFreeCommitted > 0)
                           ? fetchNextFreeArena(rt)
                           : fetchNextDecommittedArena();
    aheader->init(zone, thingKind);
    if (MOZ_UNLIKELY(!hasAvailableArenas()))
        removeFromAvailableList();

    rt->gc.bytes += ArenaSize;
    zone->gcBytes += ArenaSize;

    if (zone->gcBytes >= zone->gcTriggerBytes) {
        AutoUnlockGC unlock(rt);
        TriggerZoneGC(zone, JS::gcreason::ALLOC_TRIGGER);
    }

    return aheader;
}

inline void
Chunk::addArenaToFreeList(JSRuntime *rt, ArenaHeader *aheader)
{
    JS_ASSERT(!aheader->allocated());
    aheader->next = info.freeArenasHead;
    info.freeArenasHead = aheader;
    ++info.numArenasFreeCommitted;
    ++info.numArenasFree;
    ++rt->gc.numArenasFreeCommitted;
}

void
Chunk::recycleArena(ArenaHeader *aheader, ArenaList &dest, AllocKind thingKind)
{
    aheader->getArena()->setAsFullyUnused(thingKind);
    dest.insertAtCursor(aheader);
}

void
Chunk::releaseArena(ArenaHeader *aheader)
{
    JS_ASSERT(aheader->allocated());
    JS_ASSERT(!aheader->hasDelayedMarking);
    Zone *zone = aheader->zone;
    JSRuntime *rt = zone->runtimeFromAnyThread();
    AutoLockGC maybeLock;
    if (rt->gc.isBackgroundSweeping())
        maybeLock.lock(rt);

    JS_ASSERT(rt->gc.bytes >= ArenaSize);
    JS_ASSERT(zone->gcBytes >= ArenaSize);
    if (rt->gc.isBackgroundSweeping())
        zone->reduceGCTriggerBytes(zone->gcHeapGrowthFactor * ArenaSize);
    rt->gc.bytes -= ArenaSize;
    zone->gcBytes -= ArenaSize;

    aheader->setAsNotAllocated();
    addArenaToFreeList(rt, aheader);

    if (info.numArenasFree == 1) {
        JS_ASSERT(!info.prevp);
        JS_ASSERT(!info.next);
        addToAvailableList(zone);
    } else if (!unused()) {
        JS_ASSERT(info.prevp);
    } else {
        rt->gc.chunkSet.remove(this);
        removeFromAvailableList();
        JS_ASSERT(info.numArenasFree == ArenasPerChunk);
        decommitAllArenas(rt);
        rt->gc.chunkPool.put(this);
    }
}

inline bool
GCRuntime::wantBackgroundAllocation() const
{
    




    return helperThread.canBackgroundAllocate() &&
           chunkPool.getEmptyCount() == 0 &&
           chunkSet.count() >= 4;
}


Chunk *
GCRuntime::pickChunk(Zone *zone)
{
    Chunk **listHeadp = GetAvailableChunkList(zone);
    Chunk *chunk = *listHeadp;
    if (chunk)
        return chunk;

    chunk = chunkPool.get(rt);
    if (!chunk) {
        chunk = Chunk::allocate(rt);
        if (!chunk)
            return nullptr;
        JS_ASSERT(chunk->info.numArenasFreeCommitted == 0);
    }

    JS_ASSERT(chunk->unused());
    JS_ASSERT(!chunkSet.has(chunk));

    if (wantBackgroundAllocation())
        helperThread.startBackgroundAllocationIfIdle();

    chunkAllocationSinceLastGC = true;

    



    GCChunkSet::AddPtr p = chunkSet.lookupForAdd(chunk);
    JS_ASSERT(!p);
    if (!chunkSet.add(p, chunk)) {
        Chunk::release(rt, chunk);
        return nullptr;
    }

    chunk->info.prevp = nullptr;
    chunk->info.next = nullptr;
    chunk->addToAvailableList(zone);

    return chunk;
}

GCRuntime::GCRuntime(JSRuntime *rt) :
    rt(rt),
    systemZone(nullptr),
    systemAvailableChunkListHead(nullptr),
    userAvailableChunkListHead(nullptr),
    bytes(0),
    maxBytes(0),
    maxMallocBytes(0),
    numArenasFreeCommitted(0),
    marker(rt),
    verifyPreData(nullptr),
    verifyPostData(nullptr),
    chunkAllocationSinceLastGC(false),
    nextFullGCTime(0),
    lastGCTime(0),
    jitReleaseTime(0),
    allocationThreshold(30 * 1024 * 1024),
    highFrequencyGC(false),
    highFrequencyTimeThreshold(1000),
    highFrequencyLowLimitBytes(100 * 1024 * 1024),
    highFrequencyHighLimitBytes(500 * 1024 * 1024),
    highFrequencyHeapGrowthMax(3.0),
    highFrequencyHeapGrowthMin(1.5),
    lowFrequencyHeapGrowth(1.5),
    dynamicHeapGrowth(false),
    dynamicMarkSlice(false),
    decommitThreshold(32 * 1024 * 1024),
    shouldCleanUpEverything(false),
    grayBitsValid(false),
    isNeeded(0),
    stats(rt),
    number(0),
    startNumber(0),
    isFull(false),
    triggerReason(JS::gcreason::NO_REASON),
    strictCompartmentChecking(false),
#ifdef DEBUG
    disableStrictProxyCheckingCount(0),
#endif
    incrementalState(gc::NO_INCREMENTAL),
    lastMarkSlice(false),
    sweepOnBackgroundThread(false),
    foundBlackGrayEdges(false),
    sweepingZones(nullptr),
    zoneGroupIndex(0),
    zoneGroups(nullptr),
    currentZoneGroup(nullptr),
    sweepZone(nullptr),
    sweepKindIndex(0),
    abortSweepAfterCurrentGroup(false),
    arenasAllocatedDuringSweep(nullptr),
#ifdef DEBUG
    markingValidator(nullptr),
#endif
    interFrameGC(0),
    sliceBudget(SliceBudget::Unlimited),
    incrementalEnabled(true),
    generationalDisabled(0),
    manipulatingDeadZones(false),
    objectsMarkedInDeadZones(0),
    poke(false),
    heapState(Idle),
#ifdef JSGC_GENERATIONAL
    nursery(rt),
    storeBuffer(rt, nursery),
#endif
#ifdef JS_GC_ZEAL
    zealMode(0),
    zealFrequency(0),
    nextScheduled(0),
    deterministicOnly(false),
    incrementalLimit(0),
#endif
    validate(true),
    fullCompartmentChecks(false),
    gcCallback(nullptr),
    sliceCallback(nullptr),
    mallocBytes(0),
    mallocGCTriggered(false),
    scriptAndCountsVector(nullptr),
    alwaysPreserveCode(false),
#ifdef DEBUG
    noGCOrAllocationCheck(0),
#endif
    lock(nullptr),
    lockOwner(nullptr),
    helperThread(rt)
{
}

#ifdef JS_GC_ZEAL

extern void
js::SetGCZeal(JSRuntime *rt, uint8_t zeal, uint32_t frequency)
{
    rt->gc.setGCZeal(zeal, frequency);
}

void
GCRuntime::setGCZeal(uint8_t zeal, uint32_t frequency)
{
    if (verifyPreData)
        VerifyBarriers(rt, PreBarrierVerifier);
    if (verifyPostData)
        VerifyBarriers(rt, PostBarrierVerifier);

#ifdef JSGC_GENERATIONAL
    if (zealMode == ZealGenerationalGCValue) {
        minorGC(JS::gcreason::DEBUG_GC);
        nursery.leaveZealMode();
    }

    if (zeal == ZealGenerationalGCValue)
        nursery.enterZealMode();
#endif

    bool schedule = zeal >= js::gc::ZealAllocValue;
    zealMode = zeal;
    zealFrequency = frequency;
    nextScheduled = schedule ? frequency : 0;
}

bool
GCRuntime::initGCZeal()
{
    const char *env = getenv("JS_GC_ZEAL");
    if (!env)
        return true;

    int zeal = -1;
    int frequency = JS_DEFAULT_ZEAL_FREQ;
    if (strcmp(env, "help") != 0) {
        zeal = atoi(env);
        const char *p = strchr(env, ',');
        if (p)
            frequency = atoi(p + 1);
    }

    if (zeal < 0 || zeal > ZealLimit || frequency < 0) {
        fprintf(stderr,
                "Format: JS_GC_ZEAL=N[,F]\n"
                "N indicates \"zealousness\":\n"
                "  0: no additional GCs\n"
                "  1: additional GCs at common danger points\n"
                "  2: GC every F allocations (default: 100)\n"
                "  3: GC when the window paints (browser only)\n"
                "  4: Verify pre write barriers between instructions\n"
                "  5: Verify pre write barriers between paints\n"
                "  6: Verify stack rooting\n"
                "  7: Collect the nursery every N nursery allocations\n"
                "  8: Incremental GC in two slices: 1) mark roots 2) finish collection\n"
                "  9: Incremental GC in two slices: 1) mark all 2) new marking and finish\n"
                " 10: Incremental GC in multiple slices\n"
                " 11: Verify post write barriers between instructions\n"
                " 12: Verify post write barriers between paints\n"
                " 13: Purge analysis state every F allocations (default: 100)\n");
        return false;
    }

    setGCZeal(zeal, frequency);
    return true;
}

#endif


static const int64_t JIT_SCRIPT_RELEASE_TYPES_INTERVAL = 60 * 1000 * 1000;

bool
GCRuntime::init(uint32_t maxbytes)
{
    lock = PR_NewLock();
    if (!lock)
        return false;

    if (!chunkSet.init(INITIAL_CHUNK_CAPACITY))
        return false;

    if (!rootsHash.init(256))
        return false;

    if (!helperThread.init())
        return false;

    



    maxBytes = maxbytes;
    rt->setGCMaxMallocBytes(maxbytes);

#ifndef JS_MORE_DETERMINISTIC
    jitReleaseTime = PRMJ_Now() + JIT_SCRIPT_RELEASE_TYPES_INTERVAL;
#endif

#ifdef JSGC_GENERATIONAL
    if (!nursery.init())
        return false;

    if (!storeBuffer.enable())
        return false;
#endif

#ifdef JS_GC_ZEAL
    if (!initGCZeal())
        return false;
#endif

    if (!marker.init(mode))
        return false;

    return true;
}

void
GCRuntime::recordNativeStackTop()
{
#ifdef JS_THREADSAFE
    
    if (!rt->requestDepth)
        return;
#endif
    conservativeGC.recordStackTop();
}

void
GCRuntime::finish()
{
    



    helperThread.finish();

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

    systemAvailableChunkListHead = nullptr;
    userAvailableChunkListHead = nullptr;
    if (chunkSet.initialized()) {
        for (GCChunkSet::Range r(chunkSet.all()); !r.empty(); r.popFront())
            Chunk::release(rt, r.front());
        chunkSet.clear();
    }

    chunkPool.expireAndFree(rt, true);

    if (rootsHash.initialized())
        rootsHash.clear();

    FinishPersistentRootedChains(rt);

#ifdef JS_THREADSAFE
    if (lock) {
        PR_DestroyLock(lock);
        lock = nullptr;
    }
#endif
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

template <typename T> struct BarrierOwner {};
template <typename T> struct BarrierOwner<T *> { typedef T result; };
template <> struct BarrierOwner<Value> { typedef HeapValue result; };

template <typename T>
bool
GCRuntime::addRoot(T *rp, const char *name, JSGCRootType rootType)
{
    





    if (rt->gc.incrementalState != NO_INCREMENTAL)
        BarrierOwner<T>::result::writeBarrierPre(*rp);

    return rt->gc.rootsHash.put((void *)rp, RootInfo(name, rootType));
}

void
GCRuntime::removeRoot(void *rp)
{
    rootsHash.remove(rp);
    poke = true;
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

typedef RootedValueMap::Range RootRange;
typedef RootedValueMap::Entry RootEntry;
typedef RootedValueMap::Enum RootEnum;

static size_t
ComputeTriggerBytes(Zone *zone, size_t lastBytes, size_t maxBytes, JSGCInvocationKind gckind)
{
    size_t base = gckind == GC_SHRINK ? lastBytes : Max(lastBytes, zone->runtimeFromMainThread()->gc.allocationThreshold);
    double trigger = double(base) * zone->gcHeapGrowthFactor;
    return size_t(Min(double(maxBytes), trigger));
}

void
Zone::setGCLastBytes(size_t lastBytes, JSGCInvocationKind gckind)
{
    







    JSRuntime *rt = runtimeFromMainThread();

    if (!rt->gc.dynamicHeapGrowth) {
        gcHeapGrowthFactor = 3.0;
    } else if (lastBytes < 1 * 1024 * 1024) {
        gcHeapGrowthFactor = rt->gc.lowFrequencyHeapGrowth;
    } else {
        JS_ASSERT(rt->gc.highFrequencyHighLimitBytes > rt->gc.highFrequencyLowLimitBytes);
        uint64_t now = PRMJ_Now();
        if (rt->gc.lastGCTime && rt->gc.lastGCTime + rt->gc.highFrequencyTimeThreshold * PRMJ_USEC_PER_MSEC > now) {
            if (lastBytes <= rt->gc.highFrequencyLowLimitBytes) {
                gcHeapGrowthFactor = rt->gc.highFrequencyHeapGrowthMax;
            } else if (lastBytes >= rt->gc.highFrequencyHighLimitBytes) {
                gcHeapGrowthFactor = rt->gc.highFrequencyHeapGrowthMin;
            } else {
                double k = (rt->gc.highFrequencyHeapGrowthMin - rt->gc.highFrequencyHeapGrowthMax)
                           / (double)(rt->gc.highFrequencyHighLimitBytes - rt->gc.highFrequencyLowLimitBytes);
                gcHeapGrowthFactor = (k * (lastBytes - rt->gc.highFrequencyLowLimitBytes)
                                     + rt->gc.highFrequencyHeapGrowthMax);
                JS_ASSERT(gcHeapGrowthFactor <= rt->gc.highFrequencyHeapGrowthMax
                          && gcHeapGrowthFactor >= rt->gc.highFrequencyHeapGrowthMin);
            }
            rt->gc.highFrequencyGC = true;
        } else {
            gcHeapGrowthFactor = rt->gc.lowFrequencyHeapGrowth;
            rt->gc.highFrequencyGC = false;
        }
    }
    gcTriggerBytes = ComputeTriggerBytes(this, lastBytes, rt->gc.maxBytes, gckind);
}

void
Zone::reduceGCTriggerBytes(size_t amount)
{
    JS_ASSERT(amount > 0);
    JS_ASSERT(gcTriggerBytes >= amount);
    if (gcTriggerBytes - amount < runtimeFromAnyThread()->gc.allocationThreshold * gcHeapGrowthFactor)
        return;
    gcTriggerBytes -= amount;
}

Allocator::Allocator(Zone *zone)
  : zone_(zone)
{}

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
    const Cell *cell = reinterpret_cast<const Cell *>(thing);
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

static inline void
PushArenaAllocatedDuringSweep(JSRuntime *runtime, ArenaHeader *arena)
{
    arena->setNextAllocDuringSweep(runtime->gc.arenasAllocatedDuringSweep);
    runtime->gc.arenasAllocatedDuringSweep = arena;
}

inline void *
ArenaLists::allocateFromArenaInline(Zone *zone, AllocKind thingKind)
{
    









    AutoLockGC maybeLock;

    bool backgroundFinalizationIsRunning = false;
#ifdef JS_THREADSAFE
    ArenaLists::BackgroundFinalizeState *bfs = &backgroundFinalizeState[thingKind];
    if (*bfs != BFS_DONE) {
        




        JSRuntime *rt = zone->runtimeFromAnyThread();
        maybeLock.lock(rt);
        if (*bfs == BFS_RUN) {
            backgroundFinalizationIsRunning = true;
        } else if (*bfs == BFS_JUST_FINISHED) {
            
            *bfs = BFS_DONE;
        } else {
            JS_ASSERT(*bfs == BFS_DONE);
        }
    }
#endif 

    ArenaHeader *aheader;
    ArenaList *al = &arenaLists[thingKind];
    if (!backgroundFinalizationIsRunning && (aheader = al->arenaAfterCursor())) {
        





        JS_ASSERT(!aheader->isEmpty() || InParallelSection());

        al->moveCursorPast(aheader);

        



        FreeSpan firstFreeSpan = aheader->getFirstFreeSpan();
        freeLists[thingKind].setHead(&firstFreeSpan);
        aheader->setAsFullyUsed();
        if (MOZ_UNLIKELY(zone->wasGCStarted())) {
            if (zone->needsBarrier()) {
                aheader->allocatedDuringIncremental = true;
                zone->runtimeFromMainThread()->gc.marker.delayMarkingArena(aheader);
            } else if (zone->isGCSweeping()) {
                PushArenaAllocatedDuringSweep(zone->runtimeFromMainThread(), aheader);
            }
        }
        void *thing = freeLists[thingKind].allocate(Arena::thingSize(thingKind));
        JS_ASSERT(thing);   
        return thing;
    }

    
    JSRuntime *rt = zone->runtimeFromAnyThread();
    if (!maybeLock.locked())
        maybeLock.lock(rt);
    Chunk *chunk = rt->gc.pickChunk(zone);
    if (!chunk)
        return nullptr;

    








    JS_ASSERT(al->isCursorAtEnd());
    aheader = chunk->allocateArena(zone, thingKind);
    if (!aheader)
        return nullptr;

    if (MOZ_UNLIKELY(zone->wasGCStarted())) {
        if (zone->needsBarrier()) {
            aheader->allocatedDuringIncremental = true;
            zone->runtimeFromMainThread()->gc.marker.delayMarkingArena(aheader);
        } else if (zone->isGCSweeping()) {
            PushArenaAllocatedDuringSweep(zone->runtimeFromMainThread(), aheader);
        }
    }
    al->insertAtStart(aheader);

    




    JS_ASSERT(!aheader->hasFreeThings());
    Arena *arena = aheader->getArena();
    size_t thingSize = Arena::thingSize(thingKind);
    FreeSpan fullSpan;
    fullSpan.initFinal(arena->thingsStart(thingKind), arena->thingsEnd() - thingSize, thingSize);
    freeLists[thingKind].setHead(&fullSpan);
    return freeLists[thingKind].allocate(thingSize);
}

void *
ArenaLists::allocateFromArena(JS::Zone *zone, AllocKind thingKind)
{
    return allocateFromArenaInline(zone, thingKind);
}

void
ArenaLists::wipeDuringParallelExecution(JSRuntime *rt)
{
    JS_ASSERT(InParallelSection());

    
    
    
    
    
    
    
    
    for (unsigned i = 0; i < FINALIZE_LAST; i++) {
        AllocKind thingKind = AllocKind(i);
        if (!IsBackgroundFinalized(thingKind) && !arenaLists[thingKind].isEmpty())
            return;
    }

    
    
    FreeOp fop(rt, false);
    for (unsigned i = 0; i < FINALIZE_OBJECT_LAST; i++) {
        AllocKind thingKind = AllocKind(i);

        if (!IsBackgroundFinalized(thingKind))
            continue;

        if (!arenaLists[i].isEmpty()) {
            purge(thingKind);
            forceFinalizeNow(&fop, thingKind);
        }
    }
}

void
ArenaLists::finalizeNow(FreeOp *fop, AllocKind thingKind)
{
    JS_ASSERT(!IsBackgroundFinalized(thingKind));
    forceFinalizeNow(fop, thingKind);
}

void
ArenaLists::forceFinalizeNow(FreeOp *fop, AllocKind thingKind)
{
    JS_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE);

    ArenaHeader *arenas = arenaLists[thingKind].head();
    arenaLists[thingKind].clear();

    SliceBudget budget;
    FinalizeArenas(fop, &arenas, arenaLists[thingKind], thingKind, budget);
    JS_ASSERT(!arenas);
}

void
ArenaLists::queueForForegroundSweep(FreeOp *fop, AllocKind thingKind)
{
    JS_ASSERT(!IsBackgroundFinalized(thingKind));
    JS_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE);
    JS_ASSERT(!arenaListsToSweep[thingKind]);

    arenaListsToSweep[thingKind] = arenaLists[thingKind].head();
    arenaLists[thingKind].clear();
}

inline void
ArenaLists::queueForBackgroundSweep(FreeOp *fop, AllocKind thingKind)
{
    JS_ASSERT(IsBackgroundFinalized(thingKind));

#ifdef JS_THREADSAFE
    JS_ASSERT(!fop->runtime()->gc.helperThread.sweeping());
#endif

    ArenaList *al = &arenaLists[thingKind];
    if (al->isEmpty()) {
        JS_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE);
        return;
    }

    



    JS_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE ||
              backgroundFinalizeState[thingKind] == BFS_JUST_FINISHED);

    arenaListsToSweep[thingKind] = al->head();
    al->clear();
    backgroundFinalizeState[thingKind] = BFS_RUN;
}

 void
ArenaLists::backgroundFinalize(FreeOp *fop, ArenaHeader *listHead, bool onBackgroundThread)
{
    JS_ASSERT(listHead);
    AllocKind thingKind = listHead->getAllocKind();
    Zone *zone = listHead->zone;

    ArenaList finalized;
    SliceBudget budget;
    FinalizeArenas(fop, &listHead, finalized, thingKind, budget);
    JS_ASSERT(!listHead);

    
    
    
    
    
    ArenaLists *lists = &zone->allocator.arenas;
    ArenaList *al = &lists->arenaLists[thingKind];

    AutoLockGC lock(fop->runtime());
    JS_ASSERT(lists->backgroundFinalizeState[thingKind] == BFS_RUN);

    al->appendToListWithCursorAtEnd(finalized);

    








    if (onBackgroundThread && !finalized.isEmpty())
        lists->backgroundFinalizeState[thingKind] = BFS_JUST_FINISHED;
    else
        lists->backgroundFinalizeState[thingKind] = BFS_DONE;

    lists->arenaListsToSweep[thingKind] = nullptr;
}

void
ArenaLists::queueObjectsForSweep(FreeOp *fop)
{
    gcstats::AutoPhase ap(fop->runtime()->gc.stats, gcstats::PHASE_SWEEP_OBJECT);

    finalizeNow(fop, FINALIZE_OBJECT0);
    finalizeNow(fop, FINALIZE_OBJECT2);
    finalizeNow(fop, FINALIZE_OBJECT4);
    finalizeNow(fop, FINALIZE_OBJECT8);
    finalizeNow(fop, FINALIZE_OBJECT12);
    finalizeNow(fop, FINALIZE_OBJECT16);

    queueForBackgroundSweep(fop, FINALIZE_OBJECT0_BACKGROUND);
    queueForBackgroundSweep(fop, FINALIZE_OBJECT2_BACKGROUND);
    queueForBackgroundSweep(fop, FINALIZE_OBJECT4_BACKGROUND);
    queueForBackgroundSweep(fop, FINALIZE_OBJECT8_BACKGROUND);
    queueForBackgroundSweep(fop, FINALIZE_OBJECT12_BACKGROUND);
    queueForBackgroundSweep(fop, FINALIZE_OBJECT16_BACKGROUND);
}

void
ArenaLists::queueStringsForSweep(FreeOp *fop)
{
    gcstats::AutoPhase ap(fop->runtime()->gc.stats, gcstats::PHASE_SWEEP_STRING);

    queueForBackgroundSweep(fop, FINALIZE_FAT_INLINE_STRING);
    queueForBackgroundSweep(fop, FINALIZE_STRING);

    queueForForegroundSweep(fop, FINALIZE_EXTERNAL_STRING);
}

void
ArenaLists::queueScriptsForSweep(FreeOp *fop)
{
    gcstats::AutoPhase ap(fop->runtime()->gc.stats, gcstats::PHASE_SWEEP_SCRIPT);
    queueForForegroundSweep(fop, FINALIZE_SCRIPT);
    queueForForegroundSweep(fop, FINALIZE_LAZY_SCRIPT);
}

void
ArenaLists::queueJitCodeForSweep(FreeOp *fop)
{
    gcstats::AutoPhase ap(fop->runtime()->gc.stats, gcstats::PHASE_SWEEP_JITCODE);
    queueForForegroundSweep(fop, FINALIZE_JITCODE);
}

void
ArenaLists::queueShapesForSweep(FreeOp *fop)
{
    gcstats::AutoPhase ap(fop->runtime()->gc.stats, gcstats::PHASE_SWEEP_SHAPE);

    queueForBackgroundSweep(fop, FINALIZE_SHAPE);
    queueForBackgroundSweep(fop, FINALIZE_BASE_SHAPE);
    queueForBackgroundSweep(fop, FINALIZE_TYPE_OBJECT);
}

static void *
RunLastDitchGC(JSContext *cx, JS::Zone *zone, AllocKind thingKind)
{
    



    JS_ASSERT(!InParallelSection());

    PrepareZoneForGC(zone);

    JSRuntime *rt = cx->runtime();

    
    AutoKeepAtoms keepAtoms(cx->perThreadData);
    GC(rt, GC_NORMAL, JS::gcreason::LAST_DITCH);

    




    size_t thingSize = Arena::thingSize(thingKind);
    if (void *thing = zone->allocator.arenas.allocateFromFreeList(thingKind, thingSize))
        return thing;

    return nullptr;
}

template <AllowGC allowGC>
 void *
ArenaLists::refillFreeList(ThreadSafeContext *cx, AllocKind thingKind)
{
    JS_ASSERT(cx->allocator()->arenas.freeLists[thingKind].isEmpty());
    JS_ASSERT_IF(cx->isJSContext(), !cx->asJSContext()->runtime()->isHeapBusy());

    Zone *zone = cx->allocator()->zone_;

    bool runGC = cx->allowGC() && allowGC &&
                 cx->asJSContext()->runtime()->gc.incrementalState != NO_INCREMENTAL &&
                 zone->gcBytes > zone->gcTriggerBytes;

#ifdef JS_THREADSAFE
    JS_ASSERT_IF(cx->isJSContext() && allowGC,
                 !cx->asJSContext()->runtime()->currentThreadHasExclusiveAccess());
#endif

    for (;;) {
        if (MOZ_UNLIKELY(runGC)) {
            if (void *thing = RunLastDitchGC(cx->asJSContext(), zone, thingKind))
                return thing;
        }

        if (cx->isJSContext()) {
            







            for (bool secondAttempt = false; ; secondAttempt = true) {
                void *thing = cx->allocator()->arenas.allocateFromArenaInline(zone, thingKind);
                if (MOZ_LIKELY(!!thing))
                    return thing;
                if (secondAttempt)
                    break;

                cx->asJSContext()->runtime()->gc.helperThread.waitBackgroundSweepEnd();
            }
        } else {
#ifdef JS_THREADSAFE
            






            mozilla::Maybe<AutoLockWorkerThreadState> lock;
            JSRuntime *rt = zone->runtimeFromAnyThread();
            if (rt->exclusiveThreadsPresent()) {
                lock.construct();
                while (rt->isHeapBusy())
                    WorkerThreadState().wait(GlobalWorkerThreadState::PRODUCER);
            }

            void *thing = cx->allocator()->arenas.allocateFromArenaInline(zone, thingKind);
            if (thing)
                return thing;
#else
            MOZ_CRASH();
#endif
        }

        if (!cx->allowGC() || !allowGC)
            return nullptr;

        



        if (runGC)
            break;
        runGC = true;
    }

    JS_ASSERT(allowGC);
    js_ReportOutOfMemory(cx);
    return nullptr;
}

template void *
ArenaLists::refillFreeList<NoGC>(ThreadSafeContext *cx, AllocKind thingKind);

template void *
ArenaLists::refillFreeList<CanGC>(ThreadSafeContext *cx, AllocKind thingKind);

JSGCTraceKind
js_GetGCThingTraceKind(void *thing)
{
    return GetGCThingTraceKind(thing);
}

 int64_t
SliceBudget::TimeBudget(int64_t millis)
{
    return millis * PRMJ_USEC_PER_MSEC;
}

 int64_t
SliceBudget::WorkBudget(int64_t work)
{
    
    return -work - 1;
}

SliceBudget::SliceBudget()
{
    reset();
}

SliceBudget::SliceBudget(int64_t budget)
{
    if (budget == Unlimited) {
        reset();
    } else if (budget > 0) {
        deadline = PRMJ_Now() + budget;
        counter = CounterReset;
    } else {
        deadline = 0;
        counter = -budget - 1;
    }
}

bool
SliceBudget::checkOverBudget()
{
    bool over = PRMJ_Now() > deadline;
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
GCRuntime::requestInterrupt(JS::gcreason::Reason reason)
{
    if (isNeeded)
        return;

    isNeeded = true;
    triggerReason = reason;
    rt->requestInterrupt(JSRuntime::RequestInterruptMainThread);
}

bool
js::TriggerGC(JSRuntime *rt, JS::gcreason::Reason reason)
{
    return rt->gc.triggerGC(reason);
}

bool
GCRuntime::triggerGC(JS::gcreason::Reason reason)
{
    
    if (InParallelSection()) {
        ForkJoinContext::current()->requestGC(reason);
        return true;
    }

    
    if (rt->currentThreadOwnsInterruptLock())
        return false;

    JS_ASSERT(CurrentThreadCanAccessRuntime(rt));

    
    if (rt->isHeapCollecting())
        return false;

    JS::PrepareForFullGC(rt);
    requestInterrupt(reason);
    return true;
}

bool
js::TriggerZoneGC(Zone *zone, JS::gcreason::Reason reason)
{
    return zone->runtimeFromAnyThread()->gc.triggerZoneGC(zone,reason);
}

bool
GCRuntime::triggerZoneGC(Zone *zone, JS::gcreason::Reason reason)
{
    



    if (InParallelSection()) {
        ForkJoinContext::current()->requestZoneGC(zone, reason);
        return true;
    }

    
    if (zone->usedByExclusiveThread)
        return false;

    
    if (rt->currentThreadOwnsInterruptLock())
        return false;

    
    if (rt->isHeapCollecting())
        return false;

#ifdef JS_GC_ZEAL
    if (zealMode == ZealAllocValue) {
        TriggerGC(rt, reason);
        return true;
    }
#endif

    if (rt->isAtomsZone(zone)) {
        
        TriggerGC(rt, reason);
        return true;
    }

    PrepareZoneForGC(zone);
    requestInterrupt(reason);
    return true;
}

void
js::MaybeGC(JSContext *cx)
{
    cx->runtime()->gc.maybeGC(cx->zone());
}

void
GCRuntime::maybeGC(Zone *zone)
{
    JS_ASSERT(CurrentThreadCanAccessRuntime(rt));

#ifdef JS_GC_ZEAL
    if (zealMode == ZealAllocValue || zealMode == ZealPokeValue) {
        JS::PrepareForFullGC(rt);
        GC(rt, GC_NORMAL, JS::gcreason::MAYBEGC);
        return;
    }
#endif

    if (isNeeded) {
        GCSlice(rt, GC_NORMAL, JS::gcreason::MAYBEGC);
        return;
    }

    double factor = highFrequencyGC ? 0.85 : 0.9;
    if (zone->gcBytes > 1024 * 1024 &&
        zone->gcBytes >= factor * zone->gcTriggerBytes &&
        incrementalState == NO_INCREMENTAL &&
        !helperThread.sweeping())
    {
        PrepareZoneForGC(zone);
        GCSlice(rt, GC_NORMAL, JS::gcreason::MAYBEGC);
        return;
    }

#ifndef JS_MORE_DETERMINISTIC
    




    int64_t now = PRMJ_Now();
    if (nextFullGCTime && nextFullGCTime <= now) {
        if (chunkAllocationSinceLastGC ||
            numArenasFreeCommitted > decommitThreshold)
        {
            JS::PrepareForFullGC(rt);
            GCSlice(rt, GC_SHRINK, JS::gcreason::MAYBEGC);
        } else {
            nextFullGCTime = now + GC_IDLE_FULL_SPAN;
        }
    }
#endif
}

static void
DecommitArenasFromAvailableList(JSRuntime *rt, Chunk **availableListHeadp)
{
    Chunk *chunk = *availableListHeadp;
    if (!chunk)
        return;

    





















    JS_ASSERT(chunk->info.prevp == availableListHeadp);
    while (Chunk *next = chunk->info.next) {
        JS_ASSERT(next->info.prevp == &chunk->info.next);
        chunk = next;
    }

    for (;;) {
        while (chunk->info.numArenasFreeCommitted != 0) {
            ArenaHeader *aheader = chunk->fetchNextFreeArena(rt);

            Chunk **savedPrevp = chunk->info.prevp;
            if (!chunk->hasAvailableArenas())
                chunk->removeFromAvailableList();

            size_t arenaIndex = Chunk::arenaIndex(aheader->arenaAddress());
            bool ok;
            {
                




                Maybe<AutoUnlockGC> maybeUnlock;
                if (!rt->isHeapBusy())
                    maybeUnlock.construct(rt);
                ok = rt->gc.pageAllocator.markPagesUnused(aheader->getArena(), ArenaSize);
            }

            if (ok) {
                ++chunk->info.numArenasFree;
                chunk->decommittedArenas.set(arenaIndex);
            } else {
                chunk->addArenaToFreeList(rt, aheader);
            }
            JS_ASSERT(chunk->hasAvailableArenas());
            JS_ASSERT(!chunk->unused());
            if (chunk->info.numArenasFree == 1) {
                






                Chunk **insertPoint = savedPrevp;
                if (savedPrevp != availableListHeadp) {
                    Chunk *prev = Chunk::fromPointerToNext(savedPrevp);
                    if (!prev->hasAvailableArenas())
                        insertPoint = availableListHeadp;
                }
                chunk->insertToAvailableList(insertPoint);
            } else {
                JS_ASSERT(chunk->info.prevp);
            }

            if (rt->gc.chunkAllocationSinceLastGC || !ok) {
                



                return;
            }
        }

        



        JS_ASSERT_IF(chunk->info.prevp, *chunk->info.prevp == chunk);
        if (chunk->info.prevp == availableListHeadp || !chunk->info.prevp)
            break;

        



        chunk = chunk->getPrevious();
    }
}

static void
DecommitArenas(JSRuntime *rt)
{
    DecommitArenasFromAvailableList(rt, &rt->gc.systemAvailableChunkListHead);
    DecommitArenasFromAvailableList(rt, &rt->gc.userAvailableChunkListHead);
}


static void
ExpireChunksAndArenas(JSRuntime *rt, bool shouldShrink)
{
    if (Chunk *toFree = rt->gc.chunkPool.expire(rt, shouldShrink)) {
        AutoUnlockGC unlock(rt);
        FreeChunkList(rt, toFree);
    }

    if (shouldShrink)
        DecommitArenas(rt);
}

static void
SweepBackgroundThings(JSRuntime* rt, bool onBackgroundThread)
{
    



    FreeOp fop(rt, false);
    for (int phase = 0 ; phase < BackgroundPhaseCount ; ++phase) {
        for (Zone *zone = rt->gc.sweepingZones; zone; zone = zone->gcNextGraphNode) {
            for (int index = 0 ; index < BackgroundPhaseLength[phase] ; ++index) {
                AllocKind kind = BackgroundPhases[phase][index];
                ArenaHeader *arenas = zone->allocator.arenas.arenaListsToSweep[kind];
                if (arenas)
                    ArenaLists::backgroundFinalize(&fop, arenas, onBackgroundThread);
            }
        }
    }

    rt->gc.sweepingZones = nullptr;
}

#ifdef JS_THREADSAFE
static void
AssertBackgroundSweepingFinished(JSRuntime *rt)
{
    JS_ASSERT(!rt->gc.sweepingZones);
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        for (unsigned i = 0; i < FINALIZE_LIMIT; ++i) {
            JS_ASSERT(!zone->allocator.arenas.arenaListsToSweep[i]);
            JS_ASSERT(zone->allocator.arenas.doneBackgroundFinalize(AllocKind(i)));
        }
    }
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
#endif 

bool
GCHelperThread::init()
{
    if (!rt->useHelperThreads()) {
        backgroundAllocation = false;
        return true;
    }

#ifdef JS_THREADSAFE
    if (!(wakeup = PR_NewCondVar(rt->gc.lock)))
        return false;
    if (!(done = PR_NewCondVar(rt->gc.lock)))
        return false;

    thread = PR_CreateThread(PR_USER_THREAD, threadMain, this, PR_PRIORITY_NORMAL,
                             PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);
    if (!thread)
        return false;

    backgroundAllocation = (GetCPUCount() >= 2);
#endif 
    return true;
}

void
GCHelperThread::finish()
{
    if (!rt->useHelperThreads() || !rt->gc.lock) {
        JS_ASSERT(state == IDLE);
        return;
    }

#ifdef JS_THREADSAFE
    PRThread *join = nullptr;
    {
        AutoLockGC lock(rt);
        if (thread && state != SHUTDOWN) {
            



            JS_ASSERT(state == IDLE || state == SWEEPING);
            if (state == IDLE)
                PR_NotifyCondVar(wakeup);
            state = SHUTDOWN;
            join = thread;
        }
    }
    if (join) {
        
        PR_JoinThread(join);
    }
    if (wakeup)
        PR_DestroyCondVar(wakeup);
    if (done)
        PR_DestroyCondVar(done);
#endif 
}

#ifdef JS_THREADSAFE
#ifdef MOZ_NUWA_PROCESS
extern "C" {
MFBT_API bool IsNuwaProcess();
MFBT_API void NuwaMarkCurrentThread(void (*recreate)(void *), void *arg);
}
#endif


void
GCHelperThread::threadMain(void *arg)
{
    PR_SetCurrentThreadName("JS GC Helper");

#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess && IsNuwaProcess()) {
        JS_ASSERT(NuwaMarkCurrentThread != nullptr);
        NuwaMarkCurrentThread(nullptr, nullptr);
    }
#endif

    static_cast<GCHelperThread *>(arg)->threadLoop();
}

void
GCHelperThread::wait(PRCondVar *which)
{
    rt->gc.lockOwner = nullptr;
    PR_WaitCondVar(which, PR_INTERVAL_NO_TIMEOUT);
#ifdef DEBUG
    rt->gc.lockOwner = PR_GetCurrentThread();
#endif
}

void
GCHelperThread::threadLoop()
{
    AutoLockGC lock(rt);

    TraceLogger *logger = TraceLoggerForCurrentThread();

    




    for (;;) {
        switch (state) {
          case SHUTDOWN:
            return;
          case IDLE:
            wait(wakeup);
            break;
          case SWEEPING: {
            AutoTraceLog logSweeping(logger, TraceLogger::GCSweeping);
            doSweep();
            if (state == SWEEPING)
                state = IDLE;
            PR_NotifyAllCondVar(done);
            break;
          }
          case ALLOCATING: {
            AutoTraceLog logAllocating(logger, TraceLogger::GCAllocation);
            do {
                Chunk *chunk;
                {
                    AutoUnlockGC unlock(rt);
                    chunk = Chunk::allocate(rt);
                }

                
                if (!chunk)
                    break;
                JS_ASSERT(chunk->info.numArenasFreeCommitted == 0);
                rt->gc.chunkPool.put(chunk);
            } while (state == ALLOCATING && rt->gc.wantBackgroundAllocation());
            if (state == ALLOCATING)
                state = IDLE;
            break;
          }
          case CANCEL_ALLOCATION:
            state = IDLE;
            PR_NotifyAllCondVar(done);
            break;
        }
    }
}
#endif 

void
GCHelperThread::startBackgroundSweep(bool shouldShrink)
{
    JS_ASSERT(rt->useHelperThreads());

#ifdef JS_THREADSAFE
    AutoLockGC lock(rt);
    JS_ASSERT(state == IDLE);
    JS_ASSERT(!sweepFlag);
    sweepFlag = true;
    shrinkFlag = shouldShrink;
    state = SWEEPING;
    PR_NotifyCondVar(wakeup);
#endif 
}


void
GCHelperThread::startBackgroundShrink()
{
    JS_ASSERT(rt->useHelperThreads());

#ifdef JS_THREADSAFE
    switch (state) {
      case IDLE:
        JS_ASSERT(!sweepFlag);
        shrinkFlag = true;
        state = SWEEPING;
        PR_NotifyCondVar(wakeup);
        break;
      case SWEEPING:
        shrinkFlag = true;
        break;
      case ALLOCATING:
      case CANCEL_ALLOCATION:
        



        break;
      case SHUTDOWN:
        MOZ_ASSUME_UNREACHABLE("No shrink on shutdown");
    }
#endif 
}

void
GCHelperThread::waitBackgroundSweepEnd()
{
    if (!rt->useHelperThreads()) {
        JS_ASSERT(state == IDLE);
        return;
    }

#ifdef JS_THREADSAFE
    AutoLockGC lock(rt);
    while (state == SWEEPING)
        wait(done);
    if (rt->gc.incrementalState == NO_INCREMENTAL)
        AssertBackgroundSweepingFinished(rt);
#endif 
}

void
GCHelperThread::waitBackgroundSweepOrAllocEnd()
{
    if (!rt->useHelperThreads()) {
        JS_ASSERT(state == IDLE);
        return;
    }

#ifdef JS_THREADSAFE
    AutoLockGC lock(rt);
    if (state == ALLOCATING)
        state = CANCEL_ALLOCATION;
    while (state == SWEEPING || state == CANCEL_ALLOCATION)
        wait(done);
    if (rt->gc.incrementalState == NO_INCREMENTAL)
        AssertBackgroundSweepingFinished(rt);
#endif 
}


inline void
GCHelperThread::startBackgroundAllocationIfIdle()
{
    JS_ASSERT(rt->useHelperThreads());

#ifdef JS_THREADSAFE
    if (state == IDLE) {
        state = ALLOCATING;
        PR_NotifyCondVar(wakeup);
    }
#endif 
}

void
GCHelperThread::replenishAndFreeLater(void *ptr)
{
    JS_ASSERT(freeCursor == freeCursorEnd);
    do {
        if (freeCursor && !freeVector.append(freeCursorEnd - FREE_ARRAY_LENGTH))
            break;
        freeCursor = (void **) js_malloc(FREE_ARRAY_SIZE);
        if (!freeCursor) {
            freeCursorEnd = nullptr;
            break;
        }
        freeCursorEnd = freeCursor + FREE_ARRAY_LENGTH;
        *freeCursor++ = ptr;
        return;
    } while (false);
    js_free(ptr);
}

#ifdef JS_THREADSAFE

void
GCHelperThread::doSweep()
{
    if (sweepFlag) {
        sweepFlag = false;
        AutoUnlockGC unlock(rt);

        SweepBackgroundThings(rt, true);

        if (freeCursor) {
            void **array = freeCursorEnd - FREE_ARRAY_LENGTH;
            freeElementsAndArray(array, freeCursor);
            freeCursor = freeCursorEnd = nullptr;
        } else {
            JS_ASSERT(!freeCursorEnd);
        }
        for (void ***iter = freeVector.begin(); iter != freeVector.end(); ++iter) {
            void **array = *iter;
            freeElementsAndArray(array, array + FREE_ARRAY_LENGTH);
        }
        freeVector.resize(0);

        rt->freeLifoAlloc.freeAll();
    }

    bool shrinking = shrinkFlag;
    ExpireChunksAndArenas(rt, shrinking);

    




    if (!shrinking && shrinkFlag) {
        shrinkFlag = false;
        ExpireChunksAndArenas(rt, true);
    }
}
#endif 

bool
GCHelperThread::onBackgroundThread()
{
#ifdef JS_THREADSAFE
    return PR_GetCurrentThread() == getThread();
#else
    return false;
#endif
}

bool
GCRuntime::releaseObservedTypes()
{
    bool releaseTypes = false;

#ifdef JS_GC_ZEAL
    if (zealMode != 0)
        releaseTypes = true;
#endif

#ifndef JS_MORE_DETERMINISTIC
    int64_t now = PRMJ_Now();
    if (now >= jitReleaseTime)
        releaseTypes = true;
    if (releaseTypes)
        jitReleaseTime = now + JIT_SCRIPT_RELEASE_TYPES_INTERVAL;
#endif

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
        JS_ASSERT(!rt->isAtomsCompartment(comp));

        



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
    JS_ASSERT_IF(keepAtleastOne, !compartments.empty());
}

void
GCRuntime::sweepZones(FreeOp *fop, bool lastGC)
{
    JSZoneCallback callback = rt->destroyZoneCallback;

    
    Zone **read = zones.begin() + 1;
    Zone **end = zones.end();
    Zone **write = read;
    JS_ASSERT(zones.length() >= 1);
    JS_ASSERT(rt->isAtomsZone(zones[0]));

    while (read < end) {
        Zone *zone = *read++;

        if (zone->wasGCStarted()) {
            if ((zone->allocator.arenas.arenaListsAreEmpty() && !zone->hasMarkedCompartments()) ||
                lastGC)
            {
                zone->allocator.arenas.checkEmptyFreeLists();
                if (callback)
                    callback(zone);
                zone->sweepCompartments(fop, false, lastGC);
                JS_ASSERT(zone->compartments.empty());
                fop->delete_(zone);
                continue;
            }
            zone->sweepCompartments(fop, true, lastGC);
        }
        *write++ = zone;
    }
    zones.resize(write - zones.begin());
}

static void
PurgeRuntime(JSRuntime *rt)
{
    for (GCCompartmentsIter comp(rt); !comp.done(); comp.next())
        comp->purge();

    rt->freeLifoAlloc.transferUnusedFrom(&rt->tempLifoAlloc);
    rt->interpreterStack().purge(rt);

    rt->gsnCache.purge();
    rt->scopeCoordinateNameCache.purge();
    rt->newObjectCache.purge();
    rt->nativeIterCache.purge();
    rt->sourceDataCache.purge();
    rt->evalCache.clear();

    if (!rt->hasActiveCompilations())
        rt->parseMapPool().purgeAll();
}

bool
GCRuntime::shouldPreserveJITCode(JSCompartment *comp, int64_t currentTime)
{
    if (shouldCleanUpEverything)
        return false;

    if (alwaysPreserveCode)
        return true;
    if (comp->lastAnimationTime + PRMJ_USEC_PER_SEC >= currentTime)
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
    JS_ASSERT(thingCompartment == trc->compartment ||
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
    Cell *thing = (Cell *)*thingp;

    JSCompartment *comp = CompartmentOfCell(thing, kind);
    if (comp && trc->compartment) {
        CheckCompartment(trc, comp, thing, kind);
    } else {
        JS_ASSERT(thing->tenuredZone() == trc->zone ||
                  trc->runtime()->isAtomsZone(thing->tenuredZone()));
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
GCRuntime::beginMarkPhase()
{
    int64_t currentTime = PRMJ_Now();

#ifdef DEBUG
    if (fullCompartmentChecks)
        checkForCompartmentMismatches();
#endif

    isFull = true;
    bool any = false;

    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        
        JS_ASSERT(!zone->isCollecting());
        JS_ASSERT(!zone->compartments.empty());
        for (unsigned i = 0; i < FINALIZE_LIMIT; ++i)
            JS_ASSERT(!zone->allocator.arenas.arenaListsToSweep[i]);

        
        if (zone->isGCScheduled()) {
            if (!rt->isAtomsZone(zone)) {
                any = true;
                zone->setGCState(Zone::Mark);
            }
        } else {
            isFull = false;
        }

        zone->scheduledForDestruction = false;
        zone->maybeAlive = false;
        zone->setPreservingCode(false);
    }

    for (CompartmentsIter c(rt, WithAtoms); !c.done(); c.next()) {
        JS_ASSERT(c->gcLiveArrayBuffers.empty());
        c->marked = false;
        if (shouldPreserveJITCode(c, currentTime))
            c->zone()->setPreservingCode(true);
    }

    if (!rt->gc.shouldCleanUpEverything) {
#ifdef JS_ION
        if (JSCompartment *comp = jit::TopmostIonActivationCompartment(rt))
            comp->zone()->setPreservingCode(true);
#endif
    }

    









    if (isFull && !rt->keepAtoms()) {
        Zone *atomsZone = rt->atomsCompartment()->zone();
        if (atomsZone->isGCScheduled()) {
            JS_ASSERT(!atomsZone->isCollecting());
            atomsZone->setGCState(Zone::Mark);
            any = true;
        }
    }

    
    if (!any)
        return false;

    






    if (isIncremental) {
        for (GCZonesIter zone(rt); !zone.done(); zone.next())
            zone->allocator.arenas.purge();
    }

    marker.start();
    JS_ASSERT(!marker.callback);
    JS_ASSERT(IS_GC_MARKING_TRACER(&marker));

    
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
        PurgeRuntime(rt);
    }

    


    gcstats::AutoPhase ap1(stats, gcstats::PHASE_MARK);
    gcstats::AutoPhase ap2(stats, gcstats::PHASE_MARK_ROOTS);

    for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
        
        zone->allocator.arenas.unmarkAll();
    }

    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        
        WeakMapBase::unmarkCompartment(c);
    }

    if (isFull)
        UnmarkScriptData(rt);

    markRuntime(gcmarker);
    if (isIncremental)
        bufferGrayRoots();

    





























    
    for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            Cell *dst = e.front().key().wrapped;
            dst->tenuredZone()->maybeAlive = true;
        }
    }

    




    for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
        if (!zone->maybeAlive && !rt->isAtomsZone(zone))
            zone->scheduledForDestruction = true;
    }
    foundBlackGrayEdges = false;

    return true;
}

template <class CompartmentIterT>
void
GCRuntime::markWeakReferences(gcstats::Phase phase)
{
    JS_ASSERT(marker.isDrained());

    gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP_MARK);
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
    JS_ASSERT(marker.isDrained());
}

void
GCRuntime::markWeakReferencesInCurrentGroup(gcstats::Phase phase)
{
    markWeakReferences<GCCompartmentGroupIter>(phase);
}

template <class ZoneIterT, class CompartmentIterT>
void
GCRuntime::markGrayReferences()
{
    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP_MARK);
        gcstats::AutoPhase ap1(stats, gcstats::PHASE_SWEEP_MARK_GRAY);
        marker.setMarkColorGray();
        if (marker.hasBufferedGrayRoots()) {
            for (ZoneIterT zone(rt); !zone.done(); zone.next())
                marker.markBufferedGrayRoots(zone);
        } else {
            JS_ASSERT(!isIncremental);
            if (JSTraceDataOp op = grayRootTracer.op)
                (*op)(&marker, grayRootTracer.data);
        }
        SliceBudget budget;
        marker.drainMarkStack(budget);
    }

    markWeakReferences<CompartmentIterT>(gcstats::PHASE_SWEEP_MARK_GRAY_WEAK);

    JS_ASSERT(marker.isDrained());

    marker.setMarkColorBlack();
}

void
GCRuntime::markGrayReferencesInCurrentGroup()
{
    markGrayReferences<GCZoneGroupIter, GCCompartmentGroupIter>();
}

#ifdef DEBUG

void
GCRuntime::markAllWeakReferences(gcstats::Phase phase)
{
    markWeakReferences<GCCompartmentsIter>(phase);
}

void
GCRuntime::markAllGrayReferences()
{
    markGrayReferences<GCZonesIter, GCCompartmentsIter>();
}

class js::gc::MarkingValidator
{
  public:
    MarkingValidator(GCRuntime *gc);
    ~MarkingValidator();
    void nonIncrementalMark();
    void validate();

  private:
    GCRuntime *gc;
    bool initialized;

    typedef HashMap<Chunk *, ChunkBitmap *, GCChunkHasher, SystemAllocPolicy> BitmapMap;
    BitmapMap map;
};

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

    
    for (GCChunkSet::Range r(gc->chunkSet.all()); !r.empty(); r.popFront()) {
        ChunkBitmap *bitmap = &r.front()->bitmap;
	ChunkBitmap *entry = js_new<ChunkBitmap>();
        if (!entry)
            return;

        memcpy((void *)entry->bitmap, (void *)bitmap->bitmap, sizeof(bitmap->bitmap));
        if (!map.putNew(r.front(), entry))
            return;
    }

    




    WeakMapSet markedWeakMaps;
    if (!markedWeakMaps.init())
        return;

    ArrayBufferVector arrayBuffers;
    for (GCCompartmentsIter c(runtime); !c.done(); c.next()) {
        if (!WeakMapBase::saveCompartmentMarkedWeakMaps(c, markedWeakMaps) ||
            !ArrayBufferObject::saveArrayBufferList(c, arrayBuffers))
        {
            return;
        }
    }

    



    initialized = true;

    for (GCCompartmentsIter c(runtime); !c.done(); c.next()) {
        WeakMapBase::unmarkCompartment(c);
        ArrayBufferObject::resetArrayBufferList(c);
    }

    
    js::gc::State state = gc->incrementalState;
    gc->incrementalState = MARK_ROOTS;

    JS_ASSERT(gcmarker->isDrained());
    gcmarker->reset();

    for (GCChunkSet::Range r(gc->chunkSet.all()); !r.empty(); r.popFront())
        r.front()->bitmap.clear();

    {
        gcstats::AutoPhase ap1(gc->stats, gcstats::PHASE_MARK);
        gcstats::AutoPhase ap2(gc->stats, gcstats::PHASE_MARK_ROOTS);
        gc->markRuntime(gcmarker, true);
    }

    {
        gcstats::AutoPhase ap1(gc->stats, gcstats::PHASE_MARK);
        SliceBudget budget;
        gc->incrementalState = MARK;
        gc->marker.drainMarkStack(budget);
    }

    gc->incrementalState = SWEEP;
    {
        gcstats::AutoPhase ap(gc->stats, gcstats::PHASE_SWEEP);
        gc->markAllWeakReferences(gcstats::PHASE_SWEEP_MARK_WEAK);

        
        for (GCZonesIter zone(runtime); !zone.done(); zone.next()) {
            JS_ASSERT(zone->isGCMarkingBlack());
            zone->setGCState(Zone::MarkGray);
        }

        gc->markAllGrayReferences();

        
        for (GCZonesIter zone(runtime); !zone.done(); zone.next()) {
            JS_ASSERT(zone->isGCMarkingGray());
            zone->setGCState(Zone::Mark);
        }
    }

    
    for (GCChunkSet::Range r(gc->chunkSet.all()); !r.empty(); r.popFront()) {
        Chunk *chunk = r.front();
        ChunkBitmap *bitmap = &chunk->bitmap;
        ChunkBitmap *entry = map.lookup(chunk)->value();
        Swap(*entry, *bitmap);
    }

    for (GCCompartmentsIter c(runtime); !c.done(); c.next()) {
        WeakMapBase::unmarkCompartment(c);
        ArrayBufferObject::resetArrayBufferList(c);
    }
    WeakMapBase::restoreCompartmentMarkedWeakMaps(markedWeakMaps);
    ArrayBufferObject::restoreArrayBufferLists(arrayBuffers);

    gc->incrementalState = state;
}

void
js::gc::MarkingValidator::validate()
{
    




    if (!initialized)
        return;

    for (GCChunkSet::Range r(gc->chunkSet.all()); !r.empty(); r.popFront()) {
        Chunk *chunk = r.front();
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

                



                JS_ASSERT_IF(bitmap->isMarked(cell, BLACK), incBitmap->isMarked(cell, BLACK));

                




                JS_ASSERT_IF(!bitmap->isMarked(cell, GRAY), !incBitmap->isMarked(cell, GRAY));

                thing += Arena::thingSize(kind);
            }
        }
    }
}

#endif

void
GCRuntime::computeNonIncrementalMarkingForValidation()
{
#ifdef DEBUG
    JS_ASSERT(!markingValidator);
    if (isIncremental && validate)
        markingValidator = js_new<MarkingValidator>(this);
    if (markingValidator)
        markingValidator->nonIncrementalMark();
#endif
}

void
GCRuntime::validateIncrementalMarking()
{
#ifdef DEBUG
    if (markingValidator)
        markingValidator->validate();
#endif
}

void
GCRuntime::finishMarkingValidation()
{
#ifdef DEBUG
    js_delete(markingValidator);
    markingValidator = nullptr;
#endif
}

static void
AssertNeedsBarrierFlagsConsistent(JSRuntime *rt)
{
#ifdef DEBUG
    bool anyNeedsBarrier = false;
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next())
        anyNeedsBarrier |= zone->needsBarrier();
    JS_ASSERT(rt->needsBarrier() == anyNeedsBarrier);
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
        JS_ASSERT(kind != CrossCompartmentKey::StringWrapper);
        Cell *other = e.front().key().wrapped;
        if (kind == CrossCompartmentKey::ObjectWrapper) {
            




            if (!other->isMarked(BLACK) || other->isMarked(GRAY)) {
                JS::Zone *w = other->tenuredZone();
                if (w->isGCMarking())
                    finder.addEdgeTo(w);
            }
        } else {
            JS_ASSERT(kind == CrossCompartmentKey::DebuggerScript ||
                      kind == CrossCompartmentKey::DebuggerSource ||
                      kind == CrossCompartmentKey::DebuggerObject ||
                      kind == CrossCompartmentKey::DebuggerEnvironment);
            




            JS::Zone *w = other->tenuredZone();
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

    for (ZoneSet::Range r = gcZoneGroupEdges.all(); !r.empty(); r.popFront())
        finder.addEdgeTo(r.front());
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
        JS_ASSERT(zone->isGCMarking());
        finder.addNode(zone);
    }
    zoneGroups = finder.getResultsList();
    currentZoneGroup = zoneGroups;
    zoneGroupIndex = 0;
    JS_ASSERT_IF(!isIncremental, !currentZoneGroup->nextGroup());
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

    if (!isIncremental)
        ComponentFinder<Zone>::mergeGroups(currentZoneGroup);

    if (abortSweepAfterCurrentGroup) {
        JS_ASSERT(!isIncremental);
        for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
            JS_ASSERT(!zone->gcNextGraphComponent);
            JS_ASSERT(zone->isGCMarking());
            zone->setNeedsBarrier(false, Zone::UpdateIon);
            zone->setGCState(Zone::NoGC);
            zone->gcGrayRoots.clearAndFree();
        }
        rt->setNeedsBarrier(false);
        AssertNeedsBarrierFlagsConsistent(rt);

        for (GCCompartmentGroupIter comp(rt); !comp.done(); comp.next()) {
            ArrayBufferObject::resetArrayBufferList(comp);
            ResetGrayList(comp);
        }

        abortSweepAfterCurrentGroup = false;
        currentZoneGroup = nullptr;
    }
}



























static bool
IsGrayListObject(JSObject *obj)
{
    JS_ASSERT(obj);
    return obj->is<CrossCompartmentWrapperObject>() && !IsDeadProxyObject(obj);
}

 unsigned
ProxyObject::grayLinkSlot(JSObject *obj)
{
    JS_ASSERT(IsGrayListObject(obj));
    return ProxyObject::EXTRA_SLOT + 1;
}

#ifdef DEBUG
static void
AssertNotOnGrayList(JSObject *obj)
{
    JS_ASSERT_IF(IsGrayListObject(obj),
                 obj->getReservedSlot(ProxyObject::grayLinkSlot(obj)).isUndefined());
}
#endif

static JSObject *
CrossCompartmentPointerReferent(JSObject *obj)
{
    JS_ASSERT(IsGrayListObject(obj));
    return &obj->as<ProxyObject>().private_().toObject();
}

static JSObject *
NextIncomingCrossCompartmentPointer(JSObject *prev, bool unlink)
{
    unsigned slot = ProxyObject::grayLinkSlot(prev);
    JSObject *next = prev->getReservedSlot(slot).toObjectOrNull();
    JS_ASSERT_IF(next, IsGrayListObject(next));

    if (unlink)
        prev->setSlot(slot, UndefinedValue());

    return next;
}

void
js::DelayCrossCompartmentGrayMarking(JSObject *src)
{
    JS_ASSERT(IsGrayListObject(src));

    
    unsigned slot = ProxyObject::grayLinkSlot(src);
    JSObject *dest = CrossCompartmentPointerReferent(src);
    JSCompartment *comp = dest->compartment();

    if (src->getReservedSlot(slot).isUndefined()) {
        src->setCrossCompartmentSlot(slot, ObjectOrNullValue(comp->gcIncomingGrayPointers));
        comp->gcIncomingGrayPointers = src;
    } else {
        JS_ASSERT(src->getReservedSlot(slot).isObjectOrNull());
    }

#ifdef DEBUG
    



    JSObject *obj = comp->gcIncomingGrayPointers;
    bool found = false;
    while (obj) {
        if (obj == src)
            found = true;
        obj = NextIncomingCrossCompartmentPointer(obj, false);
    }
    JS_ASSERT(found);
#endif
}

static void
MarkIncomingCrossCompartmentPointers(JSRuntime *rt, const uint32_t color)
{
    JS_ASSERT(color == BLACK || color == GRAY);

    gcstats::AutoPhase ap(rt->gc.stats, gcstats::PHASE_SWEEP_MARK);
    static const gcstats::Phase statsPhases[] = {
        gcstats::PHASE_SWEEP_MARK_INCOMING_BLACK,
        gcstats::PHASE_SWEEP_MARK_INCOMING_GRAY
    };
    gcstats::AutoPhase ap1(rt->gc.stats, statsPhases[color]);

    bool unlinkList = color == GRAY;

    for (GCCompartmentGroupIter c(rt); !c.done(); c.next()) {
        JS_ASSERT_IF(color == GRAY, c->zone()->isGCMarkingGray());
        JS_ASSERT_IF(color == BLACK, c->zone()->isGCMarkingBlack());
        JS_ASSERT_IF(c->gcIncomingGrayPointers, IsGrayListObject(c->gcIncomingGrayPointers));

        for (JSObject *src = c->gcIncomingGrayPointers;
             src;
             src = NextIncomingCrossCompartmentPointer(src, unlinkList))
        {
            JSObject *dst = CrossCompartmentPointerReferent(src);
            JS_ASSERT(dst->compartment() == c);

            if (color == GRAY) {
                if (IsObjectMarked(&src) && src->isMarked(GRAY))
                    MarkGCThingUnbarriered(&rt->gc.marker, (void**)&dst,
                                           "cross-compartment gray pointer");
            } else {
                if (IsObjectMarked(&src) && !src->isMarked(GRAY))
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

    unsigned slot = ProxyObject::grayLinkSlot(wrapper);
    if (wrapper->getReservedSlot(slot).isUndefined())
        return false;  

    JSObject *tail = wrapper->getReservedSlot(slot).toObjectOrNull();
    wrapper->setReservedSlot(slot, UndefinedValue());

    JSCompartment *comp = CrossCompartmentPointerReferent(wrapper)->compartment();
    JSObject *obj = comp->gcIncomingGrayPointers;
    if (obj == wrapper) {
        comp->gcIncomingGrayPointers = tail;
        return true;
    }

    while (obj) {
        unsigned slot = ProxyObject::grayLinkSlot(obj);
        JSObject *next = obj->getReservedSlot(slot).toObjectOrNull();
        if (next == wrapper) {
            obj->setCrossCompartmentSlot(slot, ObjectOrNullValue(tail));
            return true;
        }
        obj = next;
    }

    MOZ_ASSUME_UNREACHABLE("object not found in gray link list");
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
    




    MarkIncomingCrossCompartmentPointers(rt, BLACK);

    markWeakReferencesInCurrentGroup(gcstats::PHASE_SWEEP_MARK_WEAK);

    





    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        JS_ASSERT(zone->isGCMarkingBlack());
        zone->setGCState(Zone::MarkGray);
    }

    
    marker.setMarkColorGray();
    MarkIncomingCrossCompartmentPointers(rt, GRAY);
    marker.setMarkColorBlack();

    
    markGrayReferencesInCurrentGroup();

    
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        JS_ASSERT(zone->isGCMarkingGray());
        zone->setGCState(Zone::Mark);
    }

    JS_ASSERT(marker.isDrained());
}

void
GCRuntime::beginSweepingZoneGroup()
{
    




    bool sweepingAtoms = false;
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        
        JS_ASSERT(zone->isGCMarking());
        zone->setGCState(Zone::Sweep);

        
        zone->allocator.arenas.purge();

        if (rt->isAtomsZone(zone))
            sweepingAtoms = true;

        if (rt->sweepZoneCallback)
            rt->sweepZoneCallback(zone);

        zone->gcLastZoneGroupIndex = zoneGroupIndex;
    }

    validateIncrementalMarking();

    FreeOp fop(rt, sweepOnBackgroundThread);

    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_FINALIZE_START);
        for (Callback<JSFinalizeCallback> *p = rt->gc.finalizeCallbacks.begin();
             p < rt->gc.finalizeCallbacks.end(); p++)
        {
            p->op(&fop, JSFINALIZE_GROUP_START, !isFull , p->data);
        }
    }

    if (sweepingAtoms) {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP_ATOMS);
        rt->sweepAtoms();
    }

    
    for (GCCompartmentGroupIter c(rt); !c.done(); c.next())
        ArrayBufferObject::sweep(c);

    
    WatchpointMap::sweepAll(rt);

    
    Debugger::sweepAll(&fop);

    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP_COMPARTMENTS);

        for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
            gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP_DISCARD_CODE);
            zone->discardJitCode(&fop);
        }

        bool releaseTypes = releaseObservedTypes();
        for (GCCompartmentGroupIter c(rt); !c.done(); c.next()) {
            gcstats::AutoSCC scc(stats, zoneGroupIndex);
            c->sweep(&fop, releaseTypes && !c->zone()->isPreservingCode());
        }

        for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
            gcstats::AutoSCC scc(stats, zoneGroupIndex);

            
            
            
            
            
            
            
            bool oom = false;
            zone->sweep(&fop, releaseTypes && !zone->isPreservingCode(), &oom);

            if (oom) {
                zone->setPreservingCode(false);
                zone->discardJitCode(&fop);
                zone->types.clearAllNewScriptAddendumsOnOOM();
            }
        }
    }

    







    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        gcstats::AutoSCC scc(stats, zoneGroupIndex);
        zone->allocator.arenas.queueObjectsForSweep(&fop);
    }
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        gcstats::AutoSCC scc(stats, zoneGroupIndex);
        zone->allocator.arenas.queueStringsForSweep(&fop);
    }
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        gcstats::AutoSCC scc(stats, zoneGroupIndex);
        zone->allocator.arenas.queueScriptsForSweep(&fop);
    }
#ifdef JS_ION
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        gcstats::AutoSCC scc(stats, zoneGroupIndex);
        zone->allocator.arenas.queueJitCodeForSweep(&fop);
    }
#endif
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        gcstats::AutoSCC scc(stats, zoneGroupIndex);
        zone->allocator.arenas.queueShapesForSweep(&fop);
        zone->allocator.arenas.gcShapeArenasToSweep =
            zone->allocator.arenas.arenaListsToSweep[FINALIZE_SHAPE];
    }

    finalizePhase = 0;
    sweepZone = currentZoneGroup;
    sweepKindIndex = 0;

    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_FINALIZE_END);
        for (Callback<JSFinalizeCallback> *p = rt->gc.finalizeCallbacks.begin();
             p < rt->gc.finalizeCallbacks.end(); p++)
        {
            p->op(&fop, JSFINALIZE_GROUP_END, !isFull , p->data);
        }
    }
}

void
GCRuntime::endSweepingZoneGroup()
{
    
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        JS_ASSERT(zone->isGCSweeping());
        zone->setGCState(Zone::Finished);
    }

    
    while (ArenaHeader *arena = arenasAllocatedDuringSweep) {
        arenasAllocatedDuringSweep = arena->getNextAllocDuringSweep();
        arena->unsetAllocDuringSweep();
    }
}

void
GCRuntime::beginSweepPhase(bool lastGC)
{
    







    JS_ASSERT(!abortSweepAfterCurrentGroup);

    computeNonIncrementalMarkingForValidation();

    gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP);

#ifdef JS_THREADSAFE
    sweepOnBackgroundThread = !lastGC && rt->useHelperThreads();
#endif

#ifdef DEBUG
    for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
        JS_ASSERT(!c->gcIncomingGrayPointers);
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
ArenaLists::foregroundFinalize(FreeOp *fop, AllocKind thingKind, SliceBudget &sliceBudget)
{
    if (!arenaListsToSweep[thingKind])
        return true;

    ArenaList &dest = arenaLists[thingKind];
    return FinalizeArenas(fop, &arenaListsToSweep[thingKind], dest, thingKind, sliceBudget);
}

bool
GCRuntime::drainMarkStack(SliceBudget &sliceBudget, gcstats::Phase phase)
{
    
    gcstats::AutoPhase ap(stats, phase);
    return marker.drainMarkStack(sliceBudget);
}

bool
GCRuntime::sweepPhase(SliceBudget &sliceBudget)
{
    gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP);
    FreeOp fop(rt, sweepOnBackgroundThread);

    bool finished = drainMarkStack(sliceBudget, gcstats::PHASE_SWEEP_MARK);
    if (!finished)
        return false;

    for (;;) {
        
        for (; finalizePhase < FinalizePhaseCount ; ++finalizePhase) {
            gcstats::AutoPhase ap(stats, FinalizePhaseStatsPhase[finalizePhase]);

            for (; sweepZone; sweepZone = sweepZone->nextNodeInGroup()) {
                Zone *zone = sweepZone;

                while (sweepKindIndex < FinalizePhaseLength[finalizePhase]) {
                    AllocKind kind = FinalizePhases[finalizePhase][sweepKindIndex];

                    if (!zone->allocator.arenas.foregroundFinalize(&fop, kind, sliceBudget))
                        return false;  

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
                while (ArenaHeader *arena = zone->allocator.arenas.gcShapeArenasToSweep) {
                    for (ArenaCellIterUnderGC i(arena); !i.done(); i.next()) {
                        Shape *shape = i.get<Shape>();
                        if (!shape->isMarked())
                            shape->sweep();
                    }

                    zone->allocator.arenas.gcShapeArenasToSweep = arena->next;
                    sliceBudget.step(Arena::thingsPerArena(Arena::thingSize(FINALIZE_SHAPE)));
                    if (sliceBudget.isOverBudget())
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
GCRuntime::endSweepPhase(JSGCInvocationKind gckind, bool lastGC)
{
    gcstats::AutoPhase ap(stats, gcstats::PHASE_SWEEP);
    FreeOp fop(rt, sweepOnBackgroundThread);

    JS_ASSERT_IF(lastGC, !sweepOnBackgroundThread);

    JS_ASSERT(marker.isDrained());
    marker.stop();

    



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
                zone->allocator.arenas.unmarkAll();
        }
    }

    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_DESTROY);

        





        if (isFull)
            SweepScriptData(rt);

        
        if (JSC::ExecutableAllocator *execAlloc = rt->maybeExecAlloc())
            execAlloc->purge();
#ifdef JS_ION
        if (rt->jitRuntime() && rt->jitRuntime()->hasIonAlloc()) {
            JSRuntime::AutoLockForInterrupt lock(rt);
            rt->jitRuntime()->ionAlloc(rt)->purge();
        }
#endif

        



        if (!lastGC)
            sweepZones(&fop, lastGC);

        if (!sweepOnBackgroundThread) {
            





            AutoLockGC lock(rt);
            ExpireChunksAndArenas(rt, gckind == GC_SHRINK);
        }
    }

    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_FINALIZE_END);

        for (Callback<JSFinalizeCallback> *p = rt->gc.finalizeCallbacks.begin();
             p < rt->gc.finalizeCallbacks.end(); p++)
        {
            p->op(&fop, JSFINALIZE_COLLECTION_END, !isFull, p->data);
        }

        
        if (isFull)
            grayBitsValid = true;
    }

    
    JS_ASSERT(!sweepingZones);
    for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
        zone->gcNextGraphNode = sweepingZones;
        sweepingZones = zone;
    }

    
    if (!sweepOnBackgroundThread) {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_DESTROY);

        SweepBackgroundThings(rt, false);

        rt->freeLifoAlloc.freeAll();

        
        if (lastGC)
            sweepZones(&fop, lastGC);
    }

    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        zone->setGCLastBytes(zone->gcBytes, gckind);
        if (zone->isCollecting()) {
            JS_ASSERT(zone->isGCFinished());
            zone->setGCState(Zone::NoGC);
        }

#ifdef DEBUG
        JS_ASSERT(!zone->isCollecting());
        JS_ASSERT(!zone->wasGCStarted());

        for (unsigned i = 0 ; i < FINALIZE_LIMIT ; ++i) {
            JS_ASSERT_IF(!IsBackgroundFinalized(AllocKind(i)) ||
                         !sweepOnBackgroundThread,
                         !zone->allocator.arenas.arenaListsToSweep[i]);
        }
#endif
    }

#ifdef DEBUG
    for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
        JS_ASSERT(!c->gcIncomingGrayPointers);
        JS_ASSERT(c->gcLiveArrayBuffers.empty());

        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            if (e.front().key().kind != CrossCompartmentKey::StringWrapper)
                AssertNotOnGrayList(&e.front().value().get().toObject());
        }
    }
#endif

    finishMarkingValidation();

    lastGCTime = PRMJ_Now();
}

namespace {


class AutoGCSession
{
    GCRuntime *gc;
    AutoTraceSession session;
    bool canceled;

  public:
    explicit AutoGCSession(GCRuntime *gc);
    ~AutoGCSession();

    void cancel() { canceled = true; }
};

} 


AutoTraceSession::AutoTraceSession(JSRuntime *rt, js::HeapState heapState)
  : lock(rt),
    runtime(rt),
    prevState(rt->gc.heapState)
{
    JS_ASSERT(rt->gc.isAllocAllowed());
    JS_ASSERT(rt->gc.heapState == Idle);
    JS_ASSERT(heapState != Idle);
#ifdef JSGC_GENERATIONAL
    JS_ASSERT_IF(heapState == MajorCollecting, rt->gc.nursery.isEmpty());
#endif

    
    
    
    
    JS_ASSERT(rt->currentThreadHasExclusiveAccess());

    if (rt->exclusiveThreadsPresent()) {
        
        
#ifdef JS_THREADSAFE
        AutoLockWorkerThreadState lock;
        rt->gc.heapState = heapState;
#else
        MOZ_CRASH();
#endif
    } else {
        rt->gc.heapState = heapState;
    }
}

AutoTraceSession::~AutoTraceSession()
{
    JS_ASSERT(runtime->isHeapBusy());

    if (runtime->exclusiveThreadsPresent()) {
#ifdef JS_THREADSAFE
        AutoLockWorkerThreadState lock;
        runtime->gc.heapState = prevState;

        
        WorkerThreadState().notifyAll(GlobalWorkerThreadState::PRODUCER);
#else
        MOZ_CRASH();
#endif
    } else {
        runtime->gc.heapState = prevState;
    }
}

AutoGCSession::AutoGCSession(GCRuntime *gc)
  : gc(gc),
    session(gc->rt, MajorCollecting),
    canceled(false)
{
    gc->isNeeded = false;
    gc->interFrameGC = true;

    gc->number++;

    
    
    JS_ASSERT(!gc->rt->mainThread.suppressGC);
}

AutoGCSession::~AutoGCSession()
{
    if (canceled)
        return;

#ifndef JS_MORE_DETERMINISTIC
    gc->nextFullGCTime = PRMJ_Now() + GC_IDLE_FULL_SPAN;
#endif

    gc->chunkAllocationSinceLastGC = false;

#ifdef JS_GC_ZEAL
    
    gc->selectedForMarking.clearAndFree();
#endif

    
    for (ZonesIter zone(gc->rt, WithAtoms); !zone.done(); zone.next()) {
        zone->resetGCMallocBytes();
        zone->unscheduleGC();
    }

    gc->rt->resetGCMallocBytes();
}

AutoCopyFreeListToArenas::AutoCopyFreeListToArenas(JSRuntime *rt, ZoneSelector selector)
  : runtime(rt),
    selector(selector)
{
    for (ZonesIter zone(rt, selector); !zone.done(); zone.next())
        zone->allocator.arenas.copyFreeListsToArenas();
}

AutoCopyFreeListToArenas::~AutoCopyFreeListToArenas()
{
    for (ZonesIter zone(runtime, selector); !zone.done(); zone.next())
        zone->allocator.arenas.clearFreeListsInArenas();
}

class AutoCopyFreeListToArenasForGC
{
    JSRuntime *runtime;

  public:
    AutoCopyFreeListToArenasForGC(JSRuntime *rt) : runtime(rt) {
        JS_ASSERT(rt->currentThreadHasExclusiveAccess());
        for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next())
            zone->allocator.arenas.copyFreeListsToArenas();
    }
    ~AutoCopyFreeListToArenasForGC() {
        for (ZonesIter zone(runtime, WithAtoms); !zone.done(); zone.next())
            zone->allocator.arenas.clearFreeListsInArenas();
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

        for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
            ArrayBufferObject::resetArrayBufferList(c);
            ResetGrayList(c);
        }

        for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
            JS_ASSERT(zone->isGCMarking());
            zone->setNeedsBarrier(false, Zone::UpdateIon);
            zone->setGCState(Zone::NoGC);
        }
        rt->setNeedsBarrier(false);
        AssertNeedsBarrierFlagsConsistent(rt);

        incrementalState = NO_INCREMENTAL;

        JS_ASSERT(!strictCompartmentChecking);

        break;
      }

      case SWEEP:
        marker.reset();

        for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next())
            zone->scheduledForDestruction = false;

        
        abortSweepAfterCurrentGroup = true;
        incrementalCollectSlice(SliceBudget::Unlimited, JS::gcreason::RESET, GC_NORMAL);

        {
            gcstats::AutoPhase ap(stats, gcstats::PHASE_WAIT_BACKGROUND_THREAD);
            helperThread.waitBackgroundSweepOrAllocEnd();
        }
        break;

      default:
        MOZ_ASSUME_UNREACHABLE("Invalid incremental GC state");
    }

    stats.reset(reason);

#ifdef DEBUG
    for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next())
        JS_ASSERT(c->gcLiveArrayBuffers.empty());

    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        JS_ASSERT(!zone->needsBarrier());
        for (unsigned i = 0; i < FINALIZE_LIMIT; ++i)
            JS_ASSERT(!zone->allocator.arenas.arenaListsToSweep[i]);
    }
#endif
}

namespace {

class AutoGCSlice {
  public:
    AutoGCSlice(JSRuntime *rt);
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
            JS_ASSERT(zone->needsBarrier());
            zone->setNeedsBarrier(false, Zone::DontUpdateIon);
        } else {
            JS_ASSERT(!zone->needsBarrier());
        }
    }
    rt->setNeedsBarrier(false);
    AssertNeedsBarrierFlagsConsistent(rt);
}

AutoGCSlice::~AutoGCSlice()
{
    
    bool haveBarriers = false;
    for (ZonesIter zone(runtime, WithAtoms); !zone.done(); zone.next()) {
        if (zone->isGCMarking()) {
            zone->setNeedsBarrier(true, Zone::UpdateIon);
            zone->allocator.arenas.prepareForIncrementalGC(runtime);
            haveBarriers = true;
        } else {
            zone->setNeedsBarrier(false, Zone::UpdateIon);
        }
    }
    runtime->setNeedsBarrier(haveBarriers);
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
GCRuntime::incrementalCollectSlice(int64_t budget,
                                   JS::gcreason::Reason reason,
                                   JSGCInvocationKind gckind)
{
    JS_ASSERT(rt->currentThreadHasExclusiveAccess());

    AutoCopyFreeListToArenasForGC copy(rt);
    AutoGCSlice slice(rt);

    bool lastGC = (reason == JS::gcreason::DESTROY_RUNTIME);

    gc::State initialState = incrementalState;

    int zeal = 0;
#ifdef JS_GC_ZEAL
    if (reason == JS::gcreason::DEBUG_GC && budget != SliceBudget::Unlimited) {
        




        zeal = zealMode;
    }
#endif

    JS_ASSERT_IF(incrementalState != NO_INCREMENTAL, isIncremental);
    isIncremental = budget != SliceBudget::Unlimited;

    if (zeal == ZealIncrementalRootsThenFinish || zeal == ZealIncrementalMarkAllThenFinish) {
        



        budget = SliceBudget::Unlimited;
    }

    SliceBudget sliceBudget(budget);

    if (incrementalState == NO_INCREMENTAL) {
        incrementalState = MARK_ROOTS;
        lastMarkSlice = false;
    }

    if (incrementalState == MARK)
        AutoGCRooter::traceAllWrappers(&marker);

    switch (incrementalState) {

      case MARK_ROOTS:
        if (!beginMarkPhase()) {
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
            sliceBudget.reset();
            isIncremental = false;
        }

        bool finished = drainMarkStack(sliceBudget, gcstats::PHASE_MARK);
        if (!finished)
            break;

        JS_ASSERT(marker.isDrained());

        if (!lastMarkSlice && isIncremental &&
            ((initialState == MARK && zeal != ZealIncrementalRootsThenFinish) ||
             zeal == ZealIncrementalMarkAllThenFinish))
        {
            




            lastMarkSlice = true;
            break;
        }

        incrementalState = SWEEP;

        



        beginSweepPhase(lastGC);
        if (sliceBudget.isOverBudget())
            break;

        



        if (isIncremental && zeal == ZealIncrementalMultipleSlices)
            break;

        
      }

      case SWEEP: {
        bool finished = sweepPhase(sliceBudget);
        if (!finished)
            break;

        endSweepPhase(gckind, lastGC);

        if (sweepOnBackgroundThread)
            helperThread.startBackgroundSweep(gckind == GC_SHRINK);

        incrementalState = NO_INCREMENTAL;
        break;
      }

      default:
        JS_ASSERT(false);
    }
}

IncrementalSafety
gc::IsIncrementalGCSafe(JSRuntime *rt)
{
    JS_ASSERT(!rt->mainThread.suppressGC);

    if (rt->keepAtoms())
        return IncrementalSafety::Unsafe("keepAtoms set");

    if (!rt->gc.incrementalEnabled)
        return IncrementalSafety::Unsafe("incremental permanently disabled");

    return IncrementalSafety::Safe();
}

void
GCRuntime::budgetIncrementalGC(int64_t *budget)
{
    IncrementalSafety safe = IsIncrementalGCSafe(rt);
    if (!safe) {
        resetIncrementalGC(safe.reason());
        *budget = SliceBudget::Unlimited;
        stats.nonincremental(safe.reason());
        return;
    }

    if (mode != JSGC_MODE_INCREMENTAL) {
        resetIncrementalGC("GC mode change");
        *budget = SliceBudget::Unlimited;
        stats.nonincremental("GC mode");
        return;
    }

    if (rt->isTooMuchMalloc()) {
        *budget = SliceBudget::Unlimited;
        stats.nonincremental("malloc bytes trigger");
    }

    bool reset = false;
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        if (zone->gcBytes >= zone->gcTriggerBytes) {
            *budget = SliceBudget::Unlimited;
            stats.nonincremental("allocation trigger");
        }

        if (incrementalState != NO_INCREMENTAL &&
            zone->isGCScheduled() != zone->wasGCStarted())
        {
            reset = true;
        }

        if (zone->isTooMuchMalloc()) {
            *budget = SliceBudget::Unlimited;
            stats.nonincremental("malloc bytes trigger");
        }
    }

    if (reset)
        resetIncrementalGC("zone change");
}










MOZ_NEVER_INLINE bool
GCRuntime::gcCycle(bool incremental, int64_t budget, JSGCInvocationKind gckind,
                   JS::gcreason::Reason reason)
{
    AutoGCSession gcsession(this);

    





    {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_WAIT_BACKGROUND_THREAD);
        helperThread.waitBackgroundSweepOrAllocEnd();
    }

    State prevState = incrementalState;

    if (!incremental) {
        
        resetIncrementalGC("requested");
        stats.nonincremental("requested");
        budget = SliceBudget::Unlimited;
    } else {
        budgetIncrementalGC(&budget);
    }

    
    if (prevState != NO_INCREMENTAL && incrementalState == NO_INCREMENTAL) {
        gcsession.cancel();
        return true;
    }

    incrementalCollectSlice(budget, reason, gckind);
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

namespace {

#ifdef JSGC_GENERATIONAL
class AutoDisableStoreBuffer
{
    StoreBuffer &sb;
    bool prior;

  public:
    AutoDisableStoreBuffer(GCRuntime *gc) : sb(gc->storeBuffer) {
        prior = sb.isEnabled();
        sb.disable();
    }
    ~AutoDisableStoreBuffer() {
        if (prior)
            sb.enable();
    }
};
#else
struct AutoDisableStoreBuffer
{
    AutoDisableStoreBuffer(GCRuntime *gc) {}
};
#endif

} 

void
GCRuntime::collect(bool incremental, int64_t budget, JSGCInvocationKind gckind,
                   JS::gcreason::Reason reason)
{
    
    JS_ASSERT(!InParallelSection());

    JS_AbortIfWrongThread(rt);

    
    JS_ASSERT(!rt->isHeapBusy());

    if (rt->mainThread.suppressGC)
        return;

    TraceLogger *logger = TraceLoggerForMainThread(rt);
    AutoTraceLog logGC(logger, TraceLogger::GC);

#ifdef JS_GC_ZEAL
    if (deterministicOnly && !IsDeterministicGCReason(reason))
        return;
#endif

    JS_ASSERT_IF(!incremental || budget != SliceBudget::Unlimited, JSGC_INCREMENTAL);

    AutoStopVerifyingBarriers av(rt, reason == JS::gcreason::SHUTDOWN_CC ||
                                     reason == JS::gcreason::DESTROY_RUNTIME);

    recordNativeStackTop();

    int zoneCount = 0;
    int compartmentCount = 0;
    int collectedCount = 0;
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        if (mode == JSGC_MODE_GLOBAL)
            zone->scheduleGC();

        
        if (incrementalState != NO_INCREMENTAL && zone->needsBarrier())
            zone->scheduleGC();

        zoneCount++;
        if (zone->isGCScheduled())
            collectedCount++;
    }

    for (CompartmentsIter c(rt, WithAtoms); !c.done(); c.next())
        compartmentCount++;

    shouldCleanUpEverything = ShouldCleanUpEverything(reason, gckind);

    bool repeat = false;
    do {
        minorGC(reason);

        



        AutoDisableStoreBuffer adsb(this);

        gcstats::AutoGCSlice agc(stats, collectedCount, zoneCount, compartmentCount, reason);

        



        if (incrementalState == NO_INCREMENTAL) {
            gcstats::AutoPhase ap(stats, gcstats::PHASE_GC_BEGIN);
            if (gcCallback)
                gcCallback(rt, JSGC_BEGIN, gcCallbackData);
        }

        poke = false;
        bool wasReset = gcCycle(incremental, budget, gckind, reason);

        if (incrementalState == NO_INCREMENTAL) {
            gcstats::AutoPhase ap(stats, gcstats::PHASE_GC_END);
            if (gcCallback)
                gcCallback(rt, JSGC_END, gcCallbackData);
        }

        
        if (poke && shouldCleanUpEverything)
            JS::PrepareForFullGC(rt);

        





        repeat = (poke && shouldCleanUpEverything) || wasReset;
    } while (repeat);

    if (incrementalState == NO_INCREMENTAL) {
#ifdef JS_THREADSAFE
        EnqueuePendingParseTasksAfterGC(rt);
#endif
    }
}

void
js::GC(JSRuntime *rt, JSGCInvocationKind gckind, JS::gcreason::Reason reason)
{
    rt->gc.collect(false, SliceBudget::Unlimited, gckind, reason);
}

void
js::GCSlice(JSRuntime *rt, JSGCInvocationKind gckind, JS::gcreason::Reason reason, int64_t millis)
{
    rt->gc.gcSlice(gckind, reason, millis);
}

void
GCRuntime::gcSlice(JSGCInvocationKind gckind, JS::gcreason::Reason reason, int64_t millis)
{
    int64_t budget;
    if (millis)
        budget = SliceBudget::TimeBudget(millis);
    else if (highFrequencyGC && dynamicMarkSlice)
        budget = sliceBudget * IGC_MARK_SLICE_MULTIPLIER;
    else
        budget = sliceBudget;

    collect(true, budget, gckind, reason);
}

void
js::GCFinalSlice(JSRuntime *rt, JSGCInvocationKind gckind, JS::gcreason::Reason reason)
{
    rt->gc.collect(true, SliceBudget::Unlimited, gckind, reason);
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
js::GCDebugSlice(JSRuntime *rt, bool limit, int64_t objCount)
{
    int64_t budget = limit ? SliceBudget::WorkBudget(objCount) : SliceBudget::Unlimited;
    if (!ZonesSelected(rt)) {
        if (JS::IsIncrementalGCInProgress(rt))
            JS::PrepareForIncrementalGC(rt);
        else
            JS::PrepareForFullGC(rt);
    }
    rt->gc.collect(true, budget, GC_NORMAL, JS::gcreason::DEBUG_GC);
}


void
js::PrepareForDebugGC(JSRuntime *rt)
{
    if (!ZonesSelected(rt))
        JS::PrepareForFullGC(rt);
}

JS_FRIEND_API(void)
JS::ShrinkGCBuffers(JSRuntime *rt)
{
    AutoLockGC lock(rt);
    JS_ASSERT(!rt->isHeapBusy());

    if (!rt->useHelperThreads())
        ExpireChunksAndArenas(rt, true);
    else
        rt->gc.startBackgroundShrink();
}

void
js::MinorGC(JSRuntime *rt, JS::gcreason::Reason reason)
{
    rt->gc.minorGC(reason);
}

void
GCRuntime::minorGC(JS::gcreason::Reason reason)
{
#ifdef JSGC_GENERATIONAL
    TraceLogger *logger = TraceLoggerForMainThread(rt);
    AutoTraceLog logMinorGC(logger, TraceLogger::MinorGC);
    nursery.collect(rt, reason, nullptr);
    JS_ASSERT_IF(!rt->mainThread.suppressGC, nursery.isEmpty());
#endif
}

void
js::MinorGC(JSContext *cx, JS::gcreason::Reason reason)
{
    
    
    cx->runtime()->gc.minorGC(cx, reason);
}

void
GCRuntime::minorGC(JSContext *cx, JS::gcreason::Reason reason)
{
#ifdef JSGC_GENERATIONAL
    TraceLogger *logger = TraceLoggerForMainThread(rt);
    AutoTraceLog logMinorGC(logger, TraceLogger::MinorGC);
    Nursery::TypeObjectList pretenureTypes;
    nursery.collect(rt, reason, &pretenureTypes);
    for (size_t i = 0; i < pretenureTypes.length(); i++) {
        if (pretenureTypes[i]->canPreTenure())
            pretenureTypes[i]->setShouldPreTenure(cx);
    }
    JS_ASSERT_IF(!rt->mainThread.suppressGC, nursery.isEmpty());
#endif
}

void
GCRuntime::disableGenerationalGC()
{
#ifdef JSGC_GENERATIONAL
    if (isGenerationalGCEnabled()) {
        minorGC(JS::gcreason::API);
        nursery.disable();
        storeBuffer.disable();
    }
#endif
    ++rt->gc.generationalDisabled;
}

void
GCRuntime::enableGenerationalGC()
{
    JS_ASSERT(generationalDisabled > 0);
    --generationalDisabled;
#ifdef JSGC_GENERATIONAL
    if (generationalDisabled == 0) {
        nursery.enable();
        storeBuffer.enable();
    }
#endif
}

void
js::gc::GCIfNeeded(JSContext *cx)
{
    cx->runtime()->gc.gcIfNeeded(cx);
}

void
GCRuntime::gcIfNeeded(JSContext *cx)
{
#ifdef JSGC_GENERATIONAL
    



    if (storeBuffer.isAboutToOverflow())
        minorGC(cx, JS::gcreason::FULL_STORE_BUFFER);
#endif

    if (isNeeded)
        gcSlice(GC_NORMAL, rt->gc.triggerReason, 0);
}

void
js::gc::FinishBackgroundFinalize(JSRuntime *rt)
{
    rt->gc.waitBackgroundSweepEnd();
}

AutoFinishGC::AutoFinishGC(JSRuntime *rt)
{
    if (JS::IsIncrementalGCInProgress(rt)) {
        JS::PrepareForIncrementalGC(rt);
        JS::FinishIncrementalGC(rt, JS::gcreason::API);
    }

    gc::FinishBackgroundFinalize(rt);
}

AutoPrepareForTracing::AutoPrepareForTracing(JSRuntime *rt, ZoneSelector selector)
  : finish(rt),
    session(rt),
    copy(rt, selector)
{
    rt->gc.recordNativeStackTop();
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

        if (!zone->init())
            return nullptr;

        zone->setGCLastBytes(8192, GC_NORMAL);

        const JSPrincipals *trusted = rt->trustedPrincipals();
        zone->isSystem = principals && principals == trusted;
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
    
    
    JS_ASSERT(source->options_.mergeable());

    JSRuntime *rt = source->runtimeFromMainThread();

    AutoPrepareForTracing prepare(rt, SkipAtoms);

    
    

    source->clearTables();

    

    for (ZoneCellIter iter(source->zone(), FINALIZE_SCRIPT); !iter.done(); iter.next()) {
        JSScript *script = iter.get<JSScript>();
        JS_ASSERT(script->compartment() == source);
        script->compartment_ = target;
    }

    for (ZoneCellIter iter(source->zone(), FINALIZE_BASE_SHAPE); !iter.done(); iter.next()) {
        BaseShape *base = iter.get<BaseShape>();
        JS_ASSERT(base->compartment() == source);
        base->compartment_ = target;
    }

    

    for (size_t thingKind = 0; thingKind != FINALIZE_LIMIT; thingKind++) {
        for (ArenaIter aiter(source->zone(), AllocKind(thingKind)); !aiter.done(); aiter.next()) {
            ArenaHeader *aheader = aiter.get();
            aheader->zone = target->zone();
        }
    }

    
    for (CompartmentsInZoneIter c(source->zone()); !c.done(); c.next())
        JS_ASSERT(c.get() == source);

    
    target->zone()->allocator.arenas.adoptArenas(rt, &source->zone()->allocator.arenas);
    target->zone()->gcBytes += source->zone()->gcBytes;
    source->zone()->gcBytes = 0;

    
    target->zone()->types.typeLifoAlloc.transferFrom(&source->zone()->types.typeLifoAlloc);
}

void
gc::RunDebugGC(JSContext *cx)
{
    cx->runtime()->gc.runDebugGC();
}

void
GCRuntime::runDebugGC()
{
#ifdef JS_GC_ZEAL
    int type = zealMode;

    if (rt->mainThread.suppressGC)
        return;

    if (type == js::gc::ZealGenerationalGCValue)
        return MinorGC(rt, JS::gcreason::DEBUG_GC);

    PrepareForDebugGC(rt);

    if (type == ZealIncrementalRootsThenFinish ||
        type == ZealIncrementalMarkAllThenFinish ||
        type == ZealIncrementalMultipleSlices)
    {
        js::gc::State initialState = incrementalState;
        int64_t budget;
        if (type == ZealIncrementalMultipleSlices) {
            




            if (initialState == NO_INCREMENTAL)
                incrementalLimit = zealFrequency / 2;
            else
                incrementalLimit *= 2;
            budget = SliceBudget::WorkBudget(incrementalLimit);
        } else {
            
            budget = SliceBudget::WorkBudget(1);
        }

        collect(true, budget, GC_NORMAL, JS::gcreason::DEBUG_GC);

        



        if (type == ZealIncrementalMultipleSlices &&
            initialState == MARK && incrementalState == SWEEP)
        {
            incrementalLimit = zealFrequency / 2;
        }
    } else {
        collect(false, SliceBudget::Unlimited, GC_NORMAL, JS::gcreason::DEBUG_GC);
    }

#endif
}

void
gc::SetDeterministicGC(JSContext *cx, bool enabled)
{
#ifdef JS_GC_ZEAL
    JSRuntime *rt = cx->runtime();
    rt->gc.deterministicOnly = enabled;
#endif
}

void
gc::SetValidateGC(JSContext *cx, bool enabled)
{
    JSRuntime *rt = cx->runtime();
    rt->gc.validate = enabled;
}

void
gc::SetFullCompartmentChecks(JSContext *cx, bool enabled)
{
    JSRuntime *rt = cx->runtime();
    rt->gc.fullCompartmentChecks = enabled;
}

#ifdef DEBUG


void PreventGCDuringInteractiveDebug()
{
    TlsPerThreadData.get()->suppressGC++;
}

#endif

void
js::ReleaseAllJITCode(FreeOp *fop)
{
#ifdef JS_ION

# ifdef JSGC_GENERATIONAL
    



    MinorGC(fop->runtime(), JS::gcreason::EVICT_NURSERY);
# endif

    for (ZonesIter zone(fop->runtime(), SkipAtoms); !zone.done(); zone.next()) {
        if (!zone->jitZone())
            continue;

# ifdef DEBUG
        
        for (ZoneCellIter i(zone, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            JS_ASSERT_IF(script->hasBaselineScript(), !script->baselineScript()->active());
        }
# endif

        
        jit::MarkActiveBaselineScripts(zone);

        jit::InvalidateAll(fop, zone);

        for (ZoneCellIter i(zone, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            jit::FinishInvalidation<SequentialExecution>(fop, script);
            jit::FinishInvalidation<ParallelExecution>(fop, script);

            



            jit::FinishDiscardBaselineScript(fop, script);
        }

        zone->jitZone()->optimizedStubSpace()->free();
    }
#endif
}

























static void
ReleaseScriptCounts(FreeOp *fop)
{
    JSRuntime *rt = fop->runtime();
    JS_ASSERT(rt->gc.scriptAndCountsVector);

    ScriptAndCountsVector &vec = *rt->gc.scriptAndCountsVector;

    for (size_t i = 0; i < vec.length(); i++)
        vec[i].scriptCounts.destroy(fop);

    fop->delete_(rt->gc.scriptAndCountsVector);
    rt->gc.scriptAndCountsVector = nullptr;
}

JS_FRIEND_API(void)
js::StartPCCountProfiling(JSContext *cx)
{
    JSRuntime *rt = cx->runtime();

    if (rt->profilingScripts)
        return;

    if (rt->gc.scriptAndCountsVector)
        ReleaseScriptCounts(rt->defaultFreeOp());

    ReleaseAllJITCode(rt->defaultFreeOp());

    rt->profilingScripts = true;
}

JS_FRIEND_API(void)
js::StopPCCountProfiling(JSContext *cx)
{
    JSRuntime *rt = cx->runtime();

    if (!rt->profilingScripts)
        return;
    JS_ASSERT(!rt->gc.scriptAndCountsVector);

    ReleaseAllJITCode(rt->defaultFreeOp());

    ScriptAndCountsVector *vec = cx->new_<ScriptAndCountsVector>(SystemAllocPolicy());
    if (!vec)
        return;

    for (ZonesIter zone(rt, SkipAtoms); !zone.done(); zone.next()) {
        for (ZoneCellIter i(zone, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            if (script->hasScriptCounts() && script->types) {
                ScriptAndCounts sac;
                sac.script = script;
                sac.scriptCounts.set(script->releaseScriptCounts());
                if (!vec->append(sac))
                    sac.scriptCounts.destroy(rt->defaultFreeOp());
            }
        }
    }

    rt->profilingScripts = false;
    rt->gc.scriptAndCountsVector = vec;
}

JS_FRIEND_API(void)
js::PurgePCCounts(JSContext *cx)
{
    JSRuntime *rt = cx->runtime();

    if (!rt->gc.scriptAndCountsVector)
        return;
    JS_ASSERT(!rt->profilingScripts);

    ReleaseScriptCounts(rt->defaultFreeOp());
}

void
js::PurgeJITCaches(Zone *zone)
{
#ifdef JS_ION
    for (ZoneCellIterUnderGC i(zone, FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();

        
        jit::PurgeCaches(script);
    }
#endif
}

void
ArenaLists::normalizeBackgroundFinalizeState(AllocKind thingKind)
{
    ArenaLists::BackgroundFinalizeState *bfs = &backgroundFinalizeState[thingKind];
    switch (*bfs) {
      case BFS_DONE:
        break;
      case BFS_JUST_FINISHED:
        
        
        *bfs = BFS_DONE;
        break;
      default:
        JS_ASSERT(!"Background finalization in progress, but it should not be.");
        break;
    }
}

void
ArenaLists::adoptArenas(JSRuntime *rt, ArenaLists *fromArenaLists)
{
    
    
    
    AutoLockGC lock(rt);

    fromArenaLists->purge();

    for (size_t thingKind = 0; thingKind != FINALIZE_LIMIT; thingKind++) {
#ifdef JS_THREADSAFE
        
        
        
        normalizeBackgroundFinalizeState(AllocKind(thingKind));
        fromArenaLists->normalizeBackgroundFinalizeState(AllocKind(thingKind));
#endif
        ArenaList *fromList = &fromArenaLists->arenaLists[thingKind];
        ArenaList *toList = &arenaLists[thingKind];
        fromList->deepCheck();
        toList->deepCheck();
        ArenaHeader *next;
        for (ArenaHeader *fromHeader = fromList->head(); fromHeader; fromHeader = next) {
            
            next = fromHeader->next;

            
            
            
            
            if (fromHeader->isEmpty())
                fromHeader->chunk()->releaseArena(fromHeader);
            else
                toList->insertAtCursor(fromHeader);
        }
        fromList->clear();
        toList->deepCheck();
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


AutoMaybeTouchDeadZones::AutoMaybeTouchDeadZones(JSContext *cx)
  : runtime(cx->runtime()),
    markCount(runtime->gc.objectsMarkedInDeadZones),
    inIncremental(JS::IsIncrementalGCInProgress(runtime)),
    manipulatingDeadZones(runtime->gc.manipulatingDeadZones)
{
    runtime->gc.manipulatingDeadZones = true;
}

AutoMaybeTouchDeadZones::AutoMaybeTouchDeadZones(JSObject *obj)
  : runtime(obj->compartment()->runtimeFromMainThread()),
    markCount(runtime->gc.objectsMarkedInDeadZones),
    inIncremental(JS::IsIncrementalGCInProgress(runtime)),
    manipulatingDeadZones(runtime->gc.manipulatingDeadZones)
{
    runtime->gc.manipulatingDeadZones = true;
}

AutoMaybeTouchDeadZones::~AutoMaybeTouchDeadZones()
{
    runtime->gc.manipulatingDeadZones = manipulatingDeadZones;

    if (inIncremental && runtime->gc.objectsMarkedInDeadZones != markCount) {
        JS::PrepareForFullGC(runtime);
        js::GC(runtime, GC_NORMAL, JS::gcreason::TRANSPLANT);
    }
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
js::UninlinedIsInsideNursery(JSRuntime *rt, const void *thing)
{
    return IsInsideNursery(rt, thing);
}

#ifdef DEBUG
AutoDisableProxyCheck::AutoDisableProxyCheck(JSRuntime *rt
                                             MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : count(rt->gc.disableStrictProxyCheckingCount)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    count++;
}

JS_FRIEND_API(void)
JS::AssertGCThingMustBeTenured(JSObject *obj)
{
    JS_ASSERT((!IsNurseryAllocable(obj->tenuredGetAllocKind()) || obj->getClass()->finalize) &&
              obj->isTenured());
}

JS_FRIEND_API(size_t)
JS::GetGCNumber()
{
    JSRuntime *rt = js::TlsPerThreadData.get()->runtimeFromMainThread();
    if (!rt)
        return 0;
    return rt->gc.number;
}

JS::AutoAssertNoGC::AutoAssertNoGC()
  : runtime(nullptr), gcNumber(0)
{
    js::PerThreadData *data = js::TlsPerThreadData.get();
    if (data) {
        





        runtime = data->runtimeIfOnOwnerThread();
        if (runtime)
            gcNumber = runtime->gc.number;
    }
}

JS::AutoAssertNoGC::AutoAssertNoGC(JSRuntime *rt)
  : runtime(rt), gcNumber(rt->gc.number)
{
}

JS::AutoAssertNoGC::~AutoAssertNoGC()
{
    if (runtime)
        MOZ_ASSERT(gcNumber == runtime->gc.number, "GC ran inside an AutoAssertNoGC scope.");
}
#endif
