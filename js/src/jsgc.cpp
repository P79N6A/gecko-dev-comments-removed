

















































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
#include "jsweakmap.h"
#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#include "methodjit/MethodJIT.h"
#include "vm/String.h"

#include "jsobjinlines.h"

#include "vm/String-inl.h"

#ifdef MOZ_VALGRIND
# define JS_VALGRIND
#endif
#ifdef JS_VALGRIND
# include <valgrind/memcheck.h>
#endif

using namespace js;
using namespace js::gc;




JS_STATIC_ASSERT(JSTRACE_OBJECT == 0);
JS_STATIC_ASSERT(JSTRACE_STRING == 1);
JS_STATIC_ASSERT(JSTRACE_SHAPE  == 2);
JS_STATIC_ASSERT(JSTRACE_XML    == 3);





JS_STATIC_ASSERT(JSTRACE_SHAPE + 1 == JSTRACE_XML);

#ifdef JS_GCMETER
# define METER(x)               ((void) (x))
# define METER_IF(condition, x) ((void) ((condition) && (x)))
#else
# define METER(x)               ((void) 0)
# define METER_IF(condition, x) ((void) 0)
#endif

# define METER_UPDATE_MAX(maxLval, rval)                                       \
    METER_IF((maxLval) < (rval), (maxLval) = (rval))

namespace js {
namespace gc {


FinalizeKind slotsToThingKind[] = {
      FINALIZE_OBJECT0,  FINALIZE_OBJECT2,  FINALIZE_OBJECT2,  FINALIZE_OBJECT4,
      FINALIZE_OBJECT4,  FINALIZE_OBJECT8,  FINALIZE_OBJECT8,  FINALIZE_OBJECT8,
      FINALIZE_OBJECT8,  FINALIZE_OBJECT12, FINALIZE_OBJECT12, FINALIZE_OBJECT12,
     FINALIZE_OBJECT12, FINALIZE_OBJECT16, FINALIZE_OBJECT16, FINALIZE_OBJECT16,
     FINALIZE_OBJECT16
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(slotsToThingKind) == SLOTS_TO_THING_KIND_LIMIT);

const uint8 GCThingSizeMap[] = {
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
    sizeof(Shape),              
#if JS_HAS_XML_SUPPORT
    sizeof(JSXML),              
#endif
    sizeof(JSShortString),      
    sizeof(JSString),           
    sizeof(JSExternalString),   
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(GCThingSizeMap) == FINALIZE_LIMIT);

#ifdef DEBUG
void
ArenaHeader::checkSynchronizedWithFreeList() const
{
    



    JS_ASSERT(compartment);

    




    if (!compartment->rt->gcRunning)
        return;

    FreeSpan firstSpan(address() + firstFreeSpanStart, address() + firstFreeSpanEnd);
    if (firstSpan.isEmpty())
        return;
    FreeSpan *list = &compartment->freeLists.lists[getThingKind()];
    if (list->isEmpty() || firstSpan.arenaAddress() != list->arenaAddress())
        return;

    



    JS_ASSERT(firstSpan.start == list->start);
    JS_ASSERT(firstSpan.end == list->end);
}
#endif

template<typename T>
inline bool
Arena::finalize(JSContext *cx)
{
    JS_ASSERT(aheader.compartment);
    JS_ASSERT(!aheader.getMarkingDelay()->link);

    uintptr_t thing = thingsStart(sizeof(T));
    uintptr_t end = thingsEnd();

    FreeSpan nextFree(aheader.getFirstFreeSpan());
    nextFree.checkSpan();

    FreeSpan newListHead;
    FreeSpan *newListTail = &newListHead;
    uintptr_t newFreeSpanStart = 0;
    bool allClear = true;
#ifdef DEBUG
    size_t nmarked = 0;
#endif
    for (;; thing += sizeof(T)) {
        JS_ASSERT(thing <= end);
        if (thing == nextFree.start) {
            JS_ASSERT(nextFree.end <= end);
            if (nextFree.end == end)
                break;
            JS_ASSERT(Arena::isAligned(nextFree.end, sizeof(T)));
            if (!newFreeSpanStart)
                newFreeSpanStart = thing;
            thing = nextFree.end;
            nextFree = *nextFree.nextSpan();
            nextFree.checkSpan();
        } else {
            T *t = reinterpret_cast<T *>(thing);
            if (t->isMarked()) {
                allClear = false;
#ifdef DEBUG
                nmarked++;
#endif
                if (newFreeSpanStart) {
                    JS_ASSERT(thing >= thingsStart(sizeof(T)) + sizeof(T));
                    newListTail->start = newFreeSpanStart;
                    newListTail->end = thing - sizeof(T);
                    newListTail = newListTail->nextSpanUnchecked();
                    newFreeSpanStart = 0;
                }
            } else {
                if (!newFreeSpanStart)
                    newFreeSpanStart = thing;
                t->finalize(cx);
#ifdef DEBUG
                memset(t, JS_FREE_PATTERN, sizeof(T));
#endif
            }
        }
    }

    if (allClear) {
        JS_ASSERT(newListTail == &newListHead);
        JS_ASSERT(newFreeSpanStart == thingsStart(sizeof(T)));
        return true;
    }

    newListTail->start = newFreeSpanStart ? newFreeSpanStart : nextFree.start;
    JS_ASSERT(Arena::isAligned(newListTail->start, sizeof(T)));
    newListTail->end = end;

#ifdef DEBUG
    size_t nfree = 0;
    for (FreeSpan *span = &newListHead; span != newListTail; span = span->nextSpan()) {
        span->checkSpan();
        JS_ASSERT(Arena::isAligned(span->start, sizeof(T)));
        JS_ASSERT(Arena::isAligned(span->end, sizeof(T)));
        nfree += (span->end - span->start) / sizeof(T) + 1;
        JS_ASSERT(nfree + nmarked <= thingsPerArena(sizeof(T)));
    }
    nfree += (newListTail->end - newListTail->start) / sizeof(T);
    JS_ASSERT(nfree + nmarked == thingsPerArena(sizeof(T)));
#endif
    aheader.setFirstFreeSpan(&newListHead);

    return false;
}





template<typename T>
static void
FinalizeArenas(JSContext *cx, ArenaHeader **listHeadp)
{
    ArenaHeader **ap = listHeadp;
    while (ArenaHeader *aheader = *ap) {
        bool allClear = aheader->getArena()->finalize<T>(cx);
        if (allClear) {
            *ap = aheader->next;
            aheader->chunk()->releaseArena(aheader);
        } else {
            ap = &aheader->next;
        }
    }
}

#ifdef DEBUG
bool
checkArenaListAllUnmarked(JSCompartment *comp)
{
    for (unsigned i = 0; i < FINALIZE_LIMIT; i++) {
        if (comp->arenas[i].markedThingsInArenaList())
            return false;
    }
    return true;
}
#endif

} 
} 

void
JSCompartment::finishArenaLists()
{
    for (unsigned i = 0; i < FINALIZE_LIMIT; i++)
        arenas[i].releaseAll(i);
}

void
Chunk::init(JSRuntime *rt)
{
    info.runtime = rt;
    info.age = 0;
    info.emptyArenaListHead = &arenas[0].aheader;
    ArenaHeader *aheader = &arenas[0].aheader;
    ArenaHeader *last = &arenas[JS_ARRAY_LENGTH(arenas) - 1].aheader;
    while (aheader < last) {
        ArenaHeader *following = reinterpret_cast<ArenaHeader *>(aheader->address() + ArenaSize);
        aheader->next = following;
        aheader->compartment = NULL;
        aheader = following;
    }
    last->next = NULL;
    last->compartment = NULL;
    info.numFree = ArenasPerChunk;
    for (size_t i = 0; i != JS_ARRAY_LENGTH(markingDelay); ++i)
        markingDelay[i].init();
}

bool
Chunk::unused()
{
    return info.numFree == ArenasPerChunk;
}

bool
Chunk::hasAvailableArenas()
{
    return info.numFree > 0;
}

bool
Chunk::withinArenasRange(Cell *cell)
{
    uintptr_t addr = uintptr_t(cell);
    if (addr >= uintptr_t(&arenas[0]) && addr < uintptr_t(&arenas[ArenasPerChunk]))
        return true;
    return false;
}

template <size_t thingSize>
ArenaHeader *
Chunk::allocateArena(JSContext *cx, unsigned thingKind)
{
    JSCompartment *comp = cx->compartment;
    JS_ASSERT(hasAvailableArenas());
    ArenaHeader *aheader = info.emptyArenaListHead;
    info.emptyArenaListHead = aheader->next;
    aheader->init(comp, thingKind, thingSize);
    --info.numFree;

    JSRuntime *rt = info.runtime;
    JS_ATOMIC_ADD(&rt->gcBytes, ArenaSize);
    JS_ATOMIC_ADD(&comp->gcBytes, ArenaSize);
    METER(JS_ATOMIC_INCREMENT(&rt->gcStats.nallarenas));
    if (comp->gcBytes >= comp->gcTriggerBytes)
        TriggerCompartmentGC(comp);

    return aheader;
}

void
Chunk::releaseArena(ArenaHeader *aheader)
{
    JSRuntime *rt = info.runtime;
#ifdef JS_THREADSAFE
    Maybe<AutoLockGC> maybeLock;
    if (rt->gcHelperThread.sweeping)
        maybeLock.construct(info.runtime);
#endif
    JSCompartment *comp = aheader->compartment;
    METER(rt->gcStats.afree++);
    JS_ASSERT(rt->gcStats.nallarenas != 0);
    METER(JS_ATOMIC_DECREMENT(&rt->gcStats.nallarenas));

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
    aheader->next = info.emptyArenaListHead;
    info.emptyArenaListHead = aheader;
    aheader->compartment = NULL;
    ++info.numFree;
    if (unused())
        info.age = 0;
}

JSRuntime *
Chunk::getRuntime()
{
    return info.runtime;
}

inline jsuword
GetGCChunk(JSRuntime *rt)
{
    void *p = rt->gcChunkAllocator->alloc();
#ifdef MOZ_GCTIMER
    if (p)
        JS_ATOMIC_INCREMENT(&newChunkCount);
#endif
    METER_IF(p, rt->gcStats.nchunks++);
    METER_UPDATE_MAX(rt->gcStats.maxnchunks, rt->gcStats.nchunks);
    return reinterpret_cast<jsuword>(p);
}

inline void
ReleaseGCChunk(JSRuntime *rt, jsuword chunk)
{
    void *p = reinterpret_cast<void *>(chunk);
    JS_ASSERT(p);
#ifdef MOZ_GCTIMER
    JS_ATOMIC_INCREMENT(&destroyChunkCount);
#endif
    JS_ASSERT(rt->gcStats.nchunks != 0);
    METER(rt->gcStats.nchunks--);
    rt->gcChunkAllocator->free_(p);
}

inline Chunk *
AllocateGCChunk(JSRuntime *rt)
{
    Chunk *p = (Chunk *)rt->gcChunkAllocator->alloc();
#ifdef MOZ_GCTIMER
    if (p)
        JS_ATOMIC_INCREMENT(&newChunkCount);
#endif
    METER_IF(p, rt->gcStats.nchunks++);
    return p;
}

inline void
ReleaseGCChunk(JSRuntime *rt, Chunk *p)
{
    JS_ASSERT(p);
#ifdef MOZ_GCTIMER
    JS_ATOMIC_INCREMENT(&destroyChunkCount);
#endif
    JS_ASSERT(rt->gcStats.nchunks != 0);
    METER(rt->gcStats.nchunks--);
    rt->gcChunkAllocator->free_(p);
}

inline Chunk *
PickChunk(JSContext *cx)
{
    Chunk *chunk = cx->compartment->chunk;
    if (chunk && chunk->hasAvailableArenas())
        return chunk;

    



    JSRuntime *rt = cx->runtime;
    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront()) {
        chunk = r.front();
        if (chunk->hasAvailableArenas()) {
            cx->compartment->chunk = chunk;
            return chunk;
        }
    }

    chunk = AllocateGCChunk(rt);
    if (!chunk)
        return NULL;

    



    GCChunkSet::AddPtr p = rt->gcChunkSet.lookupForAdd(chunk);
    JS_ASSERT(!p);
    if (!rt->gcChunkSet.add(p, chunk)) {
        ReleaseGCChunk(rt, chunk);
        return NULL;
    }

    chunk->init(rt);
    cx->compartment->chunk = chunk;
    rt->gcChunkAllocationSinceLastGC = true;
    return chunk;
}

static void
ExpireGCChunks(JSRuntime *rt, JSGCInvocationKind gckind)
{
    static const size_t MaxAge = 3;

    
    AutoLockGC lock(rt);

    rt->gcChunksWaitingToExpire = 0;
    for (GCChunkSet::Enum e(rt->gcChunkSet); !e.empty(); e.popFront()) {
        Chunk *chunk = e.front();
        JS_ASSERT(chunk->info.runtime == rt);
        if (chunk->unused()) {
            if (gckind == GC_SHRINK || chunk->info.age++ > MaxAge) {
                e.removeFront();
                ReleaseGCChunk(rt, chunk);
                continue;
            }
            rt->gcChunksWaitingToExpire++;
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






static const int64 JIT_SCRIPT_EIGHTH_LIFETIME = 120 * 1000 * 1000;

JSBool
js_InitGC(JSRuntime *rt, uint32 maxbytes)
{
    



    if (!rt->gcChunkSet.init(16))
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

    rt->gcJitReleaseTime = PRMJ_Now() + JIT_SCRIPT_EIGHTH_LIFETIME;

    METER(PodZero(&rt->gcStats));
    return true;
}

namespace js {

inline bool
InFreeList(ArenaHeader *aheader, uintptr_t addr)
{
    if (!aheader->hasFreeThings())
        return false;

    FreeSpan firstSpan(aheader->getFirstFreeSpan());

    for (FreeSpan *span = &firstSpan;;) {
        
        if (addr < span->start)
            return false;

        




        if (addr <= span->end)
            return true;

        



        span = span->nextSpan();
    }
}

template <typename T>
inline ConservativeGCTest
MarkArenaPtrConservatively(JSTracer *trc, ArenaHeader *aheader, uintptr_t addr)
{
    JS_ASSERT(aheader->compartment);
    JS_ASSERT(sizeof(T) == aheader->getThingSize());

    uintptr_t offset = addr & ArenaMask;
    uintptr_t minOffset = Arena::thingsStartOffset(sizeof(T));
    if (offset < minOffset)
        return CGCT_NOTARENA;

    
    uintptr_t shift = (offset - minOffset) % sizeof(T);
    addr -= shift;

    




    if (InFreeList(aheader, addr))
        return CGCT_NOTLIVE;

    T *thing = reinterpret_cast<T *>(addr);
    MarkRoot(trc, thing, "machine stack");

#ifdef JS_DUMP_CONSERVATIVE_GC_ROOTS
    if (IS_GC_MARKING_TRACER(trc) && static_cast<GCMarker *>(trc)->conservativeDumpFileName)
        static_cast<GCMarker *>(trc)->conservativeRoots.append(thing);
#endif

#if defined JS_DUMP_CONSERVATIVE_GC_ROOTS || defined JS_GCMETER
    if (IS_GC_MARKING_TRACER(trc) && shift)
        static_cast<GCMarker *>(trc)->conservativeStats.unaligned++;
#endif
    return CGCT_VALID;
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

    if (!aheader->compartment)
        return CGCT_FREEARENA;

    ConservativeGCTest test;
    unsigned thingKind = aheader->getThingKind();

    switch (thingKind) {
      case FINALIZE_OBJECT0:
      case FINALIZE_OBJECT0_BACKGROUND:
        test = MarkArenaPtrConservatively<JSObject>(trc, aheader, addr);
        break;
      case FINALIZE_OBJECT2:
      case FINALIZE_OBJECT2_BACKGROUND:
        test = MarkArenaPtrConservatively<JSObject_Slots2>(trc, aheader, addr);
        break;
      case FINALIZE_OBJECT4:
      case FINALIZE_OBJECT4_BACKGROUND:
        test = MarkArenaPtrConservatively<JSObject_Slots4>(trc, aheader, addr);
        break;
      case FINALIZE_OBJECT8:
      case FINALIZE_OBJECT8_BACKGROUND:
        test = MarkArenaPtrConservatively<JSObject_Slots8>(trc, aheader, addr);
        break;
      case FINALIZE_OBJECT12:
      case FINALIZE_OBJECT12_BACKGROUND:
        test = MarkArenaPtrConservatively<JSObject_Slots12>(trc, aheader, addr);
        break;
      case FINALIZE_OBJECT16:
      case FINALIZE_OBJECT16_BACKGROUND:
        test = MarkArenaPtrConservatively<JSObject_Slots16>(trc, aheader, addr);
        break;
      case FINALIZE_STRING:
        test = MarkArenaPtrConservatively<JSString>(trc, aheader, addr);
        break;
      case FINALIZE_EXTERNAL_STRING:
        test = MarkArenaPtrConservatively<JSExternalString>(trc, aheader, addr);
        break;
      case FINALIZE_SHORT_STRING:
        test = MarkArenaPtrConservatively<JSShortString>(trc, aheader, addr);
        break;
      case FINALIZE_FUNCTION:
        test = MarkArenaPtrConservatively<JSFunction>(trc, aheader, addr);
        break;
      case FINALIZE_SHAPE:
        test = MarkArenaPtrConservatively<Shape>(trc, aheader, addr);
        break;
#if JS_HAS_XML_SUPPORT
      case FINALIZE_XML:
        test = MarkArenaPtrConservatively<JSXML>(trc, aheader, addr);
        break;
#endif
      default:
        test = CGCT_WRONGTAG;
        JS_NOT_REACHED("wrong tag");
    }

    return test;
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
#ifdef JS_ARENAMETER
    JS_DumpArenaStats(stdout);
#endif
#ifdef JS_GCMETER
    if (JS_WANT_GC_METER_PRINT)
        js_DumpGCStats(rt, stdout);
#endif

    
    for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c) {
        JSCompartment *comp = *c;
        comp->finishArenaLists();
        Foreground::delete_(comp);
    }
    rt->compartments.clear();
    rt->atomsCompartment = NULL;

    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront())
        ReleaseGCChunk(rt, r.front());
    rt->gcChunkSet.clear();

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

    size_t base = gckind == GC_SHRINK ? lastBytes : Max(lastBytes, GC_ARENA_ALLOCATION_TRIGGER);
    float trigger = float(base) * GC_HEAP_GROWTH_FACTOR;
    gcTriggerBytes = size_t(Min(float(gcMaxBytes), trigger));
}

void
JSRuntime::reduceGCTriggerBytes(uint32 amount) {
    JS_ASSERT(amount > 0);
    JS_ASSERT(gcTriggerBytes - amount >= 0);
    if (gcTriggerBytes - amount < GC_ARENA_ALLOCATION_TRIGGER * GC_HEAP_GROWTH_FACTOR)
        return;
    gcTriggerBytes -= amount;
}

void
JSCompartment::setGCLastBytes(size_t lastBytes, JSGCInvocationKind gckind)
{
    gcLastBytes = lastBytes;

    size_t base = gckind == GC_SHRINK ? lastBytes : Max(lastBytes, GC_ARENA_ALLOCATION_TRIGGER);
    float trigger = float(base) * GC_HEAP_GROWTH_FACTOR;
    gcTriggerBytes = size_t(Min(float(rt->gcMaxBytes), trigger));
}

void
JSCompartment::reduceGCTriggerBytes(uint32 amount) {
    JS_ASSERT(amount > 0);
    JS_ASSERT(gcTriggerBytes - amount >= 0);
    if (gcTriggerBytes - amount < GC_ARENA_ALLOCATION_TRIGGER * GC_HEAP_GROWTH_FACTOR)
        return;
    gcTriggerBytes -= amount;
}

namespace js {
namespace gc {

inline ArenaHeader *
ArenaList::searchForFreeArena()
{
    while (ArenaHeader *aheader = *cursor) {
        cursor = &aheader->next;
        if (aheader->hasFreeThings())
            return aheader;
    }
    return NULL;
}

template <size_t thingSize>
inline ArenaHeader *
ArenaList::getArenaWithFreeList(JSContext *cx, unsigned thingKind)
{
    Chunk *chunk;

#ifdef JS_THREADSAFE
    




    if (backgroundFinalizeState == BFS_DONE) {
      check_arena_list:
        if (ArenaHeader *aheader = searchForFreeArena())
            return aheader;
    }

    AutoLockGC lock(cx->runtime);

    for (;;) {
        if (backgroundFinalizeState == BFS_JUST_FINISHED) {
            




            JS_ASSERT(*cursor);
            backgroundFinalizeState = BFS_DONE;
            goto check_arena_list;
        }

        JS_ASSERT(!*cursor);
        chunk = PickChunk(cx);
        if (chunk || backgroundFinalizeState == BFS_DONE)
            break;

        




        JS_ASSERT(backgroundFinalizeState == BFS_RUN);
        cx->runtime->gcHelperThread.waitBackgroundSweepEnd(cx->runtime, false);
        JS_ASSERT(backgroundFinalizeState == BFS_JUST_FINISHED ||
                  backgroundFinalizeState == BFS_DONE);
    }

#else 

    if (ArenaHeader *aheader = searchForFreeArena())
        return aheader;
    chunk = PickChunk(cx);

#endif 

    if (!chunk) {
        GCREASON(CHUNK);
        TriggerGC(cx->runtime);
        return NULL;
    }

    




    ArenaHeader *aheader = chunk->allocateArena<thingSize>(cx, thingKind);
    aheader->next = head;
    if (cursor == &head)
        cursor = &aheader->next;
    head = aheader;
    return aheader;
}

template<typename T>
void
ArenaList::finalizeNow(JSContext *cx)
{
#ifdef JS_THREADSAFE
    JS_ASSERT(backgroundFinalizeState == BFS_DONE);
#endif
    METER(stats.narenas = uint32(ArenaHeader::CountListLength(head)));
    FinalizeArenas<T>(cx, &head);
    METER(stats.livearenas = uint32(ArenaHeader::CountListLength(head)));
    cursor = &head;
}

#ifdef JS_THREADSAFE
template<typename T>
inline void
ArenaList::finalizeLater(JSContext *cx)
{
    JS_ASSERT_IF(head,
                 head->getThingKind() == FINALIZE_OBJECT0_BACKGROUND  ||
                 head->getThingKind() == FINALIZE_OBJECT2_BACKGROUND  ||
                 head->getThingKind() == FINALIZE_OBJECT4_BACKGROUND  ||
                 head->getThingKind() == FINALIZE_OBJECT8_BACKGROUND  ||
                 head->getThingKind() == FINALIZE_OBJECT12_BACKGROUND ||
                 head->getThingKind() == FINALIZE_OBJECT16_BACKGROUND ||
                 head->getThingKind() == FINALIZE_SHORT_STRING        ||
                 head->getThingKind() == FINALIZE_STRING);
    JS_ASSERT(!cx->runtime->gcHelperThread.sweeping);

    



    JS_ASSERT(backgroundFinalizeState == BFS_DONE ||
              backgroundFinalizeState == BFS_JUST_FINISHED);

    if (head && cx->gcBackgroundFree && cx->gcBackgroundFree->finalizeVector.append(head)) {
        head = NULL;
        cursor = &head;
        backgroundFinalizeState = BFS_RUN;
    } else {
        JS_ASSERT_IF(!head, cursor == &head);
        backgroundFinalizeState = BFS_DONE;
        finalizeNow<T>(cx);
    }
}

 void
ArenaList::backgroundFinalize(JSContext *cx, ArenaHeader *listHead)
{
    JS_ASSERT(listHead);
    unsigned thingKind = listHead->getThingKind();
    JSCompartment *comp = listHead->compartment;
    ArenaList *al = &comp->arenas[thingKind];
    METER(al->stats.narenas = uint32(ArenaHeader::CountListLength(listHead)));

    switch (thingKind) {
      default:
        JS_NOT_REACHED("wrong kind");
        break;
      case FINALIZE_OBJECT0_BACKGROUND:
        FinalizeArenas<JSObject>(cx, &listHead);
        break;
      case FINALIZE_OBJECT2_BACKGROUND:
        FinalizeArenas<JSObject_Slots2>(cx, &listHead);
        break;
      case FINALIZE_OBJECT4_BACKGROUND:
        FinalizeArenas<JSObject_Slots4>(cx, &listHead);
        break;
      case FINALIZE_OBJECT8_BACKGROUND:
        FinalizeArenas<JSObject_Slots8>(cx, &listHead);
        break;
      case FINALIZE_OBJECT12_BACKGROUND:
        FinalizeArenas<JSObject_Slots12>(cx, &listHead);
        break;
      case FINALIZE_OBJECT16_BACKGROUND:
        FinalizeArenas<JSObject_Slots16>(cx, &listHead);
        break;
      case FINALIZE_STRING:
        FinalizeArenas<JSString>(cx, &listHead);
        break;
      case FINALIZE_SHORT_STRING:
        FinalizeArenas<JSShortString>(cx, &listHead);
        break;
    }

    



    METER(al->stats.livearenas = uint32(ArenaHeader::CountListLength(listHead)));

    




    AutoLockGC lock(cx->runtime);
    JS_ASSERT(al->backgroundFinalizeState == BFS_RUN);
    JS_ASSERT(!*al->cursor);
    if (listHead) {
        *al->cursor = listHead;
        al->backgroundFinalizeState = BFS_JUST_FINISHED;
    } else {
        al->backgroundFinalizeState = BFS_DONE;
    }
    METER(UpdateCompartmentGCStats(comp, thingKind));
}

#endif 

#ifdef DEBUG
bool
CheckAllocation(JSContext *cx)
{
#ifdef JS_THREADSAFE
    JS_ASSERT(cx->thread());
#endif
    JS_ASSERT(!cx->runtime->gcRunning);
    return true;
}
#endif

inline bool
NeedLastDitchGC(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    return rt->gcIsNeeded;
}





static bool
RunLastDitchGC(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    METER(rt->gcStats.lastditch++);
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

    return rt->gcBytes < rt->gcMaxBytes;
}

static inline bool
IsGCAllowed(JSContext *cx)
{
    return !JS_ON_TRACE(cx) && !JS_THREAD_DATA(cx)->waiveGCQuota;
}

template <typename T>
inline Cell *
RefillTypedFreeList(JSContext *cx, unsigned thingKind)
{
    JS_ASSERT(!cx->runtime->gcRunning);

    



    if (cx->runtime->gcRunning)
        return NULL;

    JSCompartment *compartment = cx->compartment;
    JS_ASSERT(compartment->freeLists.lists[thingKind].isEmpty());

    bool canGC = IsGCAllowed(cx);
    bool runGC = canGC && JS_UNLIKELY(NeedLastDitchGC(cx));
    for (;;) {
        if (runGC) {
            if (!RunLastDitchGC(cx))
                break;

            




            if (Cell *thing = compartment->freeLists.getNext(thingKind, sizeof(T)))
                return thing;
        }
        ArenaHeader *aheader =
            compartment->arenas[thingKind].getArenaWithFreeList<sizeof(T)>(cx, thingKind);
        if (aheader) {
            JS_ASSERT(sizeof(T) == aheader->getThingSize());
            return compartment->freeLists.populate(aheader, thingKind, sizeof(T));
        }

        



        if (!canGC || runGC)
            break;
        runGC = true;
    }

    METER(cx->runtime->gcStats.fail++);
    js_ReportOutOfMemory(cx);
    return NULL;
}

Cell *
RefillFinalizableFreeList(JSContext *cx, unsigned thingKind)
{
    switch (thingKind) {
      case FINALIZE_OBJECT0:
      case FINALIZE_OBJECT0_BACKGROUND:
        return RefillTypedFreeList<JSObject>(cx, thingKind);
      case FINALIZE_OBJECT2:
      case FINALIZE_OBJECT2_BACKGROUND:
        return RefillTypedFreeList<JSObject_Slots2>(cx, thingKind);
      case FINALIZE_OBJECT4:
      case FINALIZE_OBJECT4_BACKGROUND:
        return RefillTypedFreeList<JSObject_Slots4>(cx, thingKind);
      case FINALIZE_OBJECT8:
      case FINALIZE_OBJECT8_BACKGROUND:
        return RefillTypedFreeList<JSObject_Slots8>(cx, thingKind);
      case FINALIZE_OBJECT12:
      case FINALIZE_OBJECT12_BACKGROUND:
        return RefillTypedFreeList<JSObject_Slots12>(cx, thingKind);
      case FINALIZE_OBJECT16:
      case FINALIZE_OBJECT16_BACKGROUND:
        return RefillTypedFreeList<JSObject_Slots16>(cx, thingKind);
      case FINALIZE_STRING:
        return RefillTypedFreeList<JSString>(cx, thingKind);
      case FINALIZE_EXTERNAL_STRING:
        return RefillTypedFreeList<JSExternalString>(cx, thingKind);
      case FINALIZE_SHORT_STRING:
        return RefillTypedFreeList<JSShortString>(cx, thingKind);
      case FINALIZE_FUNCTION:
        return RefillTypedFreeList<JSFunction>(cx, thingKind);
      case FINALIZE_SHAPE:
        return RefillTypedFreeList<Shape>(cx, thingKind);
#if JS_HAS_XML_SUPPORT
      case FINALIZE_XML:
        return RefillTypedFreeList<JSXML>(cx, thingKind);
#endif
      default:
        JS_NOT_REACHED("bad finalize kind");
        return NULL;
    }
}

} 
} 

uint32
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
    if (GCLocks::Ptr p = rt->gcLocksHash.lookupWithDefault(thing, 0))
        p->value++;
    else
        return false;

    METER(rt->gcStats.lock++);
    return true;
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

        METER(rt->gcStats.unlock++);
    }
}

namespace js {















GCMarker::GCMarker(JSContext *cx)
  : color(0),
    unmarkedArenaStackTop(MarkingDelay::stackBottom()),
    objStack(cx->runtime->gcMarkStackObjs, sizeof(cx->runtime->gcMarkStackObjs)),
    ropeStack(cx->runtime->gcMarkStackRopes, sizeof(cx->runtime->gcMarkStackRopes)),
    xmlStack(cx->runtime->gcMarkStackXMLs, sizeof(cx->runtime->gcMarkStackXMLs)),
    largeStack(cx->runtime->gcMarkStackLarges, sizeof(cx->runtime->gcMarkStackLarges))
{
    JS_TRACER_INIT(this, cx, NULL);
#ifdef DEBUG
    markLaterArenas = 0;
#endif
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
#ifdef JS_GCMETER
    
    context->runtime->gcStats.conservative.add(conservativeStats);
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
    METER(markLaterArenas++);
    METER_UPDATE_MAX(cell->compartment()->rt->gcStats.maxunmarked, markLaterArenas);
}

static void
MarkDelayedChildren(JSTracer *trc, ArenaHeader *aheader)
{
    unsigned traceKind = GetFinalizableTraceKind(aheader->getThingKind());
    size_t thingSize = aheader->getThingSize();
    Arena *a = aheader->getArena();
    uintptr_t end = a->thingsEnd();
    for (uintptr_t thing = a->thingsStart(thingSize); thing != end; thing += thingSize) {
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
#ifdef DEBUG
        JS_ASSERT(markLaterArenas);
        markLaterArenas--;
#endif
        MarkDelayedChildren(this, aheader);
    }
    JS_ASSERT(!markLaterArenas);
}

} 

#ifdef DEBUG
static void
EmptyMarkCallback(JSTracer *trc, void *thing, uint32 kind)
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

    if (ptr) {
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
    js_TraceScript(trc, fp->script());
    fp->script()->compartment->active = true;
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

      case SHAPE:
        MarkShape(trc, static_cast<AutoShapeRooter *>(this)->shape, "js::AutoShapeRooter.val");
        return;

      case PARSER:
        static_cast<Parser *>(this)->trace(trc);
        return;

      case SCRIPT:
        if (JSScript *script = static_cast<AutoScriptRooter *>(this)->script)
            js_TraceScript(trc, script);
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

      case BINDINGS: {
        static_cast<js::AutoBindingsRooter *>(this)->bindings.trace(trc);
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

void
MarkWeakReferences(GCMarker *trc)
{
    trc->drainMarkStack();
    while (js_TraceWatchPoints(trc) || WeakMapBase::markAllIteratively(trc))
        trc->drainMarkStack();
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
    js_MarkTraps(trc);

    JSContext *iter = NULL;
    while (JSContext *acx = js_ContextIterator(rt, JS_TRUE, &iter))
        MarkContext(trc, acx);

#ifdef JS_TRACER
    for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
        if ((*c)->hasTraceMonitor())
            (*c)->traceMonitor()->mark(trc);
#endif

    for (ThreadDataIter i(rt); !i.empty(); i.popFront())
        i.threadData()->mark(trc);

    if (IS_GC_MARKING_TRACER(trc)) {
        GCMarker *gcmarker = static_cast<GCMarker *>(trc);
        MarkWeakReferences(gcmarker);
    }

    



    if (rt->gcExtraRootsTraceOp)
        rt->gcExtraRootsTraceOp(trc, rt->gcExtraRootsData);

#ifdef DEBUG
    if (rt->functionMeterFilename) {
        for (int k = 0; k < 2; k++) {
            typedef JSRuntime::FunctionCountMap HM;
            HM &h = (k == 0) ? rt->methodReadBarrierCountMap : rt->unjoinedFunctionCountMap;
            for (HM::Range r = h.all(); !r.empty(); r.popFront()) {
                JSFunction *fun = r.front().key;
                JS_CALL_OBJECT_TRACER(trc, fun, "FunctionCountMap key");
            }
        }
    }
#endif
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
        if (rt->gcChunkAllocationSinceLastGC || rt->gcChunksWaitingToExpire) {
            GCREASON(MAYBEGC);
            js_GC(cx, NULL, GC_SHRINK);
        } else {
            rt->gcNextFullGCTime = now + GC_IDLE_FULL_SPAN;
        }
    }
}

} 

void
js_DestroyScriptsToGC(JSContext *cx, JSCompartment *comp)
{
    JSScript **listp, *script;

    for (size_t i = 0; i != JS_ARRAY_LENGTH(comp->scriptsToGC); ++i) {
        listp = &comp->scriptsToGC[i];
        while ((script = *listp) != NULL) {
            *listp = script->u.nextToGC;
            script->u.nextToGC = NULL;
            js_DestroyCachedScript(cx, script);
        }
    }
}

void
JSCompartment::finalizeObjectArenaLists(JSContext *cx)
{
    arenas[FINALIZE_OBJECT0]. finalizeNow<JSObject>(cx);
    arenas[FINALIZE_OBJECT2]. finalizeNow<JSObject_Slots2>(cx);
    arenas[FINALIZE_OBJECT4]. finalizeNow<JSObject_Slots4>(cx);
    arenas[FINALIZE_OBJECT8]. finalizeNow<JSObject_Slots8>(cx);
    arenas[FINALIZE_OBJECT12].finalizeNow<JSObject_Slots12>(cx);
    arenas[FINALIZE_OBJECT16].finalizeNow<JSObject_Slots16>(cx);
    arenas[FINALIZE_FUNCTION].finalizeNow<JSFunction>(cx);

#ifdef JS_THREADSAFE
    arenas[FINALIZE_OBJECT0_BACKGROUND]. finalizeLater<JSObject>(cx);
    arenas[FINALIZE_OBJECT2_BACKGROUND]. finalizeLater<JSObject_Slots2>(cx);
    arenas[FINALIZE_OBJECT4_BACKGROUND]. finalizeLater<JSObject_Slots4>(cx);
    arenas[FINALIZE_OBJECT8_BACKGROUND]. finalizeLater<JSObject_Slots8>(cx);
    arenas[FINALIZE_OBJECT12_BACKGROUND].finalizeLater<JSObject_Slots12>(cx);
    arenas[FINALIZE_OBJECT16_BACKGROUND].finalizeLater<JSObject_Slots16>(cx);
#endif

#if JS_HAS_XML_SUPPORT
    arenas[FINALIZE_XML].finalizeNow<JSXML>(cx);
#endif
}

void
JSCompartment::finalizeStringArenaLists(JSContext *cx)
{
#ifdef JS_THREADSAFE
    arenas[FINALIZE_SHORT_STRING].finalizeLater<JSShortString>(cx);
    arenas[FINALIZE_STRING].finalizeLater<JSString>(cx);
#else
    arenas[FINALIZE_SHORT_STRING].finalizeNow<JSShortString>(cx);
    arenas[FINALIZE_STRING].finalizeNow<JSString>(cx);
#endif
    arenas[FINALIZE_EXTERNAL_STRING].finalizeNow<JSExternalString>(cx);
}

void
JSCompartment::finalizeShapeArenaLists(JSContext *cx)
{
    arenas[FINALIZE_SHAPE].finalizeNow<Shape>(cx);
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
    Maybe<AutoLockGC> lock;
    if (gcUnlocked)
        lock.construct(rt);
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
        ArenaList::backgroundFinalize(cx, *i);
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
    




    uint32 releaseInterval = 0;
    int64 now = PRMJ_Now();
    if (now >= rt->gcJitReleaseTime) {
        releaseInterval = 8;
        while (now >= rt->gcJitReleaseTime) {
            if (--releaseInterval == 1)
                rt->gcJitReleaseTime = now;
            rt->gcJitReleaseTime += JIT_SCRIPT_EIGHTH_LIFETIME;
        }
    }

    






    for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
        (*c)->sweep(cx, releaseInterval);
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
            (compartment->arenaListsAreEmpty() || gckind == GC_LAST_CONTEXT))
        {
            compartment->freeLists.checkEmpty();
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

#ifdef JS_DUMP_SCOPE_METERS
    {
        extern void js_DumpScopeMeters(JSRuntime *rt);
        js_DumpScopeMeters(rt);
    }
#endif

    




    if (rt->shapeGen & SHAPE_OVERFLOW_BIT || (rt->gcZeal() && !rt->gcCurrentCompartment)) {
        rt->gcRegenShapes = true;
        rt->shapeGen = 0;
        rt->protoHazardShape = 0;
    }

    if (rt->gcCurrentCompartment) {
        rt->gcCurrentCompartment->purge(cx);
    } else {
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
            (*c)->purge(cx);
    }

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
    } else {
        js_MarkScriptFilenames(rt);
    }

    MarkRuntime(&gcmarker);

    gcmarker.drainMarkStack();

    rt->gcMarkingTracer = NULL;

    if (rt->gcCallback)
        (void) rt->gcCallback(cx, JSGC_MARK_END);

#ifdef DEBUG
    
    if (comp) {
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
            JS_ASSERT_IF(*c != comp && *c != rt->atomsCompartment, checkArenaListAllUnmarked(*c));
    }
#endif

    













    GCTIMESTAMP(startSweep);

    
    WeakMapBase::sweepAll(&gcmarker);

    js_SweepAtomState(cx);

    
    js_SweepWatchPoints(cx);

    






    if (comp) {
        comp->sweep(cx, 0);
        comp->finalizeObjectArenaLists(cx);
        GCTIMESTAMP(sweepObjectEnd);
        comp->finalizeStringArenaLists(cx);
        GCTIMESTAMP(sweepStringEnd);
        comp->finalizeShapeArenaLists(cx);
        GCTIMESTAMP(sweepShapeEnd);
    } else {
        SweepCrossCompartmentWrappers(cx);
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++)
            (*c)->finalizeObjectArenaLists(cx);

        GCTIMESTAMP(sweepObjectEnd);

        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++)
            (*c)->finalizeStringArenaLists(cx);

        GCTIMESTAMP(sweepStringEnd);

        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++)
            (*c)->finalizeShapeArenaLists(cx);

        GCTIMESTAMP(sweepShapeEnd);

#ifdef DEBUG
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
            (*c)->propertyTree.dumpShapeStats();
#endif
#ifdef JS_GCMETER
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
            UpdateAllCompartmentGCStats(*c);
#endif
    }

#ifdef DEBUG
     PropertyTree::dumpShapes(cx);
#endif

    if (!comp) {
        SweepCompartments(cx, gckind);

        





        js_SweepScriptFilenames(rt);
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
            cx->gcBackgroundFree = &rt->gcHelperThread;
            cx->gcBackgroundFree->setContext(cx);
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

    GCTIMER_BEGIN(rt, comp);

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
#ifdef JS_GCMETER
    js_DumpGCStats(cx->runtime, stderr);
#endif
    GCTIMER_END(gckind == GC_LAST_CONTEXT);
}

namespace js {

class AutoCopyFreeListToArenas {
    JSRuntime *rt;

  public:
    AutoCopyFreeListToArenas(JSRuntime *rt)
      : rt(rt) {
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
            (*c)->freeLists.copyToArenas();
    }

    ~AutoCopyFreeListToArenas() {
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
            (*c)->freeLists.clearInArenas();
    }
};

void
TraceRuntime(JSTracer *trc)
{
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

static void
IterateCompartmentCells(JSContext *cx, JSCompartment *comp, uint64 traceKindMask,
                        void *data, IterateCallback callback)
{
    for (unsigned thingKind = 0; thingKind < FINALIZE_LIMIT; thingKind++) {
        size_t traceKind = GetFinalizableTraceKind(thingKind);
        if (traceKindMask && !TraceKindInMask(traceKind, traceKindMask))
            continue;

        size_t thingSize = GCThingSizeMap[thingKind];
        ArenaHeader *aheader = comp->arenas[thingKind].getHead();
        for (; aheader; aheader = aheader->next) {
            Arena *a = aheader->getArena();
            FreeSpan firstSpan(aheader->getFirstFreeSpan());
            FreeSpan *span = &firstSpan;
            for (uintptr_t thing = a->thingsStart(thingSize);; thing += thingSize) {
                JS_ASSERT(thing <= a->thingsEnd());
                if (thing == span->start) {
                    if (!span->hasNext())
                        break;
                    thing = span->end;
                    span = span->nextSpan();
                } else {
                    (*callback)(cx, data, traceKind, reinterpret_cast<void *>(thing));
                }
            }
        }
    }
}

void
IterateCells(JSContext *cx, JSCompartment *comp, uint64 traceKindMask,
             void *data, IterateCallback callback)
{
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
    if (comp) {
        IterateCompartmentCells(cx, comp, traceKindMask, data, callback);
    } else {
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
            IterateCompartmentCells(cx, *c, traceKindMask, data, callback);
    }
}

namespace gc {

JSCompartment *
NewCompartment(JSContext *cx, JSPrincipals *principals)
{
    JSRuntime *rt = cx->runtime;
    JSCompartment *compartment = cx->new_<JSCompartment>(rt);
    if (compartment && compartment->init()) {
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
