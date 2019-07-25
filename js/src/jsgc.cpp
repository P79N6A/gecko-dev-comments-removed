








#include "mozilla/Attributes.h"
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
#include "jsscope.h"
#include "jsscript.h"
#include "jswatchpoint.h"
#include "jsweakmap.h"
#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#include "builtin/MapObject.h"
#include "frontend/Parser.h"
#include "gc/Marking.h"
#include "gc/Memory.h"
#include "methodjit/MethodJIT.h"
#include "vm/Debugger.h"
#include "vm/String.h"

#include "jsinterpinlines.h"
#include "jsobjinlines.h"

#include "vm/ScopeObject-inl.h"
#include "vm/String-inl.h"

#ifdef MOZ_VALGRIND
# define JS_VALGRIND
#endif
#ifdef JS_VALGRIND
# include <valgrind/memcheck.h>
#endif

#ifdef XP_WIN
# include "jswin.h"
#else
# include <unistd.h>
#endif

using namespace mozilla;
using namespace js;
using namespace js::gc;

namespace js {
namespace gc {




const size_t GC_ALLOCATION_THRESHOLD = 30 * 1024 * 1024;


static const uint64_t GC_IDLE_FULL_SPAN = 20 * 1000 * 1000;


const int IGC_MARK_SLICE_MULTIPLIER = 2;

#ifdef JS_GC_ZEAL
static void
StartVerifyPreBarriers(JSRuntime *rt);

static void
EndVerifyPreBarriers(JSRuntime *rt);

static void
StartVerifyPostBarriers(JSRuntime *rt);

static void
EndVerifyPostBarriers(JSRuntime *rt);

void
FinishVerifier(JSRuntime *rt);
#endif


AllocKind slotsToThingKind[] = {
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
#if JS_HAS_XML_SUPPORT
    sizeof(JSXML),              
#endif
    sizeof(JSShortString),      
    sizeof(JSString),           
    sizeof(JSExternalString),   
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
#if JS_HAS_XML_SUPPORT
    OFFSET(JSXML),              
#endif
    OFFSET(JSShortString),      
    OFFSET(JSString),           
    OFFSET(JSExternalString),   
};

#undef OFFSET





static const AllocKind FinalizePhaseShapes[] = {
    FINALIZE_SHAPE,
    FINALIZE_BASE_SHAPE,
    FINALIZE_TYPE_OBJECT
};

static const AllocKind* FinalizePhases[] = {
    FinalizePhaseShapes
};
static const int FinalizePhaseCount = sizeof(FinalizePhases) / sizeof(AllocKind*);

static const int FinalizePhaseLength[] = {
    sizeof(FinalizePhaseShapes) / sizeof(AllocKind)
};

static const gcstats::Phase FinalizePhaseStatsPhase[] = {
    gcstats::PHASE_SWEEP_SHAPE
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

static const AllocKind* BackgroundPhases[] = {
    BackgroundPhaseObjects,
    BackgroundPhaseStrings
};
static const int BackgroundPhaseCount = sizeof(BackgroundPhases) / sizeof(AllocKind*);

static const int BackgroundPhaseLength[] = {
    sizeof(BackgroundPhaseObjects) / sizeof(AllocKind),
    sizeof(BackgroundPhaseStrings) / sizeof(AllocKind)
};

#ifdef DEBUG
void
ArenaHeader::checkSynchronizedWithFreeList() const
{
    



    JS_ASSERT(allocated());

    




    if (!compartment->rt->isHeapBusy())
        return;

    FreeSpan firstSpan = FreeSpan::decodeOffsets(arenaAddress(), firstFreeSpanOffsets);
    if (firstSpan.isEmpty())
        return;
    const FreeSpan *list = compartment->arenas.getFreeList(getAllocKind());
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
    
    JS_ASSERT(thingSize % Cell::CellSize == 0);
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
inline bool
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
#if JS_HAS_XML_SUPPORT
      case FINALIZE_XML:
	return FinalizeTypedArenas<JSXML>(fop, src, dest, thingKind, budget);
#endif
      case FINALIZE_STRING:
	return FinalizeTypedArenas<JSString>(fop, src, dest, thingKind, budget);
      case FINALIZE_SHORT_STRING:
	return FinalizeTypedArenas<JSShortString>(fop, src, dest, thingKind, budget);
      case FINALIZE_EXTERNAL_STRING:
	return FinalizeTypedArenas<JSExternalString>(fop, src, dest, thingKind, budget);
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
    chunk->init();
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
Chunk::init()
{
    JS_POISON(this, JS_FREE_PATTERN, ChunkSize);

    



    bitmap.clear();

    
    decommittedArenas.clear(false);

    
    info.freeArenasHead = &arenas[0].aheader;
    info.lastDecommittedArenaOffset = 0;
    info.numArenasFree = ArenasPerChunk;
    info.numArenasFreeCommitted = ArenasPerChunk;
    info.age = 0;

    
    for (unsigned i = 0; i < ArenasPerChunk; i++) {
        arenas[i].aheader.setAsNotAllocated();
        arenas[i].aheader.next = (i + 1 < ArenasPerChunk)
                                 ? &arenas[i + 1].aheader
                                 : NULL;
    }

    
}

inline Chunk **
GetAvailableChunkList(JSCompartment *comp)
{
    JSRuntime *rt = comp->rt;
    return comp->isSystemCompartment
           ? &rt->gcSystemAvailableChunkListHead
           : &rt->gcUserAvailableChunkListHead;
}

inline void
Chunk::addToAvailableList(JSCompartment *comp)
{
    insertToAvailableList(GetAvailableChunkList(comp));
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
Chunk::allocateArena(JSCompartment *comp, AllocKind thingKind)
{
    JS_ASSERT(hasAvailableArenas());

    JSRuntime *rt = comp->rt;
    JS_ASSERT(rt->gcBytes <= rt->gcMaxBytes);
    if (rt->gcMaxBytes - rt->gcBytes < ArenaSize)
        return NULL;

    ArenaHeader *aheader = JS_LIKELY(info.numArenasFreeCommitted > 0)
                           ? fetchNextFreeArena(rt)
                           : fetchNextDecommittedArena();
    aheader->init(comp, thingKind);
    if (JS_UNLIKELY(!hasAvailableArenas()))
        removeFromAvailableList();

    Probes::resizeHeap(comp, rt->gcBytes, rt->gcBytes + ArenaSize);
    rt->gcBytes += ArenaSize;
    comp->gcBytes += ArenaSize;
    if (comp->gcBytes >= comp->gcTriggerBytes)
        TriggerCompartmentGC(comp, gcreason::ALLOC_TRIGGER);

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
    JSCompartment *comp = aheader->compartment;
    JSRuntime *rt = comp->rt;
    AutoLockGC maybeLock;
    if (rt->gcHelperThread.sweeping())
        maybeLock.lock(rt);

    Probes::resizeHeap(comp, rt->gcBytes, rt->gcBytes - ArenaSize);
    JS_ASSERT(rt->gcBytes >= ArenaSize);
    JS_ASSERT(comp->gcBytes >= ArenaSize);
    if (rt->gcHelperThread.sweeping())
        comp->reduceGCTriggerBytes(comp->gcHeapGrowthFactor * ArenaSize);
    rt->gcBytes -= ArenaSize;
    comp->gcBytes -= ArenaSize;

    aheader->setAsNotAllocated();
    addArenaToFreeList(rt, aheader);

    if (info.numArenasFree == 1) {
        JS_ASSERT(!info.prevp);
        JS_ASSERT(!info.next);
        addToAvailableList(comp);
    } else if (!unused()) {
        JS_ASSERT(info.prevp);
    } else {
        rt->gcChunkSet.remove(this);
        removeFromAvailableList();
        rt->gcChunkPool.put(this);
    }
}

} 
} 


static Chunk *
PickChunk(JSCompartment *comp)
{
    JSRuntime *rt = comp->rt;
    Chunk **listHeadp = GetAvailableChunkList(comp);
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
    chunk->addToAvailableList(comp);

    return chunk;
}


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
    return true;
}

namespace js {

inline bool
InFreeList(ArenaHeader *aheader, uintptr_t addr)
{
    if (!aheader->hasFreeThings())
        return false;

    FreeSpan firstSpan(aheader->getFirstFreeSpan());

    for (const FreeSpan *span = &firstSpan;;) {
        
        if (addr < span->first)
            return false;

        




        if (addr <= span->last)
            return true;

        



        span = span->nextSpan();
    }
}

#ifdef JSGC_USE_EXACT_ROOTING
static void
MarkExactStackRoots(JSTracer *trc)
{
    for (ContextIter cx(trc->runtime); !cx.done(); cx.next()) {
        for (unsigned i = 0; i < THING_ROOT_LIMIT; i++) {
            Rooted<void*> *rooter = cx->thingGCRooters[i];
            while (rooter) {
                void **addr = (void **)rooter->address();
                if (*addr) {
                    if (i == THING_ROOT_OBJECT) {
                        MarkObjectRoot(trc, (JSObject **)addr, "exact stackroot object");
                    } else if (i == THING_ROOT_STRING) {
                        MarkStringRoot(trc, (JSString **)addr, "exact stackroot string");
                    } else if (i == THING_ROOT_ID) {
                        MarkIdRoot(trc, (jsid *)addr, "exact stackroot id");
                    } else if (i == THING_ROOT_PROPERTY_ID) {
                        MarkIdRoot(trc, &((PropertyId *)addr)->asId(), "exact stackroot property id");
                    } else if (i == THING_ROOT_VALUE) {
                        MarkValueRoot(trc, (Value *)addr, "exact stackroot value");
                    } else if (i == THING_ROOT_TYPE) {
                        MarkTypeRoot(trc, (types::Type *)addr, "exact stackroot type");
                    } else if (i == THING_ROOT_SHAPE) {
                        MarkShapeRoot(trc, (Shape **)addr, "exact stackroot shape");
                    } else if (i == THING_ROOT_BASE_SHAPE) {
                        MarkBaseShapeRoot(trc, (BaseShape **)addr, "exact stackroot baseshape");
                    } else if (i == THING_ROOT_TYPE_OBJECT) {
                        MarkTypeObjectRoot(trc, (types::TypeObject **)addr, "exact stackroot typeobject");
                    } else if (i == THING_ROOT_SCRIPT) {
                        MarkScriptRoot(trc, (JSScript **)addr, "exact stackroot script");
                    } else if (i == THING_ROOT_XML) {
                        MarkXMLRoot(trc, (JSXML **)addr, "exact stackroot xml");
                    } else {
                        JS_NOT_REACHED("Invalid thing root kind.");
                    }
                }
                rooter = rooter->previous();
            }
        }
    }
}
#endif 

enum ConservativeGCTest
{
    CGCT_VALID,
    CGCT_LOWBITSET, 
    CGCT_NOTARENA,  
    CGCT_OTHERCOMPARTMENT,  
    CGCT_NOTCHUNK,  
    CGCT_FREEARENA, 
    CGCT_NOTLIVE,   
    CGCT_END
};





inline ConservativeGCTest
IsAddressableGCThing(JSRuntime *rt, uintptr_t w,
                     bool skipUncollectedCompartments,
                     gc::AllocKind *thingKindPtr,
                     ArenaHeader **arenaHeader,
                     void **thing)
{
    






    JS_STATIC_ASSERT(JSID_TYPE_STRING == 0 && JSID_TYPE_OBJECT == 4);
    if (w & 0x3)
        return CGCT_LOWBITSET;

    



    const uintptr_t JSID_PAYLOAD_MASK = ~uintptr_t(JSID_TYPE_MASK);
#if JS_BITS_PER_WORD == 32
    uintptr_t addr = w & JSID_PAYLOAD_MASK;
#elif JS_BITS_PER_WORD == 64
    uintptr_t addr = w & JSID_PAYLOAD_MASK & JSVAL_PAYLOAD_MASK;
#endif

    Chunk *chunk = Chunk::fromAddress(addr);

    if (!rt->gcChunkSet.has(chunk))
        return CGCT_NOTCHUNK;

    




    if (!Chunk::withinArenasRange(addr))
        return CGCT_NOTARENA;

    
    size_t arenaOffset = Chunk::arenaIndex(addr);
    if (chunk->decommittedArenas.get(arenaOffset))
        return CGCT_FREEARENA;

    ArenaHeader *aheader = &chunk->arenas[arenaOffset].aheader;

    if (!aheader->allocated())
        return CGCT_FREEARENA;

    if (skipUncollectedCompartments && !aheader->compartment->isCollecting())
        return CGCT_OTHERCOMPARTMENT;

    AllocKind thingKind = aheader->getAllocKind();
    uintptr_t offset = addr & ArenaMask;
    uintptr_t minOffset = Arena::firstThingOffset(thingKind);
    if (offset < minOffset)
        return CGCT_NOTARENA;

    
    uintptr_t shift = (offset - minOffset) % Arena::thingSize(thingKind);
    addr -= shift;

    if (thing)
        *thing = reinterpret_cast<void *>(addr);
    if (arenaHeader)
        *arenaHeader = aheader;
    if (thingKindPtr)
        *thingKindPtr = thingKind;
    return CGCT_VALID;
}





inline ConservativeGCTest
MarkIfGCThingWord(JSTracer *trc, uintptr_t w)
{
    void *thing;
    ArenaHeader *aheader;
    AllocKind thingKind;
    ConservativeGCTest status =
        IsAddressableGCThing(trc->runtime, w, IS_GC_MARKING_TRACER(trc),
                             &thingKind, &aheader, &thing);
    if (status != CGCT_VALID)
        return status;

    




    if (InFreeList(aheader, uintptr_t(thing)))
        return CGCT_NOTLIVE;

    JSGCTraceKind traceKind = MapAllocToTraceKind(thingKind);
#ifdef DEBUG
    const char pattern[] = "machine_stack %p";
    char nameBuf[sizeof(pattern) - 2 + sizeof(thing) * 2];
    JS_snprintf(nameBuf, sizeof(nameBuf), pattern, thing);
    JS_SET_TRACING_NAME(trc, nameBuf);
#endif
    JS_SET_TRACING_LOCATION(trc, (void *)w);
    void *tmp = thing;
    MarkKind(trc, &tmp, traceKind);
    JS_ASSERT(tmp == thing);

#ifdef DEBUG
    if (trc->runtime->gcIncrementalState == MARK_ROOTS)
        trc->runtime->gcSavedRoots.append(JSRuntime::SavedGCRoot(thing, traceKind));
#endif

    return CGCT_VALID;
}

static void
MarkWordConservatively(JSTracer *trc, uintptr_t w)
{
    





#ifdef JS_VALGRIND
    JS_SILENCE_UNUSED_VALUE_IN_EXPR(VALGRIND_MAKE_MEM_DEFINED(&w, sizeof(w)));
#endif

    MarkIfGCThingWord(trc, w);
}

MOZ_ASAN_BLACKLIST
static void
MarkRangeConservatively(JSTracer *trc, const uintptr_t *begin, const uintptr_t *end)
{
    JS_ASSERT(begin <= end);
    for (const uintptr_t *i = begin; i < end; ++i)
        MarkWordConservatively(trc, *i);
}

#ifndef JSGC_USE_EXACT_ROOTING

static JS_NEVER_INLINE void
MarkConservativeStackRoots(JSTracer *trc, bool useSavedRoots)
{
    JSRuntime *rt = trc->runtime;

#ifdef DEBUG
    if (useSavedRoots) {
        for (JSRuntime::SavedGCRoot *root = rt->gcSavedRoots.begin();
             root != rt->gcSavedRoots.end();
             root++)
        {
            JS_SET_TRACING_NAME(trc, "cstack");
            MarkKind(trc, &root->thing, root->kind);
        }
        return;
    }

    if (rt->gcIncrementalState == MARK_ROOTS)
        rt->gcSavedRoots.clearAndFree();
#endif

    ConservativeGCData *cgcd = &rt->conservativeGC;
    if (!cgcd->hasStackToScan()) {
#ifdef JS_THREADSAFE
        JS_ASSERT(!rt->suspendCount);
        JS_ASSERT(!rt->requestDepth);
#endif
        return;
    }

    uintptr_t *stackMin, *stackEnd;
#if JS_STACK_GROWTH_DIRECTION > 0
    stackMin = rt->nativeStackBase;
    stackEnd = cgcd->nativeStackTop;
#else
    stackMin = cgcd->nativeStackTop + 1;
    stackEnd = reinterpret_cast<uintptr_t *>(rt->nativeStackBase);
#endif

    JS_ASSERT(stackMin <= stackEnd);
    MarkRangeConservatively(trc, stackMin, stackEnd);
    MarkRangeConservatively(trc, cgcd->registerSnapshot.words,
                            ArrayEnd(cgcd->registerSnapshot.words));
}

#endif 

void
MarkStackRangeConservatively(JSTracer *trc, Value *beginv, Value *endv)
{
    const uintptr_t *begin = beginv->payloadUIntPtr();
    const uintptr_t *end = endv->payloadUIntPtr();
#ifdef JS_NUNBOX32
    



    JS_ASSERT(begin <= end);
    for (const uintptr_t *i = begin; i < end; i += sizeof(Value) / sizeof(uintptr_t))
        MarkWordConservatively(trc, *i);
#else
    MarkRangeConservatively(trc, begin, end);
#endif
}

JS_NEVER_INLINE void
ConservativeGCData::recordStackTop()
{
    
    uintptr_t dummy;
    nativeStackTop = &dummy;

    



#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable: 4611)
#endif
    (void) setjmp(registerSnapshot.jmpbuf);
#if defined(_MSC_VER)
# pragma warning(pop)
#endif
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

} 

bool
js_IsAddressableGCThing(JSRuntime *rt, uintptr_t w, gc::AllocKind *thingKind, void **thing)
{
    return js::IsAddressableGCThing(rt, w, false, thingKind, NULL, thing) == CGCT_VALID;
}

#ifdef DEBUG
static void
CheckLeakedRoots(JSRuntime *rt);
#endif

void
js_FinishGC(JSRuntime *rt)
{
    



    rt->gcHelperThread.finish();

#ifdef JS_GC_ZEAL
    
    FinishVerifier(rt);
#endif

    
    for (CompartmentsIter c(rt); !c.done(); c.next())
        Foreground::delete_(c.get());
    rt->compartments.clear();
    rt->atomsCompartment = NULL;

    rt->gcSystemAvailableChunkListHead = NULL;
    rt->gcUserAvailableChunkListHead = NULL;
    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront())
        Chunk::release(rt, r.front());
    rt->gcChunkSet.clear();

    rt->gcChunkPool.expireAndFree(rt, true);

#ifdef DEBUG
    if (!rt->gcRootsHash.empty())
        CheckLeakedRoots(rt);
#endif
    rt->gcRootsHash.clear();
    rt->gcLocksHash.clear();
}

JSBool
js_AddRoot(JSContext *cx, Value *vp, const char *name)
{
    JSBool ok = js_AddRootRT(cx->runtime, vp, name);
    if (!ok)
        JS_ReportOutOfMemory(cx);
    return ok;
}

JSBool
js_AddGCThingRoot(JSContext *cx, void **rp, const char *name)
{
    JSBool ok = js_AddGCThingRootRT(cx->runtime, rp, name);
    if (!ok)
        JS_ReportOutOfMemory(cx);
    return ok;
}

JS_FRIEND_API(JSBool)
js_AddRootRT(JSRuntime *rt, jsval *vp, const char *name)
{
    





    if (rt->gcIncrementalState == MARK)
        IncrementalValueBarrier(*vp);

    return !!rt->gcRootsHash.put((void *)vp,
                                 RootInfo(name, JS_GC_ROOT_VALUE_PTR));
}

JS_FRIEND_API(JSBool)
js_AddGCThingRootRT(JSRuntime *rt, void **rp, const char *name)
{
    





    if (rt->gcIncrementalState == MARK)
        IncrementalReferenceBarrier(*rp);

    return !!rt->gcRootsHash.put((void *)rp,
                                 RootInfo(name, JS_GC_ROOT_GCTHING_PTR));
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

#ifdef DEBUG

static void
CheckLeakedRoots(JSRuntime *rt)
{
    uint32_t leakedroots = 0;

    
    for (RootRange r = rt->gcRootsHash.all(); !r.empty(); r.popFront()) {
        RootEntry &entry = r.front();
        leakedroots++;
        fprintf(stderr,
                "JS engine warning: leaking GC root \'%s\' at %p\n",
                entry.value.name ? entry.value.name : "", entry.key);
    }

    if (leakedroots > 0) {
        if (leakedroots == 1) {
            fprintf(stderr,
"JS engine warning: 1 GC root remains after destroying the JSRuntime at %p.\n"
"                   This root may point to freed memory. Objects reachable\n"
"                   through it have not been finalized.\n",
                    (void *) rt);
        } else {
            fprintf(stderr,
"JS engine warning: %lu GC roots remain after destroying the JSRuntime at %p.\n"
"                   These roots may point to freed memory. Objects reachable\n"
"                   through them have not been finalized.\n",
                    (unsigned long) leakedroots, (void *) rt);
        }
    }
}

void
js_DumpNamedRoots(JSRuntime *rt,
                  void (*dump)(const char *name, void *rp, JSGCRootType type, void *data),
                  void *data)
{
    for (RootRange r = rt->gcRootsHash.all(); !r.empty(); r.popFront()) {
        RootEntry &entry = r.front();
        if (const char *name = entry.value.name)
            dump(name, entry.key, entry.value.type, data);
    }
}

#endif 

uint32_t
js_MapGCRoots(JSRuntime *rt, JSGCRootMapFun map, void *data)
{
    int ct = 0;
    for (RootEnum e(rt->gcRootsHash); !e.empty(); e.popFront()) {
        RootEntry &entry = e.front();

        ct++;
        int mapflags = map(entry.key, entry.value.type, entry.value.name, data);

        if (mapflags & JS_MAP_GCROOT_REMOVE)
            e.removeFront();
        if (mapflags & JS_MAP_GCROOT_STOP)
            break;
    }

    return ct;
}

static size_t
ComputeTriggerBytes(JSCompartment *comp, size_t lastBytes, size_t maxBytes, JSGCInvocationKind gckind)
{
    size_t base = gckind == GC_SHRINK ? lastBytes : Max(lastBytes, GC_ALLOCATION_THRESHOLD);
    float trigger = float(base) * comp->gcHeapGrowthFactor;
    return size_t(Min(float(maxBytes), trigger));
}

void
JSCompartment::setGCLastBytes(size_t lastBytes, size_t lastMallocBytes, JSGCInvocationKind gckind)
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
    gcTriggerMallocAndFreeBytes = ComputeTriggerBytes(this, lastMallocBytes, SIZE_MAX, gckind);
}

void
JSCompartment::reduceGCTriggerBytes(size_t amount)
{
    JS_ASSERT(amount > 0);
    JS_ASSERT(gcTriggerBytes >= amount);
    if (gcTriggerBytes - amount < GC_ALLOCATION_THRESHOLD * gcHeapGrowthFactor)
        return;
    gcTriggerBytes -= amount;
}

namespace js {
namespace gc {

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

inline void *
ArenaLists::allocateFromArena(JSCompartment *comp, AllocKind thingKind)
{
    Chunk *chunk = NULL;

    ArenaList *al = &arenaLists[thingKind];
    AutoLockGC maybeLock;

#ifdef JS_THREADSAFE
    volatile uintptr_t *bfs = &backgroundFinalizeState[thingKind];
    if (*bfs != BFS_DONE) {
        




        maybeLock.lock(comp->rt);
        if (*bfs == BFS_RUN) {
            JS_ASSERT(!*al->cursor);
            chunk = PickChunk(comp);
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
            if (JS_UNLIKELY(comp->wasGCStarted())) {
                if (comp->needsBarrier()) {
                    aheader->allocatedDuringIncremental = true;
                    comp->rt->gcMarker.delayMarkingArena(aheader);
                } else if (comp->isGCSweeping()) {
                    PushArenaAllocatedDuringSweep(comp->rt, aheader);
                }
            }
            return freeLists[thingKind].infallibleAllocate(Arena::thingSize(thingKind));
        }

        
        if (!maybeLock.locked())
            maybeLock.lock(comp->rt);
        chunk = PickChunk(comp);
        if (!chunk)
            return NULL;
    }

    








    JS_ASSERT(!*al->cursor);
    ArenaHeader *aheader = chunk->allocateArena(comp, thingKind);
    if (!aheader)
        return NULL;

    if (JS_UNLIKELY(comp->wasGCStarted())) {
        if (comp->needsBarrier()) {
            aheader->allocatedDuringIncremental = true;
            comp->rt->gcMarker.delayMarkingArena(aheader);
        } else if (comp->isGCSweeping()) {
            PushArenaAllocatedDuringSweep(comp->rt, aheader);
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
    JS_ASSERT(!fop->onBackgroundThread());
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
    JS_ASSERT(!fop->onBackgroundThread());
    JS_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE);
    JS_ASSERT(!arenaListsToSweep[thingKind]);

    arenaListsToSweep[thingKind] = arenaLists[thingKind].head;
    arenaLists[thingKind].clear();
}

inline void
ArenaLists::queueForBackgroundSweep(FreeOp *fop, AllocKind thingKind)
{
    JS_ASSERT(IsBackgroundFinalized(thingKind));
    JS_ASSERT(!fop->onBackgroundThread());

#ifdef JS_THREADSAFE
    JS_ASSERT(!fop->runtime()->gcHelperThread.sweeping());

    ArenaList *al = &arenaLists[thingKind];
    if (!al->head) {
        JS_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE);
        JS_ASSERT(al->cursor == &al->head);
        return;
    }

    



    JS_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE ||
              backgroundFinalizeState[thingKind] == BFS_JUST_FINISHED);

    if (fop->shouldFreeLater()) {
        arenaListsToSweep[thingKind] = al->head;
        al->clear();
        backgroundFinalizeState[thingKind] = BFS_RUN;
    } else {
        finalizeNow(fop, thingKind);
        backgroundFinalizeState[thingKind] = BFS_DONE;
    }

#else 

    finalizeNow(fop, thingKind);

#endif
}

 void
ArenaLists::backgroundFinalize(FreeOp *fop, ArenaHeader *listHead)
{
#ifdef JS_THREADSAFE
    JS_ASSERT(fop->onBackgroundThread());
#endif 
    JS_ASSERT(listHead);
    AllocKind thingKind = listHead->getAllocKind();
    JSCompartment *comp = listHead->compartment;

    ArenaList finalized;
    SliceBudget budget;
    FinalizeArenas(fop, &listHead, finalized, thingKind, budget);
    JS_ASSERT(!listHead);

    




    ArenaLists *lists = &comp->arenas;
    ArenaList *al = &lists->arenaLists[thingKind];

    AutoLockGC lock(fop->runtime());
    JS_ASSERT(lists->backgroundFinalizeState[thingKind] == BFS_RUN);
    JS_ASSERT(!*al->cursor);

    








    if (finalized.head) {
        *al->cursor = finalized.head;
        if (finalized.cursor != &finalized.head)
            al->cursor = finalized.cursor;
        lists->backgroundFinalizeState[thingKind] = BFS_JUST_FINISHED;
    } else {
        lists->backgroundFinalizeState[thingKind] = BFS_DONE;
    }
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

#if JS_HAS_XML_SUPPORT
    finalizeNow(fop, FINALIZE_XML);
#endif
}

void
ArenaLists::queueStringsForSweep(FreeOp *fop)
{
    gcstats::AutoPhase ap(fop->runtime()->gcStats, gcstats::PHASE_SWEEP_STRING);

    queueForBackgroundSweep(fop, FINALIZE_SHORT_STRING);
    queueForBackgroundSweep(fop, FINALIZE_STRING);

    finalizeNow(fop, FINALIZE_EXTERNAL_STRING);
}

void
ArenaLists::queueScriptsForSweep(FreeOp *fop)
{
    gcstats::AutoPhase ap(fop->runtime()->gcStats, gcstats::PHASE_SWEEP_SCRIPT);
    finalizeNow(fop, FINALIZE_SCRIPT);
}

void
ArenaLists::queueShapesForSweep(FreeOp *fop)
{
    gcstats::AutoPhase ap(fop->runtime()->gcStats, gcstats::PHASE_SWEEP_SHAPE);

    queueForForegroundSweep(fop, FINALIZE_SHAPE);
    queueForForegroundSweep(fop, FINALIZE_BASE_SHAPE);
    queueForForegroundSweep(fop, FINALIZE_TYPE_OBJECT);
}

static void
RunLastDitchGC(JSContext *cx, gcreason::Reason reason)
{
    JSRuntime *rt = cx->runtime;

    
    AutoKeepAtoms keep(rt);
    GC(rt, GC_NORMAL, reason);
}

 void *
ArenaLists::refillFreeList(JSContext *cx, AllocKind thingKind)
{
    JS_ASSERT(cx->compartment->arenas.freeLists[thingKind].isEmpty());

    JSCompartment *comp = cx->compartment;
    JSRuntime *rt = comp->rt;
    JS_ASSERT(!rt->isHeapBusy());

    bool runGC = rt->gcIncrementalState != NO_INCREMENTAL && comp->gcBytes > comp->gcTriggerBytes;
    for (;;) {
        if (JS_UNLIKELY(runGC)) {
            PrepareCompartmentForGC(comp);
            RunLastDitchGC(cx, gcreason::LAST_DITCH);

            




            size_t thingSize = Arena::thingSize(thingKind);
            if (void *thing = comp->arenas.allocateFromFreeList(thingKind, thingSize))
                return thing;
        }

        







        for (bool secondAttempt = false; ; secondAttempt = true) {
            void *thing = comp->arenas.allocateFromArena(comp, thingKind);
            if (JS_LIKELY(!!thing))
                return thing;
            if (secondAttempt)
                break;

            rt->gcHelperThread.waitBackgroundSweepEnd();
        }

        



        if (runGC)
            break;
        runGC = true;
    }

    js_ReportOutOfMemory(cx);
    return NULL;
}

} 
} 

JSGCTraceKind
js_GetGCThingTraceKind(void *thing)
{
    return GetGCThingTraceKind(thing);
}

JSBool
js_LockGCThingRT(JSRuntime *rt, void *thing)
{
    if (!thing)
        return true;

    





    if (rt->gcIncrementalState == MARK)
        IncrementalReferenceBarrier(thing);

    if (GCLocks::Ptr p = rt->gcLocksHash.lookupWithDefault(thing, 0)) {
        p->value++;
        return true;
    }

    return false;
}

void
js_UnlockGCThingRT(JSRuntime *rt, void *thing)
{
    if (!thing)
        return;

    if (GCLocks::Ptr p = rt->gcLocksHash.lookup(thing)) {
        rt->gcPoke = true;
        if (--p->value == 0)
            rt->gcLocksHash.remove(p);
    }
}

namespace js {

void
InitTracer(JSTracer *trc, JSRuntime *rt, JSTraceCallback callback)
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

GCMarker::GCMarker()
  : stack(size_t(-1)),
    color(BLACK),
    started(false),
    unmarkedArenaStackTop(NULL),
    markLaterArenas(0),
    grayFailed(false)
{
}

bool
GCMarker::init()
{
    return stack.init(MARK_STACK_LENGTH);
}

void
GCMarker::start(JSRuntime *rt)
{
    InitTracer(this, rt, NULL);
    JS_ASSERT(!started);
    started = true;
    color = BLACK;

    JS_ASSERT(!unmarkedArenaStackTop);
    JS_ASSERT(markLaterArenas == 0);

    JS_ASSERT(grayRoots.empty());
    JS_ASSERT(!grayFailed);

    



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

    JS_ASSERT(grayRoots.empty());
    grayFailed = false;

    
    stack.reset();
    grayRoots.clearAndFree();
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

    grayRoots.clearAndFree();
    grayFailed = false;
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
    gcstats::AutoPhase ap(runtime->gcStats, gcstats::PHASE_MARK_DELAYED);

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
GCMarker::checkCompartment(void *p)
{
    JS_ASSERT(started);
    JS_ASSERT(static_cast<Cell *>(p)->compartment()->isCollecting());
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
GCMarker::markBufferedGrayRoots()
{
    JS_ASSERT(!grayFailed);

    for (GrayRoot *elem = grayRoots.begin(); elem != grayRoots.end(); elem++) {
#ifdef DEBUG
        debugPrinter = elem->debugPrinter;
        debugPrintArg = elem->debugPrintArg;
        debugPrintIndex = elem->debugPrintIndex;
#endif
        JS_SET_TRACING_LOCATION(this, (void *)&elem->thing);
        void *tmp = elem->thing;
        MarkKind(this, &tmp, elem->kind);
        JS_ASSERT(tmp == elem->thing);
    }

    grayRoots.clearAndFree();
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

    if (!grayRoots.append(root)) {
        grayRoots.clearAndFree();
        grayFailed = true;
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
    return stack.sizeOfExcludingThis(mallocSizeOf) +
           grayRoots.sizeOfExcludingThis(mallocSizeOf);
}

void
SetMarkStackLimit(JSRuntime *rt, size_t limit)
{
    JS_ASSERT(!rt->isHeapBusy());
    rt->gcMarker.setSizeLimit(limit);
}

} 

static void
gc_root_traversal(JSTracer *trc, const RootEntry &entry)
{
    const char *name = entry.value.name ? entry.value.name : "root";
    if (entry.value.type == JS_GC_ROOT_GCTHING_PTR)
        MarkGCThingRoot(trc, reinterpret_cast<void **>(entry.key), name);
    else
        MarkValueRoot(trc, reinterpret_cast<Value *>(entry.key), name);
}

static void
gc_lock_traversal(const GCLocks::Entry &entry, JSTracer *trc)
{
    JS_ASSERT(entry.value >= 1);
    JS_SET_TRACING_LOCATION(trc, (void *)&entry.key);
    void *tmp = entry.key;
    MarkGCThingRoot(trc, &tmp, "locked object");
    JS_ASSERT(tmp == entry.key);
}

namespace js {

void
MarkCompartmentActive(StackFrame *fp)
{
    if (fp->isScriptFrame())
        fp->script()->compartment()->active = true;
}

} 

void
AutoIdArray::trace(JSTracer *trc)
{
    JS_ASSERT(tag == IDARRAY);
    gc::MarkIdRange(trc, idArray->length, idArray->vector, "JSAutoIdArray.idArray");
}

void
AutoEnumStateRooter::trace(JSTracer *trc)
{
    gc::MarkObjectRoot(trc, &obj, "JS::AutoEnumStateRooter.obj");
}

inline void
AutoGCRooter::trace(JSTracer *trc)
{
    switch (tag) {
      case JSVAL:
        MarkValueRoot(trc, &static_cast<AutoValueRooter *>(this)->val, "JS::AutoValueRooter.val");
        return;

      case PARSER:
        static_cast<frontend::Parser *>(this)->trace(trc);
        return;

      case ENUMERATOR:
        static_cast<AutoEnumStateRooter *>(this)->trace(trc);
        return;

      case IDARRAY: {
        JSIdArray *ida = static_cast<AutoIdArray *>(this)->idArray;
        MarkIdRange(trc, ida->length, ida->vector, "JS::AutoIdArray.idArray");
        return;
      }

      case DESCRIPTORS: {
        PropDescArray &descriptors =
            static_cast<AutoPropDescArrayRooter *>(this)->descriptors;
        for (size_t i = 0, len = descriptors.length(); i < len; i++) {
            PropDesc &desc = descriptors[i];
            MarkValueRoot(trc, &desc.pd_, "PropDesc::pd_");
            MarkValueRoot(trc, &desc.value_, "PropDesc::value_");
            MarkValueRoot(trc, &desc.get_, "PropDesc::get_");
            MarkValueRoot(trc, &desc.set_, "PropDesc::set_");
        }
        return;
      }

      case DESCRIPTOR : {
        PropertyDescriptor &desc = *static_cast<AutoPropertyDescriptorRooter *>(this);
        if (desc.obj)
            MarkObjectRoot(trc, &desc.obj, "Descriptor::obj");
        MarkValueRoot(trc, &desc.value, "Descriptor::value");
        if ((desc.attrs & JSPROP_GETTER) && desc.getter) {
            JSObject *tmp = JS_FUNC_TO_DATA_PTR(JSObject *, desc.getter);
            MarkObjectRoot(trc, &tmp, "Descriptor::get");
            desc.getter = JS_DATA_TO_FUNC_PTR(JSPropertyOp, tmp);
        }
        if (desc.attrs & JSPROP_SETTER && desc.setter) {
            JSObject *tmp = JS_FUNC_TO_DATA_PTR(JSObject *, desc.setter);
            MarkObjectRoot(trc, &tmp, "Descriptor::set");
            desc.setter = JS_DATA_TO_FUNC_PTR(JSStrictPropertyOp, tmp);
        }
        return;
      }

#if JS_HAS_XML_SUPPORT
      case NAMESPACES: {
        JSXMLArray<JSObject> &array = static_cast<AutoNamespaceArray *>(this)->array;
        MarkObjectRange(trc, array.length, array.vector, "JSXMLArray.vector");
        js_XMLArrayCursorTrace(trc, array.cursors);
        return;
      }

      case XML:
        js_TraceXML(trc, static_cast<AutoXMLRooter *>(this)->xml);
        return;
#endif

      case OBJECT:
        if (static_cast<AutoObjectRooter *>(this)->obj)
            MarkObjectRoot(trc, &static_cast<AutoObjectRooter *>(this)->obj,
                           "JS::AutoObjectRooter.obj");
        return;

      case ID:
        MarkIdRoot(trc, &static_cast<AutoIdRooter *>(this)->id_, "JS::AutoIdRooter.id_");
        return;

      case VALVECTOR: {
        AutoValueVector::VectorImpl &vector = static_cast<AutoValueVector *>(this)->vector;
        MarkValueRootRange(trc, vector.length(), vector.begin(), "js::AutoValueVector.vector");
        return;
      }

      case STRING:
        if (static_cast<AutoStringRooter *>(this)->str)
            MarkStringRoot(trc, &static_cast<AutoStringRooter *>(this)->str,
                           "JS::AutoStringRooter.str");
        return;

      case IDVECTOR: {
        AutoIdVector::VectorImpl &vector = static_cast<AutoIdVector *>(this)->vector;
        MarkIdRootRange(trc, vector.length(), vector.begin(), "js::AutoIdVector.vector");
        return;
      }

      case SHAPEVECTOR: {
        AutoShapeVector::VectorImpl &vector = static_cast<js::AutoShapeVector *>(this)->vector;
        MarkShapeRootRange(trc, vector.length(), const_cast<Shape **>(vector.begin()),
                           "js::AutoShapeVector.vector");
        return;
      }

      case OBJVECTOR: {
        AutoObjectVector::VectorImpl &vector = static_cast<AutoObjectVector *>(this)->vector;
        MarkObjectRootRange(trc, vector.length(), vector.begin(), "js::AutoObjectVector.vector");
        return;
      }

      case STRINGVECTOR: {
        AutoStringVector::VectorImpl &vector = static_cast<AutoStringVector *>(this)->vector;
        MarkStringRootRange(trc, vector.length(), vector.begin(), "js::AutoStringVector.vector");
        return;
      }

      case NAMEVECTOR: {
        AutoNameVector::VectorImpl &vector = static_cast<AutoNameVector *>(this)->vector;
        MarkStringRootRange(trc, vector.length(), vector.begin(), "js::AutoNameVector.vector");
        return;
      }

      case VALARRAY: {
        AutoValueArray *array = static_cast<AutoValueArray *>(this);
        MarkValueRootRange(trc, array->length(), array->start(), "js::AutoValueArray");
        return;
      }

      case SCRIPTVECTOR: {
        AutoScriptVector::VectorImpl &vector = static_cast<AutoScriptVector *>(this)->vector;
        for (size_t i = 0; i < vector.length(); i++)
            MarkScriptRoot(trc, &vector[i], "AutoScriptVector element");
        return;
      }

      case PROPDESC: {
        PropDesc::AutoRooter *rooter = static_cast<PropDesc::AutoRooter *>(this);
        MarkValueRoot(trc, &rooter->pd->pd_, "PropDesc::AutoRooter pd");
        MarkValueRoot(trc, &rooter->pd->value_, "PropDesc::AutoRooter value");
        MarkValueRoot(trc, &rooter->pd->get_, "PropDesc::AutoRooter get");
        MarkValueRoot(trc, &rooter->pd->set_, "PropDesc::AutoRooter set");
        return;
      }

      case SHAPERANGE: {
        Shape::Range::AutoRooter *rooter = static_cast<Shape::Range::AutoRooter *>(this);
        rooter->trace(trc);
        return;
      }

      case STACKSHAPE: {
        StackShape::AutoRooter *rooter = static_cast<StackShape::AutoRooter *>(this);
        if (rooter->shape->base)
            MarkBaseShapeRoot(trc, (BaseShape**) &rooter->shape->base, "StackShape::AutoRooter base");
        MarkIdRoot(trc, (jsid*) &rooter->shape->propid, "StackShape::AutoRooter id");
        return;
      }

      case STACKBASESHAPE: {
        StackBaseShape::AutoRooter *rooter = static_cast<StackBaseShape::AutoRooter *>(this);
        if (rooter->base->parent)
            MarkObjectRoot(trc, (JSObject**) &rooter->base->parent, "StackBaseShape::AutoRooter parent");
        if ((rooter->base->flags & BaseShape::HAS_GETTER_OBJECT) && rooter->base->rawGetter) {
            MarkObjectRoot(trc, (JSObject**) &rooter->base->rawGetter,
                           "StackBaseShape::AutoRooter getter");
        }
        if ((rooter->base->flags & BaseShape::HAS_SETTER_OBJECT) && rooter->base->rawSetter) {
            MarkObjectRoot(trc, (JSObject**) &rooter->base->rawSetter,
                           "StackBaseShape::AutoRooter setter");
        }
        return;
      }

      case BINDINGS: {
        Bindings::AutoRooter *rooter = static_cast<Bindings::AutoRooter *>(this);
        rooter->trace(trc);
        return;
      }

      case GETTERSETTER: {
        AutoRooterGetterSetter::Inner *rooter = static_cast<AutoRooterGetterSetter::Inner *>(this);
        if ((rooter->attrs & JSPROP_GETTER) && *rooter->pgetter)
            MarkObjectRoot(trc, (JSObject**) rooter->pgetter, "AutoRooterGetterSetter getter");
        if ((rooter->attrs & JSPROP_SETTER) && *rooter->psetter)
            MarkObjectRoot(trc, (JSObject**) rooter->psetter, "AutoRooterGetterSetter setter");
        return;
      }

      case REGEXPSTATICS: {
          



        return;
      }

      case HASHABLEVALUE: {
          



        return;
      }
    }

    JS_ASSERT(tag >= 0);
    MarkValueRootRange(trc, tag, static_cast<AutoArrayRooter *>(this)->array,
                       "JS::AutoArrayRooter.array");
}

 void
AutoGCRooter::traceAll(JSTracer *trc)
{
    for (js::AutoGCRooter *gcr = trc->runtime->autoGCRooters; gcr; gcr = gcr->down)
        gcr->trace(trc);
}

void
Shape::Range::AutoRooter::trace(JSTracer *trc)
{
    if (r->cursor)
        MarkShapeRoot(trc, const_cast<Shape**>(&r->cursor), "Shape::Range::AutoRooter");
}

void
Bindings::AutoRooter::trace(JSTracer *trc)
{
    bindings->trace(trc);
}

void
RegExpStatics::AutoRooter::trace(JSTracer *trc)
{
    if (statics->matchPairsInput)
        MarkStringRoot(trc, reinterpret_cast<JSString**>(&statics->matchPairsInput),
                       "RegExpStatics::AutoRooter matchPairsInput");
    if (statics->pendingInput)
        MarkStringRoot(trc, reinterpret_cast<JSString**>(&statics->pendingInput),
                       "RegExpStatics::AutoRooter pendingInput");
}

void
HashableValue::AutoRooter::trace(JSTracer *trc)
{
    MarkValueRoot(trc, reinterpret_cast<Value*>(&v->value), "HashableValue::AutoRooter");
}

namespace js {

static void
MarkRuntime(JSTracer *trc, bool useSavedRoots = false)
{
    JSRuntime *rt = trc->runtime;
    JS_ASSERT(trc->callback != GCMarker::GrayCallback);

    if (IS_GC_MARKING_TRACER(trc)) {
        for (CompartmentsIter c(rt); !c.done(); c.next()) {
            if (!c->isCollecting())
                c->markCrossCompartmentWrappers(trc);
        }
        Debugger::markCrossCompartmentDebuggerObjectReferents(trc);
    }

    AutoGCRooter::traceAll(trc);

    if (rt->hasContexts()) {
#ifdef JSGC_USE_EXACT_ROOTING
        MarkExactStackRoots(trc);
#else
        MarkConservativeStackRoots(trc, useSavedRoots);
#endif
        rt->markSelfHostedGlobal(trc);
    }

    for (RootRange r = rt->gcRootsHash.all(); !r.empty(); r.popFront())
        gc_root_traversal(trc, r.front());

    for (GCLocks::Range r = rt->gcLocksHash.all(); !r.empty(); r.popFront())
        gc_lock_traversal(r.front(), trc);

    if (rt->scriptAndCountsVector) {
        ScriptAndCountsVector &vec = *rt->scriptAndCountsVector;
        for (size_t i = 0; i < vec.length(); i++)
            MarkScriptRoot(trc, &vec[i].script, "scriptAndCountsVector");
    }

    if (!IS_GC_MARKING_TRACER(trc) || rt->atomsCompartment->isCollecting())
        MarkAtomState(trc);
    rt->staticStrings.trace(trc);

    for (ContextIter acx(rt); !acx.done(); acx.next())
        acx->mark(trc);

    
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        if (IS_GC_MARKING_TRACER(trc) && !c->isCollecting())
            continue;

        if ((c->activeAnalysis || c->isPreservingCode()) && IS_GC_MARKING_TRACER(trc)) {
            gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_MARK_TYPES);
            c->markTypes(trc);
        }

        
        if (!IS_GC_MARKING_TRACER(trc)) {
            if (c->watchpointMap)
                c->watchpointMap->markAll(trc);
        }

        
        if (rt->profilingScripts) {
            for (CellIterUnderGC i(c, FINALIZE_SCRIPT); !i.done(); i.next()) {
                JSScript *script = i.get<JSScript>();
                if (script->hasScriptCounts) {
                    MarkScriptRoot(trc, &script, "profilingScripts");
                    JS_ASSERT(script == i.get<JSScript>());
                }
            }
        }
    }

#ifdef JS_METHODJIT
    
    for (CompartmentsIter c(rt); !c.done(); c.next())
        mjit::ExpandInlineFrames(c);
#endif

    rt->stackSpace.markAndClobber(trc);
    rt->debugScopes->mark(trc);

    
    if (JSTraceDataOp op = rt->gcBlackRootsTraceOp)
        (*op)(trc, rt->gcBlackRootsData);

    
    if (JSTraceDataOp op = rt->gcGrayRootsTraceOp) {
        if (IS_GC_MARKING_TRACER(trc)) {
            GCMarker *gcmarker = static_cast<GCMarker *>(trc);
            gcmarker->startBufferingGrayRoots();
            (*op)(trc, rt->gcGrayRootsData);
            gcmarker->endBufferingGrayRoots();
        } else {
            (*op)(trc, rt->gcGrayRootsData);
        }
    }
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
TriggerGC(JSRuntime *rt, gcreason::Reason reason)
{
    JS_ASSERT(rt->onOwnerThread());

    if (rt->isHeapBusy())
        return;

    PrepareForFullGC(rt);
    TriggerOperationCallback(rt, reason);
}

void
TriggerCompartmentGC(JSCompartment *comp, gcreason::Reason reason)
{
    JSRuntime *rt = comp->rt;
    JS_ASSERT(rt->onOwnerThread());

    if (rt->isHeapBusy())
        return;

    if (rt->gcZeal() == ZealAllocValue) {
        TriggerGC(rt, reason);
        return;
    }

    if (comp == rt->atomsCompartment) {
        
        TriggerGC(rt, reason);
        return;
    }

    PrepareCompartmentForGC(comp);
    TriggerOperationCallback(rt, reason);
}

void
MaybeGC(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    JS_ASSERT(rt->onOwnerThread());

    if (rt->gcZeal() == ZealAllocValue || rt->gcZeal() == ZealPokeValue) {
        PrepareForFullGC(rt);
        GC(rt, GC_NORMAL, gcreason::MAYBEGC);
        return;
    }

    if (rt->gcIsNeeded) {
        GCSlice(rt, GC_NORMAL, gcreason::MAYBEGC);
        return;
    }

    double factor = rt->gcHighFrequencyGC ? 0.75 : 0.9;
    JSCompartment *comp = cx->compartment;
    if (comp->gcBytes > 1024 * 1024 &&
        comp->gcBytes >= factor * comp->gcTriggerBytes &&
        rt->gcIncrementalState == NO_INCREMENTAL &&
        !rt->gcHelperThread.sweeping())
    {
        PrepareCompartmentForGC(comp);
        GCSlice(rt, GC_NORMAL, gcreason::MAYBEGC);
        return;
    }

    if (comp->gcMallocAndFreeBytes > comp->gcTriggerMallocAndFreeBytes) {
        PrepareCompartmentForGC(comp);
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
AssertBackgroundSweepingFinshed(JSRuntime *rt)
{
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        JS_ASSERT(!c->gcNextCompartment);
        for (unsigned i = 0 ; i < FINALIZE_LIMIT ; ++i)
            JS_ASSERT(!c->arenas.arenaListsToSweep[i]);
    }
}

#ifdef JS_THREADSAFE
static unsigned
GetCPUCount()
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
#else
    backgroundAllocation = false;
#endif 
    return true;
}

void
GCHelperThread::finish()
{
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
#else
    


    JS_ASSERT(state == IDLE);
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
#ifdef JS_THREADSAFE
    AutoLockGC lock(rt);
    JS_ASSERT(state == IDLE);
    JS_ASSERT(!sweepFlag);
    sweepFlag = true;
    shrinkFlag = shouldShrink;
    state = SWEEPING;
    PR_NotifyCondVar(wakeup);
#else
    JS_NOT_REACHED("No background sweep if !JS_THREADSAFE");
#endif 
}

#ifdef JS_THREADSAFE

void
GCHelperThread::startBackgroundShrink()
{
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
}
#endif 

void
GCHelperThread::waitBackgroundSweepEnd()
{
#ifdef JS_THREADSAFE
    AutoLockGC lock(rt);
    while (state == SWEEPING)
        PR_WaitCondVar(done, PR_INTERVAL_NO_TIMEOUT);
    if (rt->gcIncrementalState == NO_INCREMENTAL)
        AssertBackgroundSweepingFinshed(rt);
#else
    JS_ASSERT(state == IDLE);
#endif 
}

void
GCHelperThread::waitBackgroundSweepOrAllocEnd()
{
#ifdef JS_THREADSAFE
    AutoLockGC lock(rt);
    if (state == ALLOCATING)
        state = CANCEL_ALLOCATION;
    while (state == SWEEPING || state == CANCEL_ALLOCATION)
        PR_WaitCondVar(done, PR_INTERVAL_NO_TIMEOUT);
    if (rt->gcIncrementalState == NO_INCREMENTAL)
        AssertBackgroundSweepingFinshed(rt);
#else
    JS_ASSERT(state == IDLE);
#endif 
}


inline void
GCHelperThread::startBackgroundAllocationIfIdle()
{
#ifdef JS_THREADSAFE
    if (state == IDLE) {
        state = ALLOCATING;
        PR_NotifyCondVar(wakeup);
    }
#else
    JS_ASSERT(state == IDLE);
#endif 
}

JS_FRIEND_API(void)
GCHelperThread::replenishAndFreeLater(void *ptr)
{
    JS_ASSERT(freeCursor == freeCursorEnd);
    do {
        if (freeCursor && !freeVector.append(freeCursorEnd - FREE_ARRAY_LENGTH))
            break;
        freeCursor = (void **) OffTheBooks::malloc_(FREE_ARRAY_SIZE);
        if (!freeCursor) {
            freeCursorEnd = NULL;
            break;
        }
        freeCursorEnd = freeCursor + FREE_ARRAY_LENGTH;
        *freeCursor++ = ptr;
        return;
    } while (false);
    Foreground::free_(ptr);
}

#ifdef JS_THREADSAFE

void
GCHelperThread::doSweep()
{
    if (sweepFlag) {
        sweepFlag = false;
        AutoUnlockGC unlock(rt);

        



        FreeOp fop(rt, false, true);
        for (int phase = 0; phase < BackgroundPhaseCount; ++phase) {
            for (JSCompartment *c = rt->gcSweepingCompartments; c; c = c->gcNextCompartment) {
                for (int index = 0; index < BackgroundPhaseLength[phase]; ++index) {
                    AllocKind kind = BackgroundPhases[phase][index];
                    ArenaHeader *arenas = c->arenas.arenaListsToSweep[kind];
                    if (arenas) {
                        ArenaLists::backgroundFinalize(&fop, arenas);
                    }
                }
            }
        }

        while (JSCompartment *c = rt->gcSweepingCompartments) {
            rt->gcSweepingCompartments = c->gcNextCompartment;
            c->gcNextCompartment = NULL;
        }

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

} 

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
SweepCompartments(FreeOp *fop, gcreason::Reason gcReason)
{
    JSRuntime *rt = fop->runtime();
    JS_ASSERT_IF(gcReason == gcreason::LAST_CONTEXT, !rt->hasContexts());

    JSDestroyCompartmentCallback callback = rt->destroyCompartmentCallback;

    
    JSCompartment **read = rt->compartments.begin() + 1;
    JSCompartment **end = rt->compartments.end();
    JSCompartment **write = read;
    JS_ASSERT(rt->compartments.length() >= 1);
    JS_ASSERT(*rt->compartments.begin() == rt->atomsCompartment);

    while (read < end) {
        JSCompartment *compartment = *read++;

        if (!compartment->hold && compartment->isCollecting() &&
            (compartment->arenas.arenaListsAreEmpty() || gcReason == gcreason::LAST_CONTEXT))
        {
            compartment->arenas.checkEmptyFreeLists();
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
    for (GCCompartmentsIter c(rt); !c.done(); c.next())
        c->purge();

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
ShouldPreserveJITCode(JSCompartment *c, int64_t currentTime)
{
    if (c->rt->gcShouldCleanUpEverything || !c->types.inferenceEnabled)
        return false;

    if (c->rt->alwaysPreserveCode)
        return true;
    if (c->lastAnimationTime + PRMJ_USEC_PER_SEC >= currentTime &&
        c->lastCodeRelease + (PRMJ_USEC_PER_SEC * 300) >= currentTime) {
        return true;
    }

    c->lastCodeRelease = currentTime;
    return false;
}

static void
BeginMarkPhase(JSRuntime *rt)
{
    int64_t currentTime = PRMJ_Now();

    rt->gcIsFull = true;
    DebugOnly<bool> any = false;
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        
        JS_ASSERT(!c->isCollecting());
        for (unsigned i = 0; i < FINALIZE_LIMIT; ++i)
            JS_ASSERT(!c->arenas.arenaListsToSweep[i]);

        
        if (c->isGCScheduled()) {
            any = true;
            if (c.get() != rt->atomsCompartment)
                c->setCollecting(true);
        } else {
            rt->gcIsFull = false;
        }

        c->setPreservingCode(ShouldPreserveJITCode(c, currentTime));
    }

    
    JS_ASSERT(any);

    





    if (rt->atomsCompartment->isGCScheduled() && rt->gcIsFull && !rt->gcKeepAtoms)
        rt->atomsCompartment->setCollecting(true);

    






    if (rt->gcIsIncremental) {
        for (GCCompartmentsIter c(rt); !c.done(); c.next())
            c->arenas.purge();
    }

    rt->gcMarker.start(rt);
    JS_ASSERT(!rt->gcMarker.callback);
    JS_ASSERT(IS_GC_MARKING_TRACER(&rt->gcMarker));

    
    if (rt->gcIsIncremental) {
        for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
            gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_MARK_DISCARD_CODE);
            c->discardJitCode(rt->defaultFreeOp());
        }
    }

    GCMarker *gcmarker = &rt->gcMarker;

    rt->gcStartNumber = rt->gcNumber;

    
    WeakMapBase::resetWeakMapList(rt);

    








    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_PURGE);
        PurgeRuntime(rt);
    }

    


    gcstats::AutoPhase ap1(rt->gcStats, gcstats::PHASE_MARK);
    gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_MARK_ROOTS);

    
    for (GCCompartmentsIter c(rt); !c.done(); c.next())
        c->arenas.unmarkAll();

    MarkRuntime(gcmarker);
}

void
MarkWeakReferences(GCMarker *gcmarker)
{
    JS_ASSERT(gcmarker->isDrained());
    while (WatchpointMap::markAllIteratively(gcmarker) ||
           WeakMapBase::markAllIteratively(gcmarker) ||
           Debugger::markAllIteratively(gcmarker))
    {
        SliceBudget budget;
        gcmarker->drainMarkStack(budget);
    }
    JS_ASSERT(gcmarker->isDrained());
}

static void
MarkGrayAndWeak(JSRuntime *rt)
{
    GCMarker *gcmarker = &rt->gcMarker;

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_MARK_WEAK);
        JS_ASSERT(gcmarker->isDrained());
        MarkWeakReferences(gcmarker);
    }

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_MARK_GRAY);
        gcmarker->setMarkColorGray();
        if (gcmarker->hasBufferedGrayRoots()) {
            gcmarker->markBufferedGrayRoots();
        } else {
            if (JSTraceDataOp op = rt->gcGrayRootsTraceOp)
                (*op)(gcmarker, rt->gcGrayRootsData);
        }
        SliceBudget budget;
        gcmarker->drainMarkStack(budget);
    }

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_MARK_GRAY_WEAK);
        MarkWeakReferences(gcmarker);
    }

    JS_ASSERT(gcmarker->isDrained());
}

#ifdef DEBUG
static void
ValidateIncrementalMarking(JSRuntime *rt);
#endif

static void
EndMarkPhase(JSRuntime *rt)
{
    {
        gcstats::AutoPhase ap1(rt->gcStats, gcstats::PHASE_MARK);
        MarkGrayAndWeak(rt);
    }

    JS_ASSERT(rt->gcMarker.isDrained());

#ifdef DEBUG
    if (rt->gcIsIncremental && rt->gcValidate)
        ValidateIncrementalMarking(rt);
#endif

    






    bool foundBlackGray = false;
    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        for (WrapperMap::Enum e(c->crossCompartmentWrappers); !e.empty(); e.popFront()) {
            Cell *dst = e.front().key.wrapped;
            Cell *src = ToMarkable(e.front().value);
            JS_ASSERT(src->compartment() == c.get());
            if (IsCellMarked(&src) && !src->isMarked(GRAY) && dst->isMarked(GRAY)) {
                JS_ASSERT(!dst->compartment()->isCollecting());
                foundBlackGray = true;
            }
        }
    }

    




    if (foundBlackGray) {
        for (CompartmentsIter c(rt); !c.done(); c.next()) {
            if (!c->isCollecting())
                c->arenas.unmarkAll();
        }
    }

    rt->gcMarker.stop();

    
}

#ifdef DEBUG
static void
ValidateIncrementalMarking(JSRuntime *rt)
{
    typedef HashMap<Chunk *, uintptr_t *, GCChunkHasher, SystemAllocPolicy> BitmapMap;
    BitmapMap map;
    if (!map.init())
        return;

    GCMarker *gcmarker = &rt->gcMarker;

    
    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront()) {
        ChunkBitmap *bitmap = &r.front()->bitmap;
        uintptr_t *entry = (uintptr_t *)js_malloc(sizeof(bitmap->bitmap));
        if (!entry)
            return;

        memcpy(entry, bitmap->bitmap, sizeof(bitmap->bitmap));
        if (!map.putNew(r.front(), entry))
            return;
    }

    
    WeakMapVector weakmaps;
    if (!WeakMapBase::saveWeakMapList(rt, weakmaps))
        return;

    




    
    js::gc::State state = rt->gcIncrementalState;
    rt->gcIncrementalState = MARK_ROOTS;

    
    WeakMapBase::resetWeakMapList(rt);

    JS_ASSERT(gcmarker->isDrained());
    gcmarker->reset();

    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront())
        r.front()->bitmap.clear();

    MarkRuntime(gcmarker, true);

    SliceBudget budget;
    rt->gcIncrementalState = MARK;
    rt->gcMarker.drainMarkStack(budget);
    MarkGrayAndWeak(rt);

    
    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront()) {
        Chunk *chunk = r.front();
        ChunkBitmap *bitmap = &chunk->bitmap;
        uintptr_t *entry = map.lookup(r.front())->value;
        ChunkBitmap incBitmap;

        memcpy(incBitmap.bitmap, entry, sizeof(incBitmap.bitmap));
        js_free(entry);

        for (size_t i = 0; i < ArenasPerChunk; i++) {
            if (chunk->decommittedArenas.get(i))
                continue;
            Arena *arena = &chunk->arenas[i];
            if (!arena->aheader.allocated())
                continue;
            if (!arena->aheader.compartment->isCollecting())
                continue;
            if (arena->aheader.allocatedDuringIncremental)
                continue;

            AllocKind kind = arena->aheader.getAllocKind();
            uintptr_t thing = arena->thingsStart(kind);
            uintptr_t end = arena->thingsEnd();
            while (thing < end) {
                Cell *cell = (Cell *)thing;
                JS_ASSERT_IF(bitmap->isMarked(cell, BLACK), incBitmap.isMarked(cell, BLACK));
                thing += Arena::thingSize(kind);
            }
        }

        memcpy(bitmap->bitmap, incBitmap.bitmap, sizeof(incBitmap.bitmap));
    }

    
    WeakMapBase::resetWeakMapList(rt);
    WeakMapBase::restoreWeakMapList(rt, weakmaps);

    rt->gcIncrementalState = state;
}
#endif












class PartitionCompartments
{
    typedef unsigned Node;
    typedef Vector<Node, 0, SystemAllocPolicy> NodeVector;
    typedef Vector<bool, 0, SystemAllocPolicy> BoolVector;

    static const Node Undefined = Node(-1);

    JSRuntime *runtime;

    




    Node clock, nextSCC;

    










    NodeVector discoveryTime, lowLink, stack, scc;
    BoolVector onStack;

    bool fail_;

    void processNode(Node v);
    void fail() { fail_ = true; }
    bool failed() { return fail_; }

  public:
    PartitionCompartments(JSRuntime *rt);
    void partition();
    unsigned getSCC(JSCompartment *comp) { return failed() ? 0 : scc[comp->index]; }
};

const PartitionCompartments::Node PartitionCompartments::Undefined;

PartitionCompartments::PartitionCompartments(JSRuntime *rt)
  : runtime(rt), clock(0), nextSCC(0), fail_(false)
{
    size_t n = runtime->compartments.length();
    if (!discoveryTime.reserve(n) ||
        !lowLink.reserve(n) ||
        !scc.reserve(n) ||
        !onStack.reserve(n) ||
        !stack.reserve(n))
    {
        fail();
        return;
    }

    for (Node v = 0; v < runtime->compartments.length(); v++) {
        runtime->compartments[v]->index = v;
        discoveryTime.infallibleAppend(Undefined);
        lowLink.infallibleAppend(Undefined);
        scc.infallibleAppend(Undefined);
        onStack.infallibleAppend(false);
    }
}


void
PartitionCompartments::processNode(Node v)
{
    int stackDummy;
    if (failed() || !JS_CHECK_STACK_SIZE(js::GetNativeStackLimit(runtime), &stackDummy)) {
        fail();
        return;
    }

    discoveryTime[v] = clock;
    lowLink[v] = clock;
    clock++;
    stack.infallibleAppend(v);
    onStack[v] = true;

    JSCompartment *comp = runtime->compartments[v];

    for (WrapperMap::Enum e(comp->crossCompartmentWrappers); !e.empty(); e.popFront()) {
        if (e.front().key.kind == CrossCompartmentKey::StringWrapper)
            continue;

        Cell *other = e.front().key.wrapped;
        if (other->isMarked(BLACK) && !other->isMarked(GRAY))
            continue;

        Node w = other->compartment()->index;

        if (discoveryTime[w] == Undefined) {
            processNode(w);
            lowLink[v] = Min(lowLink[v], lowLink[w]);
        } else if (onStack[w]) {
            lowLink[v] = Min(lowLink[v], discoveryTime[w]);
        }
    }

    if (lowLink[v] == discoveryTime[v]) {
        Node w;
        do {
            w = stack.popCopy();
            onStack[w] = false;
            scc[w] = nextSCC;
        } while (w != v);
        nextSCC++;
    }
}

void
PartitionCompartments::partition()
{
    for (Node n = 0; n < runtime->compartments.length(); n++) {
        if (discoveryTime[n] == Undefined)
            processNode(n);
    }
}

static void
BeginSweepPhase(JSRuntime *rt)
{
    













    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP);

    



    bool isFull = true;
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        if (!c->isCollecting())
            isFull = false;
    }
    JS_ASSERT_IF(isFull, rt->gcIsFull);

#ifdef JS_THREADSAFE
    rt->gcSweepOnBackgroundThread = rt->hasContexts();
#endif

    
    for (GCCompartmentsIter c(rt); !c.done(); c.next())
        c->arenas.purge();

    FreeOp fop(rt, rt->gcSweepOnBackgroundThread, false);

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_FINALIZE_START);
        if (rt->gcFinalizeCallback)
            rt->gcFinalizeCallback(&fop, JSFINALIZE_START, !isFull);
    }

    
    WeakMapBase::sweepAll(&rt->gcMarker);
    rt->debugScopes->sweep();

    if (rt->atomsCompartment->wasGCStarted()) {
        gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_SWEEP_ATOMS);
        SweepAtomState(rt);
    }

    
    WatchpointMap::sweepAll(rt);

    
    Debugger::sweepAll(&fop);

    PartitionCompartments partition(rt);
    partition.partition();

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_COMPARTMENTS);

        




        rt->stackSpace.markAndClobber(NULL);

        bool releaseTypes = ReleaseObservedTypes(rt);
        for (CompartmentsIter c(rt); !c.done(); c.next()) {
            gcstats::AutoSCC scc(rt->gcStats, partition.getSCC(c));
            if (c->isCollecting())
                c->sweep(&fop, releaseTypes);
            else
                c->sweepCrossCompartmentWrappers();
        }
    }

    







    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        gcstats::AutoSCC scc(rt->gcStats, partition.getSCC(c));
        c->arenas.queueObjectsForSweep(&fop);
    }
    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        gcstats::AutoSCC scc(rt->gcStats, partition.getSCC(c));
        c->arenas.queueStringsForSweep(&fop);
    }
    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        gcstats::AutoSCC scc(rt->gcStats, partition.getSCC(c));
        c->arenas.queueScriptsForSweep(&fop);
    }
    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        gcstats::AutoSCC scc(rt->gcStats, partition.getSCC(c));
        c->arenas.queueShapesForSweep(&fop);
    }

    rt->gcSweepPhase = 0;
    rt->gcSweepCompartmentIndex = 0;
    rt->gcSweepKindIndex = 0;

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_FINALIZE_END);
        if (rt->gcFinalizeCallback)
            rt->gcFinalizeCallback(&fop, JSFINALIZE_END, !rt->gcIsFull);
    }
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
SweepPhase(JSRuntime *rt, SliceBudget &sliceBudget)
{
    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP);
    FreeOp fop(rt, rt->gcSweepOnBackgroundThread, false);

    for (; rt->gcSweepPhase < FinalizePhaseCount ; ++rt->gcSweepPhase) {
        gcstats::AutoPhase ap(rt->gcStats, FinalizePhaseStatsPhase[rt->gcSweepPhase]);

        ptrdiff_t len = rt->compartments.end() - rt->compartments.begin();
        for (; rt->gcSweepCompartmentIndex < len ; ++rt->gcSweepCompartmentIndex) {
            JSCompartment *c = rt->compartments.begin()[rt->gcSweepCompartmentIndex];

            if (c->wasGCStarted()) {
                while (rt->gcSweepKindIndex < FinalizePhaseLength[rt->gcSweepPhase]) {
                    AllocKind kind = FinalizePhases[rt->gcSweepPhase][rt->gcSweepKindIndex];

                    if (!c->arenas.foregroundFinalize(&fop, kind, sliceBudget))
                        return false;
                    ++rt->gcSweepKindIndex;
                }
            }
            rt->gcSweepKindIndex = 0;
        }
        rt->gcSweepCompartmentIndex = 0;
    }

    return true;
}

static void
EndSweepPhase(JSRuntime *rt, JSGCInvocationKind gckind, gcreason::Reason gcReason)
{
    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP);
    FreeOp fop(rt, rt->gcSweepOnBackgroundThread, false);

#ifdef DEBUG
    PropertyTree::dumpShapes(rt);
#endif

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_DESTROY);

        if (!rt->gcSweepOnBackgroundThread)
            rt->freeLifoAlloc.freeAll();

        





        if (rt->gcIsFull)
            SweepScriptFilenames(rt);

        



        SweepCompartments(&fop, gcReason);

#ifndef JS_THREADSAFE
        




        ExpireChunksAndArenas(rt, gckind == GC_SHRINK);
#endif
    }

    


    while (ArenaHeader *arena = rt->gcArenasAllocatedDuringSweep) {
        rt->gcArenasAllocatedDuringSweep = arena->getNextAllocDuringSweep();
        arena->unsetAllocDuringSweep();
    }

    


    JS_ASSERT(!rt->gcSweepingCompartments);
    if (rt->gcSweepOnBackgroundThread) {
        JSCompartment **cursor = &rt->gcSweepingCompartments;
        for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
            JS_ASSERT(!c->gcNextCompartment);
            *cursor = c.get();
            cursor = &c->gcNextCompartment;
        }
    }

    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        c->setGCLastBytes(c->gcBytes, c->gcMallocAndFreeBytes, gckind);
        if (c->wasGCStarted())
            c->setCollecting(false);

        JS_ASSERT(!c->isCollecting());
        JS_ASSERT(!c->wasGCStarted());
        for (unsigned i = 0 ; i < FINALIZE_LIMIT ; ++i) {
            JS_ASSERT_IF(!IsBackgroundFinalized(AllocKind(i)) ||
                         !rt->gcSweepOnBackgroundThread,
                         !c->arenas.arenaListsToSweep[i]);
        }
    }

    rt->gcLastGCTime = PRMJ_Now();
}





class AutoTraceSession {
  public:
    AutoTraceSession(JSRuntime *rt, JSRuntime::HeapState state = JSRuntime::Tracing);
    ~AutoTraceSession();

  protected:
    JSRuntime *runtime;

  private:
    AutoTraceSession(const AutoTraceSession&) MOZ_DELETE;
    void operator=(const AutoTraceSession&) MOZ_DELETE;
};


class AutoGCSession : AutoTraceSession {
  public:
    explicit AutoGCSession(JSRuntime *rt);
    ~AutoGCSession();
};


AutoTraceSession::AutoTraceSession(JSRuntime *rt, JSRuntime::HeapState heapState)
  : runtime(rt)
{
    JS_ASSERT(!rt->noGCOrAllocationCheck);
    JS_ASSERT(!rt->isHeapBusy());
    JS_ASSERT(heapState == JSRuntime::Collecting || heapState == JSRuntime::Tracing);
    rt->heapState = heapState;
}

AutoTraceSession::~AutoTraceSession()
{
    JS_ASSERT(runtime->isHeapBusy());
    runtime->heapState = JSRuntime::Idle;
}

AutoGCSession::AutoGCSession(JSRuntime *rt)
  : AutoTraceSession(rt, JSRuntime::Collecting)
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

    
    for (CompartmentsIter c(runtime); !c.done(); c.next()) {
        c->resetGCMallocBytes();
        c->unscheduleGC();
    }

    runtime->resetGCMallocBytes();
}

static void
IncrementalCollectSlice(JSRuntime *rt,
                        int64_t budget,
                        gcreason::Reason gcReason,
                        JSGCInvocationKind gcKind);

static void
ResetIncrementalGC(JSRuntime *rt, const char *reason)
{
    if (rt->gcIncrementalState == NO_INCREMENTAL)
        return;

    if (rt->gcIncrementalState == SWEEP) {
        
        IncrementalCollectSlice(rt, SliceBudget::Unlimited, gcreason::RESET, GC_NORMAL);
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_WAIT_BACKGROUND_THREAD);
        rt->gcHelperThread.waitBackgroundSweepOrAllocEnd();
        return;
    }

    JS_ASSERT(rt->gcIncrementalState == MARK);

    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        c->setNeedsBarrier(false);
        c->setCollecting(false);
        JS_ASSERT(!c->gcNextCompartment);
        for (unsigned i = 0 ; i < FINALIZE_LIMIT ; ++i)
            JS_ASSERT(!c->arenas.arenaListsToSweep[i]);
    }

    rt->gcMarker.reset();
    rt->gcMarker.stop();

    rt->gcIncrementalState = NO_INCREMENTAL;

    JS_ASSERT(!rt->gcStrictCompartmentChecking);

    rt->gcStats.reset(reason);
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

    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        
        if (rt->gcIncrementalState == MARK) {
            JS_ASSERT(c->needsBarrier());
            c->setNeedsBarrier(false);
        } else {
            JS_ASSERT(!c->needsBarrier());
        }
    }
}

AutoGCSlice::~AutoGCSlice()
{
    for (GCCompartmentsIter c(runtime); !c.done(); c.next()) {
        if (runtime->gcIncrementalState == MARK) {
            c->setNeedsBarrier(true);
            c->arenas.prepareForIncrementalGC(runtime);
        } else {
            JS_ASSERT(runtime->gcIncrementalState == NO_INCREMENTAL ||
                      runtime->gcIncrementalState == SWEEP);
            c->setNeedsBarrier(false);
        }
    }
}

class AutoCopyFreeListToArenas {
    JSRuntime *rt;

  public:
    AutoCopyFreeListToArenas(JSRuntime *rt)
      : rt(rt) {
        for (CompartmentsIter c(rt); !c.done(); c.next())
            c->arenas.copyFreeListsToArenas();
    }

    ~AutoCopyFreeListToArenas() {
        for (CompartmentsIter c(rt); !c.done(); c.next())
            c->arenas.clearFreeListsInArenas();
    }
};

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

static bool
DrainMarkStack(JSRuntime *rt, SliceBudget &sliceBudget)
{
    
    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_MARK);
    return rt->gcMarker.drainMarkStack(sliceBudget);
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
    SliceBudget sliceBudget(budget);

    int zeal = 0;
#ifdef JS_GC_ZEAL
    if (reason == gcreason::DEBUG_GC && budget != SliceBudget::Unlimited) {
        




        zeal = rt->gcZeal();
    }
#endif

    rt->gcIsIncremental = rt->gcIncrementalState != NO_INCREMENTAL ||
                          budget != SliceBudget::Unlimited;

    if (zeal == ZealIncrementalRootsThenFinish || zeal == ZealIncrementalMarkAllThenFinish) {
        



        sliceBudget.reset();
    }

    if (rt->gcIncrementalState == NO_INCREMENTAL) {
        rt->gcIncrementalState = MARK_ROOTS;
        rt->gcLastMarkSlice = false;
    }

    switch (rt->gcIncrementalState) {

      case MARK_ROOTS:
        BeginMarkPhase(rt);
        PushZealSelectedObjects(rt);

        rt->gcIncrementalState = MARK;

        if (zeal == ZealIncrementalRootsThenFinish)
            break;

        

      case MARK: {

        
        if (!rt->gcMarker.hasBufferedGrayRoots())
            sliceBudget.reset();

        bool finished = DrainMarkStack(rt, sliceBudget);
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

        EndMarkPhase(rt);

        rt->gcIncrementalState = SWEEP;

        



        BeginSweepPhase(rt);
        if (sliceBudget.isOverBudget())
            break;

        



        if (budget != SliceBudget::Unlimited && zeal == ZealIncrementalMultipleSlices)
            break;

        
      }

      case SWEEP: {
#ifdef DEBUG
        for (CompartmentsIter c(rt); !c.done(); c.next())
            JS_ASSERT(!c->needsBarrier());
#endif

        bool finished = SweepPhase(rt, sliceBudget);
        if (!finished)
            break;

        EndSweepPhase(rt, gckind, reason);

        if (rt->gcSweepOnBackgroundThread)
            rt->gcHelperThread.startBackgroundSweep(gckind == GC_SHRINK);

        rt->gcIncrementalState = NO_INCREMENTAL;
        break;
      }

      default:
        JS_ASSERT(false);
     }
}

class IncrementalSafety
{
    const char *reason_;

    IncrementalSafety(const char *reason) : reason_(reason) {}

  public:
    static IncrementalSafety Safe() { return IncrementalSafety(NULL); }
    static IncrementalSafety Unsafe(const char *reason) { return IncrementalSafety(reason); }

    typedef void (IncrementalSafety::* ConvertibleToBool)();
    void nonNull() {}

    operator ConvertibleToBool() const {
        return reason_ == NULL ? &IncrementalSafety::nonNull : 0;
    }

    const char *reason() {
        JS_ASSERT(reason_);
        return reason_;
    }
};

static IncrementalSafety
IsIncrementalGCSafe(JSRuntime *rt)
{
    if (rt->gcKeepAtoms)
        return IncrementalSafety::Unsafe("gcKeepAtoms set");

    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        if (c->activeAnalysis)
            return IncrementalSafety::Unsafe("activeAnalysis set");
    }

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
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        if (c->gcBytes >= c->gcTriggerBytes) {
            *budget = SliceBudget::Unlimited;
            rt->gcStats.nonincremental("allocation trigger");
        }

        if (c->isTooMuchMalloc()) {
            *budget = SliceBudget::Unlimited;
            rt->gcStats.nonincremental("malloc bytes trigger");
        }

        if (rt->gcIncrementalState != NO_INCREMENTAL &&
            c->isGCScheduled() != c->wasGCStarted()) {
            reset = true;
        }
    }

    if (reset)
        ResetIncrementalGC(rt, "compartment change");
}







static JS_NEVER_INLINE void
GCCycle(JSRuntime *rt, bool incremental, int64_t budget, JSGCInvocationKind gckind, gcreason::Reason reason)
{
#ifdef DEBUG
    for (CompartmentsIter c(rt); !c.done(); c.next())
        JS_ASSERT_IF(rt->gcMode == JSGC_MODE_GLOBAL, c->isGCScheduled());
#endif

    
    if (rt->isHeapBusy())
        return;

    
    if (rt->inOOMReport)
        return;

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
    if (reason > gcreason::DEBUG_GC && reason != gcreason::CC_FORCED)
        return false;

    if (reason == gcreason::MAYBEGC)
        return false;

    return true;
}
#endif

static bool
ShouldCleanUpEverything(JSRuntime *rt, gcreason::Reason reason)
{
    
    
    
    
    
    
    
    return !rt->hasContexts() ||
           reason == gcreason::SHUTDOWN_CC ||
           reason == gcreason::DEBUG_MODE_GC;
}

static void
Collect(JSRuntime *rt, bool incremental, int64_t budget,
        JSGCInvocationKind gckind, gcreason::Reason reason)
{
    JS_AbortIfWrongThread(rt);

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

    int compartmentCount = 0;
    int collectedCount = 0;
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        if (rt->gcMode == JSGC_MODE_GLOBAL)
            c->scheduleGC();

        
        if (rt->gcIncrementalState != NO_INCREMENTAL && c->needsBarrier())
            c->scheduleGC();

        compartmentCount++;
        if (c->isGCScheduled())
            collectedCount++;
    }

    rt->gcShouldCleanUpEverything = ShouldCleanUpEverything(rt, reason);

    gcstats::AutoGCSlice agc(rt->gcStats, collectedCount, compartmentCount, reason);

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

namespace js {

void
GC(JSRuntime *rt, JSGCInvocationKind gckind, gcreason::Reason reason)
{
    Collect(rt, false, SliceBudget::Unlimited, gckind, reason);
}

void
GCSlice(JSRuntime *rt, JSGCInvocationKind gckind, gcreason::Reason reason, int64_t millis)
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
GCFinalSlice(JSRuntime *rt, JSGCInvocationKind gckind, gcreason::Reason reason)
{
    Collect(rt, true, SliceBudget::Unlimited, gckind, reason);
}

void
GCDebugSlice(JSRuntime *rt, bool limit, int64_t objCount)
{
    int64_t budget = limit ? SliceBudget::WorkBudget(objCount) : SliceBudget::Unlimited;
    PrepareForDebugGC(rt);
    Collect(rt, true, budget, GC_NORMAL, gcreason::API);
}


void
PrepareForDebugGC(JSRuntime *rt)
{
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        if (c->isGCScheduled())
            return;
    }

    PrepareForFullGC(rt);
}

void
ShrinkGCBuffers(JSRuntime *rt)
{
    AutoLockGC lock(rt);
    JS_ASSERT(!rt->isHeapBusy());
#ifndef JS_THREADSAFE
    ExpireChunksAndArenas(rt, true);
#else
    rt->gcHelperThread.startBackgroundShrink();
#endif
}

void
TraceRuntime(JSTracer *trc)
{
    JS_ASSERT(!IS_GC_MARKING_TRACER(trc));

#ifdef JS_THREADSAFE
    {
        JSRuntime *rt = trc->runtime;
        if (!rt->isHeapBusy()) {
            AutoTraceSession session(rt);

            rt->gcHelperThread.waitBackgroundSweepEnd();

            AutoCopyFreeListToArenas copy(rt);
            RecordNativeStackTopForGC(rt);
            MarkRuntime(trc);
            return;
        }
    }
#else
    AutoCopyFreeListToArenas copy(trc->runtime);
    RecordNativeStackTopForGC(trc->runtime);
#endif

    



    MarkRuntime(trc);
}

struct IterateArenaCallbackOp
{
    JSRuntime *rt;
    void *data;
    IterateArenaCallback callback;
    JSGCTraceKind traceKind;
    size_t thingSize;
    IterateArenaCallbackOp(JSRuntime *rt, void *data, IterateArenaCallback callback,
                           JSGCTraceKind traceKind, size_t thingSize)
        : rt(rt), data(data), callback(callback), traceKind(traceKind), thingSize(thingSize)
    {}
    void operator()(Arena *arena) { (*callback)(rt, data, arena, traceKind, thingSize); }
};

struct IterateCellCallbackOp
{
    JSRuntime *rt;
    void *data;
    IterateCellCallback callback;
    JSGCTraceKind traceKind;
    size_t thingSize;
    IterateCellCallbackOp(JSRuntime *rt, void *data, IterateCellCallback callback,
                          JSGCTraceKind traceKind, size_t thingSize)
        : rt(rt), data(data), callback(callback), traceKind(traceKind), thingSize(thingSize)
    {}
    void operator()(Cell *cell) { (*callback)(rt, data, cell, traceKind, thingSize); }
};

void
IterateCompartmentsArenasCells(JSRuntime *rt, void *data,
                               JSIterateCompartmentCallback compartmentCallback,
                               IterateArenaCallback arenaCallback,
                               IterateCellCallback cellCallback)
{
    JS_ASSERT(!rt->isHeapBusy());

    AutoTraceSession session(rt);
    rt->gcHelperThread.waitBackgroundSweepEnd();

    AutoCopyFreeListToArenas copy(rt);
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        (*compartmentCallback)(rt, data, c);

        for (size_t thingKind = 0; thingKind != FINALIZE_LIMIT; thingKind++) {
            JSGCTraceKind traceKind = MapAllocToTraceKind(AllocKind(thingKind));
            size_t thingSize = Arena::thingSize(AllocKind(thingKind));
            IterateArenaCallbackOp arenaOp(rt, data, arenaCallback, traceKind, thingSize);
            IterateCellCallbackOp cellOp(rt, data, cellCallback, traceKind, thingSize);
            ForEachArenaAndCell(c, AllocKind(thingKind), arenaOp, cellOp);
        }
    }
}

void
IterateChunks(JSRuntime *rt, void *data, IterateChunkCallback chunkCallback)
{
    
    JS_ASSERT(!rt->isHeapBusy());

    AutoTraceSession session(rt);
    rt->gcHelperThread.waitBackgroundSweepEnd();

    for (js::GCChunkSet::Range r = rt->gcChunkSet.all(); !r.empty(); r.popFront())
        chunkCallback(rt, data, r.front());
}

void
IterateCells(JSRuntime *rt, JSCompartment *compartment, AllocKind thingKind,
             void *data, IterateCellCallback cellCallback)
{
    
    JS_ASSERT(!rt->isHeapBusy());

    AutoTraceSession session(rt);
    rt->gcHelperThread.waitBackgroundSweepEnd();

    AutoCopyFreeListToArenas copy(rt);

    JSGCTraceKind traceKind = MapAllocToTraceKind(thingKind);
    size_t thingSize = Arena::thingSize(thingKind);

    if (compartment) {
        for (CellIterUnderGC i(compartment, thingKind); !i.done(); i.next())
            cellCallback(rt, data, i.getCell(), traceKind, thingSize);
    } else {
        for (CompartmentsIter c(rt); !c.done(); c.next()) {
            for (CellIterUnderGC i(c, thingKind); !i.done(); i.next())
                cellCallback(rt, data, i.getCell(), traceKind, thingSize);
        }
    }
}

void
IterateGrayObjects(JSCompartment *compartment, GCThingCallback *cellCallback, void *data)
{
    JS_ASSERT(compartment);
    JSRuntime *rt = compartment->rt;
    JS_ASSERT(!rt->isHeapBusy());

    AutoTraceSession session(rt);
    rt->gcHelperThread.waitBackgroundSweepEnd();

    AutoCopyFreeListToArenas copy(rt);

    for (size_t finalizeKind = 0; finalizeKind <= FINALIZE_OBJECT_LAST; finalizeKind++) {
        for (CellIterUnderGC i(compartment, AllocKind(finalizeKind)); !i.done(); i.next()) {
            Cell *cell = i.getCell();
            if (cell->isMarked(GRAY))
                cellCallback(data, cell);
        }
    }
}

namespace gc {

JSCompartment *
NewCompartment(JSContext *cx, JSPrincipals *principals)
{
    JSRuntime *rt = cx->runtime;
    JS_AbortIfWrongThread(rt);

    JSCompartment *compartment = cx->new_<JSCompartment>(rt);
    if (compartment && compartment->init(cx)) {

        
        JS_SetCompartmentPrincipals(compartment, principals);

        compartment->setGCLastBytes(8192, 8192, GC_NORMAL);

        



        {
            AutoLockGC lock(rt);
            if (rt->compartments.append(compartment))
                return compartment;
        }

        js_ReportOutOfMemory(cx);
    }
    Foreground::delete_(compartment);
    return NULL;
}

void
RunDebugGC(JSContext *cx)
{
#ifdef JS_GC_ZEAL
    JSRuntime *rt = cx->runtime;
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
    } else {
        Collect(rt, false, SliceBudget::Unlimited, GC_NORMAL, gcreason::DEBUG_GC);
    }

#endif
}

void
SetDeterministicGC(JSContext *cx, bool enabled)
{
#ifdef JS_GC_ZEAL
    JSRuntime *rt = cx->runtime;
    rt->gcDeterministicOnly = enabled;
#endif
}

void
SetValidateGC(JSContext *cx, bool enabled)
{
    JSRuntime *rt = cx->runtime;
    rt->gcValidate = enabled;
}

} 
} 

#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)

static void
CheckStackRoot(JSTracer *trc, uintptr_t *w)
{
    
#ifdef JS_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED(&w, sizeof(w));
#endif

    ConservativeGCTest test = MarkIfGCThingWord(trc, *w);

    if (test == CGCT_VALID) {
        bool matched = false;
        JSRuntime *rt = trc->runtime;
        for (ContextIter cx(rt); !cx.done(); cx.next()) {
            for (unsigned i = 0; i < THING_ROOT_LIMIT; i++) {
                Rooted<void*> *rooter = cx->thingGCRooters[i];
                while (rooter) {
                    if (rooter->address() == static_cast<void*>(w))
                        matched = true;
                    rooter = rooter->previous();
                }
            }
            SkipRoot *skip = cx->skipGCRooters;
            while (skip) {
                if (skip->contains(reinterpret_cast<uint8_t*>(w), sizeof(w)))
                    matched = true;
                skip = skip->previous();
            }
        }
        if (!matched) {
            





            PoisonPtr(w);
        }
    }
}

static void
CheckStackRootsRange(JSTracer *trc, uintptr_t *begin, uintptr_t *end)
{
    JS_ASSERT(begin <= end);
    for (uintptr_t *i = begin; i != end; ++i)
        CheckStackRoot(trc, i);
}

static void
EmptyMarkCallback(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{}

void
JS::CheckStackRoots(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    if (rt->gcZeal_ != ZealStackRootingSafeValue && rt->gcZeal_ != ZealStackRootingValue)
        return;
    if (rt->gcZeal_ == ZealStackRootingSafeValue && !rt->gcExactScanningEnabled)
        return;

    
    
    
    
    
    
    
    
    
    JS_ASSERT(!cx->rootingUnnecessary);

    
    if (cx->compartment->activeAnalysis)
        return;

    
    if (IsAtomsCompartment(cx->compartment)) {
        for (CompartmentsIter c(rt); !c.done(); c.next()) {
            if (c.get()->activeAnalysis)
                return;
        }
    }

    AutoCopyFreeListToArenas copy(rt);

    JSTracer checker;
    JS_TracerInit(&checker, rt, EmptyMarkCallback);

    ConservativeGCData *cgcd = &rt->conservativeGC;
    cgcd->recordStackTop();

    JS_ASSERT(cgcd->hasStackToScan());
    uintptr_t *stackMin, *stackEnd;
#if JS_STACK_GROWTH_DIRECTION > 0
    stackMin = rt->nativeStackBase;
    stackEnd = cgcd->nativeStackTop;
#else
    stackMin = cgcd->nativeStackTop + 1;
    stackEnd = reinterpret_cast<uintptr_t *>(rt->nativeStackBase);

    uintptr_t *&oldStackMin = cgcd->oldStackMin, *&oldStackEnd = cgcd->oldStackEnd;
    uintptr_t *&oldStackData = cgcd->oldStackData;
    uintptr_t &oldStackCapacity = cgcd->oldStackCapacity;

    



    if (stackEnd != oldStackEnd) {
        rt->free_(oldStackData);
        oldStackCapacity = rt->nativeStackQuota / sizeof(uintptr_t);
        oldStackData = (uintptr_t *) rt->malloc_(oldStackCapacity * sizeof(uintptr_t));
        if (!oldStackData) {
            oldStackCapacity = 0;
        } else {
            uintptr_t *existing = stackEnd - 1, *copy = oldStackData;
            while (existing >= stackMin && size_t(copy - oldStackData) < oldStackCapacity)
                *copy++ = *existing--;
            oldStackEnd = stackEnd;
            oldStackMin = existing + 1;
        }
    } else {
        uintptr_t *existing = stackEnd - 1, *copy = oldStackData;
        while (existing >= stackMin && existing >= oldStackMin && *existing == *copy) {
            copy++;
            existing--;
        }
        stackEnd = existing + 1;
        while (existing >= stackMin && size_t(copy - oldStackData) < oldStackCapacity)
            *copy++ = *existing--;
        oldStackMin = existing + 1;
    }
#endif

    JS_ASSERT(stackMin <= stackEnd);
    CheckStackRootsRange(&checker, stackMin, stackEnd);
    CheckStackRootsRange(&checker, cgcd->registerSnapshot.words,
                         ArrayEnd(cgcd->registerSnapshot.words));
}

#endif 

namespace js {
namespace gc {

#ifdef JS_GC_ZEAL






























struct EdgeValue
{
    void *thing;
    JSGCTraceKind kind;
    char *label;
};

struct VerifyNode
{
    void *thing;
    JSGCTraceKind kind;
    uint32_t count;
    EdgeValue edges[1];
};

typedef HashMap<void *, VerifyNode *, DefaultHasher<void *>, SystemAllocPolicy> NodeMap;














struct VerifyPreTracer : JSTracer {
    
    uint64_t number;

    
    int count;

    
    VerifyNode *curnode;
    VerifyNode *root;
    char *edgeptr;
    char *term;
    NodeMap nodemap;

    VerifyPreTracer() : root(NULL) {}
    ~VerifyPreTracer() { js_free(root); }
};





static void
AccumulateEdge(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    VerifyPreTracer *trc = (VerifyPreTracer *)jstrc;

    trc->edgeptr += sizeof(EdgeValue);
    if (trc->edgeptr >= trc->term) {
        trc->edgeptr = trc->term;
        return;
    }

    VerifyNode *node = trc->curnode;
    uint32_t i = node->count;

    node->edges[i].thing = *thingp;
    node->edges[i].kind = kind;
    node->edges[i].label = trc->debugPrinter ? NULL : (char *)trc->debugPrintArg;
    node->count++;
}

static VerifyNode *
MakeNode(VerifyPreTracer *trc, void *thing, JSGCTraceKind kind)
{
    NodeMap::AddPtr p = trc->nodemap.lookupForAdd(thing);
    if (!p) {
        VerifyNode *node = (VerifyNode *)trc->edgeptr;
        trc->edgeptr += sizeof(VerifyNode) - sizeof(EdgeValue);
        if (trc->edgeptr >= trc->term) {
            trc->edgeptr = trc->term;
            return NULL;
        }

        node->thing = thing;
        node->count = 0;
        node->kind = kind;
        trc->nodemap.add(p, thing, node);
        return node;
    }
    return NULL;
}

static
VerifyNode *
NextNode(VerifyNode *node)
{
    if (node->count == 0)
        return (VerifyNode *)((char *)node + sizeof(VerifyNode) - sizeof(EdgeValue));
    else
        return (VerifyNode *)((char *)node + sizeof(VerifyNode) +
			      sizeof(EdgeValue)*(node->count - 1));
}

static void
StartVerifyPreBarriers(JSRuntime *rt)
{
    if (rt->gcVerifyPreData || rt->gcIncrementalState != NO_INCREMENTAL)
        return;

    AutoTraceSession session(rt);

    if (!IsIncrementalGCSafe(rt))
        return;

    rt->gcHelperThread.waitBackgroundSweepOrAllocEnd();

    AutoCopyFreeListToArenas copy(rt);
    RecordNativeStackTopForGC(rt);

    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront())
        r.front()->bitmap.clear();

    VerifyPreTracer *trc = new (js_malloc(sizeof(VerifyPreTracer))) VerifyPreTracer;

    rt->gcNumber++;
    trc->number = rt->gcNumber;
    trc->count = 0;

    JS_TracerInit(trc, rt, AccumulateEdge);

    const size_t size = 64 * 1024 * 1024;
    trc->root = (VerifyNode *)js_malloc(size);
    JS_ASSERT(trc->root);
    trc->edgeptr = (char *)trc->root;
    trc->term = trc->edgeptr + size;

    if (!trc->nodemap.init())
        return;

    
    trc->curnode = MakeNode(trc, NULL, JSGCTraceKind(0));

    
    rt->gcIncrementalState = MARK_ROOTS;

    
    MarkRuntime(trc);

    VerifyNode *node = trc->curnode;
    if (trc->edgeptr == trc->term)
        goto oom;

    
    while ((char *)node < trc->edgeptr) {
        for (uint32_t i = 0; i < node->count; i++) {
            EdgeValue &e = node->edges[i];
            VerifyNode *child = MakeNode(trc, e.thing, e.kind);
            if (child) {
                trc->curnode = child;
                JS_TraceChildren(trc, e.thing, e.kind);
            }
            if (trc->edgeptr == trc->term)
                goto oom;
        }

        node = NextNode(node);
    }

    rt->gcVerifyPreData = trc;
    rt->gcIncrementalState = MARK;
    rt->gcMarker.start(rt);
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        c->setNeedsBarrier(true);
        c->arenas.purge();
    }

    return;

oom:
    rt->gcIncrementalState = NO_INCREMENTAL;
    trc->~VerifyPreTracer();
    js_free(trc);
}

static bool
IsMarkedOrAllocated(Cell *cell)
{
    return cell->isMarked() || cell->arenaHeader()->allocatedDuringIncremental;
}

const static uint32_t MAX_VERIFIER_EDGES = 1000;








static void
CheckEdge(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    VerifyPreTracer *trc = (VerifyPreTracer *)jstrc;
    VerifyNode *node = trc->curnode;

    
    if (node->count > MAX_VERIFIER_EDGES)
        return;

    for (uint32_t i = 0; i < node->count; i++) {
        if (node->edges[i].thing == *thingp) {
            JS_ASSERT(node->edges[i].kind == kind);
            node->edges[i].thing = NULL;
            return;
        }
    }
}

static void
AssertMarkedOrAllocated(const EdgeValue &edge)
{
    if (!edge.thing || IsMarkedOrAllocated(static_cast<Cell *>(edge.thing)))
        return;

    char msgbuf[1024];
    const char *label = edge.label ? edge.label : "<unknown>";

    JS_snprintf(msgbuf, sizeof(msgbuf), "[barrier verifier] Unmarked edge: %s", label);
    MOZ_ReportAssertionFailure(msgbuf, __FILE__, __LINE__);
    MOZ_CRASH();
}

static void
EndVerifyPreBarriers(JSRuntime *rt)
{
    AutoTraceSession session(rt);

    rt->gcHelperThread.waitBackgroundSweepOrAllocEnd();

    AutoCopyFreeListToArenas copy(rt);
    RecordNativeStackTopForGC(rt);

    VerifyPreTracer *trc = (VerifyPreTracer *)rt->gcVerifyPreData;

    if (!trc)
        return;

    bool compartmentCreated = false;

    
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        if (!c->needsBarrier())
            compartmentCreated = true;

        c->setNeedsBarrier(false);
    }

    



    JS_ASSERT(trc->number == rt->gcNumber);
    rt->gcNumber++;

    rt->gcVerifyPreData = NULL;
    rt->gcIncrementalState = NO_INCREMENTAL;

    if (!compartmentCreated && IsIncrementalGCSafe(rt)) {
        JS_TracerInit(trc, rt, CheckEdge);

        
        VerifyNode *node = NextNode(trc->root);
        while ((char *)node < trc->edgeptr) {
            trc->curnode = node;
            JS_TraceChildren(trc, node->thing, node->kind);

            if (node->count <= MAX_VERIFIER_EDGES) {
                for (uint32_t i = 0; i < node->count; i++)
                    AssertMarkedOrAllocated(node->edges[i]);
            }

            node = NextNode(node);
        }
    }

    rt->gcMarker.reset();
    rt->gcMarker.stop();

    trc->~VerifyPreTracer();
    js_free(trc);
}



struct VerifyPostTracer : JSTracer {
    
    uint64_t number;

    
    int count;
};






static void
StartVerifyPostBarriers(JSRuntime *rt)
{
#ifdef JSGC_GENERATIONAL
    if (!rt->gcExactScanningEnabled ||
        rt->gcVerifyPostData ||
        rt->gcIncrementalState != NO_INCREMENTAL)
    {
        return;
    }
    VerifyPostTracer *trc = new (js_malloc(sizeof(VerifyPostTracer))) VerifyPostTracer;
    rt->gcVerifyPostData = trc;
    rt->gcNumber++;
    trc->number = rt->gcNumber;
    trc->count = 0;
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        if (IsAtomsCompartment(c))
            continue;

        if (!c->gcNursery.enable())
            goto oom;

        if (!c->gcStoreBuffer.enable())
            goto oom;
    }
    return;
oom:
    trc->~VerifyPostTracer();
    js_free(trc);
    rt->gcVerifyPostData = NULL;
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        c->gcNursery.disable();
        c->gcStoreBuffer.disable();
    }
#endif
}

#ifdef JSGC_GENERATIONAL
static void
AssertStoreBufferContainsEdge(StoreBuffer *storebuf, void *loc, void *dst)
{
    if (storebuf->containsEdgeAt(loc))
        return;

    char msgbuf[1024];
    JS_snprintf(msgbuf, sizeof(msgbuf), "[post-barrier verifier] Missing edge @ %p to %p",
                loc, dst);
    MOZ_ReportAssertionFailure(msgbuf, __FILE__, __LINE__);
    MOZ_CRASH();
}

static void
PostVerifierVisitEdge(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    VerifyPostTracer *trc = (VerifyPostTracer *)jstrc;
    Cell *dst = (Cell *)*thingp;
    JSCompartment *comp = dst->compartment();

    



    if (IsAtomsCompartment(comp))
        return;

    
    if (!comp->gcNursery.isInside(dst))
        return;

    




    Cell *loc = (Cell *)(trc->realLocation != NULL ? trc->realLocation : thingp);

    AssertStoreBufferContainsEdge(&comp->gcStoreBuffer, loc, dst);
}
#endif

static void
EndVerifyPostBarriers(JSRuntime *rt)
{
#ifdef JSGC_GENERATIONAL
    AutoTraceSession session(rt);

    rt->gcHelperThread.waitBackgroundSweepOrAllocEnd();

    AutoCopyFreeListToArenas copy(rt);
    RecordNativeStackTopForGC(rt);

    VerifyPostTracer *trc = (VerifyPostTracer *)rt->gcVerifyPostData;
    JS_TracerInit(trc, rt, PostVerifierVisitEdge);
    trc->count = 0;

    if (!rt->gcExactScanningEnabled)
        goto oom;

    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        if (!c->gcStoreBuffer.coalesceForVerification())
            goto oom;
    }

    
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        if (!c->gcStoreBuffer.isEnabled() ||
             c->gcStoreBuffer.hasOverflowed() ||
             IsAtomsCompartment(c))
        {
            continue;
        }

        if (c->watchpointMap)
            c->watchpointMap->markAll(trc);

        for (size_t kind = 0; kind < FINALIZE_LIMIT; ++kind) {
            for (CellIterUnderGC cells(c, AllocKind(kind)); !cells.done(); cells.next()) {
                Cell *src = cells.getCell();
                if (!c->gcNursery.isInside(src))
                    JS_TraceChildren(trc, src, MapAllocToTraceKind(AllocKind(kind)));
            }
        }
    }

oom:
    trc->~VerifyPostTracer();
    js_free(trc);
    rt->gcVerifyPostData = NULL;
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        c->gcNursery.disable();
        c->gcStoreBuffer.disable();
        c->gcStoreBuffer.releaseVerificationData();
    }
#endif
}



static void
VerifyPreBarriers(JSRuntime *rt)
{
    if (rt->gcVerifyPreData)
        EndVerifyPreBarriers(rt);
    else
        StartVerifyPreBarriers(rt);
}

static void
VerifyPostBarriers(JSRuntime *rt)
{
    if (rt->gcVerifyPostData)
        EndVerifyPostBarriers(rt);
    else
        StartVerifyPostBarriers(rt);
}

void
VerifyBarriers(JSRuntime *rt, VerifierType type)
{
    if (type == PreBarrierVerifier)
        VerifyPreBarriers(rt);
    else
        VerifyPostBarriers(rt);
}

static void
MaybeVerifyPreBarriers(JSRuntime *rt, bool always)
{
    if (rt->gcZeal() != ZealVerifierPreValue)
        return;

    if (VerifyPreTracer *trc = (VerifyPreTracer *)rt->gcVerifyPreData) {
        if (++trc->count < rt->gcZealFrequency && !always)
            return;

        EndVerifyPreBarriers(rt);
    }
    StartVerifyPreBarriers(rt);
}

static void
MaybeVerifyPostBarriers(JSRuntime *rt, bool always)
{
    if (rt->gcZeal() != ZealVerifierPostValue)
        return;

    if (VerifyPostTracer *trc = (VerifyPostTracer *)rt->gcVerifyPostData) {
        if (++trc->count < rt->gcZealFrequency && !always)
            return;

        EndVerifyPostBarriers(rt);
    }
    StartVerifyPostBarriers(rt);
}

void
MaybeVerifyBarriers(JSContext *cx, bool always)
{
    MaybeVerifyPreBarriers(cx->runtime, always);
    MaybeVerifyPostBarriers(cx->runtime, always);
}

void
FinishVerifier(JSRuntime *rt)
{
    if (VerifyPreTracer *trc = (VerifyPreTracer *)rt->gcVerifyPreData) {
        trc->~VerifyPreTracer();
        js_free(trc);
    }
#ifdef JSGC_GENERATIONAL
    if (VerifyPostTracer *trc = (VerifyPostTracer *)rt->gcVerifyPostData) {
        trc->~VerifyPostTracer();
        js_free(trc);
        for (CompartmentsIter c(rt); !c.done(); c.next()) {
            c->gcNursery.disable();
            c->gcStoreBuffer.disable();
        }
    }
#endif
}

#endif 

} 

void ReleaseAllJITCode(FreeOp *fop)
{
#ifdef JS_METHODJIT
    for (CompartmentsIter c(fop->runtime()); !c.done(); c.next()) {
        mjit::ClearAllFrames(c);
        for (CellIter i(c, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            mjit::ReleaseScriptCode(fop, script);
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
StartPCCountProfiling(JSContext *cx)
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
StopPCCountProfiling(JSContext *cx)
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
            JSScript *script = i.get<JSScript>();
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
PurgePCCounts(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    if (!rt->scriptAndCountsVector)
        return;
    JS_ASSERT(!rt->profilingScripts);

    ReleaseScriptCounts(rt->defaultFreeOp());
}

} 

JS_PUBLIC_API(void)
JS_IterateCompartments(JSRuntime *rt, void *data,
                       JSIterateCompartmentCallback compartmentCallback)
{
    JS_ASSERT(!rt->isHeapBusy());

    AutoTraceSession session(rt);
    rt->gcHelperThread.waitBackgroundSweepOrAllocEnd();

    for (CompartmentsIter c(rt); !c.done(); c.next())
        (*compartmentCallback)(rt, data, c);
}

#if JS_HAS_XML_SUPPORT
extern size_t sE4XObjectsCreated;

JSXML *
js_NewGCXML(JSContext *cx)
{
    if (!cx->runningWithTrustedPrincipals())
        ++sE4XObjectsCreated;

    return NewGCThing<JSXML>(cx, js::gc::FINALIZE_XML, sizeof(JSXML));
}
#endif
