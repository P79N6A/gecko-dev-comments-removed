








#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Util.h"






























#include <math.h>
#include <string.h>     

#include "jstypes.h"
#include "jsutil.h"
#include "jsclist.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscompartment.h"
#include "jscrashreport.h"
#include "jscrashformat.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsdbgapi.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsprobes.h"
#include "jsproxy.h"
#include "jsscript.h"
#include "jswatchpoint.h"
#include "jsweakmap.h"

#include "builtin/MapObject.h"
#include "frontend/Parser.h"
#include "gc/FindSCCs.h"
#include "gc/GCInternals.h"
#include "gc/Marking.h"
#include "gc/Memory.h"
#include "methodjit/MethodJIT.h"
#include "vm/Debugger.h"
#include "vm/ForkJoin.h"
#include "vm/Shape.h"
#include "vm/String.h"
#include "vm/ForkJoin.h"
#include "ion/IonCode.h"
#ifdef JS_ION
# include "ion/IonMacroAssembler.h"
#include "ion/IonFrameIterator.h"
#endif

#include "jsgcinlines.h"
#include "jsinterpinlines.h"
#include "jsobjinlines.h"

#include "gc/FindSCCs-inl.h"
#include "vm/ScopeObject-inl.h"
#include "vm/String-inl.h"

#ifdef XP_WIN
# include "jswin.h"
#else
# include <unistd.h>
#endif

#if JS_TRACE_LOGGING
#include "TraceLogging.h"
#endif

using namespace js;
using namespace js::gc;

using mozilla::ArrayEnd;
using mozilla::DebugOnly;
using mozilla::Maybe;


static const uint64_t GC_IDLE_FULL_SPAN = 20 * 1000 * 1000;


static const int IGC_MARK_SLICE_MULTIPLIER = 2;


AllocKind gc::slotsToThingKind[] = {
      FINALIZE_OBJECT0,  FINALIZE_OBJECT2,  FINALIZE_OBJECT2,  FINALIZE_OBJECT4,
      FINALIZE_OBJECT4,  FINALIZE_OBJECT8,  FINALIZE_OBJECT8,  FINALIZE_OBJECT8,
      FINALIZE_OBJECT8,  FINALIZE_OBJECT12, FINALIZE_OBJECT12, FINALIZE_OBJECT12,
     FINALIZE_OBJECT12, FINALIZE_OBJECT16, FINALIZE_OBJECT16, FINALIZE_OBJECT16,
     FINALIZE_OBJECT16
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(slotsToThingKind) == SLOTS_TO_THING_KIND_LIMIT);

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
    sizeof(Shape),              
    sizeof(BaseShape),          
    sizeof(types::TypeObject),  
    sizeof(JSShortString),      
    sizeof(JSString),           
    sizeof(JSExternalString),   
    sizeof(ion::IonCode),       
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
    OFFSET(Shape),              
    OFFSET(BaseShape),          
    OFFSET(types::TypeObject),  
    OFFSET(JSShortString),      
    OFFSET(JSString),           
    OFFSET(JSExternalString),   
    OFFSET(ion::IonCode),       
};

#undef OFFSET





static const AllocKind FinalizePhaseStrings[] = {
    FINALIZE_EXTERNAL_STRING
};

static const AllocKind FinalizePhaseScripts[] = {
    FINALIZE_SCRIPT
};

static const AllocKind FinalizePhaseIonCode[] = {
    FINALIZE_IONCODE
};

static const AllocKind* FinalizePhases[] = {
    FinalizePhaseStrings,
    FinalizePhaseScripts,
    FinalizePhaseIonCode
};
static const int FinalizePhaseCount = sizeof(FinalizePhases) / sizeof(AllocKind*);

static const int FinalizePhaseLength[] = {
    sizeof(FinalizePhaseStrings) / sizeof(AllocKind),
    sizeof(FinalizePhaseScripts) / sizeof(AllocKind),
    sizeof(FinalizePhaseIonCode) / sizeof(AllocKind)
};

static const gcstats::Phase FinalizePhaseStatsPhase[] = {
    gcstats::PHASE_SWEEP_STRING,
    gcstats::PHASE_SWEEP_SCRIPT,
    gcstats::PHASE_SWEEP_IONCODE
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
    FINALIZE_SHORT_STRING,
    FINALIZE_STRING
};

static const AllocKind BackgroundPhaseShapes[] = {
    FINALIZE_SHAPE,
    FINALIZE_BASE_SHAPE,
    FINALIZE_TYPE_OBJECT
};

static const AllocKind* BackgroundPhases[] = {
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

    




    if (IsBackgroundFinalized(getAllocKind()) && !zone->rt->isHeapBusy())
        return;

    FreeSpan firstSpan = FreeSpan::decodeOffsets(arenaAddress(), firstFreeSpanOffsets);
    if (firstSpan.isEmpty())
        return;
    const FreeSpan *list = zone->allocator.arenas.getFreeList(getAllocKind());
    if (list->isEmpty() || firstSpan.arenaAddress() != list->arenaAddress())
        return;

    



    JS_ASSERT(firstSpan.isSameNonEmptySpan(list));
}
#endif

 void
Arena::staticAsserts()
{
    JS_STATIC_ASSERT(sizeof(Arena) == ArenaSize);
    JS_STATIC_ASSERT(JS_ARRAY_LENGTH(ThingSizes) == FINALIZE_LIMIT);
    JS_STATIC_ASSERT(JS_ARRAY_LENGTH(FirstThingOffsets) == FINALIZE_LIMIT);
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

    uintptr_t thing = thingsStart(thingKind);
    uintptr_t lastByte = thingsEnd() - 1;

    FreeSpan nextFree(aheader.getFirstFreeSpan());
    nextFree.checkSpan();

    FreeSpan newListHead;
    FreeSpan *newListTail = &newListHead;
    uintptr_t newFreeSpanStart = 0;
    bool allClear = true;
    DebugOnly<size_t> nmarked = 0;
    for (;; thing += thingSize) {
        JS_ASSERT(thing <= lastByte + 1);
        if (thing == nextFree.first) {
            JS_ASSERT(nextFree.last <= lastByte);
            if (nextFree.last == lastByte)
                break;
            JS_ASSERT(Arena::isAligned(nextFree.last, thingSize));
            if (!newFreeSpanStart)
                newFreeSpanStart = thing;
            thing = nextFree.last;
            nextFree = *nextFree.nextSpan();
            nextFree.checkSpan();
        } else {
            T *t = reinterpret_cast<T *>(thing);
            if (t->isMarked()) {
                allClear = false;
                nmarked++;
                if (newFreeSpanStart) {
                    JS_ASSERT(thing >= thingsStart(thingKind) + thingSize);
                    newListTail->first = newFreeSpanStart;
                    newListTail->last = thing - thingSize;
                    newListTail = newListTail->nextSpanUnchecked(thingSize);
                    newFreeSpanStart = 0;
                }
            } else {
                if (!newFreeSpanStart)
                    newFreeSpanStart = thing;
                t->finalize(fop);
                JS_POISON(t, JS_FREE_PATTERN, thingSize);
            }
        }
    }

    if (allClear) {
        JS_ASSERT(newListTail == &newListHead);
        JS_ASSERT(newFreeSpanStart == thingsStart(thingKind));
        return true;
    }

    newListTail->first = newFreeSpanStart ? newFreeSpanStart : nextFree.first;
    JS_ASSERT(Arena::isAligned(newListTail->first, thingSize));
    newListTail->last = lastByte;

#ifdef DEBUG
    size_t nfree = 0;
    for (const FreeSpan *span = &newListHead; span != newListTail; span = span->nextSpan()) {
        span->checkSpan();
        JS_ASSERT(Arena::isAligned(span->first, thingSize));
        JS_ASSERT(Arena::isAligned(span->last, thingSize));
        nfree += (span->last - span->first) / thingSize + 1;
        JS_ASSERT(nfree + nmarked <= thingsPerArena(thingSize));
    }
    nfree += (newListTail->last + 1 - newListTail->first) / thingSize;
    JS_ASSERT(nfree + nmarked == thingsPerArena(thingSize));
#endif
    aheader.setFirstFreeSpan(&newListHead);

    return false;
}





void ArenaList::insert(ArenaHeader *a)
{
    JS_ASSERT(a);
    JS_ASSERT_IF(!head, cursor == &head);
    a->next = *cursor;
    *cursor = a;
    if (!a->hasFreeThings())
        cursor = &a->next;
}

template<typename T>
static inline bool
FinalizeTypedArenas(FreeOp *fop,
                    ArenaHeader **src,
                    ArenaList &dest,
                    AllocKind thingKind,
                    SliceBudget &budget)
{
    




    size_t thingSize = Arena::thingSize(thingKind);

    while (ArenaHeader *aheader = *src) {
        *src = aheader->next;
        bool allClear = aheader->getArena()->finalize<T>(fop, thingKind, thingSize);
        if (allClear)
            aheader->chunk()->releaseArena(aheader);
        else
            dest.insert(aheader);
        budget.step(Arena::thingsPerArena(thingSize));
        if (budget.isOverBudget())
            return false;
    }

    return true;
}





static bool
FinalizeArenas(FreeOp *fop,
               ArenaHeader **src,
               ArenaList &dest,
               AllocKind thingKind,
               SliceBudget &budget)
{
    switch(thingKind) {
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
      case FINALIZE_SHAPE:
        return FinalizeTypedArenas<Shape>(fop, src, dest, thingKind, budget);
      case FINALIZE_BASE_SHAPE:
        return FinalizeTypedArenas<BaseShape>(fop, src, dest, thingKind, budget);
      case FINALIZE_TYPE_OBJECT:
        return FinalizeTypedArenas<types::TypeObject>(fop, src, dest, thingKind, budget);
      case FINALIZE_STRING:
        return FinalizeTypedArenas<JSString>(fop, src, dest, thingKind, budget);
      case FINALIZE_SHORT_STRING:
        return FinalizeTypedArenas<JSShortString>(fop, src, dest, thingKind, budget);
      case FINALIZE_EXTERNAL_STRING:
        return FinalizeTypedArenas<JSExternalString>(fop, src, dest, thingKind, budget);
      case FINALIZE_IONCODE:
#ifdef JS_ION
        return FinalizeTypedArenas<ion::IonCode>(fop, src, dest, thingKind, budget);
#endif
      default:
        JS_NOT_REACHED("Invalid alloc kind");
        return true;
    }
}

static inline Chunk *
AllocChunk() {
    return static_cast<Chunk *>(MapAlignedPages(ChunkSize, ChunkSize));
}

static inline void
FreeChunk(Chunk *p) {
    UnmapPages(static_cast<void *>(p), ChunkSize);
}

inline bool
ChunkPool::wantBackgroundAllocation(JSRuntime *rt) const
{
    




    return rt->gcHelperThread.canBackgroundAllocate() &&
           emptyCount == 0 &&
           rt->gcChunkSet.count() >= 4;
}


inline Chunk *
ChunkPool::get(JSRuntime *rt)
{
    JS_ASSERT(this == &rt->gcChunkPool);

    Chunk *chunk = emptyChunkListHead;
    if (chunk) {
        JS_ASSERT(emptyCount);
        emptyChunkListHead = chunk->info.next;
        --emptyCount;
    } else {
        JS_ASSERT(!emptyCount);
        chunk = Chunk::allocate(rt);
        if (!chunk)
            return NULL;
        JS_ASSERT(chunk->info.numArenasFreeCommitted == ArenasPerChunk);
        rt->gcNumArenasFreeCommitted += ArenasPerChunk;
    }
    JS_ASSERT(chunk->unused());
    JS_ASSERT(!rt->gcChunkSet.has(chunk));

    if (wantBackgroundAllocation(rt))
        rt->gcHelperThread.startBackgroundAllocationIfIdle();

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
    JS_ASSERT(this == &rt->gcChunkPool);

    





    Chunk *freeList = NULL;
    for (Chunk **chunkp = &emptyChunkListHead; *chunkp; ) {
        JS_ASSERT(emptyCount);
        Chunk *chunk = *chunkp;
        JS_ASSERT(chunk->unused());
        JS_ASSERT(!rt->gcChunkSet.has(chunk));
        JS_ASSERT(chunk->info.age <= MAX_EMPTY_CHUNK_AGE);
        if (releaseAll || chunk->info.age == MAX_EMPTY_CHUNK_AGE) {
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
FreeChunkList(Chunk *chunkListHead)
{
    while (Chunk *chunk = chunkListHead) {
        JS_ASSERT(!chunk->info.numArenasFreeCommitted);
        chunkListHead = chunk->info.next;
        FreeChunk(chunk);
    }
}

void
ChunkPool::expireAndFree(JSRuntime *rt, bool releaseAll)
{
    FreeChunkList(expire(rt, releaseAll));
}

 Chunk *
Chunk::allocate(JSRuntime *rt)
{
    Chunk *chunk = static_cast<Chunk *>(AllocChunk());

#ifdef JSGC_ROOT_ANALYSIS
    
    
    
    
    
    
    
    
    
    while (IsPoisonedPtr(chunk))
        chunk = static_cast<Chunk *>(AllocChunk());
#endif

    if (!chunk)
        return NULL;
    chunk->init(rt);
    rt->gcStats.count(gcstats::STAT_NEW_CHUNK);
    return chunk;
}


 inline void
Chunk::release(JSRuntime *rt, Chunk *chunk)
{
    JS_ASSERT(chunk);
    chunk->prepareToBeFreed(rt);
    FreeChunk(chunk);
}

inline void
Chunk::prepareToBeFreed(JSRuntime *rt)
{
    JS_ASSERT(rt->gcNumArenasFreeCommitted >= info.numArenasFreeCommitted);
    rt->gcNumArenasFreeCommitted -= info.numArenasFreeCommitted;
    rt->gcStats.count(gcstats::STAT_DESTROY_CHUNK);

#ifdef DEBUG
    



    info.numArenasFreeCommitted = 0;
#endif
}

void
Chunk::init(JSRuntime *rt)
{
    JS_POISON(this, JS_FREE_PATTERN, ChunkSize);

    



    bitmap.clear();

    
    decommittedArenas.clear(false);

    
    info.freeArenasHead = &arenas[0].aheader;
    info.lastDecommittedArenaOffset = 0;
    info.numArenasFree = ArenasPerChunk;
    info.numArenasFreeCommitted = ArenasPerChunk;
    info.age = 0;
    info.runtime = rt;

    
    for (unsigned i = 0; i < ArenasPerChunk; i++) {
        arenas[i].aheader.setAsNotAllocated();
        arenas[i].aheader.next = (i + 1 < ArenasPerChunk)
                                 ? &arenas[i + 1].aheader
                                 : NULL;
    }

    
}

static inline Chunk **
GetAvailableChunkList(Zone *zone)
{
    JSRuntime *rt = zone->rt;
    return zone->isSystem
           ? &rt->gcSystemAvailableChunkListHead
           : &rt->gcUserAvailableChunkListHead;
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
    info.prevp = NULL;
    info.next = NULL;
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
    JS_NOT_REACHED("No decommitted arenas found.");
    return -1;
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
    MarkPagesInUse(arena, ArenaSize);
    arena->aheader.setAsNotAllocated();

    return &arena->aheader;
}

inline ArenaHeader *
Chunk::fetchNextFreeArena(JSRuntime *rt)
{
    JS_ASSERT(info.numArenasFreeCommitted > 0);
    JS_ASSERT(info.numArenasFreeCommitted <= info.numArenasFree);
    JS_ASSERT(info.numArenasFreeCommitted <= rt->gcNumArenasFreeCommitted);

    ArenaHeader *aheader = info.freeArenasHead;
    info.freeArenasHead = aheader->next;
    --info.numArenasFreeCommitted;
    --info.numArenasFree;
    --rt->gcNumArenasFreeCommitted;

    return aheader;
}

ArenaHeader *
Chunk::allocateArena(Zone *zone, AllocKind thingKind)
{
    JS_ASSERT(hasAvailableArenas());

    JSRuntime *rt = zone->rt;
    JS_ASSERT(rt->gcBytes <= rt->gcMaxBytes);
    if (rt->gcMaxBytes - rt->gcBytes < ArenaSize)
        return NULL;

    ArenaHeader *aheader = JS_LIKELY(info.numArenasFreeCommitted > 0)
                           ? fetchNextFreeArena(rt)
                           : fetchNextDecommittedArena();
    aheader->init(zone, thingKind);
    if (JS_UNLIKELY(!hasAvailableArenas()))
        removeFromAvailableList();

    Probes::resizeHeap(zone, rt->gcBytes, rt->gcBytes + ArenaSize);
    rt->gcBytes += ArenaSize;
    zone->gcBytes += ArenaSize;
    if (zone->gcBytes >= zone->gcTriggerBytes)
        TriggerZoneGC(zone, gcreason::ALLOC_TRIGGER);

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
    ++rt->gcNumArenasFreeCommitted;
}

void
Chunk::releaseArena(ArenaHeader *aheader)
{
    JS_ASSERT(aheader->allocated());
    JS_ASSERT(!aheader->hasDelayedMarking);
    Zone *zone = aheader->zone;
    JSRuntime *rt = zone->rt;
    AutoLockGC maybeLock;
    if (rt->gcHelperThread.sweeping())
        maybeLock.lock(rt);

    Probes::resizeHeap(zone, rt->gcBytes, rt->gcBytes - ArenaSize);
    JS_ASSERT(rt->gcBytes >= ArenaSize);
    JS_ASSERT(zone->gcBytes >= ArenaSize);
    if (rt->gcHelperThread.sweeping())
        zone->reduceGCTriggerBytes(zone->gcHeapGrowthFactor * ArenaSize);
    rt->gcBytes -= ArenaSize;
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
        rt->gcChunkSet.remove(this);
        removeFromAvailableList();
        rt->gcChunkPool.put(this);
    }
}


static Chunk *
PickChunk(Zone *zone)
{
    JSRuntime *rt = zone->rt;
    Chunk **listHeadp = GetAvailableChunkList(zone);
    Chunk *chunk = *listHeadp;
    if (chunk)
        return chunk;

    chunk = rt->gcChunkPool.get(rt);
    if (!chunk)
        return NULL;

    rt->gcChunkAllocationSinceLastGC = true;

    



    GCChunkSet::AddPtr p = rt->gcChunkSet.lookupForAdd(chunk);
    JS_ASSERT(!p);
    if (!rt->gcChunkSet.add(p, chunk)) {
        Chunk::release(rt, chunk);
        return NULL;
    }

    chunk->info.prevp = NULL;
    chunk->info.next = NULL;
    chunk->addToAvailableList(zone);

    return chunk;
}

#ifdef JS_GC_ZEAL

extern void
js::SetGCZeal(JSRuntime *rt, uint8_t zeal, uint32_t frequency)
{
    if (zeal == 0) {
        if (rt->gcVerifyPreData)
            VerifyBarriers(rt, PreBarrierVerifier);
        if (rt->gcVerifyPostData)
            VerifyBarriers(rt, PostBarrierVerifier);
    }

#ifdef JS_METHODJIT
    
    for (CompartmentsIter c(rt); !c.done(); c.next())
        mjit::ClearAllFrames(c);
#endif

    bool schedule = zeal >= js::gc::ZealAllocValue;
    rt->gcZeal_ = zeal;
    rt->gcZealFrequency = frequency;
    rt->gcNextScheduled = schedule ? frequency : 0;
}

static bool
InitGCZeal(JSRuntime *rt)
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
                "  7: Verify stack rooting (yes, it's the same as 6)\n"
                "  8: Incremental GC in two slices: 1) mark roots 2) finish collection\n"
                "  9: Incremental GC in two slices: 1) mark all 2) new marking and finish\n"
                " 10: Incremental GC in multiple slices\n"
                " 11: Verify post write barriers between instructions\n"
                " 12: Verify post write barriers between paints\n"
                " 13: Purge analysis state every F allocations (default: 100)\n");
        return false;
    }

    SetGCZeal(rt, zeal, frequency);
    return true;
}

#endif


static const int64_t JIT_SCRIPT_RELEASE_TYPES_INTERVAL = 60 * 1000 * 1000;

JSBool
js_InitGC(JSRuntime *rt, uint32_t maxbytes)
{
    if (!rt->gcChunkSet.init(INITIAL_CHUNK_CAPACITY))
        return false;

    if (!rt->gcRootsHash.init(256))
        return false;

    if (!rt->gcLocksHash.init(256))
        return false;

#ifdef JS_THREADSAFE
    rt->gcLock = PR_NewLock();
    if (!rt->gcLock)
        return false;
#endif
    if (!rt->gcHelperThread.init())
        return false;

    



    rt->gcMaxBytes = maxbytes;
    rt->setGCMaxMallocBytes(maxbytes);

#ifndef JS_MORE_DETERMINISTIC
    rt->gcJitReleaseTime = PRMJ_Now() + JIT_SCRIPT_RELEASE_TYPES_INTERVAL;
#endif

#ifdef JSGC_GENERATIONAL
    if (!rt->gcNursery.enable())
        return false;

    if (!rt->gcStoreBuffer.enable())
        return false;
#endif

#ifdef JS_GC_ZEAL
    if (!InitGCZeal(rt))
        return false;
#endif

    return true;
}

static void
RecordNativeStackTopForGC(JSRuntime *rt)
{
    ConservativeGCData *cgcd = &rt->conservativeGC;

#ifdef JS_THREADSAFE
    
    if (!rt->requestDepth)
        return;
#endif
    cgcd->recordStackTop();
}

void
js_FinishGC(JSRuntime *rt)
{
    



    rt->gcHelperThread.finish();

#ifdef JS_GC_ZEAL
    
    FinishVerifier(rt);
#endif

    
    for (CompartmentsIter c(rt); !c.done(); c.next())
        js_delete(c.get());
    rt->compartments.clear();
    rt->atomsCompartment = NULL;

    rt->gcSystemAvailableChunkListHead = NULL;
    rt->gcUserAvailableChunkListHead = NULL;
    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront())
        Chunk::release(rt, r.front());
    rt->gcChunkSet.clear();

    rt->gcChunkPool.expireAndFree(rt, true);

    rt->gcRootsHash.clear();
    rt->gcLocksHash.clear();
}

template <typename T> struct BarrierOwner {};
template <typename T> struct BarrierOwner<T *> { typedef T result; };
template <> struct BarrierOwner<Value> { typedef HeapValue result; };

template <typename T>
static bool
AddRoot(JSRuntime *rt, T *rp, const char *name, JSGCRootType rootType)
{
    





    if (rt->gcIncrementalState != NO_INCREMENTAL)
        BarrierOwner<T>::result::writeBarrierPre(*rp);

    return rt->gcRootsHash.put((void *)rp, RootInfo(name, rootType));
}

template <typename T>
static bool
AddRoot(JSContext *cx, T *rp, const char *name, JSGCRootType rootType)
{
    bool ok = AddRoot(cx->runtime, rp, name, rootType);
    if (!ok)
        JS_ReportOutOfMemory(cx);
    return ok;
}

JSBool
js::AddValueRoot(JSContext *cx, Value *vp, const char *name)
{
    return AddRoot(cx, vp, name, JS_GC_ROOT_VALUE_PTR);
}

extern JSBool
js::AddValueRootRT(JSRuntime *rt, js::Value *vp, const char *name)
{
    return AddRoot(rt, vp, name, JS_GC_ROOT_VALUE_PTR);
}

extern JSBool
js::AddStringRoot(JSContext *cx, JSString **rp, const char *name)
{
    return AddRoot(cx, rp, name, JS_GC_ROOT_STRING_PTR);
}

extern JSBool
js::AddObjectRoot(JSContext *cx, JSObject **rp, const char *name)
{
    return AddRoot(cx, rp, name, JS_GC_ROOT_OBJECT_PTR);
}

extern JSBool
js::AddScriptRoot(JSContext *cx, JSScript **rp, const char *name)
{
    return AddRoot(cx, rp, name, JS_GC_ROOT_SCRIPT_PTR);
}

JS_FRIEND_API(void)
js_RemoveRoot(JSRuntime *rt, void *rp)
{
    rt->gcRootsHash.remove(rp);
    rt->gcPoke = true;
}

typedef RootedValueMap::Range RootRange;
typedef RootedValueMap::Entry RootEntry;
typedef RootedValueMap::Enum RootEnum;

static size_t
ComputeTriggerBytes(Zone *zone, size_t lastBytes, size_t maxBytes, JSGCInvocationKind gckind)
{
    size_t base = gckind == GC_SHRINK ? lastBytes : Max(lastBytes, zone->rt->gcAllocationThreshold);
    float trigger = float(base) * zone->gcHeapGrowthFactor;
    return size_t(Min(float(maxBytes), trigger));
}

void
JSCompartment::setGCLastBytes(size_t lastBytes, JSGCInvocationKind gckind)
{
    








    if (!rt->gcDynamicHeapGrowth) {
        gcHeapGrowthFactor = 3.0;
    } else if (lastBytes < 1 * 1024 * 1024) {
        gcHeapGrowthFactor = rt->gcLowFrequencyHeapGrowth;
    } else {
        JS_ASSERT(rt->gcHighFrequencyHighLimitBytes > rt->gcHighFrequencyLowLimitBytes);
        uint64_t now = PRMJ_Now();
        if (rt->gcLastGCTime && rt->gcLastGCTime + rt->gcHighFrequencyTimeThreshold * PRMJ_USEC_PER_MSEC > now) {
            if (lastBytes <= rt->gcHighFrequencyLowLimitBytes) {
                gcHeapGrowthFactor = rt->gcHighFrequencyHeapGrowthMax;
            } else if (lastBytes >= rt->gcHighFrequencyHighLimitBytes) {
                gcHeapGrowthFactor = rt->gcHighFrequencyHeapGrowthMin;
            } else {
                double k = (rt->gcHighFrequencyHeapGrowthMin - rt->gcHighFrequencyHeapGrowthMax)
                           / (double)(rt->gcHighFrequencyHighLimitBytes - rt->gcHighFrequencyLowLimitBytes);
                gcHeapGrowthFactor = (k * (lastBytes - rt->gcHighFrequencyLowLimitBytes)
                                     + rt->gcHighFrequencyHeapGrowthMax);
                JS_ASSERT(gcHeapGrowthFactor <= rt->gcHighFrequencyHeapGrowthMax
                          && gcHeapGrowthFactor >= rt->gcHighFrequencyHeapGrowthMin);
            }
            rt->gcHighFrequencyGC = true;
        } else {
            gcHeapGrowthFactor = rt->gcLowFrequencyHeapGrowth;
            rt->gcHighFrequencyGC = false;
        }
    }
    gcTriggerBytes = ComputeTriggerBytes(this, lastBytes, rt->gcMaxBytes, gckind);
}

void
JSCompartment::reduceGCTriggerBytes(size_t amount)
{
    JS_ASSERT(amount > 0);
    JS_ASSERT(gcTriggerBytes >= amount);
    if (gcTriggerBytes - amount < rt->gcAllocationThreshold * gcHeapGrowthFactor)
        return;
    gcTriggerBytes -= amount;
}

Allocator::Allocator(Zone *zone)
  : zone(zone)
{}

inline void
ArenaLists::prepareForIncrementalGC(JSRuntime *rt)
{
    for (size_t i = 0; i != FINALIZE_LIMIT; ++i) {
        FreeSpan *headSpan = &freeLists[i];
        if (!headSpan->isEmpty()) {
            ArenaHeader *aheader = headSpan->arenaHeader();
            aheader->allocatedDuringIncremental = true;
            rt->gcMarker.delayMarkingArena(aheader);
        }
    }
}

static inline void
PushArenaAllocatedDuringSweep(JSRuntime *runtime, ArenaHeader *arena)
{
    arena->setNextAllocDuringSweep(runtime->gcArenasAllocatedDuringSweep);
    runtime->gcArenasAllocatedDuringSweep = arena;
}

void *
ArenaLists::parallelAllocate(Zone *zone, AllocKind thingKind, size_t thingSize)
{
    






    void *t = allocateFromFreeList(thingKind, thingSize);
    if (t)
        return t;

    return allocateFromArena(zone, thingKind);
}

inline void *
ArenaLists::allocateFromArena(Zone *zone, AllocKind thingKind)
{
    









    Chunk *chunk = NULL;

    ArenaList *al = &arenaLists[thingKind];
    AutoLockGC maybeLock;

#ifdef JS_THREADSAFE
    volatile uintptr_t *bfs = &backgroundFinalizeState[thingKind];
    if (*bfs != BFS_DONE) {
        




        maybeLock.lock(zone->rt);
        if (*bfs == BFS_RUN) {
            JS_ASSERT(!*al->cursor);
            chunk = PickChunk(zone);
            if (!chunk) {
                



                return NULL;
            }
        } else if (*bfs == BFS_JUST_FINISHED) {
            
            *bfs = BFS_DONE;
        } else {
            JS_ASSERT(*bfs == BFS_DONE);
        }
    }
#endif 

    if (!chunk) {
        if (ArenaHeader *aheader = *al->cursor) {
            JS_ASSERT(aheader->hasFreeThings());

            



            JS_ASSERT(!aheader->isEmpty());
            al->cursor = &aheader->next;

            



            freeLists[thingKind] = aheader->getFirstFreeSpan();
            aheader->setAsFullyUsed();
            if (JS_UNLIKELY(zone->wasGCStarted())) {
                if (zone->needsBarrier()) {
                    aheader->allocatedDuringIncremental = true;
                    zone->rt->gcMarker.delayMarkingArena(aheader);
                } else if (zone->isGCSweeping()) {
                    PushArenaAllocatedDuringSweep(zone->rt, aheader);
                }
            }
            return freeLists[thingKind].infallibleAllocate(Arena::thingSize(thingKind));
        }

        
        if (!maybeLock.locked())
            maybeLock.lock(zone->rt);
        chunk = PickChunk(zone);
        if (!chunk)
            return NULL;
    }

    








    JS_ASSERT(!*al->cursor);
    ArenaHeader *aheader = chunk->allocateArena(zone, thingKind);
    if (!aheader)
        return NULL;

    if (JS_UNLIKELY(zone->wasGCStarted())) {
        if (zone->needsBarrier()) {
            aheader->allocatedDuringIncremental = true;
            zone->rt->gcMarker.delayMarkingArena(aheader);
        } else if (zone->isGCSweeping()) {
            PushArenaAllocatedDuringSweep(zone->rt, aheader);
        }
    }
    aheader->next = al->head;
    if (!al->head) {
        JS_ASSERT(al->cursor == &al->head);
        al->cursor = &aheader->next;
    }
    al->head = aheader;

    
    JS_ASSERT(!aheader->hasFreeThings());
    uintptr_t arenaAddr = aheader->arenaAddress();
    return freeLists[thingKind].allocateFromNewArena(arenaAddr,
                                                     Arena::firstThingOffset(thingKind),
                                                     Arena::thingSize(thingKind));
}

void
ArenaLists::finalizeNow(FreeOp *fop, AllocKind thingKind)
{
    JS_ASSERT(!IsBackgroundFinalized(thingKind));
    JS_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE ||
              backgroundFinalizeState[thingKind] == BFS_JUST_FINISHED);

    ArenaHeader *arenas = arenaLists[thingKind].head;
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

    arenaListsToSweep[thingKind] = arenaLists[thingKind].head;
    arenaLists[thingKind].clear();
}

inline void
ArenaLists::queueForBackgroundSweep(FreeOp *fop, AllocKind thingKind)
{
    JS_ASSERT(IsBackgroundFinalized(thingKind));

#ifdef JS_THREADSAFE
    JS_ASSERT(!fop->runtime()->gcHelperThread.sweeping());
#endif

    ArenaList *al = &arenaLists[thingKind];
    if (!al->head) {
        JS_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE);
        JS_ASSERT(al->cursor == &al->head);
        return;
    }

    



    JS_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE ||
              backgroundFinalizeState[thingKind] == BFS_JUST_FINISHED);

    arenaListsToSweep[thingKind] = al->head;
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
    JS_ASSERT(!*al->cursor);

    if (finalized.head) {
        *al->cursor = finalized.head;
        if (finalized.cursor != &finalized.head)
            al->cursor = finalized.cursor;
    }

    








    if (onBackgroundThread && finalized.head)
        lists->backgroundFinalizeState[thingKind] = BFS_JUST_FINISHED;
    else
        lists->backgroundFinalizeState[thingKind] = BFS_DONE;

    lists->arenaListsToSweep[thingKind] = NULL;
}

void
ArenaLists::queueObjectsForSweep(FreeOp *fop)
{
    gcstats::AutoPhase ap(fop->runtime()->gcStats, gcstats::PHASE_SWEEP_OBJECT);

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
    gcstats::AutoPhase ap(fop->runtime()->gcStats, gcstats::PHASE_SWEEP_STRING);

    queueForBackgroundSweep(fop, FINALIZE_SHORT_STRING);
    queueForBackgroundSweep(fop, FINALIZE_STRING);

    queueForForegroundSweep(fop, FINALIZE_EXTERNAL_STRING);
}

void
ArenaLists::queueScriptsForSweep(FreeOp *fop)
{
    gcstats::AutoPhase ap(fop->runtime()->gcStats, gcstats::PHASE_SWEEP_SCRIPT);
    queueForForegroundSweep(fop, FINALIZE_SCRIPT);
}

void
ArenaLists::queueIonCodeForSweep(FreeOp *fop)
{
    gcstats::AutoPhase ap(fop->runtime()->gcStats, gcstats::PHASE_SWEEP_IONCODE);
    queueForForegroundSweep(fop, FINALIZE_IONCODE);
}

void
ArenaLists::queueShapesForSweep(FreeOp *fop)
{
    gcstats::AutoPhase ap(fop->runtime()->gcStats, gcstats::PHASE_SWEEP_SHAPE);

    queueForBackgroundSweep(fop, FINALIZE_SHAPE);
    queueForBackgroundSweep(fop, FINALIZE_BASE_SHAPE);
    queueForBackgroundSweep(fop, FINALIZE_TYPE_OBJECT);
}

static void *
RunLastDitchGC(JSContext *cx, JS::Zone *zone, AllocKind thingKind)
{
    



    JS_ASSERT(!InParallelSection());

    PrepareZoneForGC(zone);

    JSRuntime *rt = cx->runtime;

    
    AutoKeepAtoms keep(rt);
    GC(rt, GC_NORMAL, gcreason::LAST_DITCH);

    




    size_t thingSize = Arena::thingSize(thingKind);
    if (void *thing = zone->allocator.arenas.allocateFromFreeList(thingKind, thingSize))
        return thing;

    return NULL;
}

template <AllowGC allowGC>
 void *
ArenaLists::refillFreeList(JSContext *cx, AllocKind thingKind)
{
    JS_ASSERT(cx->zone()->allocator.arenas.freeLists[thingKind].isEmpty());

    Zone *zone = cx->zone();
    JSRuntime *rt = zone->rt;
    JS_ASSERT(!rt->isHeapBusy());

    bool runGC = rt->gcIncrementalState != NO_INCREMENTAL &&
                 zone->gcBytes > zone->gcTriggerBytes &&
                 allowGC;
    for (;;) {
        if (JS_UNLIKELY(runGC)) {
            if (void *thing = RunLastDitchGC(cx, zone, thingKind))
                return thing;
        }

        







        for (bool secondAttempt = false; ; secondAttempt = true) {
            void *thing = zone->allocator.arenas.allocateFromArena(zone, thingKind);
            if (JS_LIKELY(!!thing))
                return thing;
            if (secondAttempt)
                break;

            rt->gcHelperThread.waitBackgroundSweepEnd();
        }

        if (!allowGC)
            return NULL;

        



        if (runGC)
            break;
        runGC = true;
    }

    JS_ASSERT(allowGC);
    js_ReportOutOfMemory(cx);
    return NULL;
}

template void *
ArenaLists::refillFreeList<NoGC>(JSContext *cx, AllocKind thingKind);

template void *
ArenaLists::refillFreeList<CanGC>(JSContext *cx, AllocKind thingKind);

JSGCTraceKind
js_GetGCThingTraceKind(void *thing)
{
    return GetGCThingTraceKind(thing);
}

JSBool
js_LockThing(JSRuntime *rt, void *thing)
{
    if (!thing)
        return true;

    





    if (rt->gcIncrementalState != NO_INCREMENTAL)
        IncrementalReferenceBarrier(thing, GetGCThingTraceKind(thing));

    if (GCLocks::Ptr p = rt->gcLocksHash.lookupWithDefault(thing, 0)) {
        p->value++;
        return true;
    }

    return false;
}

void
js_UnlockThing(JSRuntime *rt, void *thing)
{
    if (!thing)
        return;

    if (GCLocks::Ptr p = rt->gcLocksHash.lookup(thing)) {
        rt->gcPoke = true;
        if (--p->value == 0)
            rt->gcLocksHash.remove(p);
    }
}

void
js::InitTracer(JSTracer *trc, JSRuntime *rt, JSTraceCallback callback)
{
    trc->runtime = rt;
    trc->callback = callback;
    trc->debugPrinter = NULL;
    trc->debugPrintArg = NULL;
    trc->debugPrintIndex = size_t(-1);
    trc->eagerlyTraceWeakMaps = true;
#ifdef JS_GC_ZEAL
    trc->realLocation = NULL;
#endif
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
  : deadline(INT64_MAX),
    counter(INTPTR_MAX)
{
}

SliceBudget::SliceBudget(int64_t budget)
{
    if (budget == Unlimited) {
        deadline = INT64_MAX;
        counter = INTPTR_MAX;
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

GCMarker::GCMarker(JSRuntime *rt)
  : stack(size_t(-1)),
    color(BLACK),
    started(false),
    unmarkedArenaStackTop(NULL),
    markLaterArenas(0),
    grayFailed(false)
{
    InitTracer(this, rt, NULL);
}

bool
GCMarker::init()
{
    return stack.init(MARK_STACK_LENGTH);
}

void
GCMarker::start()
{
    JS_ASSERT(!started);
    started = true;
    color = BLACK;

    JS_ASSERT(!unmarkedArenaStackTop);
    JS_ASSERT(markLaterArenas == 0);

    



    eagerlyTraceWeakMaps = JS_FALSE;
}

void
GCMarker::stop()
{
    JS_ASSERT(isDrained());

    JS_ASSERT(started);
    started = false;

    JS_ASSERT(!unmarkedArenaStackTop);
    JS_ASSERT(markLaterArenas == 0);

    
    stack.reset();

    resetBufferedGrayRoots();
}

void
GCMarker::reset()
{
    color = BLACK;

    stack.reset();
    JS_ASSERT(isMarkStackEmpty());

    while (unmarkedArenaStackTop) {
        ArenaHeader *aheader = unmarkedArenaStackTop;
        JS_ASSERT(aheader->hasDelayedMarking);
        JS_ASSERT(markLaterArenas);
        unmarkedArenaStackTop = aheader->getNextDelayedMarking();
        aheader->unsetDelayedMarking();
        aheader->markOverflow = 0;
        aheader->allocatedDuringIncremental = 0;
        markLaterArenas--;
    }
    JS_ASSERT(isDrained());
    JS_ASSERT(!markLaterArenas);
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
    const Cell *cell = reinterpret_cast<const Cell *>(thing);
    cell->arenaHeader()->markOverflow = 1;
    delayMarkingArena(cell->arenaHeader());
}

void
GCMarker::markDelayedChildren(ArenaHeader *aheader)
{
    if (aheader->markOverflow) {
        bool always = aheader->allocatedDuringIncremental;
        aheader->markOverflow = 0;

        for (CellIterUnderGC i(aheader); !i.done(); i.next()) {
            Cell *t = i.getCell();
            if (always || t->isMarked()) {
                t->markIfUnmarked();
                JS_TraceChildren(this, t, MapAllocToTraceKind(aheader->getAllocKind()));
            }
        }
    } else {
        JS_ASSERT(aheader->allocatedDuringIncremental);
        PushArena(this, aheader);
    }
    aheader->allocatedDuringIncremental = 0;
    




}

bool
GCMarker::markDelayedChildren(SliceBudget &budget)
{
    gcstats::Phase phase = runtime->gcIncrementalState == MARK
                         ? gcstats::PHASE_MARK_DELAYED
                         : gcstats::PHASE_SWEEP_MARK_DELAYED;
    gcstats::AutoPhase ap(runtime->gcStats, phase);

    JS_ASSERT(unmarkedArenaStackTop);
    do {
        




        ArenaHeader *aheader = unmarkedArenaStackTop;
        JS_ASSERT(aheader->hasDelayedMarking);
        JS_ASSERT(markLaterArenas);
        unmarkedArenaStackTop = aheader->getNextDelayedMarking();
        aheader->unsetDelayedMarking();
        markLaterArenas--;
        markDelayedChildren(aheader);

        budget.step(150);
        if (budget.isOverBudget())
            return false;
    } while (unmarkedArenaStackTop);
    JS_ASSERT(!markLaterArenas);

    return true;
}

#ifdef DEBUG
void
GCMarker::checkZone(void *p)
{
    JS_ASSERT(started);
    JS_ASSERT(static_cast<Cell *>(p)->zone()->isCollecting());
}
#endif

bool
GCMarker::hasBufferedGrayRoots() const
{
    return !grayFailed;
}

void
GCMarker::startBufferingGrayRoots()
{
    JS_ASSERT(!grayFailed);
    for (GCZonesIter zone(runtime); !zone.done(); zone.next())
        JS_ASSERT(zone->gcGrayRoots.empty());

    JS_ASSERT(!callback);
    callback = GrayCallback;
    JS_ASSERT(IS_GC_MARKING_TRACER(this));
}

void
GCMarker::endBufferingGrayRoots()
{
    JS_ASSERT(callback == GrayCallback);
    callback = NULL;
    JS_ASSERT(IS_GC_MARKING_TRACER(this));
}

void
GCMarker::resetBufferedGrayRoots()
{
    for (GCZonesIter zone(runtime); !zone.done(); zone.next())
        zone->gcGrayRoots.clearAndFree();
    grayFailed = false;
}

void
GCMarker::markBufferedGrayRoots(JS::Zone *zone)
{
    JS_ASSERT(!grayFailed);
    JS_ASSERT(zone->isGCMarkingGray());

    for (GrayRoot *elem = zone->gcGrayRoots.begin(); elem != zone->gcGrayRoots.end(); elem++) {
#ifdef DEBUG
        debugPrinter = elem->debugPrinter;
        debugPrintArg = elem->debugPrintArg;
        debugPrintIndex = elem->debugPrintIndex;
#endif
        void *tmp = elem->thing;
        JS_SET_TRACING_LOCATION(this, (void *)&elem->thing);
        MarkKind(this, &tmp, elem->kind);
        JS_ASSERT(tmp == elem->thing);
    }
}

void
GCMarker::appendGrayRoot(void *thing, JSGCTraceKind kind)
{
    JS_ASSERT(started);

    if (grayFailed)
        return;

    GrayRoot root(thing, kind);
#ifdef DEBUG
    root.debugPrinter = debugPrinter;
    root.debugPrintArg = debugPrintArg;
    root.debugPrintIndex = debugPrintIndex;
#endif

    Zone *zone = static_cast<Cell *>(thing)->zone();
    if (zone->isCollecting()) {
        zone->maybeAlive = true;
        if (!zone->gcGrayRoots.append(root)) {
            grayFailed = true;
            resetBufferedGrayRoots();
        }
    }
}

void
GCMarker::GrayCallback(JSTracer *trc, void **thingp, JSGCTraceKind kind)
{
    GCMarker *gcmarker = static_cast<GCMarker *>(trc);
    gcmarker->appendGrayRoot(*thingp, kind);
}

size_t
GCMarker::sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf) const
{
    size_t size = stack.sizeOfExcludingThis(mallocSizeOf);
    for (ZonesIter zone(runtime); !zone.done(); zone.next())
        size += zone->gcGrayRoots.sizeOfExcludingThis(mallocSizeOf);
    return size;
}

void
js::SetMarkStackLimit(JSRuntime *rt, size_t limit)
{
    JS_ASSERT(!rt->isHeapBusy());
    rt->gcMarker.setSizeLimit(limit);
}

void
js::MarkCompartmentActive(StackFrame *fp)
{
    fp->script()->compartment()->active = true;
}

static void
TriggerOperationCallback(JSRuntime *rt, gcreason::Reason reason)
{
    if (rt->gcIsNeeded)
        return;

    rt->gcIsNeeded = true;
    rt->gcTriggerReason = reason;
    rt->triggerOperationCallback();
}

void
js::TriggerGC(JSRuntime *rt, gcreason::Reason reason)
{
    
    if (ForkJoinSlice *slice = ForkJoinSlice::Current()) {
        slice->requestGC(reason);
        return;
    }

    rt->assertValidThread();

    if (rt->isHeapBusy())
        return;

    PrepareForFullGC(rt);
    TriggerOperationCallback(rt, reason);
}

void
js::TriggerZoneGC(Zone *zone, gcreason::Reason reason)
{
    
    if (ForkJoinSlice *slice = ForkJoinSlice::Current()) {
        slice->requestZoneGC(zone, reason);
        return;
    }

    JSRuntime *rt = zone->rt;
    rt->assertValidThread();

    if (rt->isHeapBusy())
        return;

    if (rt->gcZeal() == ZealAllocValue) {
        TriggerGC(rt, reason);
        return;
    }

    if (zone == rt->atomsCompartment->zone()) {
        
        TriggerGC(rt, reason);
        return;
    }

    PrepareZoneForGC(zone);
    TriggerOperationCallback(rt, reason);
}

void
js::MaybeGC(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    rt->assertValidThread();

    if (rt->gcZeal() == ZealAllocValue || rt->gcZeal() == ZealPokeValue) {
        PrepareForFullGC(rt);
        GC(rt, GC_NORMAL, gcreason::MAYBEGC);
        return;
    }

    if (rt->gcIsNeeded) {
        GCSlice(rt, GC_NORMAL, gcreason::MAYBEGC);
        return;
    }

    double factor = rt->gcHighFrequencyGC ? 0.85 : 0.9;
    Zone *zone = cx->zone();
    if (zone->gcBytes > 1024 * 1024 &&
        zone->gcBytes >= factor * zone->gcTriggerBytes &&
        rt->gcIncrementalState == NO_INCREMENTAL &&
        !rt->gcHelperThread.sweeping())
    {
        PrepareZoneForGC(zone);
        GCSlice(rt, GC_NORMAL, gcreason::MAYBEGC);
        return;
    }

#ifndef JS_MORE_DETERMINISTIC
    




    int64_t now = PRMJ_Now();
    if (rt->gcNextFullGCTime && rt->gcNextFullGCTime <= now) {
        if (rt->gcChunkAllocationSinceLastGC ||
            rt->gcNumArenasFreeCommitted > FreeCommittedArenasThreshold)
        {
            PrepareForFullGC(rt);
            GCSlice(rt, GC_SHRINK, gcreason::MAYBEGC);
        } else {
            rt->gcNextFullGCTime = now + GC_IDLE_FULL_SPAN;
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
                ok = MarkPagesUnused(aheader->getArena(), ArenaSize);
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

            if (rt->gcChunkAllocationSinceLastGC) {
                



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
    DecommitArenasFromAvailableList(rt, &rt->gcSystemAvailableChunkListHead);
    DecommitArenasFromAvailableList(rt, &rt->gcUserAvailableChunkListHead);
}


static void
ExpireChunksAndArenas(JSRuntime *rt, bool shouldShrink)
{
    if (Chunk *toFree = rt->gcChunkPool.expire(rt, shouldShrink)) {
        AutoUnlockGC unlock(rt);
        FreeChunkList(toFree);
    }

    if (shouldShrink)
        DecommitArenas(rt);
}

static void
SweepBackgroundThings(JSRuntime* rt, bool onBackgroundThread)
{
    



    FreeOp fop(rt, false);
    for (int phase = 0 ; phase < BackgroundPhaseCount ; ++phase) {
        for (Zone *zone = rt->gcSweepingZones; zone; zone = zone->gcNextGraphNode) {
            for (int index = 0 ; index < BackgroundPhaseLength[phase] ; ++index) {
                AllocKind kind = BackgroundPhases[phase][index];
                ArenaHeader *arenas = zone->allocator.arenas.arenaListsToSweep[kind];
                if (arenas)
                    ArenaLists::backgroundFinalize(&fop, arenas, onBackgroundThread);
            }
        }
    }

    rt->gcSweepingZones = NULL;
}

#ifdef JS_THREADSAFE
static void
AssertBackgroundSweepingFinished(JSRuntime *rt)
{
    JS_ASSERT(!rt->gcSweepingZones);
    for (ZonesIter zone(rt); !zone.done(); zone.next()) {
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
    if (!(wakeup = PR_NewCondVar(rt->gcLock)))
        return false;
    if (!(done = PR_NewCondVar(rt->gcLock)))
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
    if (!rt->useHelperThreads()) {
        JS_ASSERT(state == IDLE);
        return;
    }


#ifdef JS_THREADSAFE
    PRThread *join = NULL;
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

void
GCHelperThread::threadMain(void *arg)
{
    PR_SetCurrentThreadName("JS GC Helper");
    static_cast<GCHelperThread *>(arg)->threadLoop();
}

void
GCHelperThread::threadLoop()
{
    AutoLockGC lock(rt);

    




    for (;;) {
        switch (state) {
          case SHUTDOWN:
            return;
          case IDLE:
            PR_WaitCondVar(wakeup, PR_INTERVAL_NO_TIMEOUT);
            break;
          case SWEEPING:
            doSweep();
            if (state == SWEEPING)
                state = IDLE;
            PR_NotifyAllCondVar(done);
            break;
          case ALLOCATING:
            do {
                Chunk *chunk;
                {
                    AutoUnlockGC unlock(rt);
                    chunk = Chunk::allocate(rt);
                }

                
                if (!chunk)
                    break;
                JS_ASSERT(chunk->info.numArenasFreeCommitted == ArenasPerChunk);
                rt->gcNumArenasFreeCommitted += ArenasPerChunk;
                rt->gcChunkPool.put(chunk);
            } while (state == ALLOCATING && rt->gcChunkPool.wantBackgroundAllocation(rt));
            if (state == ALLOCATING)
                state = IDLE;
            break;
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
        JS_NOT_REACHED("No shrink on shutdown");
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
        PR_WaitCondVar(done, PR_INTERVAL_NO_TIMEOUT);
    if (rt->gcIncrementalState == NO_INCREMENTAL)
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
        PR_WaitCondVar(done, PR_INTERVAL_NO_TIMEOUT);
    if (rt->gcIncrementalState == NO_INCREMENTAL)
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
            freeCursorEnd = NULL;
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
            freeCursor = freeCursorEnd = NULL;
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

static bool
ReleaseObservedTypes(JSRuntime *rt)
{
    bool releaseTypes = rt->gcZeal() != 0;

#ifndef JS_MORE_DETERMINISTIC
    int64_t now = PRMJ_Now();
    if (now >= rt->gcJitReleaseTime)
        releaseTypes = true;
    if (releaseTypes)
        rt->gcJitReleaseTime = now + JIT_SCRIPT_RELEASE_TYPES_INTERVAL;
#endif

    return releaseTypes;
}

static void
SweepCompartments(FreeOp *fop, bool lastGC)
{
    JSRuntime *rt = fop->runtime();
    JS_ASSERT_IF(lastGC, !rt->hasContexts());

    JSDestroyCompartmentCallback callback = rt->destroyCompartmentCallback;

    
    JSCompartment **read = rt->compartments.begin() + 1;
    JSCompartment **end = rt->compartments.end();
    JSCompartment **write = read;
    JS_ASSERT(rt->compartments.length() >= 1);
    JS_ASSERT(*rt->compartments.begin() == rt->atomsCompartment);

    while (read < end) {
        JSCompartment *compartment = *read++;

        if (!compartment->hold && compartment->zone()->wasGCStarted() &&
            (compartment->zone()->allocator.arenas.arenaListsAreEmpty() || lastGC))
        {
            compartment->zone()->allocator.arenas.checkEmptyFreeLists();
            if (callback)
                callback(fop, compartment);
            if (compartment->principals)
                JS_DropPrincipals(rt, compartment->principals);
            fop->delete_(compartment);
            continue;
        }
        *write++ = compartment;
    }
    rt->compartments.resize(write - rt->compartments.begin());
}

static void
PurgeRuntime(JSRuntime *rt)
{
    for (GCCompartmentsIter comp(rt); !comp.done(); comp.next())
        comp->purge();

    rt->freeLifoAlloc.transferUnusedFrom(&rt->tempLifoAlloc);

    rt->gsnCache.purge();
    rt->propertyCache.purge(rt);
    rt->newObjectCache.purge();
    rt->nativeIterCache.purge();
    rt->sourceDataCache.purge();
    rt->evalCache.clear();

    for (ContextIter acx(rt); !acx.done(); acx.next())
        acx->purge();
}

static bool
ShouldPreserveJITCode(JSCompartment *comp, int64_t currentTime)
{
    if (comp->rt->gcShouldCleanUpEverything || !comp->types.inferenceEnabled)
        return false;

    if (comp->rt->alwaysPreserveCode)
        return true;
    if (comp->lastAnimationTime + PRMJ_USEC_PER_SEC >= currentTime &&
        comp->lastCodeRelease + (PRMJ_USEC_PER_SEC * 300) >= currentTime)
    {
        return true;
    }

    comp->lastCodeRelease = currentTime;
    return false;
}

#ifdef DEBUG
struct CompartmentCheckTracer : public JSTracer
{
    Cell *src;
    JSGCTraceKind srcKind;
    JSCompartment *compartment;
};

static bool
InCrossCompartmentMap(JSObject *src, Cell *dst, JSGCTraceKind dstKind)
{
    JSCompartment *srccomp = src->compartment();

    if (dstKind == JSTRACE_OBJECT) {
        Value key = ObjectValue(*static_cast<JSObject *>(dst));
        if (WrapperMap::Ptr p = srccomp->lookupWrapper(key)) {
            if (*p->value.unsafeGet() == ObjectValue(*src))
                return true;
        }
    }

    



    for (JSCompartment::WrapperEnum e(srccomp); !e.empty(); e.popFront()) {
        if (e.front().key.wrapped == dst && ToMarkable(e.front().value) == src)
            return true;
    }

    return false;
}

static void
CheckCompartmentCallback(JSTracer *trcArg, void **thingp, JSGCTraceKind kind)
{
    CompartmentCheckTracer *trc = static_cast<CompartmentCheckTracer *>(trcArg);
    Cell *thing = (Cell *)*thingp;
    JS_ASSERT(thing->compartment() == trc->compartment ||
              thing->compartment() == trc->runtime->atomsCompartment ||
              (trc->srcKind == JSTRACE_OBJECT &&
               InCrossCompartmentMap((JSObject *)trc->src, thing, kind)));
}

static void
CheckForCompartmentMismatches(JSRuntime *rt)
{
    if (rt->gcDisableStrictProxyCheckingCount)
        return;

    CompartmentCheckTracer trc;
    JS_TracerInit(&trc, rt, CheckCompartmentCallback);

    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        trc.compartment = c;
        for (size_t thingKind = 0; thingKind < FINALIZE_LAST; thingKind++) {
            for (CellIterUnderGC i(c, AllocKind(thingKind)); !i.done(); i.next()) {
                trc.src = i.getCell();
                trc.srcKind = MapAllocToTraceKind(AllocKind(thingKind));
                JS_TraceChildren(&trc, trc.src, trc.srcKind);
            }
        }
    }
}
#endif

static bool
BeginMarkPhase(JSRuntime *rt)
{
    int64_t currentTime = PRMJ_Now();

#ifdef DEBUG
    CheckForCompartmentMismatches(rt);
#endif

    rt->gcIsFull = true;
    bool any = false;

    for (ZonesIter zone(rt); !zone.done(); zone.next()) {
        
        JS_ASSERT(!zone->isCollecting());
        for (unsigned i = 0; i < FINALIZE_LIMIT; ++i)
            JS_ASSERT(!zone->allocator.arenas.arenaListsToSweep[i]);

        
        if (zone->isGCScheduled()) {
            if (zone != rt->atomsCompartment->zone()) {
                any = true;
                zone->setGCState(Zone::Mark);
            }
        } else {
            rt->gcIsFull = false;
        }

        zone->scheduledForDestruction = false;
        zone->maybeAlive = false;
        zone->setPreservingCode(false);
    }

    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        JS_ASSERT(!c->gcLiveArrayBuffers);
        if (ShouldPreserveJITCode(c, currentTime))
            c->zone()->setPreservingCode(true);
    }

    
    if (!any)
        return false;

    





    Zone *atomsZone = rt->atomsCompartment->zone();
    if (atomsZone->isGCScheduled() && rt->gcIsFull && !rt->gcKeepAtoms) {
        JS_ASSERT(!atomsZone->isCollecting());
        atomsZone->setGCState(Zone::Mark);
    }

    






    if (rt->gcIsIncremental) {
        for (GCZonesIter zone(rt); !zone.done(); zone.next())
            zone->allocator.arenas.purge();
    }

    rt->gcMarker.start();
    JS_ASSERT(!rt->gcMarker.callback);
    JS_ASSERT(IS_GC_MARKING_TRACER(&rt->gcMarker));

    
    if (rt->gcIsIncremental) {
        for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
            gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_MARK_DISCARD_CODE);
            c->discardJitCode(rt->defaultFreeOp(), false);
        }
    }

    GCMarker *gcmarker = &rt->gcMarker;

    rt->gcStartNumber = rt->gcNumber;

    








    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_PURGE);
        PurgeRuntime(rt);
    }

    


    gcstats::AutoPhase ap1(rt->gcStats, gcstats::PHASE_MARK);
    gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_MARK_ROOTS);

    for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
        
        zone->allocator.arenas.unmarkAll();
    }

    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        
        WeakMapBase::resetCompartmentWeakMapList(c);
    }

    MarkRuntime(gcmarker);
    BufferGrayRoots(gcmarker);

    





























    
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            Cell *dst = e.front().key.wrapped;
            dst->zone()->maybeAlive = true;
        }

        if (c->hold)
            c->zone()->maybeAlive = true;
    }

    




    for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
        if (!zone->maybeAlive)
            zone->scheduledForDestruction = true;
    }
    rt->gcFoundBlackGrayEdges = false;

    return true;
}

template <class CompartmentIterT>
static void
MarkWeakReferences(JSRuntime *rt, gcstats::Phase phase)
{
    GCMarker *gcmarker = &rt->gcMarker;
    JS_ASSERT(gcmarker->isDrained());

    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_MARK);
    gcstats::AutoPhase ap1(rt->gcStats, phase);

    for (;;) {
        bool markedAny = false;
        for (CompartmentIterT c(rt); !c.done(); c.next()) {
            markedAny |= WatchpointMap::markCompartmentIteratively(c, gcmarker);
            markedAny |= WeakMapBase::markCompartmentIteratively(c, gcmarker);
        }
        markedAny |= Debugger::markAllIteratively(gcmarker);

        if (!markedAny)
            break;

        SliceBudget budget;
        gcmarker->drainMarkStack(budget);
    }
    JS_ASSERT(gcmarker->isDrained());
}

static void
MarkWeakReferencesInCurrentGroup(JSRuntime *rt, gcstats::Phase phase)
{
    MarkWeakReferences<GCCompartmentGroupIter>(rt, phase);
}

template <class ZoneIterT, class CompartmentIterT>
static void
MarkGrayReferences(JSRuntime *rt)
{
    GCMarker *gcmarker = &rt->gcMarker;

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_MARK);
        gcstats::AutoPhase ap1(rt->gcStats, gcstats::PHASE_SWEEP_MARK_GRAY);
        gcmarker->setMarkColorGray();
        if (gcmarker->hasBufferedGrayRoots()) {
            for (ZoneIterT zone(rt); !zone.done(); zone.next())
                gcmarker->markBufferedGrayRoots(zone);
        } else {
            JS_ASSERT(!rt->gcIsIncremental);
            if (JSTraceDataOp op = rt->gcGrayRootsTraceOp)
                (*op)(gcmarker, rt->gcGrayRootsData);
        }
        SliceBudget budget;
        gcmarker->drainMarkStack(budget);
    }

    MarkWeakReferences<CompartmentIterT>(rt, gcstats::PHASE_SWEEP_MARK_GRAY_WEAK);

    JS_ASSERT(gcmarker->isDrained());

    gcmarker->setMarkColorBlack();
}

static void
MarkGrayReferencesInCurrentGroup(JSRuntime *rt)
{
    MarkGrayReferences<GCZoneGroupIter, GCCompartmentGroupIter>(rt);
}

#ifdef DEBUG

static void
MarkAllWeakReferences(JSRuntime *rt, gcstats::Phase phase)
{
    MarkWeakReferences<GCCompartmentsIter>(rt, phase);
}

static void
MarkAllGrayReferences(JSRuntime *rt)
{
    MarkGrayReferences<GCZonesIter, GCCompartmentsIter>(rt);
}

class js::gc::MarkingValidator
{
  public:
    MarkingValidator(JSRuntime *rt);
    ~MarkingValidator();
    void nonIncrementalMark();
    void validate();

  private:
    JSRuntime *runtime;
    bool initialized;

    typedef HashMap<Chunk *, ChunkBitmap *, GCChunkHasher, SystemAllocPolicy> BitmapMap;
    BitmapMap map;
};

js::gc::MarkingValidator::MarkingValidator(JSRuntime *rt)
  : runtime(rt),
    initialized(false)
{}

js::gc::MarkingValidator::~MarkingValidator()
{
    if (!map.initialized())
        return;

    for (BitmapMap::Range r(map.all()); !r.empty(); r.popFront())
        js_delete(r.front().value);
}

void
js::gc::MarkingValidator::nonIncrementalMark()
{
    






    if (!map.init())
        return;

    GCMarker *gcmarker = &runtime->gcMarker;

    
    for (GCChunkSet::Range r(runtime->gcChunkSet.all()); !r.empty(); r.popFront()) {
        ChunkBitmap *bitmap = &r.front()->bitmap;
	ChunkBitmap *entry = js_new<ChunkBitmap>();
        if (!entry)
            return;

        memcpy(entry->bitmap, bitmap->bitmap, sizeof(bitmap->bitmap));
        if (!map.putNew(r.front(), entry))
            return;
    }

    



    WeakMapVector weakmaps;
    ArrayBufferVector arrayBuffers;
    for (GCCompartmentsIter c(runtime); !c.done(); c.next()) {
        if (!WeakMapBase::saveCompartmentWeakMapList(c, weakmaps) ||
            !ArrayBufferObject::saveArrayBufferList(c, arrayBuffers))
        {
            return;
        }
    }

    



    initialized = true;

    



    for (GCCompartmentsIter c(runtime); !c.done(); c.next()) {
        WeakMapBase::resetCompartmentWeakMapList(c);
        ArrayBufferObject::resetArrayBufferList(c);
    }

    
    js::gc::State state = runtime->gcIncrementalState;
    runtime->gcIncrementalState = MARK_ROOTS;

    JS_ASSERT(gcmarker->isDrained());
    gcmarker->reset();

    for (GCChunkSet::Range r(runtime->gcChunkSet.all()); !r.empty(); r.popFront())
        r.front()->bitmap.clear();

    {
        gcstats::AutoPhase ap1(runtime->gcStats, gcstats::PHASE_MARK);
        gcstats::AutoPhase ap2(runtime->gcStats, gcstats::PHASE_MARK_ROOTS);
        MarkRuntime(gcmarker, true);
    }

    SliceBudget budget;
    runtime->gcIncrementalState = MARK;
    runtime->gcMarker.drainMarkStack(budget);

    {
        gcstats::AutoPhase ap(runtime->gcStats, gcstats::PHASE_SWEEP);
        MarkAllWeakReferences(runtime, gcstats::PHASE_SWEEP_MARK_WEAK);

        
        for (GCZonesIter zone(runtime); !zone.done(); zone.next()) {
            JS_ASSERT(zone->isGCMarkingBlack());
            zone->setGCState(Zone::MarkGray);
        }

        MarkAllGrayReferences(runtime);

        
        for (GCZonesIter zone(runtime); !zone.done(); zone.next()) {
            JS_ASSERT(zone->isGCMarkingGray());
            zone->setGCState(Zone::Mark);
        }
    }

    
    for (GCChunkSet::Range r(runtime->gcChunkSet.all()); !r.empty(); r.popFront()) {
        Chunk *chunk = r.front();
        ChunkBitmap *bitmap = &chunk->bitmap;
        ChunkBitmap *entry = map.lookup(chunk)->value;
        js::Swap(*entry, *bitmap);
    }

    
    for (GCCompartmentsIter c(runtime); !c.done(); c.next()) {
        WeakMapBase::resetCompartmentWeakMapList(c);
        ArrayBufferObject::resetArrayBufferList(c);
    }
    WeakMapBase::restoreCompartmentWeakMapLists(weakmaps);
    ArrayBufferObject::restoreArrayBufferLists(arrayBuffers);

    runtime->gcIncrementalState = state;
}

void
js::gc::MarkingValidator::validate()
{
    




    if (!initialized)
        return;

    for (GCChunkSet::Range r(runtime->gcChunkSet.all()); !r.empty(); r.popFront()) {
        Chunk *chunk = r.front();
        BitmapMap::Ptr ptr = map.lookup(chunk);
        if (!ptr)
            continue;  

        ChunkBitmap *bitmap = ptr->value;
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

static void
ComputeNonIncrementalMarkingForValidation(JSRuntime *rt)
{
#ifdef DEBUG
    JS_ASSERT(!rt->gcMarkingValidator);
    if (rt->gcIsIncremental && rt->gcValidate)
        rt->gcMarkingValidator = js_new<MarkingValidator>(rt);
    if (rt->gcMarkingValidator)
        rt->gcMarkingValidator->nonIncrementalMark();
#endif
}

static void
ValidateIncrementalMarking(JSRuntime *rt)
{
#ifdef DEBUG
    if (rt->gcMarkingValidator)
        rt->gcMarkingValidator->validate();
#endif
}

static void
FinishMarkingValidation(JSRuntime *rt)
{
#ifdef DEBUG
    js_delete(rt->gcMarkingValidator);
    rt->gcMarkingValidator = NULL;
#endif
}

static void
DropStringWrappers(JSRuntime *rt)
{
    




    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            if (e.front().key.kind == CrossCompartmentKey::StringWrapper)
                e.removeFront();
        }
    }
}
















void
JSCompartment::findOutgoingEdgesFromCompartment(ComponentFinder<JS::Zone> &finder)
{
    for (js::WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront()) {
        CrossCompartmentKey::Kind kind = e.front().key.kind;
        JS_ASSERT(kind != CrossCompartmentKey::StringWrapper);
        Cell *other = e.front().key.wrapped;
        if (kind == CrossCompartmentKey::ObjectWrapper) {
            




            if (!other->isMarked(BLACK) || other->isMarked(GRAY)) {
                JS::Zone *w = other->zone();
                if (w->isGCMarking())
                    finder.addEdgeTo(w);
            }
        } else {
            JS_ASSERT(kind == CrossCompartmentKey::DebuggerScript ||
                      kind == CrossCompartmentKey::DebuggerObject ||
                      kind == CrossCompartmentKey::DebuggerEnvironment);
            




            JS::Zone *w = other->zone();
            if (w->isGCMarking())
                finder.addEdgeTo(w);
        }

#ifdef DEBUG
        JSObject *wrapper = &e.front().value.toObject();
        JS_ASSERT_IF(IsFunctionProxy(wrapper), &GetProxyCall(wrapper).toObject() == other);
#endif
    }

    Debugger::findCompartmentEdges(zone(), finder);
}

void
JSCompartment::findOutgoingEdges(ComponentFinder<JS::Zone> &finder)
{
    



    if (rt->atomsCompartment->isGCMarking())
        finder.addEdgeTo(rt->atomsCompartment);

    findOutgoingEdgesFromCompartment(finder);
}

static void
FindZoneGroups(JSRuntime *rt)
{
    ComponentFinder<Zone> finder(rt->mainThread.nativeStackLimit);
    if (!rt->gcIsIncremental)
        finder.useOneComponent();

    for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
        JS_ASSERT(zone->isGCMarking());
        finder.addNode(zone);
    }
    rt->gcZoneGroups = finder.getResultsList();
    rt->gcCurrentZoneGroup = rt->gcZoneGroups;
    rt->gcZoneGroupIndex = 0;
}

static void
ResetGrayList(JSCompartment* comp);

static void
GetNextZoneGroup(JSRuntime *rt)
{
    rt->gcCurrentZoneGroup = rt->gcCurrentZoneGroup->nextGroup();
    ++rt->gcZoneGroupIndex;

    if (!rt->gcIsIncremental)
        ComponentFinder<Zone>::mergeGroups(rt->gcCurrentZoneGroup);

    if (rt->gcAbortSweepAfterCurrentGroup) {
        JS_ASSERT(!rt->gcIsIncremental);
        for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
            JS_ASSERT(!zone->gcNextGraphComponent);
            JS_ASSERT(zone->isGCMarking());
            zone->setNeedsBarrier(false, Zone::UpdateIon);
            zone->setGCState(Zone::NoGC);
            zone->gcGrayRoots.clearAndFree();
        }

        for (GCCompartmentGroupIter comp(rt); !comp.done(); comp.next()) {
            ArrayBufferObject::resetArrayBufferList(comp);
            ResetGrayList(comp);
        }

        rt->gcAbortSweepAfterCurrentGroup = false;
        rt->gcCurrentZoneGroup = NULL;
    }
}



























static bool
IsGrayListObject(RawObject obj)
{
    JS_ASSERT(obj);
    return IsCrossCompartmentWrapper(obj) && !IsDeadProxyObject(obj);
}

const unsigned JSSLOT_GC_GRAY_LINK = JSSLOT_PROXY_EXTRA + 1;

static unsigned
GrayLinkSlot(RawObject obj)
{
    JS_ASSERT(IsGrayListObject(obj));
    return JSSLOT_GC_GRAY_LINK;
}

#ifdef DEBUG
static void
AssertNotOnGrayList(RawObject obj)
{
    JS_ASSERT_IF(IsGrayListObject(obj), obj->getReservedSlot(GrayLinkSlot(obj)).isUndefined());
}
#endif

static Cell *
CrossCompartmentPointerReferent(RawObject obj)
{
    JS_ASSERT(IsGrayListObject(obj));
    return (Cell *)GetProxyPrivate(obj).toGCThing();
}

static RawObject
NextIncomingCrossCompartmentPointer(RawObject prev, bool unlink)
{
    unsigned slot = GrayLinkSlot(prev);
    RawObject next = prev->getReservedSlot(slot).toObjectOrNull();
    JS_ASSERT_IF(next, IsGrayListObject(next));

    if (unlink)
        prev->setSlot(slot, UndefinedValue());

    return next;
}

void
js::DelayCrossCompartmentGrayMarking(RawObject src)
{
    JS_ASSERT(IsGrayListObject(src));

    
    unsigned slot = GrayLinkSlot(src);
    Cell *dest = CrossCompartmentPointerReferent(src);
    JSCompartment *comp = dest->compartment();

    if (src->getReservedSlot(slot).isUndefined()) {
        src->setCrossCompartmentSlot(slot, ObjectOrNullValue(comp->gcIncomingGrayPointers));
        comp->gcIncomingGrayPointers = src;
    } else {
        JS_ASSERT(src->getReservedSlot(slot).isObjectOrNull());
    }

#ifdef DEBUG
    



    RawObject obj = comp->gcIncomingGrayPointers;
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

    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_MARK);
    static const gcstats::Phase statsPhases[] = {
        gcstats::PHASE_SWEEP_MARK_INCOMING_BLACK,
        gcstats::PHASE_SWEEP_MARK_INCOMING_GRAY
    };
    gcstats::AutoPhase ap1(rt->gcStats, statsPhases[color]);

    bool unlinkList = color == GRAY;

    for (GCCompartmentGroupIter c(rt); !c.done(); c.next()) {
        JS_ASSERT_IF(color == GRAY, c->zone()->isGCMarkingGray());
        JS_ASSERT_IF(color == BLACK, c->zone()->isGCMarkingBlack());
        JS_ASSERT_IF(c->gcIncomingGrayPointers, IsGrayListObject(c->gcIncomingGrayPointers));

        for (RawObject src = c->gcIncomingGrayPointers;
             src;
             src = NextIncomingCrossCompartmentPointer(src, unlinkList))
        {
            Cell *dst = CrossCompartmentPointerReferent(src);
            JS_ASSERT(dst->compartment() == c);

            if (color == GRAY) {
                if (IsObjectMarked(&src) && src->isMarked(GRAY))
                    MarkGCThingUnbarriered(&rt->gcMarker, (void**)&dst,
                                           "cross-compartment gray pointer");
            } else {
                if (IsObjectMarked(&src) && !src->isMarked(GRAY))
                    MarkGCThingUnbarriered(&rt->gcMarker, (void**)&dst,
                                           "cross-compartment black pointer");
            }
        }

        if (unlinkList)
            c->gcIncomingGrayPointers = NULL;
    }

    SliceBudget budget;
    rt->gcMarker.drainMarkStack(budget);
}

static bool
RemoveFromGrayList(RawObject wrapper)
{
    if (!IsGrayListObject(wrapper))
        return false;

    unsigned slot = GrayLinkSlot(wrapper);
    if (wrapper->getReservedSlot(slot).isUndefined())
        return false;  

    RawObject tail = wrapper->getReservedSlot(slot).toObjectOrNull();
    wrapper->setReservedSlot(slot, UndefinedValue());

    JSCompartment *comp = CrossCompartmentPointerReferent(wrapper)->compartment();
    RawObject obj = comp->gcIncomingGrayPointers;
    if (obj == wrapper) {
        comp->gcIncomingGrayPointers = tail;
        return true;
    }

    while (obj) {
        unsigned slot = GrayLinkSlot(obj);
        RawObject next = obj->getReservedSlot(slot).toObjectOrNull();
        if (next == wrapper) {
            obj->setCrossCompartmentSlot(slot, ObjectOrNullValue(tail));
            return true;
        }
        obj = next;
    }

    JS_NOT_REACHED("object not found in gray link list");
    return false;
}

static void
ResetGrayList(JSCompartment *comp)
{
    RawObject src = comp->gcIncomingGrayPointers;
    while (src)
        src = NextIncomingCrossCompartmentPointer(src, true);
    comp->gcIncomingGrayPointers = NULL;
}

void
js::NotifyGCNukeWrapper(RawObject obj)
{
    



    RemoveFromGrayList(obj);
}

enum {
    JS_GC_SWAP_OBJECT_A_REMOVED = 1 << 0,
    JS_GC_SWAP_OBJECT_B_REMOVED = 1 << 1
};

unsigned
js::NotifyGCPreSwap(RawObject a, RawObject b)
{
    




    return (RemoveFromGrayList(a) ? JS_GC_SWAP_OBJECT_A_REMOVED : 0) |
           (RemoveFromGrayList(b) ? JS_GC_SWAP_OBJECT_B_REMOVED : 0);
}

void
js::NotifyGCPostSwap(RawObject a, RawObject b, unsigned removedFlags)
{
    



    if (removedFlags & JS_GC_SWAP_OBJECT_A_REMOVED)
        DelayCrossCompartmentGrayMarking(b);
    if (removedFlags & JS_GC_SWAP_OBJECT_B_REMOVED)
        DelayCrossCompartmentGrayMarking(a);
}

static void
EndMarkingZoneGroup(JSRuntime *rt)
{
    




    MarkIncomingCrossCompartmentPointers(rt, BLACK);

    MarkWeakReferencesInCurrentGroup(rt, gcstats::PHASE_SWEEP_MARK_WEAK);

    





    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        JS_ASSERT(zone->isGCMarkingBlack());
        zone->setGCState(Zone::MarkGray);
    }

    
    rt->gcMarker.setMarkColorGray();
    MarkIncomingCrossCompartmentPointers(rt, GRAY);
    rt->gcMarker.setMarkColorBlack();

    
    MarkGrayReferencesInCurrentGroup(rt);

    
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        JS_ASSERT(zone->isGCMarkingGray());
        zone->setGCState(Zone::Mark);
    }

    JS_ASSERT(rt->gcMarker.isDrained());
}

static void
BeginSweepingZoneGroup(JSRuntime *rt)
{
    




    bool sweepingAtoms = false;
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        
        JS_ASSERT(zone->isGCMarking());
        zone->setGCState(Zone::Sweep);

        
        zone->allocator.arenas.purge();

        if (zone == rt->atomsCompartment->zone())
            sweepingAtoms = true;
    }

    ValidateIncrementalMarking(rt);

    FreeOp fop(rt, rt->gcSweepOnBackgroundThread);

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_FINALIZE_START);
        if (rt->gcFinalizeCallback)
            rt->gcFinalizeCallback(&fop, JSFINALIZE_GROUP_START, !rt->gcIsFull );
    }

    if (sweepingAtoms) {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_ATOMS);
        SweepAtoms(rt);
    }

    
    for (GCCompartmentGroupIter c(rt); !c.done(); c.next())
        ArrayBufferObject::sweep(c);

    
    WatchpointMap::sweepAll(rt);

    
    Debugger::sweepAll(&fop);

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_COMPARTMENTS);

        bool releaseTypes = ReleaseObservedTypes(rt);
        for (GCCompartmentGroupIter c(rt); !c.done(); c.next()) {
            gcstats::AutoSCC scc(rt->gcStats, rt->gcZoneGroupIndex);
            c->sweep(&fop, releaseTypes);
        }
    }

    







    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        gcstats::AutoSCC scc(rt->gcStats, rt->gcZoneGroupIndex);
        zone->allocator.arenas.queueObjectsForSweep(&fop);
    }
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        gcstats::AutoSCC scc(rt->gcStats, rt->gcZoneGroupIndex);
        zone->allocator.arenas.queueStringsForSweep(&fop);
    }
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        gcstats::AutoSCC scc(rt->gcStats, rt->gcZoneGroupIndex);
        zone->allocator.arenas.queueScriptsForSweep(&fop);
    }
#ifdef JS_ION
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        gcstats::AutoSCC scc(rt->gcStats, rt->gcZoneGroupIndex);
        zone->allocator.arenas.queueIonCodeForSweep(&fop);
    }
#endif
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        gcstats::AutoSCC scc(rt->gcStats, rt->gcZoneGroupIndex);
        zone->allocator.arenas.queueShapesForSweep(&fop);
        zone->allocator.arenas.gcShapeArenasToSweep =
            zone->allocator.arenas.arenaListsToSweep[FINALIZE_SHAPE];
    }

    rt->gcSweepPhase = 0;
    rt->gcSweepZone = rt->gcCurrentZoneGroup;
    rt->gcSweepKindIndex = 0;

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_FINALIZE_END);
        if (rt->gcFinalizeCallback)
            rt->gcFinalizeCallback(&fop, JSFINALIZE_GROUP_END, !rt->gcIsFull );
    }
}

static void
EndSweepingZoneGroup(JSRuntime *rt)
{
    
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        JS_ASSERT(zone->isGCSweeping());
        zone->setGCState(Zone::Finished);
    }

    
    while (ArenaHeader *arena = rt->gcArenasAllocatedDuringSweep) {
        rt->gcArenasAllocatedDuringSweep = arena->getNextAllocDuringSweep();
        arena->unsetAllocDuringSweep();
    }
}

static void
BeginSweepPhase(JSRuntime *rt)
{
    







    JS_ASSERT(!rt->gcAbortSweepAfterCurrentGroup);

    ComputeNonIncrementalMarkingForValidation(rt);

    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP);

#ifdef JS_THREADSAFE
    rt->gcSweepOnBackgroundThread = rt->hasContexts() && rt->useHelperThreads();
#endif

#ifdef DEBUG
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        JS_ASSERT(!c->gcIncomingGrayPointers);
        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            if (e.front().key.kind != CrossCompartmentKey::StringWrapper)
                AssertNotOnGrayList(&e.front().value.get().toObject());
        }
    }
#endif

    DropStringWrappers(rt);
    FindZoneGroups(rt);
    EndMarkingZoneGroup(rt);
    BeginSweepingZoneGroup(rt);
}

bool
ArenaLists::foregroundFinalize(FreeOp *fop, AllocKind thingKind, SliceBudget &sliceBudget)
{
    if (!arenaListsToSweep[thingKind])
        return true;

    ArenaList &dest = arenaLists[thingKind];
    return FinalizeArenas(fop, &arenaListsToSweep[thingKind], dest, thingKind, sliceBudget);
}

static bool
DrainMarkStack(JSRuntime *rt, SliceBudget &sliceBudget, gcstats::Phase phase)
{
    
    gcstats::AutoPhase ap(rt->gcStats, phase);
    return rt->gcMarker.drainMarkStack(sliceBudget);
}

static bool
SweepPhase(JSRuntime *rt, SliceBudget &sliceBudget)
{
    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP);
    FreeOp fop(rt, rt->gcSweepOnBackgroundThread);

    bool finished = DrainMarkStack(rt, sliceBudget, gcstats::PHASE_SWEEP_MARK);
    if (!finished)
        return false;

    for (;;) {
        
        for (; rt->gcSweepPhase < FinalizePhaseCount ; ++rt->gcSweepPhase) {
            gcstats::AutoPhase ap(rt->gcStats, FinalizePhaseStatsPhase[rt->gcSweepPhase]);

            for (; rt->gcSweepZone; rt->gcSweepZone = rt->gcSweepZone->nextNodeInGroup()) {
                Zone *zone = rt->gcSweepZone;

                while (rt->gcSweepKindIndex < FinalizePhaseLength[rt->gcSweepPhase]) {
                    AllocKind kind = FinalizePhases[rt->gcSweepPhase][rt->gcSweepKindIndex];

                    if (!zone->allocator.arenas.foregroundFinalize(&fop, kind, sliceBudget))
                        return false;  

                    ++rt->gcSweepKindIndex;
                }
                rt->gcSweepKindIndex = 0;
            }
            rt->gcSweepZone = rt->gcCurrentZoneGroup;
        }

        
        {
            gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_SHAPE);

            for (; rt->gcSweepZone; rt->gcSweepZone = rt->gcSweepZone->nextNodeInGroup()) {
                Zone *zone = rt->gcSweepZone;
                while (ArenaHeader *arena = zone->allocator.arenas.gcShapeArenasToSweep) {
                    for (CellIterUnderGC i(arena); !i.done(); i.next()) {
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

        EndSweepingZoneGroup(rt);
        GetNextZoneGroup(rt);
        if (!rt->gcCurrentZoneGroup)
            return true;  
        EndMarkingZoneGroup(rt);
        BeginSweepingZoneGroup(rt);
    }
}

static void
EndSweepPhase(JSRuntime *rt, JSGCInvocationKind gckind, bool lastGC)
{
    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP);
    FreeOp fop(rt, rt->gcSweepOnBackgroundThread);

    JS_ASSERT_IF(lastGC, !rt->gcSweepOnBackgroundThread);

    JS_ASSERT(rt->gcMarker.isDrained());
    rt->gcMarker.stop();

    



    if (rt->gcIsFull) {
        for (ZonesIter zone(rt); !zone.done(); zone.next()) {
            if (!zone->isCollecting()) {
                rt->gcIsFull = false;
                break;
            }
        }
    }

    





    if (rt->gcFoundBlackGrayEdges) {
        for (ZonesIter zone(rt); !zone.done(); zone.next()) {
            if (!zone->isCollecting())
                zone->allocator.arenas.unmarkAll();
        }
    }

#ifdef DEBUG
    PropertyTree::dumpShapes(rt);
#endif

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_DESTROY);

        





        if (rt->gcIsFull) {
            SweepScriptFilenames(rt);
            SweepScriptData(rt);
        }

        
        if (JSC::ExecutableAllocator *execAlloc = rt->maybeExecAlloc())
            execAlloc->purge();

        



        if (!lastGC)
            SweepCompartments(&fop, lastGC);

        if (!rt->gcSweepOnBackgroundThread) {
            





            AutoLockGC lock(rt);
            ExpireChunksAndArenas(rt, gckind == GC_SHRINK);
        }
    }

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_FINALIZE_END);

        if (rt->gcFinalizeCallback)
            rt->gcFinalizeCallback(&fop, JSFINALIZE_COLLECTION_END, !rt->gcIsFull);

        
        if (rt->gcIsFull)
            rt->gcGrayBitsValid = true;
    }

    
    JS_ASSERT(!rt->gcSweepingZones);
    for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
        zone->gcNextGraphNode = rt->gcSweepingZones;
        rt->gcSweepingZones = zone;
    }

    
    if (!rt->gcSweepOnBackgroundThread) {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_DESTROY);

        SweepBackgroundThings(rt, false);

        rt->freeLifoAlloc.freeAll();

        
        if (lastGC)
            SweepCompartments(&fop, lastGC);
    }

    for (ZonesIter zone(rt); !zone.done(); zone.next()) {
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
                         !rt->gcSweepOnBackgroundThread,
                         !zone->allocator.arenas.arenaListsToSweep[i]);
        }
#endif
    }

#ifdef DEBUG
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        JS_ASSERT(!c->gcIncomingGrayPointers);
        JS_ASSERT(!c->gcLiveArrayBuffers);

        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            if (e.front().key.kind != CrossCompartmentKey::StringWrapper)
                AssertNotOnGrayList(&e.front().value.get().toObject());
        }
    }
#endif

    FinishMarkingValidation(rt);

    rt->gcLastGCTime = PRMJ_Now();
}


class AutoGCSession : AutoTraceSession {
  public:
    explicit AutoGCSession(JSRuntime *rt);
    ~AutoGCSession();
};


AutoTraceSession::AutoTraceSession(JSRuntime *rt, js::HeapState heapState)
  : runtime(rt),
    prevState(rt->heapState)
{
    JS_ASSERT(!rt->noGCOrAllocationCheck);
    JS_ASSERT(!rt->isHeapBusy());
    JS_ASSERT(heapState == Collecting || heapState == Tracing);
    rt->heapState = heapState;
}

AutoTraceSession::~AutoTraceSession()
{
    JS_ASSERT(runtime->isHeapBusy());
    runtime->heapState = prevState;
}

AutoGCSession::AutoGCSession(JSRuntime *rt)
  : AutoTraceSession(rt, Collecting)
{
    runtime->gcIsNeeded = false;
    runtime->gcInterFrameGC = true;

    runtime->gcNumber++;
}

AutoGCSession::~AutoGCSession()
{
#ifndef JS_MORE_DETERMINISTIC
    runtime->gcNextFullGCTime = PRMJ_Now() + GC_IDLE_FULL_SPAN;
#endif

    runtime->gcChunkAllocationSinceLastGC = false;

#ifdef JS_GC_ZEAL
    
    runtime->gcSelectedForMarking.clearAndFree();
#endif

    
    for (ZonesIter zone(runtime); !zone.done(); zone.next()) {
        zone->resetGCMallocBytes();
        zone->unscheduleGC();
    }

    runtime->resetGCMallocBytes();
}

AutoCopyFreeListToArenas::AutoCopyFreeListToArenas(JSRuntime *rt)
  : runtime(rt)
{
    for (ZonesIter zone(rt); !zone.done(); zone.next())
        zone->allocator.arenas.copyFreeListsToArenas();
}

AutoCopyFreeListToArenas::~AutoCopyFreeListToArenas()
{
    for (ZonesIter zone(runtime); !zone.done(); zone.next())
        zone->allocator.arenas.clearFreeListsInArenas();
}

static void
IncrementalCollectSlice(JSRuntime *rt,
                        int64_t budget,
                        gcreason::Reason gcReason,
                        JSGCInvocationKind gcKind);

static void
ResetIncrementalGC(JSRuntime *rt, const char *reason)
{
    switch (rt->gcIncrementalState) {
      case NO_INCREMENTAL:
        return;

      case MARK: {
        
        AutoCopyFreeListToArenas copy(rt);

        rt->gcMarker.reset();
        rt->gcMarker.stop();

        for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
            ArrayBufferObject::resetArrayBufferList(c);
            ResetGrayList(c);
        }

        for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
            JS_ASSERT(zone->isGCMarking());
            zone->setNeedsBarrier(false, Zone::UpdateIon);
            zone->setGCState(Zone::NoGC);
        }

        rt->gcIncrementalState = NO_INCREMENTAL;

        JS_ASSERT(!rt->gcStrictCompartmentChecking);

        break;
      }

      case SWEEP:
        rt->gcMarker.reset();

        for (ZonesIter zone(rt); !zone.done(); zone.next())
            zone->scheduledForDestruction = false;

        
        rt->gcAbortSweepAfterCurrentGroup = true;
        IncrementalCollectSlice(rt, SliceBudget::Unlimited, gcreason::RESET, GC_NORMAL);

        {
            gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_WAIT_BACKGROUND_THREAD);
            rt->gcHelperThread.waitBackgroundSweepOrAllocEnd();
        }
        break;

      default:
        JS_NOT_REACHED("Invalid incremental GC state");
    }

    rt->gcStats.reset(reason);

#ifdef DEBUG
    for (CompartmentsIter c(rt); !c.done(); c.next())
        JS_ASSERT(!c->gcLiveArrayBuffers);

    for (ZonesIter zone(rt); !zone.done(); zone.next()) {
        JS_ASSERT(!zone->needsBarrier());
        for (unsigned i = 0; i < FINALIZE_LIMIT; ++i)
            JS_ASSERT(!zone->allocator.arenas.arenaListsToSweep[i]);
    }
#endif
}

class AutoGCSlice {
  public:
    AutoGCSlice(JSRuntime *rt);
    ~AutoGCSlice();

  private:
    JSRuntime *runtime;
};

AutoGCSlice::AutoGCSlice(JSRuntime *rt)
  : runtime(rt)
{
    





    rt->stackSpace.markActiveCompartments();

    for (GCZonesIter zone(rt); !zone.done(); zone.next()) {
        





        if (zone->isGCMarking()) {
            JS_ASSERT(zone->needsBarrier());
            zone->setNeedsBarrier(false, Zone::DontUpdateIon);
        } else {
            JS_ASSERT(!zone->needsBarrier());
        }
    }
}

AutoGCSlice::~AutoGCSlice()
{
    
    for (ZonesIter zone(runtime); !zone.done(); zone.next()) {
        if (zone->isGCMarking()) {
            zone->setNeedsBarrier(true, Zone::UpdateIon);
            zone->allocator.arenas.prepareForIncrementalGC(runtime);
        } else {
            zone->setNeedsBarrier(false, Zone::UpdateIon);
        }
    }
}

static void
PushZealSelectedObjects(JSRuntime *rt)
{
#ifdef JS_GC_ZEAL
    
    for (JSObject **obj = rt->gcSelectedForMarking.begin();
         obj != rt->gcSelectedForMarking.end(); obj++)
    {
        MarkObjectUnbarriered(&rt->gcMarker, obj, "selected obj");
    }
#endif
}

static void
IncrementalCollectSlice(JSRuntime *rt,
                        int64_t budget,
                        gcreason::Reason reason,
                        JSGCInvocationKind gckind)
{
    AutoCopyFreeListToArenas copy(rt);
    AutoGCSlice slice(rt);

    gc::State initialState = rt->gcIncrementalState;

    int zeal = 0;
#ifdef JS_GC_ZEAL
    if (reason == gcreason::DEBUG_GC && budget != SliceBudget::Unlimited) {
        




        zeal = rt->gcZeal();
    }
#endif

    JS_ASSERT_IF(rt->gcIncrementalState != NO_INCREMENTAL, rt->gcIsIncremental);
    rt->gcIsIncremental = budget != SliceBudget::Unlimited;

    if (zeal == ZealIncrementalRootsThenFinish || zeal == ZealIncrementalMarkAllThenFinish) {
        



        budget = SliceBudget::Unlimited;
    }

    SliceBudget sliceBudget(budget);

    if (rt->gcIncrementalState == NO_INCREMENTAL) {
        rt->gcIncrementalState = MARK_ROOTS;
        rt->gcLastMarkSlice = false;
    }

    if (rt->gcIncrementalState == MARK)
        AutoGCRooter::traceAllWrappers(&rt->gcMarker);

    switch (rt->gcIncrementalState) {

      case MARK_ROOTS:
        if (!BeginMarkPhase(rt)) {
            rt->gcIncrementalState = NO_INCREMENTAL;
            return;
        }

        if (rt->hasContexts())
            PushZealSelectedObjects(rt);

        rt->gcIncrementalState = MARK;

        if (zeal == ZealIncrementalRootsThenFinish)
            break;

        

      case MARK: {
        
        if (!rt->gcMarker.hasBufferedGrayRoots())
            sliceBudget.reset();

        bool finished = DrainMarkStack(rt, sliceBudget, gcstats::PHASE_MARK);
        if (!finished)
            break;

        JS_ASSERT(rt->gcMarker.isDrained());

        if (!rt->gcLastMarkSlice &&
            ((initialState == MARK && budget != SliceBudget::Unlimited) ||
             zeal == ZealIncrementalMarkAllThenFinish))
        {
            




            rt->gcLastMarkSlice = true;
            break;
        }

        rt->gcIncrementalState = SWEEP;

        



        BeginSweepPhase(rt);
        if (sliceBudget.isOverBudget())
            break;

        



        if (zeal == ZealIncrementalMultipleSlices)
            break;

        
      }

      case SWEEP: {
        bool finished = SweepPhase(rt, sliceBudget);
        if (!finished)
            break;

        EndSweepPhase(rt, gckind, reason == gcreason::LAST_CONTEXT);

        if (rt->gcSweepOnBackgroundThread)
            rt->gcHelperThread.startBackgroundSweep(gckind == GC_SHRINK);

        rt->gcIncrementalState = NO_INCREMENTAL;
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

    if (rt->gcKeepAtoms)
        return IncrementalSafety::Unsafe("gcKeepAtoms set");

    if (!rt->gcIncrementalEnabled)
        return IncrementalSafety::Unsafe("incremental permanently disabled");

    return IncrementalSafety::Safe();
}

static void
BudgetIncrementalGC(JSRuntime *rt, int64_t *budget)
{
    IncrementalSafety safe = IsIncrementalGCSafe(rt);
    if (!safe) {
        ResetIncrementalGC(rt, safe.reason());
        *budget = SliceBudget::Unlimited;
        rt->gcStats.nonincremental(safe.reason());
        return;
    }

    if (rt->gcMode != JSGC_MODE_INCREMENTAL) {
        ResetIncrementalGC(rt, "GC mode change");
        *budget = SliceBudget::Unlimited;
        rt->gcStats.nonincremental("GC mode");
        return;
    }

    if (rt->isTooMuchMalloc()) {
        *budget = SliceBudget::Unlimited;
        rt->gcStats.nonincremental("malloc bytes trigger");
    }

    bool reset = false;
    for (ZonesIter zone(rt); !zone.done(); zone.next()) {
        if (zone->gcBytes >= zone->gcTriggerBytes) {
            *budget = SliceBudget::Unlimited;
            rt->gcStats.nonincremental("allocation trigger");
        }

        if (rt->gcIncrementalState != NO_INCREMENTAL &&
            zone->isGCScheduled() != zone->wasGCStarted())
        {
            reset = true;
        }

        if (zone->isTooMuchMalloc()) {
            *budget = SliceBudget::Unlimited;
            rt->gcStats.nonincremental("malloc bytes trigger");
        }
    }

    if (reset)
        ResetIncrementalGC(rt, "zone change");
}







static JS_NEVER_INLINE void
GCCycle(JSRuntime *rt, bool incremental, int64_t budget, JSGCInvocationKind gckind, gcreason::Reason reason)
{
    
    JS_ASSERT(!rt->isHeapBusy());

#ifdef DEBUG
    for (ZonesIter zone(rt); !zone.done(); zone.next())
        JS_ASSERT_IF(rt->gcMode == JSGC_MODE_GLOBAL, zone->isGCScheduled());
#endif

    AutoGCSession gcsession(rt);

    





    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_WAIT_BACKGROUND_THREAD);
        rt->gcHelperThread.waitBackgroundSweepOrAllocEnd();
    }

    {
        if (!incremental) {
            
            ResetIncrementalGC(rt, "requested");
            rt->gcStats.nonincremental("requested");
            budget = SliceBudget::Unlimited;
        } else {
            BudgetIncrementalGC(rt, &budget);
        }

        IncrementalCollectSlice(rt, budget, reason, gckind);
    }
}

#ifdef JS_GC_ZEAL
static bool
IsDeterministicGCReason(gcreason::Reason reason)
{
    if (reason > gcreason::DEBUG_GC &&
        reason != gcreason::CC_FORCED && reason != gcreason::SHUTDOWN_CC)
    {
        return false;
    }

    if (reason == gcreason::MAYBEGC)
        return false;

    return true;
}
#endif

static bool
ShouldCleanUpEverything(JSRuntime *rt, gcreason::Reason reason, JSGCInvocationKind gckind)
{
    
    
    
    
    
    
    
    return !rt->hasContexts() ||
           reason == gcreason::SHUTDOWN_CC ||
           reason == gcreason::DEBUG_MODE_GC ||
           gckind == GC_SHRINK;
}

static void
Collect(JSRuntime *rt, bool incremental, int64_t budget,
        JSGCInvocationKind gckind, gcreason::Reason reason)
{
    
    JS_ASSERT(!InParallelSection());

    JS_AbortIfWrongThread(rt);

    if (rt->mainThread.suppressGC)
        return;

#if JS_TRACE_LOGGING
    AutoTraceLog logger(TraceLogging::defaultLogger(),
                        TraceLogging::GC_START,
                        TraceLogging::GC_STOP);
#endif

    ContextIter cx(rt);
    if (!cx.done())
        MaybeCheckStackRoots(cx);

#ifdef JS_GC_ZEAL
    if (rt->gcDeterministicOnly && !IsDeterministicGCReason(reason))
        return;
#endif

    JS_ASSERT_IF(!incremental || budget != SliceBudget::Unlimited, JSGC_INCREMENTAL);

#ifdef JS_GC_ZEAL
    bool isShutdown = reason == gcreason::SHUTDOWN_CC || !rt->hasContexts();
    struct AutoVerifyBarriers {
        JSRuntime *runtime;
        bool restartPreVerifier;
        bool restartPostVerifier;
        AutoVerifyBarriers(JSRuntime *rt, bool isShutdown)
          : runtime(rt)
        {
            restartPreVerifier = !isShutdown && rt->gcVerifyPreData;
            restartPostVerifier = !isShutdown && rt->gcVerifyPostData;
            if (rt->gcVerifyPreData)
                EndVerifyPreBarriers(rt);
            if (rt->gcVerifyPostData)
                EndVerifyPostBarriers(rt);
        }
        ~AutoVerifyBarriers() {
            if (restartPreVerifier)
                StartVerifyPreBarriers(runtime);
            if (restartPostVerifier)
                StartVerifyPostBarriers(runtime);
        }
    } av(rt, isShutdown);
#endif

    RecordNativeStackTopForGC(rt);

    int zoneCount = 0;
    int compartmentCount = 0;
    int collectedCount = 0;
    for (ZonesIter zone(rt); !zone.done(); zone.next()) {
        if (rt->gcMode == JSGC_MODE_GLOBAL)
            zone->scheduleGC();

        
        if (rt->gcIncrementalState != NO_INCREMENTAL && zone->needsBarrier())
            zone->scheduleGC();

        zoneCount++;
        if (zone->isGCScheduled())
            collectedCount++;
    }

    for (CompartmentsIter c(rt); !c.done(); c.next())
        compartmentCount++;

    rt->gcShouldCleanUpEverything = ShouldCleanUpEverything(rt, reason, gckind);

    gcstats::AutoGCSlice agc(rt->gcStats, collectedCount, zoneCount, compartmentCount, reason);

    do {
        



        if (rt->gcIncrementalState == NO_INCREMENTAL) {
            gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_GC_BEGIN);
            if (JSGCCallback callback = rt->gcCallback)
                callback(rt, JSGC_BEGIN);
        }

        rt->gcPoke = false;
        GCCycle(rt, incremental, budget, gckind, reason);

        if (rt->gcIncrementalState == NO_INCREMENTAL) {
            gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_GC_END);
            if (JSGCCallback callback = rt->gcCallback)
                callback(rt, JSGC_END);
        }

        
        if (rt->gcPoke && rt->gcShouldCleanUpEverything)
            PrepareForFullGC(rt);

        



    } while (rt->gcPoke && rt->gcShouldCleanUpEverything);
}

void
js::GC(JSRuntime *rt, JSGCInvocationKind gckind, gcreason::Reason reason)
{
    Collect(rt, false, SliceBudget::Unlimited, gckind, reason);
}

void
js::GCSlice(JSRuntime *rt, JSGCInvocationKind gckind, gcreason::Reason reason, int64_t millis)
{
    int64_t sliceBudget;
    if (millis)
        sliceBudget = SliceBudget::TimeBudget(millis);
    else if (rt->gcHighFrequencyGC && rt->gcDynamicMarkSlice)
        sliceBudget = rt->gcSliceBudget * IGC_MARK_SLICE_MULTIPLIER;
    else
        sliceBudget = rt->gcSliceBudget;

    Collect(rt, true, sliceBudget, gckind, reason);
}

void
js::GCFinalSlice(JSRuntime *rt, JSGCInvocationKind gckind, gcreason::Reason reason)
{
    Collect(rt, true, SliceBudget::Unlimited, gckind, reason);
}

static bool
ZonesSelected(JSRuntime *rt)
{
    for (ZonesIter zone(rt); !zone.done(); zone.next()) {
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
        if (IsIncrementalGCInProgress(rt))
            PrepareForIncrementalGC(rt);
        else
            PrepareForFullGC(rt);
    }
    Collect(rt, true, budget, GC_NORMAL, gcreason::DEBUG_GC);
}


void
js::PrepareForDebugGC(JSRuntime *rt)
{
    if (!ZonesSelected(rt))
        PrepareForFullGC(rt);
}

JS_FRIEND_API(void)
JS::ShrinkGCBuffers(JSRuntime *rt)
{
    AutoLockGC lock(rt);
    JS_ASSERT(!rt->isHeapBusy());

    if (!rt->useHelperThreads())
        ExpireChunksAndArenas(rt, true);
    else
        rt->gcHelperThread.startBackgroundShrink();
}

void
js::gc::FinishBackgroundFinalize(JSRuntime *rt)
{
    rt->gcHelperThread.waitBackgroundSweepEnd();
}

AutoFinishGC::AutoFinishGC(JSRuntime *rt)
{
    if (IsIncrementalGCInProgress(rt)) {
        PrepareForIncrementalGC(rt);
        FinishIncrementalGC(rt, gcreason::API);
    }

    gc::FinishBackgroundFinalize(rt);
}

AutoPrepareForTracing::AutoPrepareForTracing(JSRuntime *rt)
  : finish(rt),
    session(rt),
    copy(rt)
{
    RecordNativeStackTopForGC(rt);
}

JSCompartment *
gc::NewCompartment(JSContext *cx, JSPrincipals *principals)
{
    JSRuntime *rt = cx->runtime;
    JS_AbortIfWrongThread(rt);

    JSCompartment *compartment = cx->new_<JSCompartment>(rt);
    if (compartment && compartment->init(cx)) {

        
        JS_SetCompartmentPrincipals(compartment, principals);

        compartment->setGCLastBytes(8192, GC_NORMAL);

        



        {
            AutoLockGC lock(rt);
            if (rt->compartments.append(compartment))
                return compartment;
        }

        js_ReportOutOfMemory(cx);
    }
    js_delete(compartment);
    return NULL;
}

void
gc::RunDebugGC(JSContext *cx)
{
#ifdef JS_GC_ZEAL
    JSRuntime *rt = cx->runtime;

    if (rt->mainThread.suppressGC)
        return;

    PrepareForDebugGC(cx->runtime);

    int type = rt->gcZeal();
    if (type == ZealIncrementalRootsThenFinish ||
        type == ZealIncrementalMarkAllThenFinish ||
        type == ZealIncrementalMultipleSlices)
    {
        js::gc::State initialState = rt->gcIncrementalState;
        int64_t budget;
        if (type == ZealIncrementalMultipleSlices) {
            




            if (initialState == NO_INCREMENTAL)
                rt->gcIncrementalLimit = rt->gcZealFrequency / 2;
            else
                rt->gcIncrementalLimit *= 2;
            budget = SliceBudget::WorkBudget(rt->gcIncrementalLimit);
        } else {
            
            budget = SliceBudget::WorkBudget(1);
        }

        Collect(rt, true, budget, GC_NORMAL, gcreason::DEBUG_GC);

        



        if (type == ZealIncrementalMultipleSlices &&
            initialState == MARK && rt->gcIncrementalState == SWEEP)
        {
            rt->gcIncrementalLimit = rt->gcZealFrequency / 2;
        }
    } else if (type == ZealPurgeAnalysisValue) {
        cx->compartment->types.maybePurgeAnalysis(cx,  true);
    } else {
        Collect(rt, false, SliceBudget::Unlimited, GC_NORMAL, gcreason::DEBUG_GC);
    }

#endif
}

void
gc::SetDeterministicGC(JSContext *cx, bool enabled)
{
#ifdef JS_GC_ZEAL
    JSRuntime *rt = cx->runtime;
    rt->gcDeterministicOnly = enabled;
#endif
}

void
gc::SetValidateGC(JSContext *cx, bool enabled)
{
    JSRuntime *rt = cx->runtime;
    rt->gcValidate = enabled;
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
#ifdef JS_METHODJIT
    for (CompartmentsIter c(fop->runtime()); !c.done(); c.next()) {
        mjit::ClearAllFrames(c);
# ifdef JS_ION
        ion::InvalidateAll(fop, c);
# endif

        for (CellIter i(c, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            mjit::ReleaseScriptCode(fop, script);
# ifdef JS_ION
            ion::FinishInvalidation(fop, script);
# endif
        }
    }
#endif
}

























static void
ReleaseScriptCounts(FreeOp *fop)
{
    JSRuntime *rt = fop->runtime();
    JS_ASSERT(rt->scriptAndCountsVector);

    ScriptAndCountsVector &vec = *rt->scriptAndCountsVector;

    for (size_t i = 0; i < vec.length(); i++)
        vec[i].scriptCounts.destroy(fop);

    fop->delete_(rt->scriptAndCountsVector);
    rt->scriptAndCountsVector = NULL;
}

JS_FRIEND_API(void)
js::StartPCCountProfiling(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    if (rt->profilingScripts)
        return;

    if (rt->scriptAndCountsVector)
        ReleaseScriptCounts(rt->defaultFreeOp());

    ReleaseAllJITCode(rt->defaultFreeOp());

    rt->profilingScripts = true;
}

JS_FRIEND_API(void)
js::StopPCCountProfiling(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    if (!rt->profilingScripts)
        return;
    JS_ASSERT(!rt->scriptAndCountsVector);

    ReleaseAllJITCode(rt->defaultFreeOp());

    ScriptAndCountsVector *vec = cx->new_<ScriptAndCountsVector>(SystemAllocPolicy());
    if (!vec)
        return;

    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        for (CellIter i(c, FINALIZE_SCRIPT); !i.done(); i.next()) {
            RawScript script = i.get<JSScript>();
            if (script->hasScriptCounts && script->types) {
                ScriptAndCounts sac;
                sac.script = script;
                sac.scriptCounts.set(script->releaseScriptCounts());
                if (!vec->append(sac))
                    sac.scriptCounts.destroy(rt->defaultFreeOp());
            }
        }
    }

    rt->profilingScripts = false;
    rt->scriptAndCountsVector = vec;
}

JS_FRIEND_API(void)
js::PurgePCCounts(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    if (!rt->scriptAndCountsVector)
        return;
    JS_ASSERT(!rt->profilingScripts);

    ReleaseScriptCounts(rt->defaultFreeOp());
}

void
js::PurgeJITCaches(JSCompartment *c)
{
#ifdef JS_METHODJIT
    mjit::ClearAllFrames(c);

    for (CellIterUnderGC i(c, FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();

        
        mjit::PurgeCaches(script);

#ifdef JS_ION

        
        ion::PurgeCaches(script, c);

#endif
    }
#endif
}


void
ArenaLists::adoptArenas(JSRuntime *rt, ArenaLists *fromArenaLists)
{
    
    
    
    AutoLockGC lock(rt);

    fromArenaLists->purge();

    for (size_t thingKind = 0; thingKind != FINALIZE_LIMIT; thingKind++) {
#ifdef JS_THREADSAFE
        
        
        
        volatile uintptr_t *bfs = &backgroundFinalizeState[thingKind];
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
#endif 

        ArenaList *fromList = &fromArenaLists->arenaLists[thingKind];
        ArenaList *toList = &arenaLists[thingKind];
        while (fromList->head != NULL) {
            ArenaHeader *fromHeader = fromList->head;
            fromList->head = fromHeader->next;
            fromHeader->next = NULL;

            toList->insert(fromHeader);
        }
    }
}

bool
ArenaLists::containsArena(JSRuntime *rt, ArenaHeader *needle)
{
    AutoLockGC lock(rt);
    size_t allocKind = needle->getAllocKind();
    for (ArenaHeader *aheader = arenaLists[allocKind].head;
         aheader != NULL;
         aheader = aheader->next)
    {
        if (aheader == needle)
            return true;
    }
    return false;
}


AutoMaybeTouchDeadZones::AutoMaybeTouchDeadZones(JSContext *cx)
  : runtime(cx->runtime),
    markCount(runtime->gcObjectsMarkedInDeadZones),
    inIncremental(IsIncrementalGCInProgress(runtime)),
    manipulatingDeadZones(runtime->gcManipulatingDeadZones)
{
    runtime->gcManipulatingDeadZones = true;
}

AutoMaybeTouchDeadZones::AutoMaybeTouchDeadZones(JSObject *obj)
  : runtime(obj->compartment()->rt),
    markCount(runtime->gcObjectsMarkedInDeadZones),
    inIncremental(IsIncrementalGCInProgress(runtime)),
    manipulatingDeadZones(runtime->gcManipulatingDeadZones)
{
    runtime->gcManipulatingDeadZones = true;
}

AutoMaybeTouchDeadZones::~AutoMaybeTouchDeadZones()
{
    if (inIncremental && runtime->gcObjectsMarkedInDeadZones != markCount) {
        PrepareForFullGC(runtime);
        js::GC(runtime, GC_NORMAL, gcreason::TRANSPLANT);
    }

    runtime->gcManipulatingDeadZones = manipulatingDeadZones;
}
