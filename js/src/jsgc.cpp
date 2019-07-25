









































#include "mozilla/Attributes.h"
#include "mozilla/Util.h"



































#include <math.h>
#include <string.h>     

#include "jstypes.h"
#include "jsutil.h"
#include "jshash.h"
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
#include "jsgcmark.h"
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

#include "frontend/Parser.h"
#include "gc/Memory.h"
#include "methodjit/MethodJIT.h"
#include "vm/Debugger.h"
#include "ion/IonCode.h"
#ifdef JS_ION
# include "ion/IonMacroAssembler.h"
#endif
#include "ion/IonFrameIterator.h"

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







const float GC_HEAP_GROWTH_FACTOR = 3.0f;


static const uint64_t GC_IDLE_FULL_SPAN = 20 * 1000 * 1000;

#ifdef JS_GC_ZEAL
static void
StartVerifyBarriers(JSContext *cx);

static void
EndVerifyBarriers(JSContext *cx);

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
#if JS_HAS_XML_SUPPORT
    OFFSET(JSXML),              
#endif
    OFFSET(JSShortString),      
    OFFSET(JSString),           
    OFFSET(JSExternalString),   
    OFFSET(ion::IonCode),       
};

#undef OFFSET

class GCCompartmentsIter {
  private:
    JSCompartment **it, **end;

  public:
    GCCompartmentsIter(JSRuntime *rt) {
        if (rt->gcCurrentCompartment) {
            it = &rt->gcCurrentCompartment;
            end = &rt->gcCurrentCompartment + 1;
        } else {
            it = rt->compartments.begin();
            end = rt->compartments.end();
        }
    }

    bool done() const { return it == end; }

    void next() {
        JS_ASSERT(!done());
        it++;
    }

    JSCompartment *get() const {
        JS_ASSERT(!done());
        return *it;
    }

    operator JSCompartment *() const { return get(); }
    JSCompartment *operator->() const { return get(); }
};

#ifdef DEBUG
void
ArenaHeader::checkSynchronizedWithFreeList() const
{
    



    JS_ASSERT(allocated());

    




    if (!compartment->rt->gcRunning)
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
Arena::finalize(JSContext *cx, AllocKind thingKind, size_t thingSize, bool background)
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
                t->finalize(cx, background);
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

template<typename T>
inline void
FinalizeTypedArenas(JSContext *cx, ArenaLists::ArenaList *al, AllocKind thingKind, bool background)
{
    




    JS_ASSERT_IF(!al->head, al->cursor == &al->head);
    ArenaLists::ArenaList available;
    ArenaHeader **ap = &al->head;
    size_t thingSize = Arena::thingSize(thingKind);
    while (ArenaHeader *aheader = *ap) {
        bool allClear = aheader->getArena()->finalize<T>(cx, thingKind, thingSize, background);
        if (allClear) {
            *ap = aheader->next;
            aheader->chunk()->releaseArena(aheader);
        } else if (aheader->hasFreeThings()) {
            *ap = aheader->next;
            *available.cursor = aheader;
            available.cursor = &aheader->next;
        } else {
            ap = &aheader->next;
        }
    }

    
    *available.cursor = NULL;
    *ap = available.head;
    al->cursor = ap;
    JS_ASSERT_IF(!al->head, al->cursor == &al->head);
}





static void
FinalizeArenas(JSContext *cx, ArenaLists::ArenaList *al, AllocKind thingKind, bool background)
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
        FinalizeTypedArenas<JSObject>(cx, al, thingKind, background);
        break;
      case FINALIZE_SCRIPT:
	FinalizeTypedArenas<JSScript>(cx, al, thingKind, background);
        break;
      case FINALIZE_SHAPE:
	FinalizeTypedArenas<Shape>(cx, al, thingKind, background);
        break;
      case FINALIZE_BASE_SHAPE:
        FinalizeTypedArenas<BaseShape>(cx, al, thingKind, background);
        break;
      case FINALIZE_TYPE_OBJECT:
	FinalizeTypedArenas<types::TypeObject>(cx, al, thingKind, background);
        break;
#if JS_HAS_XML_SUPPORT
      case FINALIZE_XML:
	FinalizeTypedArenas<JSXML>(cx, al, thingKind, background);
        break;
#endif
      case FINALIZE_STRING:
	FinalizeTypedArenas<JSString>(cx, al, thingKind, background);
        break;
      case FINALIZE_SHORT_STRING:
	FinalizeTypedArenas<JSShortString>(cx, al, thingKind, background);
        break;
      case FINALIZE_EXTERNAL_STRING:
	FinalizeTypedArenas<JSExternalString>(cx, al, thingKind, background);
        break;
      case FINALIZE_IONCODE:
#ifdef JS_ION
        FinalizeTypedArenas<ion::IonCode>(cx, al, thingKind, false);
#endif
        break;
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

#ifdef JS_THREADSAFE
inline bool
ChunkPool::wantBackgroundAllocation(JSRuntime *rt) const
{
    




    return rt->gcHelperThread.canBackgroundAllocate() &&
           emptyCount == 0 &&
           rt->gcChunkSet.count() >= 4;
}
#endif


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

#ifdef JS_THREADSAFE
    if (wantBackgroundAllocation(rt))
        rt->gcHelperThread.startBackgroundAllocationIfIdle();
#endif

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

JS_FRIEND_API(int64_t)
ChunkPool::countCleanDecommittedArenas(JSRuntime *rt)
{
    JS_ASSERT(this == &rt->gcChunkPool);

    int64_t numDecommitted = 0;
    Chunk *chunk = emptyChunkListHead;
    while (chunk) {
        for (uint32_t i = 0; i < ArenasPerChunk; ++i)
            if (chunk->decommittedArenas.get(i))
                ++numDecommitted;
        chunk = chunk->info.next;
    }
    return numDecommitted;
}

 Chunk *
Chunk::allocate(JSRuntime *rt)
{
    Chunk *chunk = static_cast<Chunk *>(AllocChunk());
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

    
    for (jsuint i = 0; i < ArenasPerChunk; i++) {
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







jsuint
Chunk::findDecommittedArenaOffset()
{
    
    for (jsuint i = info.lastDecommittedArenaOffset; i < ArenasPerChunk; i++)
        if (decommittedArenas.get(i))
            return i;
    for (jsuint i = 0; i < info.lastDecommittedArenaOffset; i++)
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

    jsuint offset = findDecommittedArenaOffset();
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
#ifdef JS_THREADSAFE
    AutoLockGC maybeLock;
    if (rt->gcHelperThread.sweeping())
        maybeLock.lock(rt);
#endif

    Probes::resizeHeap(comp, rt->gcBytes, rt->gcBytes - ArenaSize);
    JS_ASSERT(rt->gcBytes >= ArenaSize);
    JS_ASSERT(comp->gcBytes >= ArenaSize);
#ifdef JS_THREADSAFE
    if (rt->gcHelperThread.sweeping())
        comp->reduceGCTriggerBytes(GC_HEAP_GROWTH_FACTOR * ArenaSize);
#endif
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

JS_FRIEND_API(bool)
IsAboutToBeFinalized(const Cell *thing)
{
    JSCompartment *thingCompartment = reinterpret_cast<const Cell *>(thing)->compartment();
    JSRuntime *rt = thingCompartment->rt;
    if (rt->gcCurrentCompartment != NULL && rt->gcCurrentCompartment != thingCompartment)
        return false;
    return !reinterpret_cast<const Cell *>(thing)->isMarked();
}

bool
IsAboutToBeFinalized(const Value &v)
{
    JS_ASSERT(v.isMarkable());
    return IsAboutToBeFinalized((Cell *)v.toGCThing());
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
    if (!rt->gcHelperThread.init())
        return false;
#endif

    



    rt->gcMaxBytes = maxbytes;
    rt->setGCMaxMallocBytes(maxbytes);

    rt->gcJitReleaseTime = PRMJ_Now() + JIT_SCRIPT_RELEASE_TYPES_INTERVAL;
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
                     gc::AllocKind *thingKindPtr, ArenaHeader **arenaHeader, void **thing)
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

    JSCompartment *curComp = rt->gcCurrentCompartment;
    if (curComp && curComp != aheader->compartment)
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
    ConservativeGCTest status = IsAddressableGCThing(trc->runtime, w, &thingKind, &aheader, &thing);
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
    MarkKind(trc, thing, traceKind);

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

static void
MarkRangeConservatively(JSTracer *trc, const uintptr_t *begin, const uintptr_t *end)
{
    JS_ASSERT(begin <= end);

    for (const uintptr_t *i = begin; i < end; ++i)
        MarkWordConservatively(trc, *i);
}

static void
MarkRangeConservativelyAndSkipIon(JSTracer *trc, JSRuntime *rt, const uintptr_t *begin, const uintptr_t *end)
{
    const uintptr_t *i = begin;

#if JS_STACK_GROWTH_DIRECTION < 0 && defined(JS_ION)
    
    
    
    for (ion::IonActivationIterator ion(rt); ion.more(); ++ion) {
        ion::IonFrameIterator frames(ion.top());
        while (frames.more())
            ++frames;

        uintptr_t *ionMin = (uintptr_t *)ion.top();
        uintptr_t *ionEnd = (uintptr_t *)frames.fp();

        MarkRangeConservatively(trc, i, ionMin);
        i = ionEnd;
    }
#endif
    
    
    MarkRangeConservatively(trc, i, end);
}

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
            MarkKind(trc, root->thing, root->kind);
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
        JS_ASSERT(rt->requestDepth <= cgcd->requestThreshold);
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
    MarkRangeConservativelyAndSkipIon(trc, rt, stackMin, stackEnd);
    MarkRangeConservatively(trc, cgcd->registerSnapshot.words,
                            ArrayEnd(cgcd->registerSnapshot.words));
}

void
MarkStackRangeConservatively(JSTracer *trc, Value *beginv, Value *endv)
{
    






    struct AutoSkipChecking {
        JSRuntime *runtime;
        JSCompartment *savedCompartment;

        AutoSkipChecking(JSRuntime *rt)
          : runtime(rt), savedCompartment(rt->gcCheckCompartment) {
            rt->gcCheckCompartment = NULL;
        }
        ~AutoSkipChecking() { runtime->gcCheckCompartment = savedCompartment; }
    } as(trc->runtime);

    const uintptr_t *begin = beginv->payloadWord();
    const uintptr_t *end = endv->payloadWord();
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

void
RecordNativeStackTopForGC(JSContext *cx)
{
    ConservativeGCData *cgcd = &cx->runtime->conservativeGC;

#ifdef JS_THREADSAFE
    
    JS_ASSERT(cx->runtime->requestDepth >= cgcd->requestThreshold);
    if (cx->runtime->requestDepth == cgcd->requestThreshold)
        return;
#endif
    cgcd->recordStackTop();
}

} 

bool
js_IsAddressableGCThing(JSRuntime *rt, uintptr_t w, gc::AllocKind *thingKind, void **thing)
{
    return js::IsAddressableGCThing(rt, w, thingKind, NULL, thing) == CGCT_VALID;
}

#ifdef DEBUG
static void
CheckLeakedRoots(JSRuntime *rt);
#endif

void
js_FinishGC(JSRuntime *rt)
{
    



#ifdef JS_THREADSAFE
    rt->gcHelperThread.finish();
#endif

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
    






    AutoLockGC lock(rt);

    return !!rt->gcRootsHash.put((void *)vp,
                                 RootInfo(name, JS_GC_ROOT_VALUE_PTR));
}

JS_FRIEND_API(JSBool)
js_AddGCThingRootRT(JSRuntime *rt, void **rp, const char *name)
{
    






    AutoLockGC lock(rt);

    return !!rt->gcRootsHash.put((void *)rp,
                                 RootInfo(name, JS_GC_ROOT_GCTHING_PTR));
}

JS_FRIEND_API(JSBool)
js_RemoveRoot(JSRuntime *rt, void *rp)
{
    



    AutoLockGC lock(rt);
    rt->gcRootsHash.remove(rp);
    rt->gcPoke = JS_TRUE;
    return JS_TRUE;
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
    AutoLockGC lock(rt);
    int ct = 0;
    for (RootEnum e(rt->gcRootsHash); !e.empty(); e.popFront()) {
        RootEntry &entry = e.front();

        ct++;
        intN mapflags = map(entry.key, entry.value.type, entry.value.name, data);

        if (mapflags & JS_MAP_GCROOT_REMOVE)
            e.removeFront();
        if (mapflags & JS_MAP_GCROOT_STOP)
            break;
    }

    return ct;
}

void
JSCompartment::setGCLastBytes(size_t lastBytes, JSGCInvocationKind gckind)
{
    gcLastBytes = lastBytes;

    size_t base = gckind == GC_SHRINK ? lastBytes : Max(lastBytes, GC_ALLOCATION_THRESHOLD);
    float trigger = float(base) * GC_HEAP_GROWTH_FACTOR;
    gcTriggerBytes = size_t(Min(float(rt->gcMaxBytes), trigger));
}

void
JSCompartment::reduceGCTriggerBytes(size_t amount)
{
    JS_ASSERT(amount > 0);
    JS_ASSERT(gcTriggerBytes - amount >= 0);
    if (gcTriggerBytes - amount < GC_ALLOCATION_THRESHOLD * GC_HEAP_GROWTH_FACTOR)
        return;
    gcTriggerBytes -= amount;
}

namespace js {
namespace gc {

inline void
ArenaLists::prepareForIncrementalGC(JSCompartment *comp)
{
    for (size_t i = 0; i != FINALIZE_LIMIT; ++i) {
        FreeSpan *headSpan = &freeLists[i];
        if (!headSpan->isEmpty()) {
            ArenaHeader *aheader = headSpan->arenaHeader();
            aheader->allocatedDuringIncremental = true;
            comp->barrierMarker_.delayMarkingArena(aheader);
        }
    }
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
            if (JS_UNLIKELY(comp->needsBarrier())) {
                aheader->allocatedDuringIncremental = true;
                comp->barrierMarker_.delayMarkingArena(aheader);
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

    if (JS_UNLIKELY(comp->needsBarrier())) {
        aheader->allocatedDuringIncremental = true;
        comp->barrierMarker_.delayMarkingArena(aheader);
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
ArenaLists::finalizeNow(JSContext *cx, AllocKind thingKind)
{
#ifdef JS_THREADSAFE
    JS_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE);
#endif
    FinalizeArenas(cx, &arenaLists[thingKind], thingKind, false);
}

inline void
ArenaLists::finalizeLater(JSContext *cx, AllocKind thingKind)
{
    JS_ASSERT(thingKind == FINALIZE_OBJECT0_BACKGROUND  ||
              thingKind == FINALIZE_OBJECT2_BACKGROUND  ||
              thingKind == FINALIZE_OBJECT4_BACKGROUND  ||
              thingKind == FINALIZE_OBJECT8_BACKGROUND  ||
              thingKind == FINALIZE_OBJECT12_BACKGROUND ||
              thingKind == FINALIZE_OBJECT16_BACKGROUND ||
              thingKind == FINALIZE_SHORT_STRING        ||
              thingKind == FINALIZE_STRING);

#ifdef JS_THREADSAFE
    JS_ASSERT(!cx->runtime->gcHelperThread.sweeping());

    ArenaList *al = &arenaLists[thingKind];
    if (!al->head) {
        JS_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE);
        JS_ASSERT(al->cursor == &al->head);
        return;
    }

    



    JS_ASSERT(backgroundFinalizeState[thingKind] == BFS_DONE ||
              backgroundFinalizeState[thingKind] == BFS_JUST_FINISHED);

    if (cx->gcBackgroundFree) {
        




        cx->gcBackgroundFree->finalizeVector.infallibleAppend(al->head);
        al->clear();
        backgroundFinalizeState[thingKind] = BFS_RUN;
    } else {
        FinalizeArenas(cx, al, thingKind, false);
        backgroundFinalizeState[thingKind] = BFS_DONE;
    }

#else 

    finalizeNow(cx, thingKind);

#endif
}

#ifdef JS_THREADSAFE
 void
ArenaLists::backgroundFinalize(JSContext *cx, ArenaHeader *listHead)
{
    JS_ASSERT(listHead);
    AllocKind thingKind = listHead->getAllocKind();
    JSCompartment *comp = listHead->compartment;
    ArenaList finalized;
    finalized.head = listHead;
    FinalizeArenas(cx, &finalized, thingKind, true);

    




    ArenaLists *lists = &comp->arenas;
    ArenaList *al = &lists->arenaLists[thingKind];

    AutoLockGC lock(cx->runtime);
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
}
#endif 

void
ArenaLists::finalizeObjects(JSContext *cx)
{
    finalizeNow(cx, FINALIZE_OBJECT0);
    finalizeNow(cx, FINALIZE_OBJECT2);
    finalizeNow(cx, FINALIZE_OBJECT4);
    finalizeNow(cx, FINALIZE_OBJECT8);
    finalizeNow(cx, FINALIZE_OBJECT12);
    finalizeNow(cx, FINALIZE_OBJECT16);

#ifdef JS_THREADSAFE
    finalizeLater(cx, FINALIZE_OBJECT0_BACKGROUND);
    finalizeLater(cx, FINALIZE_OBJECT2_BACKGROUND);
    finalizeLater(cx, FINALIZE_OBJECT4_BACKGROUND);
    finalizeLater(cx, FINALIZE_OBJECT8_BACKGROUND);
    finalizeLater(cx, FINALIZE_OBJECT12_BACKGROUND);
    finalizeLater(cx, FINALIZE_OBJECT16_BACKGROUND);
#endif

#if JS_HAS_XML_SUPPORT
    finalizeNow(cx, FINALIZE_XML);
#endif
}

void
ArenaLists::finalizeStrings(JSContext *cx)
{
    finalizeLater(cx, FINALIZE_SHORT_STRING);
    finalizeLater(cx, FINALIZE_STRING);

    finalizeNow(cx, FINALIZE_EXTERNAL_STRING);
}

void
ArenaLists::finalizeShapes(JSContext *cx)
{
    finalizeNow(cx, FINALIZE_SHAPE);
    finalizeNow(cx, FINALIZE_BASE_SHAPE);
    finalizeNow(cx, FINALIZE_TYPE_OBJECT);
}

void
ArenaLists::finalizeScripts(JSContext *cx)
{
    finalizeNow(cx, FINALIZE_SCRIPT);
}

void
ArenaLists::finalizeIonCode(JSContext *cx)
{
    finalizeNow(cx, FINALIZE_IONCODE);
}

static void
RunLastDitchGC(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    
    AutoKeepAtoms keep(rt);
    GC(cx, rt->gcTriggerCompartment, GC_NORMAL, gcreason::LAST_DITCH);
}

 void *
ArenaLists::refillFreeList(JSContext *cx, AllocKind thingKind)
{
    JS_ASSERT(cx->compartment->arenas.freeLists[thingKind].isEmpty());

    JSCompartment *comp = cx->compartment;
    JSRuntime *rt = comp->rt;
    JS_ASSERT(!rt->gcRunning);

    bool runGC = rt->gcIncrementalState != NO_INCREMENTAL && comp->gcBytes > comp->gcTriggerBytes;
    for (;;) {
        if (JS_UNLIKELY(runGC)) {
            RunLastDitchGC(cx);

            




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

            AutoLockGC lock(rt);
#ifdef JS_THREADSAFE
            rt->gcHelperThread.waitBackgroundSweepEnd();
#endif
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

    AutoLockGC lock(rt);
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

    AutoLockGC lock(rt);
    GCLocks::Ptr p = rt->gcLocksHash.lookup(thing);

    if (p) {
        rt->gcPoke = true;
        if (--p->value == 0)
            rt->gcLocksHash.remove(p);
    }
}

namespace js {

void
InitTracer(JSTracer *trc, JSRuntime *rt, JSContext *cx, JSTraceCallback callback)
{
    trc->runtime = rt;
    trc->context = cx;
    trc->callback = callback;
    trc->debugPrinter = NULL;
    trc->debugPrintArg = NULL;
    trc->debugPrintIndex = size_t(-1);
    trc->eagerlyTraceWeakMaps = true;
}

 int64_t
SliceBudget::TimeBudget(int64_t millis)
{
    return millis * PRMJ_USEC_PER_MSEC;
}

 int64_t
SliceBudget::WorkBudget(int64_t work)
{
    return -work;
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
        counter = -budget;
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

GCMarker::GCMarker(size_t sizeLimit)
  : stack(sizeLimit),
    color(BLACK),
    started(false),
    unmarkedArenaStackTop(NULL),
    markLaterArenas(0),
    grayFailed(false)
{
}

bool
GCMarker::init(bool lazy)
{
    if (!stack.init(lazy ? 0 : MARK_STACK_LENGTH))
        return false;
    return true;
}

void
GCMarker::start(JSRuntime *rt, JSContext *cx)
{
    InitTracer(this, rt, cx, NULL);
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
        aheader->hasDelayedMarking = 0;
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
        aheader->hasDelayedMarking = 0;
        markLaterArenas--;
        markDelayedChildren(aheader);

        if (budget.checkOverBudget())
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

    Cell *cell = static_cast<Cell *>(p);
    if (runtime->gcRunning && runtime->gcCurrentCompartment)
        JS_ASSERT(cell->compartment() == runtime->gcCurrentCompartment);
    else if (runtime->gcIncrementalCompartment)
        JS_ASSERT(cell->compartment() == runtime->gcIncrementalCompartment);
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
        MarkKind(this, elem->thing, elem->kind);
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
    JS_ASSERT(!rt->gcRunning);
    rt->gcMarker.setSizeLimit(limit);
    for (CompartmentsIter c(rt); !c.done(); c.next())
        c->barrierMarker_.setSizeLimit(limit);
}

} 

#ifdef DEBUG
static void
EmptyMarkCallback(JSTracer *trc, void **thingp, JSGCTraceKind kind)
{
}
#endif

static void
gc_root_traversal(JSTracer *trc, const RootEntry &entry)
{
#ifdef DEBUG
    void *ptr;
    if (entry.value.type == JS_GC_ROOT_GCTHING_PTR) {
        ptr = *reinterpret_cast<void **>(entry.key);
    } else {
        Value *vp = reinterpret_cast<Value *>(entry.key);
        ptr = vp->isGCThing() ? vp->toGCThing() : NULL;
    }

    if (ptr && !trc->runtime->gcCurrentCompartment) {
        




        JSTracer checker;
        JS_ASSERT(trc->runtime == trc->context->runtime);
        JS_TracerInit(&checker, trc->context, EmptyMarkCallback);
        ConservativeGCTest test = MarkIfGCThingWord(&checker, reinterpret_cast<uintptr_t>(ptr));
        if (test != CGCT_VALID && entry.value.name) {
            fprintf(stderr,
"JS API usage error: the address passed to JS_AddNamedRoot currently holds an\n"
"invalid gcthing.  This is usually caused by a missing call to JS_RemoveRoot.\n"
"The root's name is \"%s\".\n",
                    entry.value.name);
        }
        JS_ASSERT(test == CGCT_VALID);
    }
#endif
    const char *name = entry.value.name ? entry.value.name : "root";
    if (entry.value.type == JS_GC_ROOT_GCTHING_PTR)
        MarkGCThingRoot(trc, *reinterpret_cast<void **>(entry.key), name);
    else
        MarkValueRoot(trc, reinterpret_cast<Value *>(entry.key), name);
}

static void
gc_lock_traversal(const GCLocks::Entry &entry, JSTracer *trc)
{
    JS_ASSERT(entry.value >= 1);
    MarkGCThingRoot(trc, entry.key, "locked object");
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
        static_cast<Parser *>(this)->trace(trc);
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
            MarkValueRoot(trc, &desc.pd, "PropDesc::pd");
            MarkValueRoot(trc, &desc.value, "PropDesc::value");
            MarkValueRoot(trc, &desc.get, "PropDesc::get");
            MarkValueRoot(trc, &desc.set, "PropDesc::set");
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

      case NAMESPACES: {
        JSXMLArray<JSObject> &array = static_cast<AutoNamespaceArray *>(this)->array;
        MarkObjectRange(trc, array.length, array.vector, "JSXMLArray.vector");
        js_XMLArrayCursorTrace(trc, array.cursors);
        return;
      }

      case XML:
        js_TraceXML(trc, static_cast<AutoXMLRooter *>(this)->xml);
        return;

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

      case VALARRAY: {
        AutoValueArray *array = static_cast<AutoValueArray *>(this);
        MarkValueRootRange(trc, array->length(), array->start(), "js::AutoValueArray");
        return;
      }

      case IONMASM: {
#ifdef JS_ION
        static_cast<js::ion::MacroAssembler::AutoRooter *>(this)->masm()->trace(trc);
#endif
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
    for (js::AutoGCRooter *gcr = this; gcr; gcr = gcr->down)
        gcr->trace(trc);
}

namespace js {

static void
MarkRuntime(JSTracer *trc, bool useSavedRoots = false)
{
    JSRuntime *rt = trc->runtime;
    JS_ASSERT(trc->callback != GCMarker::GrayCallback);
    if (rt->gcCurrentCompartment) {
        for (CompartmentsIter c(rt); !c.done(); c.next())
            c->markCrossCompartmentWrappers(trc);
        Debugger::markCrossCompartmentDebuggerObjectReferents(trc);
    }

    if (rt->hasContexts())
        MarkConservativeStackRoots(trc, useSavedRoots);

    for (RootRange r = rt->gcRootsHash.all(); !r.empty(); r.popFront())
        gc_root_traversal(trc, r.front());

    for (GCLocks::Range r = rt->gcLocksHash.all(); !r.empty(); r.popFront())
        gc_lock_traversal(r.front(), trc);

    if (rt->scriptPCCounters) {
        ScriptOpcodeCountsVector &vec = *rt->scriptPCCounters;
        for (size_t i = 0; i < vec.length(); i++)
            MarkScriptRoot(trc, &vec[i].script, "scriptPCCounters");
    }

    js_TraceAtomState(trc);
    rt->staticStrings.trace(trc);

    JSContext *iter = NULL;
    while (JSContext *acx = js_ContextIterator(rt, JS_TRUE, &iter))
        acx->mark(trc);

    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        if (c->activeAnalysis)
            c->markTypes(trc);

        
        if (!IS_GC_MARKING_TRACER(trc)) {
            if (c->watchpointMap)
                c->watchpointMap->markAll(trc);
        }

        
        if (rt->profilingScripts) {
            for (CellIterUnderGC i(c, FINALIZE_SCRIPT); !i.done(); i.next()) {
                JSScript *script = i.get<JSScript>();
                if (script->pcCounters) {
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

    rt->stackSpace.mark(trc);
#ifdef JS_ION
    ion::MarkIonActivations(rt, trc);
#endif

    for (GCCompartmentsIter c(rt); !c.done(); c.next())
        c->mark(trc);

    
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
void
TriggerGC(JSRuntime *rt, gcreason::Reason reason)
{
    JS_ASSERT(rt->onOwnerThread());

    if (rt->gcRunning || rt->gcIsNeeded)
        return;

    
    rt->gcIsNeeded = true;
    rt->gcTriggerCompartment = NULL;
    rt->gcTriggerReason = reason;
    rt->triggerOperationCallback();
}

void
TriggerCompartmentGC(JSCompartment *comp, gcreason::Reason reason)
{
    JSRuntime *rt = comp->rt;
    JS_ASSERT(!rt->gcRunning);

    if (rt->gcZeal() == ZealAllocValue) {
        TriggerGC(rt, reason);
        return;
    }

    if (rt->gcMode == JSGC_MODE_GLOBAL || comp == rt->atomsCompartment) {
        
        TriggerGC(rt, reason);
        return;
    }

    if (rt->gcIsNeeded) {
        
        if (rt->gcTriggerCompartment != comp)
            rt->gcTriggerCompartment = NULL;
        return;
    }

    



    rt->gcIsNeeded = true;
    rt->gcTriggerCompartment = comp;
    rt->gcTriggerReason = reason;
    comp->rt->triggerOperationCallback();
}

void
MaybeGC(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    JS_ASSERT(rt->onOwnerThread());

    if (rt->gcZeal() == ZealAllocValue || rt->gcZeal() == ZealPokeValue) {
        GC(cx, NULL, GC_NORMAL, gcreason::MAYBEGC);
        return;
    }

    JSCompartment *comp = cx->compartment;
    if (rt->gcIsNeeded) {
        GCSlice(cx, (comp == rt->gcTriggerCompartment) ? comp : NULL,
                GC_NORMAL, gcreason::MAYBEGC);
        return;
    }

    if (comp->gcBytes > 8192 &&
        comp->gcBytes >= 3 * (comp->gcTriggerBytes / 4) &&
        rt->gcIncrementalState == NO_INCREMENTAL)
    {
        GCSlice(cx, NULL, GC_NORMAL, gcreason::MAYBEGC);
        return;
    }

    




    int64_t now = PRMJ_Now();
    if (rt->gcNextFullGCTime && rt->gcNextFullGCTime <= now) {
        if (rt->gcChunkAllocationSinceLastGC ||
            rt->gcNumArenasFreeCommitted > FreeCommittedArenasThreshold)
        {
            GCSlice(cx, NULL, GC_SHRINK, gcreason::MAYBEGC);
        } else {
            rt->gcNextFullGCTime = now + GC_IDLE_FULL_SPAN;
        }
    }
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
                if (!rt->gcRunning)
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

bool
GCHelperThread::init()
{
    if (!(wakeup = PR_NewCondVar(rt->gcLock)))
        return false;
    if (!(done = PR_NewCondVar(rt->gcLock)))
        return false;

    thread = PR_CreateThread(PR_USER_THREAD, threadMain, this, PR_PRIORITY_NORMAL,
                             PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);
    if (!thread)
        return false;

    backgroundAllocation = (GetCPUCount() >= 2);
    return true;
}

void
GCHelperThread::finish()
{
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
}


void
GCHelperThread::threadMain(void *arg)
{
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

bool
GCHelperThread::prepareForBackgroundSweep()
{
    JS_ASSERT(state == IDLE);
    size_t maxArenaLists = MAX_BACKGROUND_FINALIZE_KINDS * rt->compartments.length();
    return finalizeVector.reserve(maxArenaLists);
}


void
GCHelperThread::startBackgroundSweep(JSContext *cx, bool shouldShrink)
{
    
    JS_ASSERT(state == IDLE);
    JS_ASSERT(cx);
    JS_ASSERT(!finalizationContext);
    finalizationContext = cx;
    shrinkFlag = shouldShrink;
    state = SWEEPING;
    PR_NotifyCondVar(wakeup);
}


void
GCHelperThread::startBackgroundShrink()
{
    switch (state) {
      case IDLE:
        JS_ASSERT(!finalizationContext);
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


void
GCHelperThread::waitBackgroundSweepEnd()
{
    while (state == SWEEPING)
        PR_WaitCondVar(done, PR_INTERVAL_NO_TIMEOUT);
}


void
GCHelperThread::waitBackgroundSweepOrAllocEnd()
{
    if (state == ALLOCATING)
        state = CANCEL_ALLOCATION;
    while (state == SWEEPING || state == CANCEL_ALLOCATION)
        PR_WaitCondVar(done, PR_INTERVAL_NO_TIMEOUT);
}


inline void
GCHelperThread::startBackgroundAllocationIfIdle()
{
    if (state == IDLE) {
        state = ALLOCATING;
        PR_NotifyCondVar(wakeup);
    }
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


void
GCHelperThread::doSweep()
{
    if (JSContext *cx = finalizationContext) {
        finalizationContext = NULL;
        AutoUnlockGC unlock(rt);

        



        for (ArenaHeader **i = finalizeVector.begin(); i != finalizeVector.end(); ++i)
            ArenaLists::backgroundFinalize(cx, *i);
        finalizeVector.resize(0);

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
ReleaseObservedTypes(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    bool releaseTypes = false;
    int64_t now = PRMJ_Now();
    if (now >= rt->gcJitReleaseTime) {
        releaseTypes = true;
        rt->gcJitReleaseTime = now + JIT_SCRIPT_RELEASE_TYPES_INTERVAL;
    }

    return releaseTypes;
}

static void
SweepCompartments(JSContext *cx, JSGCInvocationKind gckind)
{
    JSRuntime *rt = cx->runtime;
    JSCompartmentCallback callback = rt->compartmentCallback;

    
    JSCompartment **read = rt->compartments.begin() + 1;
    JSCompartment **end = rt->compartments.end();
    JSCompartment **write = read;
    JS_ASSERT(rt->compartments.length() >= 1);
    JS_ASSERT(*rt->compartments.begin() == rt->atomsCompartment);

    while (read < end) {
        JSCompartment *compartment = *read++;

        if (!compartment->hold &&
            (compartment->arenas.arenaListsAreEmpty() || !rt->hasContexts()))
        {
            compartment->arenas.checkEmptyFreeLists();
            if (callback)
                JS_ALWAYS_TRUE(callback(cx, compartment, JSCOMPARTMENT_DESTROY));
            if (compartment->principals)
                JSPRINCIPALS_DROP(cx, compartment->principals);
            cx->delete_(compartment);
            continue;
        }
        *write++ = compartment;
    }
    rt->compartments.resize(write - rt->compartments.begin());
}

static void
PurgeRuntime(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    for (GCCompartmentsIter c(rt); !c.done(); c.next())
        c->purge(cx);

    rt->purge(cx);

    {
        JSContext *iter = NULL;
        while (JSContext *acx = js_ContextIterator(rt, JS_TRUE, &iter))
            acx->purge();
    }
}

static void
BeginMarkPhase(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    GCMarker *gcmarker = &rt->gcMarker;

    rt->gcStartNumber = rt->gcNumber;

    
    WeakMapBase::resetWeakMapList(rt);

    








    PurgeRuntime(cx);

    


    gcstats::AutoPhase ap1(rt->gcStats, gcstats::PHASE_MARK);
    gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_MARK_ROOTS);

    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront())
        r.front()->bitmap.clear();

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
MarkGrayAndWeak(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    FullGCMarker *gcmarker = &rt->gcMarker;

    JS_ASSERT(gcmarker->isDrained());
    MarkWeakReferences(gcmarker);

    gcmarker->setMarkColorGray();
    if (gcmarker->hasBufferedGrayRoots()) {
        gcmarker->markBufferedGrayRoots();
    } else {
        if (JSTraceDataOp op = rt->gcGrayRootsTraceOp)
            (*op)(gcmarker, rt->gcGrayRootsData);
    }
    SliceBudget budget;
    gcmarker->drainMarkStack(budget);
    MarkWeakReferences(gcmarker);
    JS_ASSERT(gcmarker->isDrained());
}

#ifdef DEBUG
static void
ValidateIncrementalMarking(JSContext *cx);
#endif

static void
EndMarkPhase(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    {
        gcstats::AutoPhase ap1(rt->gcStats, gcstats::PHASE_MARK);
        gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_MARK_OTHER);
        MarkGrayAndWeak(cx);
    }

    JS_ASSERT(rt->gcMarker.isDrained());

#ifdef DEBUG
    if (rt->gcIncrementalState != NO_INCREMENTAL)
        ValidateIncrementalMarking(cx);
#endif

    if (rt->gcCallback)
        (void) rt->gcCallback(cx, JSGC_MARK_END);

#ifdef DEBUG
    
    if (rt->gcCurrentCompartment) {
        for (CompartmentsIter c(rt); !c.done(); c.next()) {
            JS_ASSERT_IF(c != rt->gcCurrentCompartment && c != rt->atomsCompartment,
                         c->arenas.checkArenaListAllUnmarked());
        }
    }
#endif
}

#ifdef DEBUG
static void
ValidateIncrementalMarking(JSContext *cx)
{
    typedef HashMap<Chunk *, uintptr_t *, GCChunkHasher, SystemAllocPolicy> BitmapMap;
    BitmapMap map;
    if (!map.init())
        return;

    JSRuntime *rt = cx->runtime;
    FullGCMarker *gcmarker = &rt->gcMarker;

    
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
    rt->gcIncrementalState = NO_INCREMENTAL;

    
    WeakMapBase::resetWeakMapList(rt);

    JS_ASSERT(gcmarker->isDrained());
    gcmarker->reset();

    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront())
        r.front()->bitmap.clear();

    MarkRuntime(gcmarker, true);
    SliceBudget budget;
    rt->gcMarker.drainMarkStack(budget);
    MarkGrayAndWeak(cx);

    
    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront()) {
        Chunk *chunk = r.front();
        ChunkBitmap *bitmap = &chunk->bitmap;
        uintptr_t *entry = map.lookup(r.front())->value;
        ChunkBitmap incBitmap;

        memcpy(incBitmap.bitmap, entry, sizeof(incBitmap.bitmap));
        js_free(entry);

        for (size_t i = 0; i < ArenasPerChunk; i++) {
            Arena *arena = &chunk->arenas[i];
            if (!arena->aheader.allocated())
                continue;
            if (rt->gcCurrentCompartment && arena->aheader.compartment != rt->gcCurrentCompartment)
                continue;
            if (arena->aheader.allocatedDuringIncremental)
                continue;

            AllocKind kind = arena->aheader.getAllocKind();
            uintptr_t thing = arena->thingsStart(kind);
            uintptr_t end = arena->thingsEnd();
            while (thing < end) {
                Cell *cell = (Cell *)thing;
                if (bitmap->isMarked(cell, BLACK) && !incBitmap.isMarked(cell, BLACK)) {
                    JS_DumpHeap(cx, stdout, NULL, JSGCTraceKind(0), NULL, 100000, NULL);
                    printf("Assertion cell: %p (%d)\n", (void *)cell, cell->getAllocKind());
                }
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

static void
SweepPhase(JSContext *cx, JSGCInvocationKind gckind)
{
    JSRuntime *rt = cx->runtime;

#ifdef JS_THREADSAFE
    if (rt->hasContexts() && rt->gcHelperThread.prepareForBackgroundSweep())
        cx->gcBackgroundFree = &rt->gcHelperThread;
#endif

    
    for (GCCompartmentsIter c(rt); !c.done(); c.next())
        c->arenas.purge();

    













    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP);

    
    WeakMapBase::sweepAll(&rt->gcMarker);

    js_SweepAtomState(rt);

    
    WatchpointMap::sweepAll(rt);

    if (!rt->gcCurrentCompartment)
        Debugger::sweepAll(cx);

    bool releaseTypes = !rt->gcCurrentCompartment && ReleaseObservedTypes(cx);
    for (GCCompartmentsIter c(rt); !c.done(); c.next())
        c->sweep(cx, releaseTypes);

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_OBJECT);

        



        for (GCCompartmentsIter c(rt); !c.done(); c.next())
            c->arenas.finalizeObjects(cx);
    }

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_STRING);
        for (GCCompartmentsIter c(rt); !c.done(); c.next())
            c->arenas.finalizeStrings(cx);
    }

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_SCRIPT);
        for (GCCompartmentsIter c(rt); !c.done(); c.next())
            c->arenas.finalizeScripts(cx);
    }

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_SHAPE);
        for (GCCompartmentsIter c(rt); !c.done(); c.next())
            c->arenas.finalizeShapes(cx);
    }
	
    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_IONCODE);
        for (GCCompartmentsIter c(rt); !c.done(); c.next())
            c->arenas.finalizeIonCode(cx);
    }

#ifdef DEBUG
     PropertyTree::dumpShapes(cx);
#endif

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_DESTROY);

        





        for (GCCompartmentsIter c(rt); !c.done(); c.next())
            js_SweepScriptFilenames(c);

        



        if (!rt->gcCurrentCompartment)
            SweepCompartments(cx, gckind);

#ifndef JS_THREADSAFE
        




        ExpireChunksAndArenas(rt, gckind == GC_SHRINK);
#endif
    }

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_XPCONNECT);
        if (rt->gcCallback)
            (void) rt->gcCallback(cx, JSGC_FINALIZE_END);
    }

    for (CompartmentsIter c(rt); !c.done(); c.next())
        c->setGCLastBytes(c->gcBytes, gckind);
}


static void
MarkAndSweep(JSContext *cx, JSGCInvocationKind gckind)
{
    JSRuntime *rt = cx->runtime;

    AutoUnlockGC unlock(rt);

    rt->gcMarker.start(rt, cx);
    JS_ASSERT(!rt->gcMarker.callback);

    BeginMarkPhase(cx);
    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_MARK);
        SliceBudget budget;
        rt->gcMarker.drainMarkStack(budget);
    }
    EndMarkPhase(cx);
    SweepPhase(cx, gckind);

    rt->gcMarker.stop();
}





class AutoHeapSession {
  public:
    explicit AutoHeapSession(JSContext *cx);
    ~AutoHeapSession();

  protected:
    JSContext *context;

  private:
    AutoHeapSession(const AutoHeapSession&) MOZ_DELETE;
    void operator=(const AutoHeapSession&) MOZ_DELETE;
};


class AutoGCSession : AutoHeapSession {
  public:
    explicit AutoGCSession(JSContext *cx, JSCompartment *comp);
    ~AutoGCSession();

  private:
    



    SwitchToCompartment switcher;
};


AutoHeapSession::AutoHeapSession(JSContext *cx)
  : context(cx)
{
    JS_ASSERT(!cx->runtime->noGCOrAllocationCheck);
    JSRuntime *rt = cx->runtime;
    JS_ASSERT(!rt->gcRunning);
    rt->gcRunning = true;
}

AutoHeapSession::~AutoHeapSession()
{
    JSRuntime *rt = context->runtime;
    rt->gcRunning = false;
}

AutoGCSession::AutoGCSession(JSContext *cx, JSCompartment *comp)
  : AutoHeapSession(cx),
    switcher(cx, (JSCompartment *)NULL)
{
    JSRuntime *rt = cx->runtime;

    JS_ASSERT(!rt->gcCurrentCompartment);
    rt->gcCurrentCompartment = comp;

    rt->gcIsNeeded = false;
    rt->gcTriggerCompartment = NULL;
    rt->gcInterFrameGC = true;

    rt->gcNumber++;

    rt->resetGCMallocBytes();

    
    for (CompartmentsIter c(rt); !c.done(); c.next())
        c->resetGCMallocBytes();
}

AutoGCSession::~AutoGCSession()
{
    JSRuntime *rt = context->runtime;

    rt->gcCurrentCompartment = NULL;
    rt->gcNextFullGCTime = PRMJ_Now() + GC_IDLE_FULL_SPAN;
    rt->gcChunkAllocationSinceLastGC = false;
}

static void
ResetIncrementalGC(JSRuntime *rt)
{
    if (rt->gcIncrementalState == NO_INCREMENTAL)
        return;

    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        if (!rt->gcIncrementalCompartment || rt->gcIncrementalCompartment == c) {
            c->needsBarrier_ = false;
            c->barrierMarker_.reset();
            c->barrierMarker_.stop();
        }
        JS_ASSERT(!c->needsBarrier_);
    }

    rt->gcIncrementalCompartment = NULL;
    rt->gcMarker.reset();
    rt->gcMarker.stop();
    rt->gcIncrementalState = NO_INCREMENTAL;

    rt->gcStats.reset();
}

class AutoGCSlice {
  public:
    AutoGCSlice(JSContext *cx);
    ~AutoGCSlice();

  private:
    JSContext *context;
};

AutoGCSlice::AutoGCSlice(JSContext *cx)
  : context(cx)
{
    JSRuntime *rt = context->runtime;

    





    rt->stackSpace.markActiveCompartments();

    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        
        if (rt->gcIncrementalState == MARK)
            c->needsBarrier_ = false;
        else
            JS_ASSERT(!c->needsBarrier_);
    }
}

AutoGCSlice::~AutoGCSlice()
{
    JSRuntime *rt = context->runtime;

    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        if (rt->gcIncrementalState == MARK) {
            c->needsBarrier_ = true;
            c->arenas.prepareForIncrementalGC(c);
        } else {
            JS_ASSERT(rt->gcIncrementalState == NO_INCREMENTAL);

            c->needsBarrier_ = false;
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
IncrementalGCSlice(JSContext *cx, int64_t budget, JSGCInvocationKind gckind)
{
    JS_ASSERT(budget != SliceBudget::Unlimited);

    JSRuntime *rt = cx->runtime;

    AutoUnlockGC unlock(rt);
    AutoGCSlice slice(cx);

    gc::State initialState = rt->gcIncrementalState;

    if (rt->gcIncrementalState == NO_INCREMENTAL) {
        JS_ASSERT(!rt->gcIncrementalCompartment);
        rt->gcIncrementalCompartment = rt->gcCurrentCompartment;
        rt->gcIncrementalState = MARK_ROOTS;
        rt->gcLastMarkSlice = false;
    }

    if (rt->gcIncrementalState == MARK_ROOTS) {
        rt->gcMarker.start(rt, cx);
        JS_ASSERT(IS_GC_MARKING_TRACER(&rt->gcMarker));

        for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
            c->discardJitCode(cx);
            c->barrierMarker_.start(rt, NULL);
        }

        BeginMarkPhase(cx);

        rt->gcIncrementalState = MARK;
    }

    if (rt->gcIncrementalState == MARK) {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_MARK);
        SliceBudget sliceBudget(budget);

        
        if (!rt->gcMarker.hasBufferedGrayRoots())
            sliceBudget.reset();

        rt->gcMarker.context = cx;
        bool finished = rt->gcMarker.drainMarkStack(sliceBudget);

        for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
            c->barrierMarker_.context = cx;
            finished &= c->barrierMarker_.drainMarkStack(sliceBudget);
            c->barrierMarker_.context = NULL;
        }

        if (finished) {
            JS_ASSERT(rt->gcMarker.isDrained());
#ifdef DEBUG
            for (GCCompartmentsIter c(rt); !c.done(); c.next())
                JS_ASSERT(c->barrierMarker_.isDrained());
#endif
            if (initialState == MARK && !rt->gcLastMarkSlice)
                rt->gcLastMarkSlice = true;
            else
                rt->gcIncrementalState = SWEEP;
        }
    }

    if (rt->gcIncrementalState == SWEEP) {
        EndMarkPhase(cx);
        SweepPhase(cx, gckind);

        rt->gcMarker.stop();

        
        for (GCCompartmentsIter c(rt); !c.done(); c.next())
            c->barrierMarker_.stop();

        rt->gcIncrementalCompartment = NULL;

        rt->gcIncrementalState = NO_INCREMENTAL;
    }
}

static bool
IsIncrementalGCSafe(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    if (rt->gcCompartmentCreated) {
        rt->gcCompartmentCreated = false;
        return false;
    }

    if (rt->gcKeepAtoms)
        return false;

    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        if (c->activeAnalysis)
            return false;
    }

    if (rt->gcIncrementalState != NO_INCREMENTAL &&
        rt->gcCurrentCompartment != rt->gcIncrementalCompartment)
    {
        return false;
    }

    if (!rt->gcIncrementalEnabled)
        return false;

    return true;
}

static bool
IsIncrementalGCAllowed(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    if (rt->gcMode != JSGC_MODE_INCREMENTAL)
        return false;

#ifdef ANDROID
    
    return false;
#endif

    if (!IsIncrementalGCSafe(cx))
        return false;

    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        if (c->gcBytes > c->gcTriggerBytes)
            return false;
    }

    return true;
}







static JS_NEVER_INLINE void
GCCycle(JSContext *cx, JSCompartment *comp, int64_t budget, JSGCInvocationKind gckind)
{
    JSRuntime *rt = cx->runtime;

    JS_ASSERT_IF(comp, comp != rt->atomsCompartment);
    JS_ASSERT_IF(comp, rt->gcMode != JSGC_MODE_GLOBAL);

    
    if (rt->gcRunning)
        return;

    AutoGCSession gcsession(cx, comp);

    
    if (rt->inOOMReport)
        return;

#ifdef JS_THREADSAFE
    





    JS_ASSERT(!cx->gcBackgroundFree);
    rt->gcHelperThread.waitBackgroundSweepOrAllocEnd();
#endif

    if (budget != SliceBudget::Unlimited) {
        if (!IsIncrementalGCAllowed(cx))
            budget = SliceBudget::Unlimited;
    }

    if (budget == SliceBudget::Unlimited)
        ResetIncrementalGC(rt);

    AutoCopyFreeListToArenas copy(rt);

    if (budget == SliceBudget::Unlimited)
        MarkAndSweep(cx, gckind);
    else
        IncrementalGCSlice(cx, budget, gckind);

#ifdef DEBUG
    if (rt->gcIncrementalState == NO_INCREMENTAL) {
        for (CompartmentsIter c(rt); !c.done(); c.next())
            JS_ASSERT(!c->needsBarrier_);
    }
#endif
#ifdef JS_THREADSAFE
    if (rt->gcIncrementalState == NO_INCREMENTAL) {
        if (cx->gcBackgroundFree) {
            JS_ASSERT(cx->gcBackgroundFree == &rt->gcHelperThread);
            cx->gcBackgroundFree = NULL;
            rt->gcHelperThread.startBackgroundSweep(cx, gckind == GC_SHRINK);
        }
    }
#endif
}

static void
Collect(JSContext *cx, JSCompartment *comp, int64_t budget,
        JSGCInvocationKind gckind, gcreason::Reason reason)
{
    JSRuntime *rt = cx->runtime;
    JS_AbortIfWrongThread(rt);

    JS_ASSERT_IF(budget != SliceBudget::Unlimited, JSGC_INCREMENTAL);

#ifdef JS_GC_ZEAL
    struct AutoVerifyBarriers {
        JSContext *cx;
        bool inVerify;
        AutoVerifyBarriers(JSContext *cx) : cx(cx), inVerify(cx->runtime->gcVerifyData) {
            if (inVerify) EndVerifyBarriers(cx);
        }
        ~AutoVerifyBarriers() { if (inVerify) StartVerifyBarriers(cx); }
    } av(cx);
#endif

    RecordNativeStackTopForGC(cx);

    
    if (rt->gcIncrementalState != NO_INCREMENTAL && !rt->gcIncrementalCompartment)
        comp = NULL;

    gcstats::AutoGCSlice agc(rt->gcStats, comp, reason);

    do {
        



        if (rt->gcIncrementalState == NO_INCREMENTAL) {
            if (JSGCCallback callback = rt->gcCallback) {
                if (!callback(cx, JSGC_BEGIN) && rt->hasContexts())
                    return;
            }
        }

        {
            
            AutoLockGC lock(rt);
            rt->gcPoke = false;
            GCCycle(cx, comp, budget, gckind);
        }

        if (rt->gcIncrementalState == NO_INCREMENTAL) {
            if (JSGCCallback callback = rt->gcCallback)
                (void) callback(cx, JSGC_END);
        }

        



    } while (!rt->hasContexts() && rt->gcPoke);
}

namespace js {

void
GC(JSContext *cx, JSCompartment *comp, JSGCInvocationKind gckind, gcreason::Reason reason)
{
    Collect(cx, comp, SliceBudget::Unlimited, gckind, reason);
}

void
GCSlice(JSContext *cx, JSCompartment *comp, JSGCInvocationKind gckind, gcreason::Reason reason)
{
    Collect(cx, comp, cx->runtime->gcSliceBudget, gckind, reason);
}

void
GCDebugSlice(JSContext *cx, int64_t objCount)
{
    Collect(cx, NULL, SliceBudget::WorkBudget(objCount), GC_NORMAL, gcreason::API);
}

void
ShrinkGCBuffers(JSRuntime *rt)
{
    AutoLockGC lock(rt);
    JS_ASSERT(!rt->gcRunning);
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
        JSContext *cx = trc->context;
        JSRuntime *rt = cx->runtime;
        if (!rt->gcRunning) {
            AutoLockGC lock(rt);
            AutoHeapSession session(cx);

            rt->gcHelperThread.waitBackgroundSweepEnd();
            AutoUnlockGC unlock(rt);

            AutoCopyFreeListToArenas copy(rt);
            RecordNativeStackTopForGC(trc->context);
            MarkRuntime(trc);
            return;
        }
    }
#else
    AutoCopyFreeListToArenas copy(trc->runtime);
    RecordNativeStackTopForGC(trc->context);
#endif

    



    MarkRuntime(trc);
}

struct IterateArenaCallbackOp
{
    JSContext *cx;
    void *data;
    IterateArenaCallback callback;
    JSGCTraceKind traceKind;
    size_t thingSize;
    IterateArenaCallbackOp(JSContext *cx, void *data, IterateArenaCallback callback,
                           JSGCTraceKind traceKind, size_t thingSize)
        : cx(cx), data(data), callback(callback), traceKind(traceKind), thingSize(thingSize)
    {}
    void operator()(Arena *arena) { (*callback)(cx, data, arena, traceKind, thingSize); }
};

struct IterateCellCallbackOp
{
    JSContext *cx;
    void *data;
    IterateCellCallback callback;
    JSGCTraceKind traceKind;
    size_t thingSize;
    IterateCellCallbackOp(JSContext *cx, void *data, IterateCellCallback callback,
                          JSGCTraceKind traceKind, size_t thingSize)
        : cx(cx), data(data), callback(callback), traceKind(traceKind), thingSize(thingSize)
    {}
    void operator()(Cell *cell) { (*callback)(cx, data, cell, traceKind, thingSize); }
};

void
IterateCompartmentsArenasCells(JSContext *cx, void *data,
                               JSIterateCompartmentCallback compartmentCallback,
                               IterateArenaCallback arenaCallback,
                               IterateCellCallback cellCallback)
{
    CHECK_REQUEST(cx);

    JSRuntime *rt = cx->runtime;
    JS_ASSERT(!rt->gcRunning);

    AutoLockGC lock(rt);
    AutoHeapSession session(cx);
#ifdef JS_THREADSAFE
    rt->gcHelperThread.waitBackgroundSweepEnd();
#endif
    AutoUnlockGC unlock(rt);

    AutoCopyFreeListToArenas copy(rt);
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        (*compartmentCallback)(cx, data, c);

        for (size_t thingKind = 0; thingKind != FINALIZE_LIMIT; thingKind++) {
            JSGCTraceKind traceKind = MapAllocToTraceKind(AllocKind(thingKind));
            size_t thingSize = Arena::thingSize(AllocKind(thingKind));
            IterateArenaCallbackOp arenaOp(cx, data, arenaCallback, traceKind, thingSize);
            IterateCellCallbackOp cellOp(cx, data, cellCallback, traceKind, thingSize);
            ForEachArenaAndCell(c, AllocKind(thingKind), arenaOp, cellOp);
        }
    }
}

void
IterateChunks(JSContext *cx, void *data, IterateChunkCallback chunkCallback)
{
    
    CHECK_REQUEST(cx);

    JSRuntime *rt = cx->runtime;
    JS_ASSERT(!rt->gcRunning);

    AutoLockGC lock(rt);
    AutoHeapSession session(cx);
#ifdef JS_THREADSAFE
    rt->gcHelperThread.waitBackgroundSweepEnd();
#endif
    AutoUnlockGC unlock(rt);

    for (js::GCChunkSet::Range r = rt->gcChunkSet.all(); !r.empty(); r.popFront())
        chunkCallback(cx, data, r.front());
}

void
IterateCells(JSContext *cx, JSCompartment *compartment, AllocKind thingKind,
             void *data, IterateCellCallback cellCallback)
{
    
    CHECK_REQUEST(cx);

    JSRuntime *rt = cx->runtime;
    JS_ASSERT(!rt->gcRunning);

    AutoLockGC lock(rt);
    AutoHeapSession session(cx);
#ifdef JS_THREADSAFE
    rt->gcHelperThread.waitBackgroundSweepEnd();
#endif
    AutoUnlockGC unlock(rt);

    AutoCopyFreeListToArenas copy(rt);

    JSGCTraceKind traceKind = MapAllocToTraceKind(thingKind);
    size_t thingSize = Arena::thingSize(thingKind);

    if (compartment) {
        for (CellIterUnderGC i(compartment, thingKind); !i.done(); i.next())
            cellCallback(cx, data, i.getCell(), traceKind, thingSize);
    } else {
        for (CompartmentsIter c(rt); !c.done(); c.next()) {
            for (CellIterUnderGC i(c, thingKind); !i.done(); i.next())
                cellCallback(cx, data, i.getCell(), traceKind, thingSize);
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
        
        
        compartment->isSystemCompartment = principals && rt->trustedPrincipals() == principals;
        if (principals) {
            compartment->principals = principals;
            JSPRINCIPALS_HOLD(cx, principals);
        }

        compartment->setGCLastBytes(8192, GC_NORMAL);

        



        {
            AutoLockGC lock(rt);

            




            if (rt->gcIncrementalState == MARK) {
                rt->gcCompartmentCreated = true;

                



                if (!rt->gcIncrementalCompartment)
                    compartment->barrierMarker_.start(rt, NULL);
            }

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

    



    rt->gcTriggerCompartment = rt->gcDebugCompartmentGC ? cx->compartment : NULL;
    if (rt->gcTriggerCompartment == rt->atomsCompartment)
        rt->gcTriggerCompartment = NULL;

    RunLastDitchGC(cx);
#endif
}

#if defined(DEBUG) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)

static void
CheckStackRoot(JSTracer *trc, uintptr_t *w)
{
    
#ifdef JS_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED(&w, sizeof(w));
#endif

    ConservativeGCTest test = MarkIfGCThingWord(trc, *w, DONT_MARK_THING);

    if (test == CGCT_VALID) {
        JSContext *iter = NULL;
        bool matched = false;
        JSRuntime *rt = trc->runtime;
        while (JSContext *acx = js_ContextIterator(rt, JS_TRUE, &iter)) {
            for (unsigned i = 0; i < THING_ROOT_COUNT; i++) {
                Root<Cell*> *rooter = acx->thingGCRooters[i];
                while (rooter) {
                    if (rooter->address() == (Cell **) w)
                        matched = true;
                    rooter = rooter->previous();
                }
            }
            CheckRoot *check = acx->checkGCRooters;
            while (check) {
                if (check->contains(static_cast<uint8_t*>(w), sizeof(w)))
                    matched = true;
                check = check->previous();
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

void
CheckStackRoots(JSContext *cx)
{
    AutoCopyFreeListToArenas copy(cx->runtime);

    JSTracer checker;
    JS_TracerInit(&checker, cx, EmptyMarkCallback);

    ThreadData *td = JS_THREAD_DATA(cx);

    ConservativeGCThreadData *ctd = &td->conservativeGC;
    ctd->recordStackTop();

    JS_ASSERT(ctd->hasStackToScan());
    uintptr_t *stackMin, *stackEnd;
#if JS_STACK_GROWTH_DIRECTION > 0
    stackMin = td->nativeStackBase;
    stackEnd = ctd->nativeStackTop;
#else
    stackMin = ctd->nativeStackTop + 1;
    stackEnd = td->nativeStackBase;
#endif

    JS_ASSERT(stackMin <= stackEnd);
    CheckStackRootsRange(&checker, stackMin, stackEnd);
    CheckStackRootsRange(&checker, ctd->registerSnapshot.words,
                         ArrayEnd(ctd->registerSnapshot.words));
}

#endif 

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

typedef HashMap<void *, VerifyNode *> NodeMap;














struct VerifyTracer : JSTracer {
    
    uint64_t number;

    
    uint32_t count;

    
    VerifyNode *curnode;
    VerifyNode *root;
    char *edgeptr;
    char *term;
    NodeMap nodemap;

    VerifyTracer(JSContext *cx) : root(NULL), nodemap(cx) {}
    ~VerifyTracer() { js_free(root); }
};





static void
AccumulateEdge(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    VerifyTracer *trc = (VerifyTracer *)jstrc;

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
MakeNode(VerifyTracer *trc, void *thing, JSGCTraceKind kind)
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
StartVerifyBarriers(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    if (rt->gcVerifyData || rt->gcIncrementalState != NO_INCREMENTAL)
        return;

    AutoLockGC lock(rt);
    AutoHeapSession session(cx);

    if (!IsIncrementalGCSafe(cx))
        return;

#ifdef JS_THREADSAFE
    rt->gcHelperThread.waitBackgroundSweepOrAllocEnd();
#endif

    AutoUnlockGC unlock(rt);

    AutoCopyFreeListToArenas copy(rt);
    RecordNativeStackTopForGC(cx);

    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront())
        r.front()->bitmap.clear();

    for (CompartmentsIter c(rt); !c.done(); c.next())
        c->discardJitCode(cx);

    PurgeRuntime(cx);

    VerifyTracer *trc = new (js_malloc(sizeof(VerifyTracer))) VerifyTracer(cx);

    rt->gcNumber++;
    trc->number = rt->gcNumber;
    trc->count = 0;

    JS_TracerInit(trc, cx, AccumulateEdge);

    const size_t size = 64 * 1024 * 1024;
    trc->root = (VerifyNode *)js_malloc(size);
    JS_ASSERT(trc->root);
    trc->edgeptr = (char *)trc->root;
    trc->term = trc->edgeptr + size;

    trc->nodemap.init();

    
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

    rt->gcVerifyData = trc;
    rt->gcIncrementalState = MARK;
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        c->needsBarrier_ = true;
        c->barrierMarker_.start(rt, NULL);
        c->arenas.prepareForIncrementalGC(c);
    }

    return;

oom:
    rt->gcIncrementalState = NO_INCREMENTAL;
    trc->~VerifyTracer();
    js_free(trc);
}

static void
MarkFromAutorooter(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    static_cast<Cell *>(*thingp)->markIfUnmarked();
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
    VerifyTracer *trc = (VerifyTracer *)jstrc;
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

    



    NodeMap::Ptr p = trc->nodemap.lookup(*thingp);
    JS_ASSERT_IF(!p, IsMarkedOrAllocated(static_cast<Cell *>(*thingp)));
}

static void
CheckReachable(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    VerifyTracer *trc = (VerifyTracer *)jstrc;
    NodeMap::Ptr p = trc->nodemap.lookup(*thingp);
    JS_ASSERT_IF(!p, IsMarkedOrAllocated(static_cast<Cell *>(*thingp)));
}

static void
EndVerifyBarriers(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    AutoLockGC lock(rt);
    AutoHeapSession session(cx);

#ifdef JS_THREADSAFE
    rt->gcHelperThread.waitBackgroundSweepOrAllocEnd();
#endif

    AutoUnlockGC unlock(rt);

    AutoCopyFreeListToArenas copy(rt);
    RecordNativeStackTopForGC(cx);

    VerifyTracer *trc = (VerifyTracer *)rt->gcVerifyData;

    if (!trc)
        return;

    



    JS_ASSERT(trc->number == rt->gcNumber);
    rt->gcNumber++;

    
    for (CompartmentsIter c(rt); !c.done(); c.next())
        c->needsBarrier_ = false;

    for (CompartmentsIter c(rt); !c.done(); c.next())
        c->discardJitCode(cx);

    rt->gcVerifyData = NULL;
    rt->gcIncrementalState = NO_INCREMENTAL;

    JS_TracerInit(trc, cx, MarkFromAutorooter);

    JSContext *iter = NULL;
    while (JSContext *acx = js_ContextIterator(rt, JS_TRUE, &iter)) {
        if (acx->autoGCRooters)
            acx->autoGCRooters->traceAll(trc);
    }

    if (IsIncrementalGCSafe(cx)) {
        



        JS_TracerInit(trc, cx, CheckReachable);
        MarkRuntime(trc, true);

        JS_TracerInit(trc, cx, CheckEdge);

        
        VerifyNode *node = NextNode(trc->root);
        while ((char *)node < trc->edgeptr) {
            trc->curnode = node;
            JS_TraceChildren(trc, node->thing, node->kind);

            if (node->count <= MAX_VERIFIER_EDGES) {
                for (uint32_t i = 0; i < node->count; i++) {
                    void *thing = node->edges[i].thing;
                    JS_ASSERT_IF(thing, IsMarkedOrAllocated(static_cast<Cell *>(thing)));
                }
            }

            node = NextNode(node);
        }
    }

    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        c->barrierMarker_.reset();
        c->barrierMarker_.stop();
    }

    trc->~VerifyTracer();
    js_free(trc);
}

void
FinishVerifier(JSRuntime *rt)
{
    if (VerifyTracer *trc = (VerifyTracer *)rt->gcVerifyData) {
        trc->~VerifyTracer();
        js_free(trc);
    }
}

void
VerifyBarriers(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    if (rt->gcVerifyData)
        EndVerifyBarriers(cx);
    else
        StartVerifyBarriers(cx);
}

void
MaybeVerifyBarriers(JSContext *cx, bool always)
{
    if (cx->runtime->gcZeal() != ZealVerifierValue)
        return;

    uint32_t freq = cx->runtime->gcZealFrequency;

    JSRuntime *rt = cx->runtime;
    if (VerifyTracer *trc = (VerifyTracer *)rt->gcVerifyData) {
        if (++trc->count < freq && !always)
            return;

        EndVerifyBarriers(cx);
    }
    StartVerifyBarriers(cx);
}

#endif 

} 

void
ReleaseAllJITCode(JSContext *cx, JSCompartment *c, bool resetUseCounts)
{
#ifdef JS_METHODJIT
    mjit::ClearAllFrames(c);

# ifdef JS_ION
    ion::InvalidateAll(cx, c);
# endif

    for (CellIter i(cx, c, FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        mjit::ReleaseScriptCode(cx, script);

        




        if (resetUseCounts)
            script->resetUseCount();

# ifdef JS_ION
        ion::FinishInvalidation(cx, script);
# endif
    }
#endif
}

void
ReleaseAllJITCode(JSContext *cx, bool resetUseCounts)
{
    



    for (GCCompartmentsIter c(cx->runtime); !c.done(); c.next())
        ReleaseAllJITCode(cx, c, resetUseCounts);
}

























static void
ReleaseScriptPCCounters(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    JS_ASSERT(rt->scriptPCCounters);

    ScriptOpcodeCountsVector &vec = *rt->scriptPCCounters;

    for (size_t i = 0; i < vec.length(); i++)
        vec[i].counters.destroy(cx);

    cx->delete_(rt->scriptPCCounters);
    rt->scriptPCCounters = NULL;
}

JS_FRIEND_API(void)
StartPCCountProfiling(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    AutoLockGC lock(rt);

    if (rt->profilingScripts)
        return;

    if (rt->scriptPCCounters)
        ReleaseScriptPCCounters(cx);

    ReleaseAllJITCode(cx, false);

    rt->profilingScripts = true;
}

JS_FRIEND_API(void)
StopPCCountProfiling(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    AutoLockGC lock(rt);

    if (!rt->profilingScripts)
        return;
    JS_ASSERT(!rt->scriptPCCounters);

    ReleaseAllJITCode(cx, false);

    ScriptOpcodeCountsVector *vec = cx->new_<ScriptOpcodeCountsVector>(SystemAllocPolicy());
    if (!vec)
        return;

    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        for (CellIter i(cx, c, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            if (script->pcCounters && script->types) {
                ScriptOpcodeCountsPair info;
                info.script = script;
                info.counters.steal(script->pcCounters);
                if (!vec->append(info))
                    info.counters.destroy(cx);
            }
        }
    }

    rt->profilingScripts = false;
    rt->scriptPCCounters = vec;
}

JS_FRIEND_API(void)
PurgePCCounts(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    AutoLockGC lock(rt);

    if (!rt->scriptPCCounters)
        return;
    JS_ASSERT(!rt->profilingScripts);

    ReleaseScriptPCCounters(cx);
}

} 

JS_PUBLIC_API(void)
JS_IterateCompartments(JSContext *cx, void *data,
                       JSIterateCompartmentCallback compartmentCallback)
{
    CHECK_REQUEST(cx);

    JSRuntime *rt = cx->runtime;
    JS_ASSERT(!rt->gcRunning);

    AutoLockGC lock(rt);
    AutoHeapSession session(cx);

    for (CompartmentsIter c(rt); !c.done(); c.next())
        (*compartmentCallback)(cx, data, c);
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

