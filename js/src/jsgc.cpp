

















































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
#include "jscntxt.h"
#include "jsversion.h"
#include "jsdbg.h"
#include "jsdbgapi.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcchunk.h"
#include "jsgcmark.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsparse.h"
#include "jsproxy.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstaticcheck.h"
#include "jsstr.h"
#include "methodjit/MethodJIT.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#include "jsprobes.h"
#include "jsobjinlines.h"
#include "jshashtable.h"
#include "jsweakmap.h"

#include "jsstrinlines.h"
#include "jscompartment.h"

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

#ifdef DEBUG
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
    sizeof(JSString),           
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(GCThingSizeMap) == FINALIZE_LIMIT);

JS_FRIEND_API(size_t)
ArenaHeader::getThingSize() const
{
    return GCThingSizeMap[getThingKind()];
}
#endif


template<typename T>
inline FreeCell *
Arena<T>::buildFreeList()
{
    T *first = &t.things[0];
    T *last = &t.things[JS_ARRAY_LENGTH(t.things) - 1];
    for (T *thing = first; thing != last;) {
        T *following = thing + 1;
        thing->asFreeCell()->link = following->asFreeCell();
        thing = following;
    }
    last->asFreeCell()->link = NULL;
    return first->asFreeCell();
}

template<typename T>
inline bool
Arena<T>::finalize(JSContext *cx)
{
    JS_ASSERT(aheader.compartment);
    JS_ASSERT(!aheader.getMarkingDelay()->link);
    
    FreeCell *nextFree = aheader.freeList;
    FreeCell *freeList = NULL;
    FreeCell **tailp = &freeList;
    bool allClear = true;

    T *thingsEnd = &t.things[ThingsPerArena-1];
    T *thing = &t.things[0];
    thingsEnd++;

    if (!nextFree) {
        nextFree = thingsEnd->asFreeCell();
    } else {
        JS_ASSERT(thing->asFreeCell() <= nextFree);
        JS_ASSERT(nextFree < thingsEnd->asFreeCell());
    }

    for (;; thing++) {
        if (thing->asFreeCell() == nextFree) {
            if (thing == thingsEnd)
                break;
            nextFree = nextFree->link;
            if (!nextFree) {
                nextFree = thingsEnd->asFreeCell();
            } else {
                JS_ASSERT(thing->asFreeCell() < nextFree);
                JS_ASSERT(nextFree < thingsEnd->asFreeCell());
            }
        } else if (thing->asFreeCell()->isMarked()) {
            allClear = false;
            continue;
        } else {
            thing->finalize(cx);
#ifdef DEBUG
            memset(thing, JS_FREE_PATTERN, sizeof(T));
#endif
        }
        FreeCell *t = thing->asFreeCell();
        *tailp = t;
        tailp = &t->link;
    }

#ifdef DEBUG
    
    unsigned nfree = 0;
    if (freeList) {
        JS_ASSERT(tailp != &freeList);
        FreeCell *t = freeList;
        for (;;) {
            ++nfree;
            if (&t->link == tailp)
                break;
            JS_ASSERT(t < t->link);
            t = t->link;
        }
    }
    if (allClear) {
        JS_ASSERT(nfree == ThingsPerArena);
        JS_ASSERT(freeList == static_cast<Cell *>(&t.things[0]));
        JS_ASSERT(tailp == &t.things[ThingsPerArena-1].asFreeCell()->link);
    } else {
        JS_ASSERT(nfree < ThingsPerArena);
    }
#endif
    *tailp = NULL;
    aheader.freeList = freeList;
    return allClear;
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
Chunk::clearMarkBitmap()
{
    PodZero(&bitmaps[0], ArenasPerChunk);
}

bool
Chunk::init(JSRuntime *rt)
{
    info.runtime = rt;
    info.age = 0;
    info.emptyArenaLists.init();
    info.emptyArenaLists.cellFreeList = &arenas[0].aheader;
#ifdef JS_THREADSAFE
    info.chunkLock = JS_NEW_LOCK();
    if (!info.chunkLock)
        return false;
#endif
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
    return true;
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

template <typename T>
ArenaHeader *
Chunk::allocateArena(JSContext *cx, unsigned thingKind)
{
#ifdef JS_THREADSAFE
    Maybe<AutoLock> maybeLock;
    if (cx->runtime->gcHelperThread.sweeping)
        maybeLock.construct(info.chunkLock);
#endif
    JSCompartment *comp = cx->compartment;
    JS_ASSERT(hasAvailableArenas());
    ArenaHeader *aheader = info.emptyArenaLists.getTypedFreeList(thingKind);
    if (!aheader) {
        aheader = info.emptyArenaLists.getOtherArena();
        aheader->freeList = aheader->getArena<T>()->buildFreeList();
    }
    JS_ASSERT(!aheader->compartment);
    JS_ASSERT(!aheader->getMarkingDelay()->link);
    aheader->compartment = comp;
    aheader->setThingKind(thingKind);
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
    Maybe<AutoLock> maybeLock;
    if (rt->gcHelperThread.sweeping)
        maybeLock.construct(info.chunkLock);
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
    JS_ATOMIC_ADD(&rt->gcBytes, -ArenaSize);
    JS_ATOMIC_ADD(&comp->gcBytes, -ArenaSize);
    info.emptyArenaLists.insert(aheader);
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
#ifdef JS_THREADSAFE
    JS_DESTROY_LOCK(((Chunk *)chunk)->info.chunkLock);
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
#ifdef JS_THREADSAFE
    JS_DESTROY_LOCK(p->info.chunkLock);
#endif
    JS_ASSERT(rt->gcStats.nchunks != 0);
    METER(rt->gcStats.nchunks--);
    rt->gcChunkAllocator->free_(p);
}

static Chunk *
PickChunk(JSRuntime *rt)
{
    Chunk *chunk;
    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront()) {
        if (r.front()->hasAvailableArenas())
            return r.front();
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

    if (!chunk->init(rt)) {
        ReleaseGCChunk(rt, chunk);
        return NULL;
    }

    return chunk;
}

static void
ExpireGCChunks(JSRuntime *rt)
{
    static const size_t MaxAge = 3;

    
    AutoLockGC lock(rt);

    rt->gcChunksWaitingToExpire = 0;
    for (GCChunkSet::Enum e(rt->gcChunkSet); !e.empty(); e.popFront()) {
        Chunk *chunk = e.front();
        JS_ASSERT(chunk->info.runtime == rt);
        if (chunk->unused()) {
            if (chunk->info.age++ > MaxAge) {
                e.removeFront();
                ReleaseGCChunk(rt, chunk);
                continue;
            }
            rt->gcChunksWaitingToExpire++;
        }
    }
}

template <typename T>
static ArenaHeader *
AllocateArena(JSContext *cx, unsigned thingKind)
{
    JSRuntime *rt = cx->runtime;
    AutoLockGC lock(rt);
    Chunk *chunk = cx->compartment->chunk;
    if (!chunk || !chunk->hasAvailableArenas()) {
        chunk = PickChunk(rt);
        if (!chunk) {
            TriggerGC(rt);
            return NULL;
        }
        cx->compartment->chunk = chunk;
    }
    return chunk->allocateArena<T>(cx, thingKind);
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

    rt->gcTriggerFactor = uint32(100.0f * GC_HEAP_GROWTH_FACTOR);

    



    rt->setGCLastBytes(8192);

    rt->gcJitReleaseTime = PRMJ_Now() + JIT_SCRIPT_EIGHTH_LIFETIME;

    METER(PodZero(&rt->gcStats));
    return true;
}

namespace js {

inline bool
InFreeList(ArenaHeader *aheader, void *thing)
{
    for (FreeCell *cursor = aheader->freeList; cursor; cursor = cursor->link) {
        JS_ASSERT(!cursor->isMarked());
        JS_ASSERT_IF(cursor->link, cursor < cursor->link);

        
        if (thing < cursor)
            break;

        
        if (thing == cursor)
            return true;
    }
    return false;
}

template <typename T>
inline ConservativeGCTest
MarkArenaPtrConservatively(JSTracer *trc, ArenaHeader *aheader, uintptr_t addr)
{
    JS_ASSERT(aheader->compartment);
    JS_ASSERT(sizeof(T) == aheader->getThingSize());

    uintptr_t offset = (addr & ArenaMask) - Arena<T>::FirstThingOffset;
    if (offset >= Arena<T>::ThingsSpan)
        return CGCT_NOTARENA;

    
    uintptr_t shift = offset % sizeof(T);
    T *thing = reinterpret_cast<T *>(addr - shift);

    if (InFreeList(aheader, thing))
        return CGCT_NOTLIVE;

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

    rt->gcWeakMapList = NULL;

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
JSRuntime::setGCTriggerFactor(uint32 factor)
{
    JS_ASSERT(factor >= 100);

    gcTriggerFactor = factor;
    setGCLastBytes(gcLastBytes);

    for (JSCompartment **c = compartments.begin(); c != compartments.end(); ++c)
        (*c)->setGCLastBytes(gcLastBytes);
}

void
JSRuntime::setGCLastBytes(size_t lastBytes)
{
    gcLastBytes = lastBytes;

    
    float trigger1 = float(lastBytes) * float(gcTriggerFactor) / 100.0f;
    float trigger2 = float(Max(lastBytes, GC_ARENA_ALLOCATION_TRIGGER)) *
                     GC_HEAP_GROWTH_FACTOR;
    float maxtrigger = Max(trigger1, trigger2);
    gcTriggerBytes = (float(gcMaxBytes) < maxtrigger) ? gcMaxBytes : size_t(maxtrigger);
}

void
JSRuntime::reduceGCTriggerBytes(uint32 amount) {
    JS_ASSERT(amount > 0);
    JS_ASSERT((gcTriggerBytes - amount) > 0);
    if (gcTriggerBytes - amount < GC_ARENA_ALLOCATION_TRIGGER * GC_HEAP_GROWTH_FACTOR)
        return;
    gcTriggerBytes -= amount;
}

void
JSCompartment::setGCLastBytes(size_t lastBytes)
{
    gcLastBytes = lastBytes;

    
    float trigger1 = float(lastBytes) * float(rt->gcTriggerFactor) / 100.0f;
    float trigger2 = float(Max(lastBytes, GC_ARENA_ALLOCATION_TRIGGER)) *
                     GC_HEAP_GROWTH_FACTOR;
    float maxtrigger = Max(trigger1, trigger2);
    gcTriggerBytes = (float(rt->gcMaxBytes) < maxtrigger) ? rt->gcMaxBytes : size_t(maxtrigger);
}

void
JSCompartment::reduceGCTriggerBytes(uint32 amount) {
    JS_ASSERT(amount > 0);
    JS_ASSERT((gcTriggerBytes - amount) > 0);
    if (gcTriggerBytes - amount < GC_ARENA_ALLOCATION_TRIGGER * GC_HEAP_GROWTH_FACTOR)
        return;
    gcTriggerBytes -= amount;
}

void
FreeLists::purge()
{
    



    for (FreeCell ***p = finalizables; p != JS_ARRAY_END(finalizables); ++p)
        *p = NULL;
}

ArenaList *
GetFinalizableArenaList(JSCompartment *c, unsigned thingKind) {
    JS_ASSERT(thingKind < FINALIZE_LIMIT);
    return &c->arenas[thingKind];
}

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
#ifdef JS_GC_ZEAL
    if (rt->gcZeal >= 1)
        return true;
#endif
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
    js_GC(cx, rt->gcTriggerCompartment, GC_NORMAL);

#ifdef JS_THREADSAFE
    if (rt->gcBytes >= rt->gcMaxBytes)
        cx->runtime->gcHelperThread.waitBackgroundSweepEnd(cx->runtime);
#endif

    return rt->gcBytes < rt->gcMaxBytes;
}

template <typename T>
inline bool
RefillTypedFreeList(JSContext *cx, unsigned thingKind)
{
    JSCompartment *compartment = cx->compartment;
    JS_ASSERT_IF(compartment->freeLists.finalizables[thingKind],
                 !*compartment->freeLists.finalizables[thingKind]);

    JS_ASSERT(!cx->runtime->gcRunning);
    if (cx->runtime->gcRunning)
        return false;

    bool canGC = !JS_ON_TRACE(cx) && !JS_THREAD_DATA(cx)->waiveGCQuota;
#ifdef JS_THREADSAFE
    bool waited = false;
#endif

    do {
        if (canGC && JS_UNLIKELY(NeedLastDitchGC(cx))) {
            if (!RunLastDitchGC(cx))
                break;

            




            if (compartment->freeLists.finalizables[thingKind])
                return true;
            canGC = false;
        }

        ArenaList *arenaList = GetFinalizableArenaList(compartment, thingKind);
#ifdef JS_THREADSAFE
try_again:
#endif
        ArenaHeader *aheader = NULL;
        if (!arenaList->hasToBeFinalized) {
            aheader = arenaList->getNextWithFreeList();
            if (aheader) {
                JS_ASSERT(aheader->freeList);
                JS_ASSERT(sizeof(T) == aheader->getThingSize());
                compartment->freeLists.populate(aheader, thingKind);
                return true;
            }
        }

        



        aheader = AllocateArena<T>(cx, thingKind);
        if (aheader) {
            compartment->freeLists.populate(aheader, thingKind);
            arenaList->insert(aheader);
            return true;
        }
#ifdef JS_THREADSAFE
        if (!waited) {
            
            cx->runtime->gcHelperThread.waitBackgroundSweepEnd(cx->runtime);
            waited = true;
            goto try_again;
        }
#endif
    } while (canGC);

    METER(cx->runtime->gcStats.fail++);
    js_ReportOutOfMemory(cx);
    return false;
}

bool
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
        return false;
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

template<typename T>
static void
MarkDelayedChilderen(JSTracer *trc, ArenaHeader *aheader)
{
    Arena<T> *a = aheader->getArena<T>();
    T *end = &a->t.things[Arena<T>::ThingsPerArena];
    for (T* thing = &a->t.things[0]; thing != end; ++thing) {
        if (thing->isMarked())
            js::gc::MarkChildren(trc, thing);
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

        switch (aheader->getThingKind()) {
          case FINALIZE_OBJECT0:
          case FINALIZE_OBJECT0_BACKGROUND:
            MarkDelayedChilderen<JSObject>(this, aheader);
            break;
          case FINALIZE_OBJECT2:
          case FINALIZE_OBJECT2_BACKGROUND:
            MarkDelayedChilderen<JSObject_Slots2>(this, aheader);
            break;
          case FINALIZE_OBJECT4:
          case FINALIZE_OBJECT4_BACKGROUND:
            MarkDelayedChilderen<JSObject_Slots4>(this, aheader);
            break;
          case FINALIZE_OBJECT8:
          case FINALIZE_OBJECT8_BACKGROUND:
            MarkDelayedChilderen<JSObject_Slots8>(this, aheader);
            break;
          case FINALIZE_OBJECT12:
          case FINALIZE_OBJECT12_BACKGROUND:
            MarkDelayedChilderen<JSObject_Slots12>(this, aheader);
            break;
          case FINALIZE_OBJECT16:
          case FINALIZE_OBJECT16_BACKGROUND:
            MarkDelayedChilderen<JSObject_Slots16>(this, aheader);
            break;
          case FINALIZE_STRING:
            MarkDelayedChilderen<JSString>(this, aheader);
            break;
          case FINALIZE_EXTERNAL_STRING:
            MarkDelayedChilderen<JSExternalString>(this, aheader);
            break;
          case FINALIZE_SHORT_STRING:
            JS_NOT_REACHED("no delayed marking");
            break;
          case FINALIZE_FUNCTION:
            MarkDelayedChilderen<JSFunction>(this, aheader);
            break;
          case FINALIZE_SHAPE:
            MarkDelayedChilderen<Shape>(this, aheader);
            break;
#if JS_HAS_XML_SUPPORT
          case FINALIZE_XML:
            MarkDelayedChilderen<JSXML>(this, aheader);
            break;
#endif
          default:
            JS_NOT_REACHED("wrong thingkind");
        }
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
        (*c)->traceMonitor.mark(trc);
#endif

    for (ThreadDataIter i(rt); !i.empty(); i.popFront())
        i.threadData()->mark(trc);

    



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

#ifdef JS_GC_ZEAL
    if (rt->gcZeal >= 1) {
        TriggerGC(rt);
        return;
    }
#endif

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

#ifdef JS_GC_ZEAL
    if (rt->gcZeal > 0) {
        js_GC(cx, NULL, GC_NORMAL);
        return;
    }
#endif

    JSCompartment *comp = cx->compartment;
    if (rt->gcIsNeeded) {
        js_GC(cx, (comp == rt->gcTriggerCompartment) ? comp : NULL, GC_NORMAL);
        return;
    }

    if (comp->gcBytes > 8192 && comp->gcBytes >= 3 * (comp->gcTriggerBytes / 4))
        js_GC(cx, (rt->gcMode == JSGC_MODE_COMPARTMENT) ? comp : NULL, GC_NORMAL);
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

template<typename T>
static void
FinalizeArenaList(JSCompartment *comp, JSContext *cx, JSGCInvocationKind gckind, unsigned thingKind)
{
    JS_STATIC_ASSERT(!(sizeof(T) & Cell::CellMask));
    ArenaList *arenaList = GetFinalizableArenaList(comp, thingKind);
    ArenaHeader **ap = &arenaList->head;

#ifdef JS_GCMETER
    uint32 nlivearenas = 0, nkilledarenas = 0, nthings = 0;
#endif
    while (ArenaHeader *aheader = *ap) {
        JS_ASSERT(aheader->getThingKind() == thingKind);
        JS_ASSERT(aheader->getThingSize() == sizeof(T));
        bool allClear = aheader->getArena<T>()->finalize(cx);
        if (allClear) {
            *ap = aheader->next;
            aheader->chunk()->releaseArena(aheader);
            METER(nkilledarenas++);
        } else {
            ap = &aheader->next;
            METER(nlivearenas++);
        }
    }
    arenaList->cursor = arenaList->head;
    METER(UpdateCompartmentStats(comp, thingKind, nlivearenas, nkilledarenas, nthings));
}

template<typename T>
static void
FinalizeArenaListLater(JSContext *cx, ArenaList *arenaList, ArenaHeader *listHead)
{
    JS_STATIC_ASSERT(!(sizeof(T) & Cell::CellMask));
    JS_ASSERT(arenaList->hasToBeFinalized);
    ArenaHeader **ap = &listHead;
    ArenaHeader *aheader = *ap;
    JS_ASSERT(aheader);
#ifdef DEBUG
    int thingKind = listHead->getThingKind();
    JSCompartment *comp = listHead->compartment;
#endif
    JS_ASSERT(sizeof(T) == listHead->getThingSize());

#ifdef JS_GCMETER
    uint32 nlivearenas = 0, nkilledarenas = 0, nthings = 0;
#endif
    for (;;) {
        bool allClear = aheader->getArena<T>()->finalize(cx);

        



        if (allClear && (aheader != listHead)) {
            *ap = aheader->next;
            aheader->chunk()->releaseArena(aheader);
            METER(nkilledarenas++);
        } else {
            ap = &aheader->next;
            METER(nlivearenas++);
        }
        if (!(aheader = *ap))
            break;
    }
    arenaList->cursor = listHead;
    arenaList->hasToBeFinalized = false;
    METER(UpdateCompartmentStats(comp, thingKind, nlivearenas, nkilledarenas, nthings));
}

void
FinalizeArenaList(JSContext *cx, ArenaList *list, ArenaHeader *listHead)
{
    JS_ASSERT(list->head);
    JS_ASSERT(listHead);
    FinalizeKind kind = js::gc::FinalizeKind(listHead->getThingKind());

    switch (kind) {
      case FINALIZE_OBJECT0:
      case FINALIZE_OBJECT2:
      case FINALIZE_OBJECT4:
      case FINALIZE_OBJECT8:
      case FINALIZE_OBJECT12:
      case FINALIZE_OBJECT16:
      case FINALIZE_FUNCTION:
      case FINALIZE_SHAPE:
      case FINALIZE_EXTERNAL_STRING:
        JS_NOT_REACHED("no background finalization");
        break;
      case FINALIZE_OBJECT0_BACKGROUND:
        FinalizeArenaListLater<JSObject>(cx, list, listHead);
        break;
      case FINALIZE_OBJECT2_BACKGROUND:
        FinalizeArenaListLater<JSObject_Slots2>(cx, list, listHead);
        break;
      case FINALIZE_OBJECT4_BACKGROUND:
        FinalizeArenaListLater<JSObject_Slots4>(cx, list, listHead);
        break;
      case FINALIZE_OBJECT8_BACKGROUND:
        FinalizeArenaListLater<JSObject_Slots8>(cx, list, listHead);
        break;
      case FINALIZE_OBJECT12_BACKGROUND:
        FinalizeArenaListLater<JSObject_Slots12>(cx, list, listHead);
        break;
      case FINALIZE_OBJECT16_BACKGROUND:
        FinalizeArenaListLater<JSObject_Slots16>(cx, list, listHead);
        break;
      case FINALIZE_STRING:
        FinalizeArenaListLater<JSString>(cx, list, listHead);
        break;
      case FINALIZE_SHORT_STRING:
        FinalizeArenaListLater<JSShortString>(cx, list, listHead);
        break;
 #if JS_HAS_XML_SUPPORT
      case FINALIZE_XML:
        JS_NOT_REACHED("no background finalization");
        break;
#endif
      default:
        JS_NOT_REACHED("wrong kind");
    }
}

#ifdef JS_THREADSAFE
template<typename T>
void BackgroundFinalize(JSCompartment *comp, JSContext *cx, JSGCInvocationKind gckind, unsigned thingKind)
{
    ArenaList *list = GetFinalizableArenaList(comp, thingKind);
    if (!(list->head && list->head->next && cx->gcBackgroundFree->finalizeLater(list)))
        FinalizeArenaList<T>(comp, cx, gckind, thingKind);
}
#endif

void
JSCompartment::finalizeObjectArenaLists(JSContext *cx, JSGCInvocationKind gckind)
{
    FinalizeArenaList<JSObject>(this, cx, gckind, FINALIZE_OBJECT0);
    FinalizeArenaList<JSObject_Slots2>(this, cx, gckind, FINALIZE_OBJECT2);
    FinalizeArenaList<JSObject_Slots4>(this, cx, gckind, FINALIZE_OBJECT4);
    FinalizeArenaList<JSObject_Slots8>(this, cx, gckind, FINALIZE_OBJECT8);
    FinalizeArenaList<JSObject_Slots12>(this, cx, gckind, FINALIZE_OBJECT12);
    FinalizeArenaList<JSObject_Slots16>(this, cx, gckind, FINALIZE_OBJECT16);
    FinalizeArenaList<JSFunction>(this, cx, gckind, FINALIZE_FUNCTION);

#ifdef JS_THREADSAFE
    if (cx->gcBackgroundFree && gckind != GC_LAST_CONTEXT && cx->runtime->state != JSRTS_LANDING) {
        BackgroundFinalize<JSObject>(this, cx, gckind, FINALIZE_OBJECT0_BACKGROUND);
        BackgroundFinalize<JSObject_Slots2>(this, cx, gckind, FINALIZE_OBJECT2_BACKGROUND);
        BackgroundFinalize<JSObject_Slots4>(this, cx, gckind, FINALIZE_OBJECT4_BACKGROUND);
        BackgroundFinalize<JSObject_Slots8>(this, cx, gckind, FINALIZE_OBJECT8_BACKGROUND);
        BackgroundFinalize<JSObject_Slots12>(this, cx, gckind, FINALIZE_OBJECT12_BACKGROUND);
        BackgroundFinalize<JSObject_Slots16>(this, cx, gckind, FINALIZE_OBJECT16_BACKGROUND);
    } else {
        FinalizeArenaList<JSObject>(this, cx, gckind, FINALIZE_OBJECT0_BACKGROUND);
        FinalizeArenaList<JSObject_Slots2>(this, cx, gckind, FINALIZE_OBJECT2_BACKGROUND);
        FinalizeArenaList<JSObject_Slots4>(this, cx, gckind, FINALIZE_OBJECT4_BACKGROUND);
        FinalizeArenaList<JSObject_Slots8>(this, cx, gckind, FINALIZE_OBJECT8_BACKGROUND);
        FinalizeArenaList<JSObject_Slots12>(this, cx, gckind, FINALIZE_OBJECT12_BACKGROUND);
        FinalizeArenaList<JSObject_Slots16>(this, cx, gckind, FINALIZE_OBJECT16_BACKGROUND);
    }
#else
    FinalizeArenaList<JSObject>(this, cx, gckind, FINALIZE_OBJECT0_BACKGROUND);
    FinalizeArenaList<JSObject_Slots2>(this, cx, gckind, FINALIZE_OBJECT2_BACKGROUND);
    FinalizeArenaList<JSObject_Slots4>(this, cx, gckind, FINALIZE_OBJECT4_BACKGROUND);
    FinalizeArenaList<JSObject_Slots8>(this, cx, gckind, FINALIZE_OBJECT8_BACKGROUND);
    FinalizeArenaList<JSObject_Slots12>(this, cx, gckind, FINALIZE_OBJECT12_BACKGROUND);
    FinalizeArenaList<JSObject_Slots16>(this, cx, gckind, FINALIZE_OBJECT16_BACKGROUND);
#endif

#if JS_HAS_XML_SUPPORT
    FinalizeArenaList<JSXML>(this, cx, gckind, FINALIZE_XML);
#endif
}

void
JSCompartment::finalizeStringArenaLists(JSContext *cx, JSGCInvocationKind gckind)
{
#ifdef JS_THREADSAFE
    if (cx->gcBackgroundFree && gckind != GC_LAST_CONTEXT && cx->runtime->state != JSRTS_LANDING) {
        BackgroundFinalize<JSShortString>(this, cx, gckind, FINALIZE_SHORT_STRING);
        BackgroundFinalize<JSString>(this, cx, gckind, FINALIZE_STRING);
    } else {
        FinalizeArenaList<JSShortString>(this, cx, gckind, FINALIZE_SHORT_STRING);
        FinalizeArenaList<JSString>(this, cx, gckind, FINALIZE_STRING);
    }
    FinalizeArenaList<JSExternalString>(this, cx, gckind, FINALIZE_EXTERNAL_STRING);
#else
    FinalizeArenaList<JSShortString>(this, cx, gckind, FINALIZE_SHORT_STRING);
    FinalizeArenaList<JSString>(this, cx, gckind, FINALIZE_STRING);
    FinalizeArenaList<JSExternalString>(this, cx, gckind, FINALIZE_EXTERNAL_STRING);
#endif
}

void
JSCompartment::finalizeShapeArenaLists(JSContext *cx, JSGCInvocationKind gckind)
{
    FinalizeArenaList<Shape>(this, cx, gckind, FINALIZE_SHAPE);
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
GCHelperThread::startBackgroundSweep(JSRuntime *rt)
{
    
    JS_ASSERT(!sweeping);
    sweeping = true;
    PR_NotifyCondVar(wakeup);
}

void
GCHelperThread::waitBackgroundSweepEnd(JSRuntime *rt)
{
    AutoLockGC lock(rt);
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
    for (FinalizeListAndHead *i = finalizeVector.begin(); i != finalizeVector.end(); ++i)
        FinalizeArenaList(cx, i->list, i->head);
    finalizeVector.resize(0);
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

        if (compartment->isAboutToBeCollected(gckind)) {
            JS_ASSERT(compartment->freeLists.isEmpty());
            if (callback)
                (void) callback(cx, compartment, JSCOMPARTMENT_DESTROY);
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
PreGCCleanup(JSContext *cx, JSGCInvocationKind gckind)
{
    JSRuntime *rt = cx->runtime;

    
    rt->gcIsNeeded = false;
    rt->gcTriggerCompartment = NULL;

    
    rt->resetGCMallocBytes();

#ifdef JS_DUMP_SCOPE_METERS
    {
        extern void js_DumpScopeMeters(JSRuntime *rt);
        js_DumpScopeMeters(rt);
    }
#endif

    




    if (rt->shapeGen & SHAPE_OVERFLOW_BIT
#ifdef JS_GC_ZEAL
        || rt->gcZeal >= 1
#endif
        ) {
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
}









static void
MarkAndSweep(JSContext *cx, JSCompartment *comp, JSGCInvocationKind gckind GCTIMER_PARAM)
{
    JSRuntime *rt = cx->runtime;
    rt->gcNumber++;
    JS_ASSERT_IF(comp, !rt->gcRegenShapes);
    JS_ASSERT_IF(comp, gckind != GC_LAST_CONTEXT);
    JS_ASSERT_IF(comp, comp != rt->atomsCompartment);
    JS_ASSERT_IF(comp, comp->rt->gcMode == JSGC_MODE_COMPARTMENT);

    


    GCMarker gcmarker(cx);
    JS_ASSERT(IS_GC_MARKING_TRACER(&gcmarker));
    JS_ASSERT(gcmarker.getMarkColor() == BLACK);
    rt->gcMarkingTracer = &gcmarker;
#ifdef JS_THREADSAFE
    



    if (!cx->gcBackgroundFree) {
        
        rt->gcHelperThread.waitBackgroundSweepEnd(rt);
        cx->gcBackgroundFree = &rt->gcHelperThread;
    } else {
        rt->gcHelperThread.waitBackgroundSweepEnd(rt);
    }
    JS_ASSERT(!rt->gcHelperThread.sweeping);
    cx->gcBackgroundFree->setContext(cx);
#endif
    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront())
         r.front()->clearMarkBitmap();

    if (comp) {
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
            (*c)->markCrossCompartmentWrappers(&gcmarker);
    } else {
        js_MarkScriptFilenames(rt);
    }

    MarkRuntime(&gcmarker);

    gcmarker.drainMarkStack();

    


    while (true) {
        if (!js_TraceWatchPoints(&gcmarker) &&
            !WeakMap::markIteratively(&gcmarker) &&
            !Debug::mark(&gcmarker, comp, gckind))
        {
            break;
        }
        gcmarker.drainMarkStack();
    }

    rt->gcMarkingTracer = NULL;

    if (rt->gcCallback)
        (void) rt->gcCallback(cx, JSGC_MARK_END);

#ifdef DEBUG
    
    if (comp) {
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
            JS_ASSERT_IF(*c != comp && *c != rt->atomsCompartment, checkArenaListAllUnmarked(*c));
    }
#endif

    













    TIMESTAMP(startSweep);

    
    WeakMap::sweep(cx);

    js_SweepAtomState(cx);

    
    js_SweepWatchPoints(cx);

    if (comp)
        Debug::sweepCompartment(comp);
    else
        Debug::sweepAll(rt);

    






    if (comp) {
        comp->sweep(cx, 0);
        comp->finalizeObjectArenaLists(cx, gckind);
        TIMESTAMP(sweepObjectEnd);
        comp->finalizeStringArenaLists(cx, gckind);
        TIMESTAMP(sweepStringEnd);
        comp->finalizeShapeArenaLists(cx, gckind);
        TIMESTAMP(sweepShapeEnd);
    } else {
        SweepCrossCompartmentWrappers(cx);
        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++)
            (*c)->finalizeObjectArenaLists(cx, gckind);

        TIMESTAMP(sweepObjectEnd);

        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++)
            (*c)->finalizeStringArenaLists(cx, gckind);

        TIMESTAMP(sweepStringEnd);

        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++)
            (*c)->finalizeShapeArenaLists(cx, gckind);

        TIMESTAMP(sweepShapeEnd);

        for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
            (*c)->propertyTree.dumpShapeStats();
    }

    PropertyTree::dumpShapes(cx);

    if (!comp) {
        SweepCompartments(cx, gckind);

        





        js_SweepScriptFilenames(rt);
    }

    



    ExpireGCChunks(rt);
    TIMESTAMP(sweepDestroyEnd);

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





static void
GCUntilDone(JSContext *cx, JSCompartment *comp, JSGCInvocationKind gckind  GCTIMER_PARAM)
{
    if (JS_ON_TRACE(cx))
        return;

    JSRuntime *rt = cx->runtime;

    
    if (rt->gcMarkAndSweep) {
        rt->gcPoke = true;
#ifdef JS_THREADSAFE
        JS_ASSERT(rt->gcThread);
        if (rt->gcThread != cx->thread()) {
            
            LetOtherGCFinish(cx);
        }
#endif
        return;
    }

    AutoGCSession gcsession(cx);

    



    SwitchToCompartment sc(cx, (JSCompartment *)NULL);

    JS_ASSERT(!rt->gcCurrentCompartment);
    rt->gcCurrentCompartment = comp;

    METER(rt->gcStats.poke++);

    bool firstRun = true;
    rt->gcMarkAndSweep = true;
#ifdef JS_THREADSAFE
    JS_ASSERT(!cx->gcBackgroundFree);
#endif
    do {
        rt->gcPoke = false;

        AutoUnlockGC unlock(rt);
        if (firstRun) {
            PreGCCleanup(cx, gckind);
            TIMESTAMP(startMark);
            firstRun = false;
        }

        MarkAndSweep(cx, comp, gckind  GCTIMER_ARG);

#ifdef JS_THREADSAFE
        JS_ASSERT(cx->gcBackgroundFree == &rt->gcHelperThread);
        if (rt->gcPoke) {
            AutoLockGC lock(rt);
            cx->gcBackgroundFree = NULL;
            rt->gcHelperThread.startBackgroundSweep(rt);
        }
#endif

        
        
        
        
    } while (rt->gcPoke);

#ifdef JS_THREADSAFE
    JS_ASSERT(cx->gcBackgroundFree == &rt->gcHelperThread);
    cx->gcBackgroundFree = NULL;
    rt->gcHelperThread.startBackgroundSweep(rt);
#endif

    rt->gcMarkAndSweep = false;
    rt->gcRegenShapes = false;
    rt->setGCLastBytes(rt->gcBytes);
    rt->gcCurrentCompartment = NULL;

    for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c)
        (*c)->setGCLastBytes((*c)->gcBytes);
}

void
js_GC(JSContext *cx, JSCompartment *comp, JSGCInvocationKind gckind)
{
    JSRuntime *rt = cx->runtime;

    





    if (rt->state != JSRTS_UP && gckind != GC_LAST_CONTEXT)
        return;

    RecordNativeStackTopForGC(cx);

#ifdef DEBUG
    int stackDummy;
# if JS_STACK_GROWTH_DIRECTION > 0
    
    JS_ASSERT_IF(cx->stackLimit != jsuword(-1),
                 JS_CHECK_STACK_SIZE(cx->stackLimit + (1 << 14), &stackDummy));
# else
    
    JS_ASSERT_IF(cx->stackLimit, JS_CHECK_STACK_SIZE(cx->stackLimit - (1 << 14), &stackDummy));
# endif
#endif

    GCTIMER_BEGIN();

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

            GCUntilDone(cx, comp, gckind  GCTIMER_ARG);
        }

        
        if (JSGCCallback callback = rt->gcCallback)
            (void) callback(cx, JSGC_END);

        



    } while (gckind == GC_LAST_CONTEXT && rt->gcPoke);
#ifdef JS_GCMETER
    js_DumpGCStats(cx->runtime, stderr);
#endif
    GCTIMER_END(gckind == GC_LAST_CONTEXT);
}

namespace js {
namespace gc {

bool
SetProtoCheckingForCycles(JSContext *cx, JSObject *obj, JSObject *proto)
{
    



#ifdef JS_THREADSAFE
    JS_ASSERT(cx->thread()->data.requestDepth);

    




    RecordNativeStackTopForGC(cx);
#endif

    JSRuntime *rt = cx->runtime;
    AutoLockGC lock(rt);
    AutoGCSession gcsession(cx);
    AutoUnlockGC unlock(rt);

    bool cycle = false;
    for (JSObject *obj2 = proto; obj2;) {
        if (obj2 == obj) {
            cycle = true;
            break;
        }
        obj2 = obj2->getProto();
    }
    if (!cycle)
        obj->setProto(proto);

    return !cycle;
}

JSCompartment *
NewCompartment(JSContext *cx, JSPrincipals *principals)
{
    JSRuntime *rt = cx->runtime;
    JSCompartment *compartment = cx->new_<JSCompartment>(rt);
    if (!compartment || !compartment->init()) {
        Foreground::delete_(compartment);
        JS_ReportOutOfMemory(cx);
        return NULL;
    }

    if (principals) {
        compartment->principals = principals;
        JSPRINCIPALS_HOLD(cx, principals);
    }

    compartment->setGCLastBytes(8192);

    {
        AutoLockGC lock(rt);

        if (!rt->compartments.append(compartment)) {
            AutoUnlockGC unlock(rt);
            Foreground::delete_(compartment);
            JS_ReportOutOfMemory(cx);
            return NULL;
        }
    }

    JSCompartmentCallback callback = rt->compartmentCallback;
    if (callback && !callback(cx, compartment, JSCOMPARTMENT_NEW)) {
        AutoLockGC lock(rt);
        rt->compartments.popBack();
        Foreground::delete_(compartment);
        return NULL;
    }
    return compartment;
}

} 

void
TraceRuntime(JSTracer *trc)
{
    LeaveTrace(trc->context);

#ifdef JS_THREADSAFE
    {
        JSContext *cx = trc->context;
        JSRuntime *rt = cx->runtime;
        AutoLockGC lock(rt);

        if (rt->gcThread != cx->thread()) {
            AutoGCSession gcsession(cx);
            AutoUnlockGC unlock(rt);
            RecordNativeStackTopForGC(trc->context);
            MarkRuntime(trc);
            return;
        }
    }
#else
    RecordNativeStackTopForGC(trc->context);
#endif

    



    MarkRuntime(trc);
}

} 
