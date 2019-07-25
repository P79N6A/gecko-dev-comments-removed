

















































#include <math.h>
#include <string.h>     
#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jshash.h"
#include "jsbit.h"
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
#include "jsgcchunk.h"
#include "jsgcmark.h"
#include "jshashtable.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsparse.h"
#include "jsprobes.h"
#include "jsproxy.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstaticcheck.h"
#include "jswatchpoint.h"
#include "jsweakmap.h"
#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#include "methodjit/MethodJIT.h"
#include "vm/String.h"
#include "vm/Debugger.h"

#include "jsobjinlines.h"

#include "vm/String-inl.h"
#include "vm/CallObject-inl.h"

#ifdef MOZ_VALGRIND
# define JS_VALGRIND
#endif
#ifdef JS_VALGRIND
# include <valgrind/memcheck.h>
#endif

using namespace js;
using namespace js::gc;

namespace js {
namespace gc {


AllocKind slotsToThingKind[] = {
      FINALIZE_OBJECT0,  FINALIZE_OBJECT2,  FINALIZE_OBJECT2,  FINALIZE_OBJECT4,
      FINALIZE_OBJECT4,  FINALIZE_OBJECT8,  FINALIZE_OBJECT8,  FINALIZE_OBJECT8,
      FINALIZE_OBJECT8,  FINALIZE_OBJECT12, FINALIZE_OBJECT12, FINALIZE_OBJECT12,
     FINALIZE_OBJECT12, FINALIZE_OBJECT16, FINALIZE_OBJECT16, FINALIZE_OBJECT16,
     FINALIZE_OBJECT16
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(slotsToThingKind) == SLOTS_TO_THING_KIND_LIMIT);

const uint32 Arena::ThingSizes[] = {
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
    sizeof(JSFunction),         
    sizeof(JSScript),           
    sizeof(Shape),              
    sizeof(types::TypeObject),  
#if JS_HAS_XML_SUPPORT
    sizeof(JSXML),              
#endif
    sizeof(JSShortString),      
    sizeof(JSString),           
    sizeof(JSExternalString),   
};

#define OFFSET(type) uint32(sizeof(ArenaHeader) + (ArenaSize - sizeof(ArenaHeader)) % sizeof(type))

const uint32 Arena::FirstThingOffsets[] = {
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
    OFFSET(JSFunction),         
    OFFSET(JSScript),           
    OFFSET(Shape),              
    OFFSET(types::TypeObject),  
#if JS_HAS_XML_SUPPORT
    OFFSET(JSXML),              
#endif
    OFFSET(JSShortString),      
    OFFSET(JSString),           
    OFFSET(JSExternalString),   
};

#undef OFFSET

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
Arena::finalize(JSContext *cx, AllocKind thingKind, size_t thingSize)
{
    
    JS_ASSERT(thingSize % Cell::CellSize == 0);
    JS_ASSERT(thingSize <= 255);

    JS_ASSERT(aheader.allocated());
    JS_ASSERT(thingKind == aheader.getAllocKind());
    JS_ASSERT(thingSize == aheader.getThingSize());
    JS_ASSERT(!aheader.getMarkingDelay()->link);

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
                t->finalize(cx);
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
FinalizeTypedArenas(JSContext *cx, ArenaLists::ArenaList *al, AllocKind thingKind)
{
    




    JS_ASSERT_IF(!al->head, al->cursor == &al->head);
    ArenaLists::ArenaList available;
    ArenaHeader **ap = &al->head;
    size_t thingSize = Arena::thingSize(thingKind);
    while (ArenaHeader *aheader = *ap) {
        bool allClear = aheader->getArena()->finalize<T>(cx, thingKind, thingSize);
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
FinalizeArenas(JSContext *cx, ArenaLists::ArenaList *al, AllocKind thingKind)
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
      case FINALIZE_FUNCTION:
	FinalizeTypedArenas<JSObject>(cx, al, thingKind);
        break;
      case FINALIZE_SCRIPT:
	FinalizeTypedArenas<JSScript>(cx, al, thingKind);
        break;
      case FINALIZE_SHAPE:
	FinalizeTypedArenas<Shape>(cx, al, thingKind);
        break;
      case FINALIZE_TYPE_OBJECT:
	FinalizeTypedArenas<types::TypeObject>(cx, al, thingKind);
        break;
#if JS_HAS_XML_SUPPORT
      case FINALIZE_XML:
	FinalizeTypedArenas<JSXML>(cx, al, thingKind);
        break;
#endif
      case FINALIZE_STRING:
	FinalizeTypedArenas<JSString>(cx, al, thingKind);
        break;
      case FINALIZE_SHORT_STRING:
	FinalizeTypedArenas<JSShortString>(cx, al, thingKind);
        break;
      case FINALIZE_EXTERNAL_STRING:
	FinalizeTypedArenas<JSExternalString>(cx, al, thingKind);
        break;
    }
}

} 
} 

void
Chunk::init(JSRuntime *rt)
{
    info.runtime = rt;
    info.age = 0;
    info.numFree = ArenasPerChunk;

    
    ArenaHeader **prevp = &info.emptyArenaListHead;
    Arena *end = &arenas[JS_ARRAY_LENGTH(arenas)];
    for (Arena *a = &arenas[0]; a != end; ++a) {
#ifdef DEBUG
        memset(a, ArenaSize, JS_FREE_PATTERN);
#endif
        *prevp = &a->aheader;
        a->aheader.setAsNotAllocated();
        prevp = &a->aheader.next;
    }
    *prevp = NULL;

    for (size_t i = 0; i != JS_ARRAY_LENGTH(markingDelay); ++i)
        markingDelay[i].init();

    

    bitmap.clear();

    
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
    Chunk **listHeadp = GetAvailableChunkList(comp);
    JS_ASSERT(!info.prevp);
    JS_ASSERT(!info.next);
    info.prevp = listHeadp;
    Chunk *head = *listHeadp;
    if (head) {
        JS_ASSERT(head->info.prevp == listHeadp);
        head->info.prevp = &info.next;
    }
    info.next = head;
    *listHeadp = this;
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

ArenaHeader *
Chunk::allocateArena(JSContext *cx, AllocKind thingKind)
{
    JSCompartment *comp = cx->compartment;
    JS_ASSERT(hasAvailableArenas());
    ArenaHeader *aheader = info.emptyArenaListHead;
    info.emptyArenaListHead = aheader->next;
    aheader->init(comp, thingKind);
    --info.numFree;

    if (!hasAvailableArenas())
        removeFromAvailableList();

    JSRuntime *rt = info.runtime;
    Probes::resizeHeap(comp, rt->gcBytes, rt->gcBytes + ArenaSize);
    JS_ATOMIC_ADD(&rt->gcBytes, ArenaSize);
    JS_ATOMIC_ADD(&comp->gcBytes, ArenaSize);
    if (comp->gcBytes >= comp->gcTriggerBytes)
        TriggerCompartmentGC(comp);

    return aheader;
}

void
Chunk::releaseArena(ArenaHeader *aheader)
{
    JS_ASSERT(aheader->allocated());
    JSRuntime *rt = info.runtime;
#ifdef JS_THREADSAFE
    AutoLockGC maybeLock;
    if (rt->gcHelperThread.sweeping)
        maybeLock.lock(info.runtime);
#endif
    JSCompartment *comp = aheader->compartment;

    Probes::resizeHeap(comp, rt->gcBytes, rt->gcBytes - ArenaSize);
    JS_ASSERT(size_t(rt->gcBytes) >= ArenaSize);
    JS_ASSERT(size_t(comp->gcBytes) >= ArenaSize);
#ifdef JS_THREADSAFE
    if (rt->gcHelperThread.sweeping) {
        rt->reduceGCTriggerBytes(GC_HEAP_GROWTH_FACTOR * ArenaSize);
        comp->reduceGCTriggerBytes(GC_HEAP_GROWTH_FACTOR * ArenaSize);
    }
#endif
    JS_ATOMIC_ADD(&rt->gcBytes, -int32(ArenaSize));
    JS_ATOMIC_ADD(&comp->gcBytes, -int32(ArenaSize));

    aheader->setAsNotAllocated();
    aheader->next = info.emptyArenaListHead;
    info.emptyArenaListHead = aheader;
    ++info.numFree;
    if (info.numFree == 1) {
        JS_ASSERT(!info.prevp);
        JS_ASSERT(!info.next);
        addToAvailableList(aheader->compartment);
    } else if (!unused()) {
        JS_ASSERT(info.prevp);
    } else {
        rt->gcChunkSet.remove(this);
        removeFromAvailableList();

        





        info.age = 0;
        info.next = rt->gcEmptyChunkListHead;
        rt->gcEmptyChunkListHead = this;
        rt->gcEmptyChunkCount++;
    }
}

inline Chunk *
AllocateGCChunk(JSRuntime *rt)
{
    Chunk *p = static_cast<Chunk *>(AllocGCChunk());
#ifdef MOZ_GCTIMER
    if (p)
        JS_ATOMIC_INCREMENT(&newChunkCount);
#endif
    return p;
}

inline void
ReleaseGCChunk(JSRuntime *rt, Chunk *p)
{
    JS_ASSERT(p);
#ifdef MOZ_GCTIMER
    JS_ATOMIC_INCREMENT(&destroyChunkCount);
#endif
    FreeGCChunk(p);
}


static Chunk *
PickChunk(JSContext *cx)
{
    JSCompartment *comp = cx->compartment;
    JSRuntime *rt = comp->rt;
    Chunk **listHeadp = GetAvailableChunkList(comp);
    Chunk *chunk = *listHeadp;
    if (chunk)
        return chunk;

    



    chunk = rt->gcEmptyChunkListHead;
    if (chunk) {
        JS_ASSERT(chunk->unused());
        JS_ASSERT(!rt->gcChunkSet.has(chunk));
        JS_ASSERT(rt->gcEmptyChunkCount >= 1);
        rt->gcEmptyChunkListHead = chunk->info.next;
        rt->gcEmptyChunkCount--;
    } else {
        chunk = AllocateGCChunk(rt);
        if (!chunk)
            return NULL;

        chunk->init(rt);
        rt->gcChunkAllocationSinceLastGC = true;
    }

    



    GCChunkSet::AddPtr p = rt->gcChunkSet.lookupForAdd(chunk);
    JS_ASSERT(!p);
    if (!rt->gcChunkSet.add(p, chunk)) {
        ReleaseGCChunk(rt, chunk);
        return NULL;
    }

    chunk->info.prevp = NULL;
    chunk->info.next = NULL;
    chunk->addToAvailableList(comp);

    return chunk;
}

static void
ExpireGCChunks(JSRuntime *rt, JSGCInvocationKind gckind)
{
    AutoLockGC lock(rt);

    
    for (Chunk **chunkp = &rt->gcEmptyChunkListHead; *chunkp; ) {
        JS_ASSERT(rt->gcEmptyChunkCount);
        Chunk *chunk = *chunkp;
        JS_ASSERT(chunk->unused());
        JS_ASSERT(!rt->gcChunkSet.has(chunk));
        JS_ASSERT(chunk->info.age <= MAX_EMPTY_CHUNK_AGE);
        if (gckind == GC_SHRINK || chunk->info.age == MAX_EMPTY_CHUNK_AGE) {
            *chunkp = chunk->info.next;
            --rt->gcEmptyChunkCount;
            ReleaseGCChunk(rt, chunk);
        } else {
            
            ++chunk->info.age;
            chunkp = &chunk->info.next;
        }
    }
}

JS_FRIEND_API(bool)
IsAboutToBeFinalized(JSContext *cx, const void *thing)
{
    if (JSAtom::isStatic(thing))
        return false;
    JS_ASSERT(cx);

    JSCompartment *thingCompartment = reinterpret_cast<const Cell *>(thing)->compartment();
    JSRuntime *rt = cx->runtime;
    JS_ASSERT(rt == thingCompartment->rt);
    if (rt->gcCurrentCompartment != NULL && rt->gcCurrentCompartment != thingCompartment)
        return false;

    return !reinterpret_cast<const Cell *>(thing)->isMarked();
}

JS_FRIEND_API(bool)
js_GCThingIsMarked(void *thing, uintN color = BLACK)
{
    JS_ASSERT(thing);
    AssertValidColor(thing, color);
    JS_ASSERT(!JSAtom::isStatic(thing));
    return reinterpret_cast<Cell *>(thing)->isMarked(color);
}


static const int64 JIT_SCRIPT_RELEASE_TYPES_INTERVAL = 60 * 1000 * 1000;

JSBool
js_InitGC(JSRuntime *rt, uint32 maxbytes)
{
    if (!rt->gcChunkSet.init(INITIAL_CHUNK_CAPACITY))
        return false;

    if (!rt->gcRootsHash.init(256))
        return false;

    if (!rt->gcLocksHash.init(256))
        return false;

#ifdef JS_THREADSAFE
    rt->gcLock = JS_NEW_LOCK();
    if (!rt->gcLock)
        return false;
    rt->gcDone = JS_NEW_CONDVAR(rt->gcLock);
    if (!rt->gcDone)
        return false;
    rt->requestDone = JS_NEW_CONDVAR(rt->gcLock);
    if (!rt->requestDone)
        return false;
    if (!rt->gcHelperThread.init(rt))
        return false;
#endif

    



    rt->gcMaxBytes = maxbytes;
    rt->setGCMaxMallocBytes(maxbytes);
    rt->gcEmptyArenaPoolLifespan = 30000;

    



    rt->setGCLastBytes(8192, GC_NORMAL);

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





inline ConservativeGCTest
MarkIfGCThingWord(JSTracer *trc, jsuword w)
{
    






    JS_STATIC_ASSERT(JSID_TYPE_STRING == 0 && JSID_TYPE_OBJECT == 4);
    if (w & 0x3)
        return CGCT_LOWBITSET;

    



    const jsuword JSID_PAYLOAD_MASK = ~jsuword(JSID_TYPE_MASK);
#if JS_BITS_PER_WORD == 32
    jsuword addr = w & JSID_PAYLOAD_MASK;
#elif JS_BITS_PER_WORD == 64
    jsuword addr = w & JSID_PAYLOAD_MASK & JSVAL_PAYLOAD_MASK;
#endif

    Chunk *chunk = Chunk::fromAddress(addr);

    if (!trc->context->runtime->gcChunkSet.has(chunk))
        return CGCT_NOTCHUNK;

    




    if (!Chunk::withinArenasRange(addr))
        return CGCT_NOTARENA;

    ArenaHeader *aheader = &chunk->arenas[Chunk::arenaIndex(addr)].aheader;

    if (!aheader->allocated())
        return CGCT_FREEARENA;

    AllocKind thingKind = aheader->getAllocKind();
    uintptr_t offset = addr & ArenaMask;
    uintptr_t minOffset = Arena::firstThingOffset(thingKind);
    if (offset < minOffset)
        return CGCT_NOTARENA;

    
    uintptr_t shift = (offset - minOffset) % Arena::thingSize(thingKind);
    addr -= shift;

    




    if (InFreeList(aheader, addr))
        return CGCT_NOTLIVE;

    void *thing = reinterpret_cast<void *>(addr);

#ifdef DEBUG
    const char pattern[] = "machine_stack %lx";
    char nameBuf[sizeof(pattern) - 3 + sizeof(addr) * 2];
    JS_snprintf(nameBuf, sizeof(nameBuf), "machine_stack %lx", (unsigned long) addr);
    JS_SET_TRACING_NAME(trc, nameBuf);
#endif
    MarkKind(trc, thing, MapAllocToTraceKind(thingKind));

#ifdef JS_DUMP_CONSERVATIVE_GC_ROOTS
    if (IS_GC_MARKING_TRACER(trc)) {
        GCMarker *marker = static_cast<GCMarker *>(trc);
        if (marker->conservativeDumpFileName)
            marker->conservativeRoots.append(thing);
        if (shift)
            marker->conservativeStats.unaligned++;
    }
#endif
    return CGCT_VALID;
}

static void
MarkWordConservatively(JSTracer *trc, jsuword w)
{
    





#ifdef JS_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED(&w, sizeof(w));
#endif

    MarkIfGCThingWord(trc, w);
}

static void
MarkRangeConservatively(JSTracer *trc, const jsuword *begin, const jsuword *end)
{
    JS_ASSERT(begin <= end);
    for (const jsuword *i = begin; i != end; ++i)
        MarkWordConservatively(trc, *i);
}

static void
MarkThreadDataConservatively(JSTracer *trc, ThreadData *td)
{
    ConservativeGCThreadData *ctd = &td->conservativeGC;
    JS_ASSERT(ctd->hasStackToScan());
    jsuword *stackMin, *stackEnd;
#if JS_STACK_GROWTH_DIRECTION > 0
    stackMin = td->nativeStackBase;
    stackEnd = ctd->nativeStackTop;
#else
    stackMin = ctd->nativeStackTop + 1;
    stackEnd = td->nativeStackBase;
#endif
    JS_ASSERT(stackMin <= stackEnd);
    MarkRangeConservatively(trc, stackMin, stackEnd);
    MarkRangeConservatively(trc, ctd->registerSnapshot.words,
                            JS_ARRAY_END(ctd->registerSnapshot.words));
}

void
MarkStackRangeConservatively(JSTracer *trc, Value *beginv, Value *endv)
{
    const jsuword *begin = beginv->payloadWord();
    const jsuword *end = endv->payloadWord();;
#ifdef JS_NUNBOX32
    



    JS_ASSERT(begin <= end);
    for (const jsuword *i = begin; i != end; i += sizeof(Value)/sizeof(jsuword))
        MarkWordConservatively(trc, *i);
#else
    MarkRangeConservatively(trc, begin, end);
#endif
}

void
MarkConservativeStackRoots(JSTracer *trc)
{
#ifdef JS_THREADSAFE
    for (JSThread::Map::Range r = trc->context->runtime->threads.all(); !r.empty(); r.popFront()) {
        JSThread *thread = r.front().value;
        ConservativeGCThreadData *ctd = &thread->data.conservativeGC;
        if (ctd->hasStackToScan()) {
            JS_ASSERT_IF(!thread->data.requestDepth, thread->suspendCount);
            MarkThreadDataConservatively(trc, &thread->data);
        } else {
            JS_ASSERT(!thread->suspendCount);
            JS_ASSERT(thread->data.requestDepth <= ctd->requestThreshold);
        }
    }
#else
    MarkThreadDataConservatively(trc, &trc->context->runtime->threadData);
#endif
}

JS_NEVER_INLINE void
ConservativeGCThreadData::recordStackTop()
{
    
    jsuword dummy;
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

static inline void
RecordNativeStackTopForGC(JSContext *cx)
{
    ConservativeGCThreadData *ctd = &JS_THREAD_DATA(cx)->conservativeGC;

#ifdef JS_THREADSAFE
    
    JS_ASSERT(cx->thread()->data.requestDepth >= ctd->requestThreshold);
    if (cx->thread()->data.requestDepth == ctd->requestThreshold)
        return;
#endif
    ctd->recordStackTop();
}

} 

#ifdef DEBUG
static void
CheckLeakedRoots(JSRuntime *rt);
#endif

void
js_FinishGC(JSRuntime *rt)
{
    
    for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
        Foreground::delete_(*c);
    rt->compartments.clear();
    rt->atomsCompartment = NULL;

    rt->gcSystemAvailableChunkListHead = NULL;
    rt->gcUserAvailableChunkListHead = NULL;
    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront())
        ReleaseGCChunk(rt, r.front());
    rt->gcChunkSet.clear();
    for (Chunk *chunk = rt->gcEmptyChunkListHead; chunk; ) {
        Chunk *next = chunk->info.next;
        ReleaseGCChunk(rt, chunk);
        chunk = next;
    }
    rt->gcEmptyChunkListHead = NULL;
    rt->gcEmptyChunkCount = 0;

#ifdef JS_THREADSAFE
    rt->gcHelperThread.finish(rt);
#endif

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
    JSBool ok = js_AddRootRT(cx->runtime, Jsvalify(vp), name);
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
    js_WaitForGC(rt);

    return !!rt->gcRootsHash.put((void *)vp,
                                 RootInfo(name, JS_GC_ROOT_VALUE_PTR));
}

JS_FRIEND_API(JSBool)
js_AddGCThingRootRT(JSRuntime *rt, void **rp, const char *name)
{
    






    AutoLockGC lock(rt);
    js_WaitForGC(rt);

    return !!rt->gcRootsHash.put((void *)rp,
                                 RootInfo(name, JS_GC_ROOT_GCTHING_PTR));
}

JS_FRIEND_API(JSBool)
js_RemoveRoot(JSRuntime *rt, void *rp)
{
    



    AutoLockGC lock(rt);
    js_WaitForGC(rt);
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
    uint32 leakedroots = 0;

    
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

uint32
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
JSRuntime::setGCLastBytes(size_t lastBytes, JSGCInvocationKind gckind)
{
    gcLastBytes = lastBytes;

    size_t base = gckind == GC_SHRINK ? lastBytes : Max(lastBytes, GC_ALLOCATION_THRESHOLD);
    float trigger = float(base) * GC_HEAP_GROWTH_FACTOR;
    gcTriggerBytes = size_t(Min(float(gcMaxBytes), trigger));
}

void
JSRuntime::reduceGCTriggerBytes(uint32 amount) {
    JS_ASSERT(amount > 0);
    JS_ASSERT(gcTriggerBytes - amount >= 0);
    if (gcTriggerBytes - amount < GC_ALLOCATION_THRESHOLD * GC_HEAP_GROWTH_FACTOR)
        return;
    gcTriggerBytes -= amount;
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
JSCompartment::reduceGCTriggerBytes(uint32 amount) {
    JS_ASSERT(amount > 0);
    JS_ASSERT(gcTriggerBytes - amount >= 0);
    if (gcTriggerBytes - amount < GC_ALLOCATION_THRESHOLD * GC_HEAP_GROWTH_FACTOR)
        return;
    gcTriggerBytes -= amount;
}

namespace js {
namespace gc {

inline void *
ArenaLists::allocateFromArena(JSContext *cx, AllocKind thingKind)
{
    Chunk *chunk = NULL;

    ArenaList *al = &arenaLists[thingKind];
    AutoLockGC maybeLock;

#ifdef JS_THREADSAFE
    volatile uintptr_t *bfs = &backgroundFinalizeState[thingKind];
    if (*bfs != BFS_DONE) {
        




        maybeLock.lock(cx->runtime);
        for (;;) {
            if (*bfs == BFS_DONE)
                break;

            if (*bfs == BFS_JUST_FINISHED) {
                




                *bfs = BFS_DONE;
                break;
            }

            JS_ASSERT(!*al->cursor);
            chunk = PickChunk(cx);
            if (chunk)
                break;

            




            JS_ASSERT(*bfs == BFS_RUN);
            cx->runtime->gcHelperThread.waitBackgroundSweepEnd(cx->runtime, false);
            JS_ASSERT(*bfs == BFS_JUST_FINISHED || *bfs == BFS_DONE);
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
            return freeLists[thingKind].infallibleAllocate(Arena::thingSize(thingKind));
        }

        
        if (!maybeLock.locked())
            maybeLock.lock(cx->runtime);
        chunk = PickChunk(cx);
        if (!chunk)
            return NULL;
    }

    








    JS_ASSERT(!*al->cursor);
    ArenaHeader *aheader = chunk->allocateArena(cx, thingKind);
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
    FinalizeArenas(cx, &arenaLists[thingKind], thingKind);
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
              thingKind == FINALIZE_FUNCTION            ||
              thingKind == FINALIZE_SHORT_STRING        ||
              thingKind == FINALIZE_STRING);

#ifdef JS_THREADSAFE
    JS_ASSERT(!cx->runtime->gcHelperThread.sweeping);

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
        FinalizeArenas(cx, al, thingKind);
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
    FinalizeArenas(cx, &finalized, thingKind);

    




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

    




    finalizeLater(cx, FINALIZE_FUNCTION);

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
    finalizeNow(cx, FINALIZE_TYPE_OBJECT);
}

void
ArenaLists::finalizeScripts(JSContext *cx)
{
    finalizeNow(cx, FINALIZE_SCRIPT);
}

static void
RunLastDitchGC(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
#ifdef JS_THREADSAFE
    Maybe<AutoUnlockAtomsCompartment> maybeUnlockAtomsCompartment;
    if (cx->compartment == rt->atomsCompartment && rt->atomsCompartmentIsLocked)
        maybeUnlockAtomsCompartment.construct(cx);
#endif
    
    AutoKeepAtoms keep(rt);
    GCREASON(LASTDITCH);
    js_GC(cx, rt->gcTriggerCompartment, GC_NORMAL);

#ifdef JS_THREADSAFE
    if (rt->gcBytes >= rt->gcMaxBytes)
        cx->runtime->gcHelperThread.waitBackgroundSweepEnd(cx->runtime);
#endif
}

inline bool
IsGCAllowed(JSContext *cx)
{
    return !JS_ON_TRACE(cx) && !JS_THREAD_DATA(cx)->waiveGCQuota;
}

 void *
ArenaLists::refillFreeList(JSContext *cx, AllocKind thingKind)
{
    JS_ASSERT(cx->compartment->arenas.freeLists[thingKind].isEmpty());

    



    JSRuntime *rt = cx->runtime;
    JS_ASSERT(!rt->gcRunning);
    if (rt->gcRunning)
        return NULL;

    bool runGC = !!rt->gcIsNeeded;
    for (;;) {
        if (JS_UNLIKELY(runGC) && IsGCAllowed(cx)) {
            RunLastDitchGC(cx);

            
            if (rt->gcBytes > rt->gcMaxBytes)
                break;

            




            size_t thingSize = Arena::thingSize(thingKind);
            if (void *thing = cx->compartment->arenas.allocateFromFreeList(thingKind, thingSize))
                return thing;
        }
        void *thing = cx->compartment->arenas.allocateFromArena(cx, thingKind);
        if (JS_LIKELY(!!thing))
            return thing;

        



        if (runGC || !IsGCAllowed(cx)) {
            AutoLockGC lock(rt);
            GCREASON(REFILL);
            TriggerGC(rt);
            break;
        }
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















GCMarker::GCMarker(JSContext *cx)
  : color(0),
    unmarkedArenaStackTop(MarkingDelay::stackBottom()),
    objStack(cx->runtime->gcMarkStackObjs, sizeof(cx->runtime->gcMarkStackObjs)),
    ropeStack(cx->runtime->gcMarkStackRopes, sizeof(cx->runtime->gcMarkStackRopes)),
    typeStack(cx->runtime->gcMarkStackTypes, sizeof(cx->runtime->gcMarkStackTypes)),
    xmlStack(cx->runtime->gcMarkStackXMLs, sizeof(cx->runtime->gcMarkStackXMLs)),
    largeStack(cx->runtime->gcMarkStackLarges, sizeof(cx->runtime->gcMarkStackLarges))
{
    JS_TRACER_INIT(this, cx, NULL);
    markLaterArenas = 0;
#ifdef JS_DUMP_CONSERVATIVE_GC_ROOTS
    conservativeDumpFileName = getenv("JS_DUMP_CONSERVATIVE_GC_ROOTS");
    memset(&conservativeStats, 0, sizeof(conservativeStats));
#endif
}

GCMarker::~GCMarker()
{
#ifdef JS_DUMP_CONSERVATIVE_GC_ROOTS
    dumpConservativeRoots();
#endif
}

void
GCMarker::delayMarkingChildren(const void *thing)
{
    const Cell *cell = reinterpret_cast<const Cell *>(thing);
    ArenaHeader *aheader = cell->arenaHeader();
    if (aheader->getMarkingDelay()->link) {
        
        return;
    }
    aheader->getMarkingDelay()->link = unmarkedArenaStackTop;
    unmarkedArenaStackTop = aheader;
    markLaterArenas++;
}

static void
MarkDelayedChildren(JSTracer *trc, ArenaHeader *aheader)
{
    AllocKind thingKind = aheader->getAllocKind();
    JSGCTraceKind traceKind = MapAllocToTraceKind(thingKind);
    size_t thingSize = aheader->getThingSize();
    Arena *a = aheader->getArena();
    uintptr_t end = a->thingsEnd();
    for (uintptr_t thing = a->thingsStart(thingKind); thing != end; thing += thingSize) {
        Cell *t = reinterpret_cast<Cell *>(thing);
        if (t->isMarked())
            JS_TraceChildren(trc, t, traceKind);
    }
}

void
GCMarker::markDelayedChildren()
{
    while (unmarkedArenaStackTop != MarkingDelay::stackBottom()) {
        




        ArenaHeader *aheader = unmarkedArenaStackTop;
        unmarkedArenaStackTop = aheader->getMarkingDelay()->link;
        JS_ASSERT(unmarkedArenaStackTop);
        aheader->getMarkingDelay()->link = NULL;
        JS_ASSERT(markLaterArenas);
        markLaterArenas--;
        MarkDelayedChildren(this, aheader);
    }
    JS_ASSERT(!markLaterArenas);
}

} 

#ifdef DEBUG
static void
EmptyMarkCallback(JSTracer *trc, void *thing, JSGCTraceKind kind)
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

    if (ptr && !trc->context->runtime->gcCurrentCompartment) {
        if (!JSAtom::isStatic(ptr)) {
            




            JSTracer checker;
            JS_TRACER_INIT(&checker, trc->context, EmptyMarkCallback);
            ConservativeGCTest test = MarkIfGCThingWord(&checker, reinterpret_cast<jsuword>(ptr));
            if (test != CGCT_VALID && entry.value.name) {
                fprintf(stderr,
"JS API usage error: the address passed to JS_AddNamedRoot currently holds an\n"
"invalid gcthing.  This is usually caused by a missing call to JS_RemoveRoot.\n"
"The root's name is \"%s\".\n",
                        entry.value.name);
            }
            JS_ASSERT(test == CGCT_VALID);
        }
    }
#endif
    JS_SET_TRACING_NAME(trc, entry.value.name ? entry.value.name : "root");
    if (entry.value.type == JS_GC_ROOT_GCTHING_PTR)
        MarkGCThing(trc, *reinterpret_cast<void **>(entry.key));
    else
        MarkValueRaw(trc, *reinterpret_cast<Value *>(entry.key));
}

static void
gc_lock_traversal(const GCLocks::Entry &entry, JSTracer *trc)
{
    JS_ASSERT(entry.value >= 1);
    MarkGCThing(trc, entry.key, "locked object");
}

void
js_TraceStackFrame(JSTracer *trc, StackFrame *fp)
{
    MarkObject(trc, fp->scopeChain(), "scope chain");
    if (fp->isDummyFrame())
        return;
    if (fp->hasArgsObj())
        MarkObject(trc, fp->argsObj(), "arguments");
    MarkScript(trc, fp->script(), "script");
    fp->script()->compartment()->active = true;
    MarkValue(trc, fp->returnValue(), "rval");
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
    gc::MarkObject(trc, *obj, "js::AutoEnumStateRooter.obj");
}

inline void
AutoGCRooter::trace(JSTracer *trc)
{
    switch (tag) {
      case JSVAL:
        MarkValue(trc, static_cast<AutoValueRooter *>(this)->val, "js::AutoValueRooter.val");
        return;

      case PARSER:
        static_cast<Parser *>(this)->trace(trc);
        return;

      case ENUMERATOR:
        static_cast<AutoEnumStateRooter *>(this)->trace(trc);
        return;

      case IDARRAY: {
        JSIdArray *ida = static_cast<AutoIdArray *>(this)->idArray;
        MarkIdRange(trc, ida->length, ida->vector, "js::AutoIdArray.idArray");
        return;
      }

      case DESCRIPTORS: {
        PropDescArray &descriptors =
            static_cast<AutoPropDescArrayRooter *>(this)->descriptors;
        for (size_t i = 0, len = descriptors.length(); i < len; i++) {
            PropDesc &desc = descriptors[i];
            MarkValue(trc, desc.pd, "PropDesc::pd");
            MarkValue(trc, desc.value, "PropDesc::value");
            MarkValue(trc, desc.get, "PropDesc::get");
            MarkValue(trc, desc.set, "PropDesc::set");
        }
        return;
      }

      case DESCRIPTOR : {
        PropertyDescriptor &desc = *static_cast<AutoPropertyDescriptorRooter *>(this);
        if (desc.obj)
            MarkObject(trc, *desc.obj, "Descriptor::obj");
        MarkValue(trc, desc.value, "Descriptor::value");
        if ((desc.attrs & JSPROP_GETTER) && desc.getter)
            MarkObject(trc, *CastAsObject(desc.getter), "Descriptor::get");
        if (desc.attrs & JSPROP_SETTER && desc.setter)
            MarkObject(trc, *CastAsObject(desc.setter), "Descriptor::set");
        return;
      }

      case NAMESPACES: {
        JSXMLArray &array = static_cast<AutoNamespaceArray *>(this)->array;
        MarkObjectRange(trc, array.length, reinterpret_cast<JSObject **>(array.vector),
                        "JSXMLArray.vector");
        array.cursors->trace(trc);
        return;
      }

      case XML:
        js_TraceXML(trc, static_cast<AutoXMLRooter *>(this)->xml);
        return;

      case OBJECT:
        if (JSObject *obj = static_cast<AutoObjectRooter *>(this)->obj)
            MarkObject(trc, *obj, "js::AutoObjectRooter.obj");
        return;

      case ID:
        MarkId(trc, static_cast<AutoIdRooter *>(this)->id_, "js::AutoIdRooter.val");
        return;

      case VALVECTOR: {
        AutoValueVector::VectorImpl &vector = static_cast<AutoValueVector *>(this)->vector;
        MarkValueRange(trc, vector.length(), vector.begin(), "js::AutoValueVector.vector");
        return;
      }

      case STRING:
        if (JSString *str = static_cast<AutoStringRooter *>(this)->str)
            MarkString(trc, str, "js::AutoStringRooter.str");
        return;

      case IDVECTOR: {
        AutoIdVector::VectorImpl &vector = static_cast<AutoIdVector *>(this)->vector;
        MarkIdRange(trc, vector.length(), vector.begin(), "js::AutoIdVector.vector");
        return;
      }

      case SHAPEVECTOR: {
        AutoShapeVector::VectorImpl &vector = static_cast<js::AutoShapeVector *>(this)->vector;
        MarkShapeRange(trc, vector.length(), vector.begin(), "js::AutoShapeVector.vector");
        return;
      }

      case OBJVECTOR: {
        AutoObjectVector::VectorImpl &vector = static_cast<AutoObjectVector *>(this)->vector;
        MarkObjectRange(trc, vector.length(), vector.begin(), "js::AutoObjectVector.vector");
        return;
      }

      case VALARRAY: {
        AutoValueArray *array = static_cast<AutoValueArray *>(this);
        MarkValueRange(trc, array->length(), array->start(), "js::AutoValueArray");
        return;
      }
    }

    JS_ASSERT(tag >= 0);
    MarkValueRange(trc, tag, static_cast<AutoArrayRooter *>(this)->array, "js::AutoArrayRooter.array");
}

namespace js {

JS_FRIEND_API(void)
MarkContext(JSTracer *trc, JSContext *acx)
{
    

    
    if (acx->globalObject && !acx->hasRunOption(JSOPTION_UNROOTED_GLOBAL))
        MarkObject(trc, *acx->globalObject, "global object");
    if (acx->isExceptionPending())
        MarkValue(trc, acx->getPendingException(), "exception");

    for (js::AutoGCRooter *gcr = acx->autoGCRooters; gcr; gcr = gcr->down)
        gcr->trace(trc);

    if (acx->sharpObjectMap.depth > 0)
        js_TraceSharpMap(trc, &acx->sharpObjectMap);

    MarkValue(trc, acx->iterValue, "iterValue");
}

#define PER_COMPARTMENT_OP(rt, op)                                      \
    if ((rt)->gcCurrentCompartment) {                                   \
        JSCompartment *c = (rt)->gcCurrentCompartment;                  \
        op;                                                             \
    } else {                                                            \
        for (JSCompartment **i = rt->compartments.begin(); i != rt->compartments.end(); ++i) { \
            JSCompartment *c = *i;                                      \
            op;                                                         \
        }                                                               \
    }

JS_REQUIRES_STACK void
MarkRuntime(JSTracer *trc)
{
    JSRuntime *rt = trc->context->runtime;

    if (rt->state != JSRTS_LANDING)
        MarkConservativeStackRoots(trc);

    for (RootRange r = rt->gcRootsHash.all(); !r.empty(); r.popFront())
        gc_root_traversal(trc, r.front());

    for (GCLocks::Range r = rt->gcLocksHash.all(); !r.empty(); r.popFront())
        gc_lock_traversal(r.front(), trc);

    js_TraceAtomState(trc);

    JSContext *iter = NULL;
    while (JSContext *acx = js_ContextIterator(rt, JS_TRUE, &iter))
        MarkContext(trc, acx);

    PER_COMPARTMENT_OP(rt, if (c->activeAnalysis) c->markTypes(trc));

#ifdef JS_TRACER
    PER_COMPARTMENT_OP(rt, if (c->hasTraceMonitor()) c->traceMonitor()->mark(trc));
#endif

    for (ThreadDataIter i(rt); !i.empty(); i.popFront())
        i.threadData()->mark(trc);

    



    if (rt->gcExtraRootsTraceOp)
        rt->gcExtraRootsTraceOp(trc, rt->gcExtraRootsData);
}

void
TriggerGC(JSRuntime *rt)
{
    JS_ASSERT(!rt->gcRunning);
    if (rt->gcIsNeeded)
        return;

    



    rt->gcIsNeeded = true;
    rt->gcTriggerCompartment = NULL;
    TriggerAllOperationCallbacks(rt);
}

void
TriggerCompartmentGC(JSCompartment *comp)
{
    JSRuntime *rt = comp->rt;
    JS_ASSERT(!rt->gcRunning);
    GCREASON(COMPARTMENT);

    if (rt->gcZeal()) {
        TriggerGC(rt);
        return;
    }

    if (rt->gcMode != JSGC_MODE_COMPARTMENT || comp == rt->atomsCompartment) {
        
        TriggerGC(rt);
        return;
    }

    if (rt->gcIsNeeded) {
        
        if (rt->gcTriggerCompartment != comp)
            rt->gcTriggerCompartment = NULL;
        return;
    }

    if (rt->gcBytes > 8192 && rt->gcBytes >= 3 * (rt->gcTriggerBytes / 2)) {
        
        TriggerGC(rt);
        return;
    }

    



    rt->gcIsNeeded = true;
    rt->gcTriggerCompartment = comp;
    TriggerAllOperationCallbacks(comp->rt);
}

void
MaybeGC(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    if (rt->gcZeal()) {
        GCREASON(MAYBEGC);
        js_GC(cx, NULL, GC_NORMAL);
        return;
    }

    JSCompartment *comp = cx->compartment;
    if (rt->gcIsNeeded) {
        GCREASON(MAYBEGC);
        js_GC(cx, (comp == rt->gcTriggerCompartment) ? comp : NULL, GC_NORMAL);
        return;
    }

    if (comp->gcBytes > 8192 && comp->gcBytes >= 3 * (comp->gcTriggerBytes / 4)) {
        GCREASON(MAYBEGC);
        js_GC(cx, (rt->gcMode == JSGC_MODE_COMPARTMENT) ? comp : NULL, GC_NORMAL);
        return;
    }

    



    int64 now = PRMJ_Now();
    if (rt->gcNextFullGCTime && rt->gcNextFullGCTime <= now) {
        if (rt->gcChunkAllocationSinceLastGC || rt->gcEmptyChunkListHead) {
            GCREASON(MAYBEGC);
            js_GC(cx, NULL, GC_SHRINK);
        } else {
            rt->gcNextFullGCTime = now + GC_IDLE_FULL_SPAN;
        }
    }
}

} 

#ifdef JS_THREADSAFE

namespace js {

bool
GCHelperThread::init(JSRuntime *rt)
{
    if (!(wakeup = PR_NewCondVar(rt->gcLock)))
        return false;
    if (!(sweepingDone = PR_NewCondVar(rt->gcLock)))
        return false;

    thread = PR_CreateThread(PR_USER_THREAD, threadMain, rt, PR_PRIORITY_NORMAL,
                             PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);
    return !!thread;

}

void
GCHelperThread::finish(JSRuntime *rt)
{
    PRThread *join = NULL;
    {
        AutoLockGC lock(rt);
        if (thread && !shutdown) {
            shutdown = true;
            PR_NotifyCondVar(wakeup);
            join = thread;
        }
    }
    if (join) {
        
        PR_JoinThread(join);
    }
    if (wakeup)
        PR_DestroyCondVar(wakeup);
    if (sweepingDone)
        PR_DestroyCondVar(sweepingDone);
}


void
GCHelperThread::threadMain(void *arg)
{
    JSRuntime *rt = static_cast<JSRuntime *>(arg);
    rt->gcHelperThread.threadLoop(rt);
}

void
GCHelperThread::threadLoop(JSRuntime *rt)
{
    AutoLockGC lock(rt);
    while (!shutdown) {
        




        if (!sweeping)
            PR_WaitCondVar(wakeup, PR_INTERVAL_NO_TIMEOUT);
        if (sweeping) {
            AutoUnlockGC unlock(rt);
            doSweep();
        }
        sweeping = false;
        PR_NotifyAllCondVar(sweepingDone);
    }
}

bool
GCHelperThread::prepareForBackgroundSweep(JSContext *context) {
    size_t maxArenaLists = MAX_BACKGROUND_FINALIZE_KINDS * context->runtime->compartments.length();
    if (!finalizeVector.reserve(maxArenaLists))
        return false;
    cx = context;
    return true;
}

void
GCHelperThread::startBackgroundSweep(JSRuntime *rt, JSGCInvocationKind gckind)
{
    
    JS_ASSERT(!sweeping);
    lastGCKind = gckind;
    sweeping = true;
    PR_NotifyCondVar(wakeup);
}

void
GCHelperThread::waitBackgroundSweepEnd(JSRuntime *rt, bool gcUnlocked)
{
    AutoLockGC maybeLock;
    if (gcUnlocked)
        maybeLock.lock(rt);
    while (sweeping)
        PR_WaitCondVar(sweepingDone, PR_INTERVAL_NO_TIMEOUT);
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
    JS_ASSERT(cx);

    



    for (ArenaHeader **i = finalizeVector.begin(); i != finalizeVector.end(); ++i)
        ArenaLists::backgroundFinalize(cx, *i);
    finalizeVector.resize(0);
    ExpireGCChunks(cx->runtime, lastGCKind);
    cx = NULL;

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

}

#endif 

static void
SweepCrossCompartmentWrappers(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    bool releaseTypes = false;
    int64 now = PRMJ_Now();
    if (now >= rt->gcJitReleaseTime) {
        releaseTypes = true;
        rt->gcJitReleaseTime = now + JIT_SCRIPT_RELEASE_TYPES_INTERVAL;
    }

    






    for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
        (*c)->sweep(cx, releaseTypes);
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
            (compartment->arenas.arenaListsAreEmpty() || gckind == GC_LAST_CONTEXT))
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
MarkAndSweep(JSContext *cx, JSCompartment *comp, JSGCInvocationKind gckind GCTIMER_PARAM)
{
    JS_ASSERT_IF(comp, gckind != GC_LAST_CONTEXT);
    JS_ASSERT_IF(comp, comp != comp->rt->atomsCompartment);
    JS_ASSERT_IF(comp, comp->rt->gcMode == JSGC_MODE_COMPARTMENT);

    JSRuntime *rt = cx->runtime;
    rt->gcNumber++;

    
    rt->gcIsNeeded = false;
    rt->gcTriggerCompartment = NULL;

    
    rt->resetGCMallocBytes();

    




    if (rt->shapeGen & SHAPE_OVERFLOW_BIT || (rt->gcZeal() && !rt->gcCurrentCompartment)) {
        rt->gcRegenShapes = true;
        rt->shapeGen = 0;
        rt->protoHazardShape = 0;
    }

    PER_COMPARTMENT_OP(rt, c->purge(cx));

    js_PurgeThreads(cx);
    {
        JSContext *iter = NULL;
        while (JSContext *acx = js_ContextIterator(rt, JS_TRUE, &iter))
            acx->purge();
    }

    JS_ASSERT_IF(comp, !rt->gcRegenShapes);

    


    GCTIMESTAMP(startMark);
    GCMarker gcmarker(cx);
    JS_ASSERT(IS_GC_MARKING_TRACER(&gcmarker));
    JS_ASSERT(gcmarker.getMarkColor() == BLACK);
    rt->gcMarkingTracer = &gcmarker;

    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront())
        r.front()->bitmap.clear();

    if (comp) {
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
            (*c)->markCrossCompartmentWrappers(&gcmarker);
        Debugger::markCrossCompartmentDebuggerObjectReferents(&gcmarker);
    }

    MarkRuntime(&gcmarker);
    gcmarker.drainMarkStack();

    


    while (WatchpointMap::markAllIteratively(&gcmarker) ||
           WeakMapBase::markAllIteratively(&gcmarker) ||
           Debugger::markAllIteratively(&gcmarker))
    {
        gcmarker.drainMarkStack();
    }

    rt->gcMarkingTracer = NULL;

    if (rt->gcCallback)
        (void) rt->gcCallback(cx, JSGC_MARK_END);

#ifdef DEBUG
    
    if (comp) {
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
            JS_ASSERT_IF(*c != comp && *c != rt->atomsCompartment,
                         (*c)->arenas.checkArenaListAllUnmarked());
    }
#endif

    













    GCTIMESTAMP(startSweep);

    
    WeakMapBase::sweepAll(&gcmarker);

    js_SweepAtomState(cx);

    
    WatchpointMap::sweepAll(cx);

    






    if (comp) {
        Probes::GCStartSweepPhase(comp);
        comp->sweep(cx, 0);
        comp->arenas.finalizeObjects(cx);
        GCTIMESTAMP(sweepObjectEnd);
        comp->arenas.finalizeStrings(cx);
        GCTIMESTAMP(sweepStringEnd);
        comp->arenas.finalizeScripts(cx);
        GCTIMESTAMP(sweepScriptEnd);
        comp->arenas.finalizeShapes(cx);
        GCTIMESTAMP(sweepShapeEnd);
        Probes::GCEndSweepPhase(comp);
    } else {
        




        Probes::GCStartSweepPhase(NULL);

        Debugger::sweepAll(cx);
        SweepCrossCompartmentWrappers(cx);
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++) {
            Probes::GCStartSweepPhase(*c);
            (*c)->arenas.finalizeObjects(cx);
        }

        GCTIMESTAMP(sweepObjectEnd);

        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++)
            (*c)->arenas.finalizeStrings(cx);

        GCTIMESTAMP(sweepStringEnd);

        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++) {
            (*c)->arenas.finalizeScripts(cx);
        }

        GCTIMESTAMP(sweepScriptEnd);

        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++) {
            (*c)->arenas.finalizeShapes(cx);
            Probes::GCEndSweepPhase(*c);
        }

        GCTIMESTAMP(sweepShapeEnd);
    }

#ifdef DEBUG
     PropertyTree::dumpShapes(cx);
#endif

    





    PER_COMPARTMENT_OP(rt, js_SweepScriptFilenames(c));

    if (!comp) {
        SweepCompartments(cx, gckind);

        
        Probes::GCEndSweepPhase(NULL);
    }

#ifndef JS_THREADSAFE
    




    ExpireGCChunks(rt, gckind);
#endif
    GCTIMESTAMP(sweepDestroyEnd);

    if (rt->gcCallback)
        (void) rt->gcCallback(cx, JSGC_FINALIZE_END);
#ifdef DEBUG_srcnotesize
  { extern void DumpSrcNoteSizeHist();
    DumpSrcNoteSizeHist();
    printf("GC HEAP SIZE %lu\n", (unsigned long)rt->gcBytes);
  }
#endif
}

#ifdef JS_THREADSAFE











void
js_WaitForGC(JSRuntime *rt)
{
    if (rt->gcRunning && rt->gcThread->id != js_CurrentThreadId()) {
        do {
            JS_AWAIT_GC_DONE(rt);
        } while (rt->gcRunning);
    }
}





static void
LetOtherGCFinish(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    JS_ASSERT(rt->gcThread);
    JS_ASSERT(cx->thread() != rt->gcThread);

    size_t requestDebit = cx->thread()->data.requestDepth ? 1 : 0;
    JS_ASSERT(requestDebit <= rt->requestCount);
#ifdef JS_TRACER
    JS_ASSERT_IF(requestDebit == 0, !JS_ON_TRACE(cx));
#endif
    if (requestDebit != 0) {
#ifdef JS_TRACER
        if (JS_ON_TRACE(cx)) {
            




            AutoUnlockGC unlock(rt);
            LeaveTrace(cx);
        }
#endif
        rt->requestCount -= requestDebit;
        if (rt->requestCount == 0)
            JS_NOTIFY_REQUEST_DONE(rt);

        



        RecordNativeStackTopForGC(cx);
    }

    



    JS_ASSERT(rt->gcThread);

    





    do {
        JS_AWAIT_GC_DONE(rt);
    } while (rt->gcThread);

    rt->requestCount += requestDebit;
}

#endif

class AutoGCSession {
  public:
    explicit AutoGCSession(JSContext *cx);
    ~AutoGCSession();

  private:
    JSContext   *context;

    
    AutoGCSession(const AutoGCSession&);
    void operator=(const AutoGCSession&);
};








AutoGCSession::AutoGCSession(JSContext *cx)
  : context(cx)
{
    JS_ASSERT(!JS_THREAD_DATA(cx)->noGCOrAllocationCheck);
    JSRuntime *rt = cx->runtime;

#ifdef JS_THREADSAFE
    if (rt->gcThread && rt->gcThread != cx->thread())
        LetOtherGCFinish(cx);
#endif

    JS_ASSERT(!rt->gcRunning);

#ifdef JS_THREADSAFE
    
    JS_ASSERT(!rt->gcThread);
    rt->gcThread = cx->thread();

    





    for (JSThread::Map::Range r = rt->threads.all(); !r.empty(); r.popFront()) {
        JSThread *thread = r.front().value;
        if (thread != cx->thread())
            thread->data.triggerOperationCallback(rt);
    }

    





    size_t requestDebit = cx->thread()->data.requestDepth ? 1 : 0;
    JS_ASSERT(requestDebit <= rt->requestCount);
    if (requestDebit != rt->requestCount) {
        rt->requestCount -= requestDebit;

        do {
            JS_AWAIT_REQUEST_DONE(rt);
        } while (rt->requestCount > 0);
        rt->requestCount += requestDebit;
    }

#endif 

    





    rt->gcRunning = true;
}


AutoGCSession::~AutoGCSession()
{
    JSRuntime *rt = context->runtime;
    rt->gcRunning = false;
#ifdef JS_THREADSAFE
    JS_ASSERT(rt->gcThread == context->thread());
    rt->gcThread = NULL;
    JS_NOTIFY_GC_DONE(rt);
#endif
}







static JS_NEVER_INLINE void
GCCycle(JSContext *cx, JSCompartment *comp, JSGCInvocationKind gckind  GCTIMER_PARAM)
{
    JSRuntime *rt = cx->runtime;

    



    if (rt->gcMarkAndSweep) {
#ifdef JS_THREADSAFE
        JS_ASSERT(rt->gcThread);
        if (rt->gcThread != cx->thread()) {
            
            LetOtherGCFinish(cx);
        }
#endif
        return;
    }

    AutoGCSession gcsession(cx);

    





    if (rt->inOOMReport) {
        JS_ASSERT(gckind != GC_LAST_CONTEXT);
        return;
    }

    



    SwitchToCompartment sc(cx, (JSCompartment *)NULL);

    JS_ASSERT(!rt->gcCurrentCompartment);
    rt->gcCurrentCompartment = comp;

    rt->gcMarkAndSweep = true;
    {
        AutoUnlockGC unlock(rt);

#ifdef JS_THREADSAFE
        



        JS_ASSERT(!cx->gcBackgroundFree);
        rt->gcHelperThread.waitBackgroundSweepEnd(rt);
        if (gckind != GC_LAST_CONTEXT && rt->state != JSRTS_LANDING) {
            if (rt->gcHelperThread.prepareForBackgroundSweep(cx))
                cx->gcBackgroundFree = &rt->gcHelperThread;
        }
#endif
        MarkAndSweep(cx, comp, gckind  GCTIMER_ARG);
    }

#ifdef JS_THREADSAFE
    if (gckind != GC_LAST_CONTEXT && rt->state != JSRTS_LANDING) {
        JS_ASSERT(cx->gcBackgroundFree == &rt->gcHelperThread);
        cx->gcBackgroundFree = NULL;
        rt->gcHelperThread.startBackgroundSweep(rt, gckind);
    } else {
        JS_ASSERT(!cx->gcBackgroundFree);
    }
#endif

    rt->gcMarkAndSweep = false;
    rt->gcRegenShapes = false;
    rt->setGCLastBytes(rt->gcBytes, gckind);
    rt->gcCurrentCompartment = NULL;
    rt->gcWeakMapList = NULL;

    for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
        (*c)->setGCLastBytes((*c)->gcBytes, gckind);
}

struct GCCrashData
{
    int isRegen;
    int isCompartment;
};

void
js_GC(JSContext *cx, JSCompartment *comp, JSGCInvocationKind gckind)
{
    JSRuntime *rt = cx->runtime;

    





    if (rt->state != JSRTS_UP && gckind != GC_LAST_CONTEXT)
        return;

    if (JS_ON_TRACE(cx)) {
        JS_ASSERT(gckind != GC_LAST_CONTEXT);
        return;
    }

    RecordNativeStackTopForGC(cx);

    GCCrashData crashData;
    crashData.isRegen = rt->shapeGen & SHAPE_OVERFLOW_BIT;
    crashData.isCompartment = !!comp;
    crash::SaveCrashData(crash::JS_CRASH_TAG_GC, &crashData, sizeof(crashData));

    GCTIMER_BEGIN(rt, comp);

    struct AutoGCProbe {
        JSCompartment *comp;
        AutoGCProbe(JSCompartment *comp) : comp(comp) {
            Probes::GCStart(comp);
        }
        ~AutoGCProbe() {
            Probes::GCEnd(comp); 
        }
    } autoGCProbe(comp);

    do {
        





        if (JSGCCallback callback = rt->gcCallback) {
            if (!callback(cx, JSGC_BEGIN) && gckind != GC_LAST_CONTEXT)
                return;
        }

        {
#ifdef JS_THREADSAFE
            rt->gcHelperThread.waitBackgroundSweepEnd(rt);
#endif
            
            AutoLockGC lock(rt);
            rt->gcPoke = false;
            GCCycle(cx, comp, gckind  GCTIMER_ARG);
        }

        
        if (JSGCCallback callback = rt->gcCallback)
            (void) callback(cx, JSGC_END);

        



    } while (gckind == GC_LAST_CONTEXT && rt->gcPoke);

    rt->gcNextFullGCTime = PRMJ_Now() + GC_IDLE_FULL_SPAN;

    rt->gcChunkAllocationSinceLastGC = false;
    GCTIMER_END(gckind == GC_LAST_CONTEXT);

    crash::SnapshotGCStack();
}

namespace js {

class AutoCopyFreeListToArenas {
    JSRuntime *rt;

  public:
    AutoCopyFreeListToArenas(JSRuntime *rt)
      : rt(rt) {
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
            (*c)->arenas.copyFreeListsToArenas();
    }

    ~AutoCopyFreeListToArenas() {
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
            (*c)->arenas.clearFreeListsInArenas();
    }
};

void
TraceRuntime(JSTracer *trc)
{
    JS_ASSERT(!IS_GC_MARKING_TRACER(trc));
    LeaveTrace(trc->context);

#ifdef JS_THREADSAFE
    {
        JSContext *cx = trc->context;
        JSRuntime *rt = cx->runtime;
        if (rt->gcThread != cx->thread()) {
            AutoLockGC lock(rt);
            AutoGCSession gcsession(cx);

            rt->gcHelperThread.waitBackgroundSweepEnd(rt, false);
            AutoUnlockGC unlock(rt);

            AutoCopyFreeListToArenas copy(rt);
            RecordNativeStackTopForGC(trc->context);
            MarkRuntime(trc);
            return;
        }
    }
#else
    AutoCopyFreeListToArenas copy(trc->context->runtime);
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
                               IterateCompartmentCallback compartmentCallback,
                               IterateArenaCallback arenaCallback,
                               IterateCellCallback cellCallback)
{
    CHECK_REQUEST(cx);
    LeaveTrace(cx);

    JSRuntime *rt = cx->runtime;
    JS_ASSERT(!rt->gcRunning);

    AutoLockGC lock(rt);
    AutoGCSession gcsession(cx);
#ifdef JS_THREADSAFE
    rt->gcHelperThread.waitBackgroundSweepEnd(rt, false);
#endif
    AutoUnlockGC unlock(rt);

    AutoCopyFreeListToArenas copy(rt);
    for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c) {
        JSCompartment *compartment = *c;
        (*compartmentCallback)(cx, data, compartment);

        for (size_t thingKind = 0; thingKind != FINALIZE_LIMIT; thingKind++) {
            JSGCTraceKind traceKind = MapAllocToTraceKind(AllocKind(thingKind));
            size_t thingSize = Arena::thingSize(AllocKind(thingKind));
            IterateArenaCallbackOp arenaOp(cx, data, arenaCallback, traceKind, thingSize);
            IterateCellCallbackOp cellOp(cx, data, cellCallback, traceKind, thingSize);
            ForEachArenaAndCell(compartment, AllocKind(thingKind), arenaOp, cellOp);
        }
    }
}

void
IterateCells(JSContext *cx, JSCompartment *compartment, AllocKind thingKind,
             void *data, IterateCellCallback cellCallback)
{
    
    CHECK_REQUEST(cx);

    LeaveTrace(cx);

    JSRuntime *rt = cx->runtime;
    JS_ASSERT(!rt->gcRunning);

    AutoLockGC lock(rt);
    AutoGCSession gcsession(cx);
#ifdef JS_THREADSAFE
    rt->gcHelperThread.waitBackgroundSweepEnd(rt, false);
#endif
    AutoUnlockGC unlock(rt);

    AutoCopyFreeListToArenas copy(rt);

    JSGCTraceKind traceKind = MapAllocToTraceKind(thingKind);
    size_t thingSize = Arena::thingSize(thingKind);

    if (compartment) {
        for (CellIterUnderGC i(compartment, thingKind); !i.done(); i.next())
            cellCallback(cx, data, i.getCell(), traceKind, thingSize);
    } else {
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c) {
            for (CellIterUnderGC i(*c, thingKind); !i.done(); i.next())
                cellCallback(cx, data, i.getCell(), traceKind, thingSize);
        }
    }
}

namespace gc {

JSCompartment *
NewCompartment(JSContext *cx, JSPrincipals *principals)
{
    JSRuntime *rt = cx->runtime;
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
    if (IsGCAllowed(cx)) {
        JSRuntime *rt = cx->runtime;

        



        rt->gcTriggerCompartment = rt->gcDebugCompartmentGC ? cx->compartment : NULL;
        if (rt->gcTriggerCompartment == rt->atomsCompartment)
            rt->gcTriggerCompartment = NULL;

        RunLastDitchGC(cx);
    }
#endif
}

} 

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
