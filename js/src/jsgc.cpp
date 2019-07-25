







































#define __STDC_LIMIT_MACROS











#include <stdlib.h>     
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
#include "jsdbgapi.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcchunk.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsparse.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstaticcheck.h"
#include "jsstr.h"
#include "jstask.h"
#include "jstracer.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#include "jsdtracef.h"
#include "jscntxtinlines.h"
#include "jsobjinlines.h"




#if defined(XP_WIN)
# include <windows.h>
#endif
#if defined(XP_UNIX) || defined(XP_BEOS)
# include <unistd.h>
# include <sys/mman.h>
#endif

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
# define MAP_ANONYMOUS MAP_ANON
#endif
#if !defined(MAP_ANONYMOUS)
# define MAP_ANONYMOUS 0
#endif

using namespace js;





JS_STATIC_ASSERT(JSTRACE_OBJECT == 0);
JS_STATIC_ASSERT(JSTRACE_STRING == 1);
JS_STATIC_ASSERT(JSTRACE_DOUBLE == 2);
JS_STATIC_ASSERT(JSTRACE_XML    == 3);





JS_STATIC_ASSERT(JSTRACE_DOUBLE + 1 == JSTRACE_XML);




JS_STATIC_ASSERT(FINALIZE_EXTERNAL_STRING_LAST - FINALIZE_EXTERNAL_STRING0 ==
                 JS_EXTERNAL_STRING_LIMIT - 1);


































































































const jsuword GC_ARENA_SHIFT = 12;
const jsuword GC_ARENA_MASK = JS_BITMASK(GC_ARENA_SHIFT);
const jsuword GC_ARENA_SIZE = JS_BIT(GC_ARENA_SHIFT);

const jsuword GC_MAX_CHUNK_AGE = 3;

const size_t GC_CELL_SHIFT = 3;
const size_t GC_CELL_SIZE = size_t(1) << GC_CELL_SHIFT;
const size_t GC_CELL_MASK = GC_CELL_SIZE - 1;

const size_t BITS_PER_GC_CELL = GC_CELL_SIZE * JS_BITS_PER_BYTE;

const size_t GC_CELLS_PER_ARENA = size_t(1) << (GC_ARENA_SHIFT - GC_CELL_SHIFT);
const size_t GC_MARK_BITMAP_SIZE = GC_CELLS_PER_ARENA / JS_BITS_PER_BYTE;
const size_t GC_MARK_BITMAP_WORDS = GC_CELLS_PER_ARENA / JS_BITS_PER_WORD;

JS_STATIC_ASSERT(sizeof(jsbitmap) == sizeof(jsuword));

JS_STATIC_ASSERT(sizeof(JSString) % GC_CELL_SIZE == 0);
JS_STATIC_ASSERT(sizeof(JSObject) % GC_CELL_SIZE == 0);
JS_STATIC_ASSERT(sizeof(JSFunction) % GC_CELL_SIZE == 0);
#ifdef JSXML
JS_STATIC_ASSERT(sizeof(JSXML) % GC_CELL_SIZE == 0);
#endif

JS_STATIC_ASSERT(GC_CELL_SIZE == sizeof(jsdouble));
const size_t DOUBLES_PER_ARENA = GC_CELLS_PER_ARENA;

struct JSGCArenaInfo {
    


    JSGCArenaList   *list;

    





    JSGCArena       *prev;

    JSGCThing       *freeList;

    
    bool        hasMarkedDoubles;

    static inline JSGCArenaInfo *fromGCThing(void* thing);
};


struct JSGCMarkingDelay {
    JSGCArena       *link;
    jsuword         unmarkedChildren;
};

struct JSGCArena {
    uint8 data[GC_ARENA_SIZE];

    void checkAddress() const {
        JS_ASSERT(!(reinterpret_cast<jsuword>(this) & GC_ARENA_MASK));
    }

    jsuword toPageStart() const {
        checkAddress();
        return reinterpret_cast<jsuword>(this);
    }

    static inline JSGCArena *fromGCThing(void* thing);

    static inline JSGCArena *fromChunkAndIndex(jsuword chunk, size_t index);

    jsuword getChunk() {
        return toPageStart() & ~GC_CHUNK_MASK;
    }

    jsuword getIndex() {
        return (toPageStart() & GC_CHUNK_MASK) >> GC_ARENA_SHIFT;
    }

    inline JSGCArenaInfo *getInfo();

    inline JSGCMarkingDelay *getMarkingDelay();

    inline jsbitmap *getMarkBitmap();

    inline void clearMarkBitmap();
};

struct JSGCChunkInfo {
    JSRuntime       *runtime;
    size_t          numFreeArenas;
    size_t          gcChunkAge;

    inline void init(JSRuntime *rt);

    inline jsbitmap *getFreeArenaBitmap();

    inline jsuword getChunk();

    static inline JSGCChunkInfo *fromChunk(jsuword chunk);
};


JS_STATIC_ASSERT(sizeof(JSGCArena) == GC_ARENA_SIZE);
JS_STATIC_ASSERT(GC_MARK_BITMAP_WORDS % sizeof(jsuword) == 0);
JS_STATIC_ASSERT(sizeof(JSGCArenaInfo) % sizeof(jsuword) == 0);
JS_STATIC_ASSERT(sizeof(JSGCMarkingDelay) % sizeof(jsuword) == 0);

const size_t GC_ARENA_ALL_WORDS = (GC_ARENA_SIZE + GC_MARK_BITMAP_SIZE +
                                   sizeof(JSGCArenaInfo) +
                                   sizeof(JSGCMarkingDelay)) / sizeof(jsuword);


const size_t GC_ARENAS_PER_CHUNK =
    (GC_CHUNK_SIZE - sizeof(JSGCChunkInfo)) * JS_BITS_PER_BYTE /
    (JS_BITS_PER_WORD * GC_ARENA_ALL_WORDS + 1);

const size_t GC_FREE_ARENA_BITMAP_WORDS = (GC_ARENAS_PER_CHUNK +
                                           JS_BITS_PER_WORD - 1) /
                                          JS_BITS_PER_WORD;

const size_t GC_FREE_ARENA_BITMAP_SIZE = GC_FREE_ARENA_BITMAP_WORDS *
                                         sizeof(jsuword);


JS_STATIC_ASSERT(GC_ARENAS_PER_CHUNK * GC_ARENA_ALL_WORDS +
                 GC_FREE_ARENA_BITMAP_WORDS <=
                 (GC_CHUNK_SIZE - sizeof(JSGCChunkInfo)) / sizeof(jsuword));

JS_STATIC_ASSERT((GC_ARENAS_PER_CHUNK + 1) * GC_ARENA_ALL_WORDS +
                 (GC_ARENAS_PER_CHUNK + 1 + JS_BITS_PER_WORD - 1) /
                 JS_BITS_PER_WORD >
                 (GC_CHUNK_SIZE - sizeof(JSGCChunkInfo)) / sizeof(jsuword));


const size_t GC_MARK_BITMAP_ARRAY_OFFSET = GC_ARENAS_PER_CHUNK
                                           << GC_ARENA_SHIFT;

const size_t GC_ARENA_INFO_ARRAY_OFFSET =
    GC_MARK_BITMAP_ARRAY_OFFSET + GC_MARK_BITMAP_SIZE * GC_ARENAS_PER_CHUNK;

const size_t GC_MARKING_DELAY_ARRAY_OFFSET =
    GC_ARENA_INFO_ARRAY_OFFSET + sizeof(JSGCArenaInfo) * GC_ARENAS_PER_CHUNK;

const size_t GC_CHUNK_INFO_OFFSET = GC_CHUNK_SIZE - GC_FREE_ARENA_BITMAP_SIZE -
                                    sizeof(JSGCChunkInfo);

inline jsuword
JSGCChunkInfo::getChunk() {
    jsuword addr = reinterpret_cast<jsuword>(this);
    JS_ASSERT((addr & GC_CHUNK_MASK) == GC_CHUNK_INFO_OFFSET);
    jsuword chunk = addr & ~GC_CHUNK_MASK;
    return chunk;
}


inline JSGCChunkInfo *
JSGCChunkInfo::fromChunk(jsuword chunk) {
    JS_ASSERT(!(chunk & GC_CHUNK_MASK));
    jsuword addr = chunk | GC_CHUNK_INFO_OFFSET;
    return reinterpret_cast<JSGCChunkInfo *>(addr);
}

inline jsbitmap *
JSGCChunkInfo::getFreeArenaBitmap()
{
    jsuword addr = reinterpret_cast<jsuword>(this);
    return reinterpret_cast<jsbitmap *>(addr + sizeof(JSGCChunkInfo));
}

inline void
JSGCChunkInfo::init(JSRuntime *rt)
{
    runtime = rt;
    numFreeArenas = GC_ARENAS_PER_CHUNK;
    gcChunkAge = 0;

    





    memset(getFreeArenaBitmap(), 0xFF, GC_FREE_ARENA_BITMAP_SIZE);
}

inline void
CheckValidGCThingPtr(void *thing)
{
#ifdef DEBUG
    JS_ASSERT(!JSString::isStatic(thing));
    jsuword addr = reinterpret_cast<jsuword>(thing);
    JS_ASSERT(!(addr & GC_CELL_MASK));
    JS_ASSERT((addr & GC_CHUNK_MASK) < GC_MARK_BITMAP_ARRAY_OFFSET);
#endif
}


inline JSGCArenaInfo *
JSGCArenaInfo::fromGCThing(void* thing)
{
    CheckValidGCThingPtr(thing);
    jsuword addr = reinterpret_cast<jsuword>(thing);
    jsuword chunk = addr & ~GC_CHUNK_MASK;
    JSGCArenaInfo *array =
        reinterpret_cast<JSGCArenaInfo *>(chunk | GC_ARENA_INFO_ARRAY_OFFSET);
    size_t arenaIndex = (addr & GC_CHUNK_MASK) >> GC_ARENA_SHIFT;
    return array + arenaIndex;
}


inline JSGCArena *
JSGCArena::fromGCThing(void* thing)
{
    CheckValidGCThingPtr(thing);
    jsuword addr = reinterpret_cast<jsuword>(thing);
    return reinterpret_cast<JSGCArena *>(addr & ~GC_ARENA_MASK);
}


inline JSGCArena *
JSGCArena::fromChunkAndIndex(jsuword chunk, size_t index) {
    JS_ASSERT(chunk);
    JS_ASSERT(!(chunk & GC_CHUNK_MASK));
    JS_ASSERT(index < GC_ARENAS_PER_CHUNK);
    return reinterpret_cast<JSGCArena *>(chunk | (index << GC_ARENA_SHIFT));
}

inline JSGCArenaInfo *
JSGCArena::getInfo()
{
    jsuword chunk = getChunk();
    jsuword index = getIndex();
    jsuword offset = GC_ARENA_INFO_ARRAY_OFFSET + index * sizeof(JSGCArenaInfo);
    return reinterpret_cast<JSGCArenaInfo *>(chunk | offset);
}

inline JSGCMarkingDelay *
JSGCArena::getMarkingDelay()
{
    jsuword chunk = getChunk();
    jsuword index = getIndex();
    jsuword offset = GC_MARKING_DELAY_ARRAY_OFFSET +
                     index * sizeof(JSGCMarkingDelay);
    return reinterpret_cast<JSGCMarkingDelay *>(chunk | offset);
}

inline jsbitmap *
JSGCArena::getMarkBitmap()
{
    jsuword chunk = getChunk();
    jsuword index = getIndex();
    jsuword offset = GC_MARK_BITMAP_ARRAY_OFFSET + index * GC_MARK_BITMAP_SIZE;
    return reinterpret_cast<jsbitmap *>(chunk | offset);
}

inline void
JSGCArena::clearMarkBitmap()
{
    PodZero(getMarkBitmap(), GC_MARK_BITMAP_WORDS);
}





inline jsbitmap *
GetGCThingMarkBit(void *thing, size_t &bitIndex)
{
    CheckValidGCThingPtr(thing);
    jsuword addr = reinterpret_cast<jsuword>(thing);
    jsuword chunk = addr & ~GC_CHUNK_MASK;
    bitIndex = (addr & GC_CHUNK_MASK) >> GC_CELL_SHIFT;
    return reinterpret_cast<jsbitmap *>(chunk | GC_MARK_BITMAP_ARRAY_OFFSET);
}

inline bool
IsMarkedGCThing(void *thing)
{
    size_t index;
    jsbitmap *markBitmap = GetGCThingMarkBit(thing, index);
    return !!JS_TEST_BIT(markBitmap, index);
}

inline bool
MarkIfUnmarkedGCThing(void *thing)
{
    size_t index;
    jsbitmap *markBitmap = GetGCThingMarkBit(thing, index);
    if (JS_TEST_BIT(markBitmap, index))
        return false;
    JS_SET_BIT(markBitmap, index);
    return true;
}

inline size_t
ThingsPerArena(size_t thingSize)
{
    JS_ASSERT(!(thingSize & GC_CELL_MASK));
    JS_ASSERT(thingSize <= GC_ARENA_SIZE);
    return GC_ARENA_SIZE / thingSize;
}


inline size_t
GCThingToArenaIndex(void *thing)
{
    CheckValidGCThingPtr(thing);
    jsuword addr = reinterpret_cast<jsuword>(thing);
    jsuword offsetInArena = addr & GC_ARENA_MASK;
    JSGCArenaInfo *a = JSGCArenaInfo::fromGCThing(thing);
    JS_ASSERT(a->list);
    JS_ASSERT(offsetInArena % a->list->thingSize == 0);
    return offsetInArena / a->list->thingSize;
}


inline uint8 *
GCArenaIndexToThing(JSGCArena *a, JSGCArenaInfo *ainfo, size_t index)
{
    JS_ASSERT(a->getInfo() == ainfo);

    




    JS_ASSERT(index <= ThingsPerArena(ainfo->list->thingSize));
    jsuword offsetInArena = index * ainfo->list->thingSize;
    return reinterpret_cast<uint8 *>(a->toPageStart() + offsetInArena);
}




struct JSGCThing {
    JSGCThing   *link;
};

static inline JSGCThing *
MakeNewArenaFreeList(JSGCArena *a, size_t thingSize)
{
    jsuword thingsStart = a->toPageStart();
    jsuword lastThingMinAddr = thingsStart + GC_ARENA_SIZE - thingSize * 2 + 1;
    jsuword thingPtr = thingsStart;
    do {
        jsuword nextPtr = thingPtr + thingSize;
        JS_ASSERT((nextPtr & GC_ARENA_MASK) + thingSize <= GC_ARENA_SIZE);
        JSGCThing *thing = reinterpret_cast<JSGCThing *>(thingPtr);
        thing->link = reinterpret_cast<JSGCThing *>(nextPtr);
        thingPtr = nextPtr;
    } while (thingPtr < lastThingMinAddr);

    JSGCThing *lastThing = reinterpret_cast<JSGCThing *>(thingPtr);
    lastThing->link = NULL;
    return reinterpret_cast<JSGCThing *>(thingsStart);
}

#ifdef JS_GCMETER
# define METER(x)               ((void) (x))
# define METER_IF(condition, x) ((void) ((condition) && (x)))
#else
# define METER(x)               ((void) 0)
# define METER_IF(condition, x) ((void) 0)
#endif

#define METER_UPDATE_MAX(maxLval, rval)                                       \
    METER_IF((maxLval) < (rval), (maxLval) = (rval))

#ifdef MOZ_GCTIMER
static jsrefcount newChunkCount = 0;
static jsrefcount destroyChunkCount = 0;
#endif

inline void *
GetGCChunk(JSRuntime *rt)
{
    void *p = AllocGCChunk();
#ifdef MOZ_GCTIMER
    if (p)
        JS_ATOMIC_INCREMENT(&newChunkCount);
#endif
    METER_IF(p, rt->gcStats.nchunks++);
    METER_UPDATE_MAX(rt->gcStats.maxnchunks, rt->gcStats.nchunks);
    return p;
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
    FreeGCChunk(p);
}

static JSGCArena *
NewGCArena(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    if (!JS_THREAD_DATA(cx)->waiveGCQuota && rt->gcBytes >= rt->gcMaxBytes) {
        




        if (!JS_ON_TRACE(cx))
            return NULL;
        js_TriggerGC(cx, true);
    }

    size_t nchunks = rt->gcChunks.length();

    JSGCChunkInfo *ci;
    for (;; ++rt->gcChunkCursor) {
        if (rt->gcChunkCursor == nchunks) {
            ci = NULL;
            break;
        }
        ci = rt->gcChunks[rt->gcChunkCursor];
        if (ci->numFreeArenas != 0)
            break;
    }
    if (!ci) {
        if (!rt->gcChunks.reserve(nchunks + 1))
            return NULL;
        void *chunkptr = GetGCChunk(rt);
        if (!chunkptr)
            return NULL;
        ci = JSGCChunkInfo::fromChunk(reinterpret_cast<jsuword>(chunkptr));
        ci->init(rt);
        JS_ALWAYS_TRUE(rt->gcChunks.append(ci));
    }

    
    jsbitmap *freeArenas = ci->getFreeArenaBitmap();
    size_t arenaIndex = 0;
    while (!*freeArenas) {
        arenaIndex += JS_BITS_PER_WORD;
        freeArenas++;
    }
    size_t bit = CountTrailingZeros(*freeArenas);
    arenaIndex += bit;
    JS_ASSERT(arenaIndex < GC_ARENAS_PER_CHUNK);
    JS_ASSERT(*freeArenas & (jsuword(1) << bit));
    *freeArenas &= ~(jsuword(1) << bit);
    --ci->numFreeArenas;

    rt->gcBytes += GC_ARENA_SIZE;
    METER(rt->gcStats.nallarenas++);
    METER_UPDATE_MAX(rt->gcStats.maxnallarenas, rt->gcStats.nallarenas);

    return JSGCArena::fromChunkAndIndex(ci->getChunk(), arenaIndex);
}





static void
ReleaseGCArena(JSRuntime *rt, JSGCArena *a)
{
    METER(rt->gcStats.afree++);
    JS_ASSERT(rt->gcBytes >= GC_ARENA_SIZE);
    rt->gcBytes -= GC_ARENA_SIZE;
    JS_ASSERT(rt->gcStats.nallarenas != 0);
    METER(rt->gcStats.nallarenas--);

    jsuword chunk = a->getChunk();
    JSGCChunkInfo *ci = JSGCChunkInfo::fromChunk(chunk);
    JS_ASSERT(ci->numFreeArenas <= GC_ARENAS_PER_CHUNK - 1);
    jsbitmap *freeArenas = ci->getFreeArenaBitmap();
    JS_ASSERT(!JS_TEST_BIT(freeArenas, a->getIndex()));
    JS_SET_BIT(freeArenas, a->getIndex());
    ci->numFreeArenas++;
    if (ci->numFreeArenas == GC_ARENAS_PER_CHUNK)
        ci->gcChunkAge = 0;

#ifdef DEBUG
    a->getInfo()->prev = rt->gcEmptyArenaList;
    rt->gcEmptyArenaList = a;
#endif
}

static void
FreeGCChunks(JSRuntime *rt)
{
#ifdef DEBUG
    while (rt->gcEmptyArenaList) {
        JSGCArena *next = rt->gcEmptyArenaList->getInfo()->prev;
        memset(rt->gcEmptyArenaList, JS_FREE_PATTERN, GC_ARENA_SIZE);
        rt->gcEmptyArenaList = next;
    }
#endif

    
    size_t available = 0;
    for (JSGCChunkInfo **i = rt->gcChunks.begin(); i != rt->gcChunks.end(); ++i) {
        JSGCChunkInfo *ci = *i;
        JS_ASSERT(ci->runtime == rt);
        if (ci->numFreeArenas == GC_ARENAS_PER_CHUNK) {
            if (ci->gcChunkAge > GC_MAX_CHUNK_AGE) {
                ReleaseGCChunk(rt, ci->getChunk());
                continue;
            }
            ci->gcChunkAge++;
        }
        rt->gcChunks[available++] = ci;
    }
    rt->gcChunks.resize(available);
    rt->gcChunkCursor = 0;
}

static inline size_t
GetFinalizableThingSize(unsigned thingKind)
{
    JS_STATIC_ASSERT(JS_EXTERNAL_STRING_LIMIT == 8);

    static const uint8 map[FINALIZE_LIMIT] = {
        sizeof(JSObject),   
        sizeof(JSFunction), 
#if JS_HAS_XML_SUPPORT
        sizeof(JSXML),      
#endif
        sizeof(JSString),   
        sizeof(JSString),   
        sizeof(JSString),   
        sizeof(JSString),   
        sizeof(JSString),   
        sizeof(JSString),   
        sizeof(JSString),   
        sizeof(JSString),   
        sizeof(JSString),   
    };

    JS_ASSERT(thingKind < FINALIZE_LIMIT);
    return map[thingKind];
}

static inline size_t
GetFinalizableTraceKind(size_t thingKind)
{
    JS_STATIC_ASSERT(JS_EXTERNAL_STRING_LIMIT == 8);

    static const uint8 map[FINALIZE_LIMIT] = {
        JSTRACE_OBJECT,     
        JSTRACE_OBJECT,     
#if JS_HAS_XML_SUPPORT      
        JSTRACE_XML,
#endif                      
        JSTRACE_STRING,
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
        JSTRACE_STRING,     
    };

    JS_ASSERT(thingKind < FINALIZE_LIMIT);
    return map[thingKind];
}

static inline size_t
GetFinalizableArenaTraceKind(JSGCArenaInfo *ainfo)
{
    JS_ASSERT(ainfo->list);
    return GetFinalizableTraceKind(ainfo->list->thingKind);
}

static inline size_t
GetFinalizableThingTraceKind(void *thing)
{
    JSGCArenaInfo *ainfo = JSGCArenaInfo::fromGCThing(thing);
    return GetFinalizableArenaTraceKind(ainfo);
}

static void
InitGCArenaLists(JSRuntime *rt)
{
    for (unsigned i = 0; i != FINALIZE_LIMIT; ++i) {
        JSGCArenaList *arenaList = &rt->gcArenaList[i];
        arenaList->head = NULL;
        arenaList->cursor = NULL;
        arenaList->thingKind = i;
        arenaList->thingSize = GetFinalizableThingSize(i);
    }
    rt->gcDoubleArenaList.head = NULL;
    rt->gcDoubleArenaList.cursor = NULL;
}

static void
FinishGCArenaLists(JSRuntime *rt)
{
    for (unsigned i = 0; i < FINALIZE_LIMIT; i++) {
        rt->gcArenaList[i].head = NULL;
        rt->gcArenaList[i].cursor = NULL;
    }
    rt->gcDoubleArenaList.head = NULL;
    rt->gcDoubleArenaList.cursor = NULL;

    rt->gcBytes = 0;

    for (JSGCChunkInfo **i = rt->gcChunks.begin(); i != rt->gcChunks.end(); ++i)
        ReleaseGCChunk(rt, (*i)->getChunk());
    rt->gcChunks.clear();
    rt->gcChunkCursor = 0;
}

intN
js_GetExternalStringGCType(JSString *str)
{
    JS_STATIC_ASSERT(FINALIZE_STRING + 1 == FINALIZE_EXTERNAL_STRING0);
    JS_ASSERT(!JSString::isStatic(str));

    unsigned thingKind = JSGCArenaInfo::fromGCThing(str)->list->thingKind;
    JS_ASSERT(IsFinalizableStringKind(thingKind));
    return intN(thingKind) - intN(FINALIZE_EXTERNAL_STRING0);
}

JS_FRIEND_API(uint32)
js_GetGCThingTraceKind(void *thing)
{
    if (JSString::isStatic(thing))
        return JSTRACE_STRING;

    JSGCArenaInfo *ainfo = JSGCArenaInfo::fromGCThing(thing);
    if (!ainfo->list)
        return JSTRACE_DOUBLE;
    return GetFinalizableArenaTraceKind(ainfo);
}

JSRuntime *
js_GetGCThingRuntime(void *thing)
{
    jsuword chunk = JSGCArena::fromGCThing(thing)->getChunk();
    return JSGCChunkInfo::fromChunk(chunk)->runtime;
}

bool
js_IsAboutToBeFinalized(void *thing)
{
    if (JSString::isStatic(thing))
        return false;

    JSGCArenaInfo *ainfo = JSGCArenaInfo::fromGCThing(thing);
    if (!ainfo->list) {
        




        if (!ainfo->hasMarkedDoubles)
            return true;
    }
    return !IsMarkedGCThing(thing);
}





const uint32 GC_ROOTS_SIZE = 256;

struct JSGCLockHashEntry : public JSDHashEntryHdr
{
    const void      *thing;
    uint32          count;
};

JSBool
js_InitGC(JSRuntime *rt, uint32 maxbytes)
{
    InitGCArenaLists(rt);
    if (!rt->gcRootsHash.init(GC_ROOTS_SIZE))
        return false;
    if (!JS_DHashTableInit(&rt->gcLocksHash, JS_DHashGetStubOps(), NULL,
                           sizeof(JSGCLockHashEntry), GC_ROOTS_SIZE)) {
        rt->gcLocksHash.ops = NULL;
        return false;
    }

#ifdef JS_THREADSAFE
    if (!rt->gcHelperThread.init())
        return false;
#endif

    



    rt->gcMaxBytes = maxbytes;
    rt->setGCMaxMallocBytes(maxbytes);

    rt->gcEmptyArenaPoolLifespan = 30000;

    



    rt->setGCTriggerFactor((uint32) -1);

    



    rt->setGCLastBytes(8192);

    METER(PodZero(&rt->gcStats));
    return true;
}

#ifdef JS_GCMETER

static void
UpdateArenaStats(JSGCArenaStats *st, uint32 nlivearenas, uint32 nkilledArenas,
                 uint32 nthings)
{
    size_t narenas;

    narenas = nlivearenas + nkilledArenas;
    JS_ASSERT(narenas >= st->livearenas);

    st->newarenas = narenas - st->livearenas;
    st->narenas = narenas;
    st->livearenas = nlivearenas;
    if (st->maxarenas < narenas)
        st->maxarenas = narenas;
    st->totalarenas += narenas;

    st->nthings = nthings;
    if (st->maxthings < nthings)
        st->maxthings = nthings;
    st->totalthings += nthings;
}

JS_FRIEND_API(void)
js_DumpGCStats(JSRuntime *rt, FILE *fp)
{
    static const char *const GC_ARENA_NAMES[] = {
        "double",
        "object",
        "function",
#if JS_HAS_XML_SUPPORT
        "xml",
#endif
        "string",
        "external_string_0",
        "external_string_1",
        "external_string_2",
        "external_string_3",
        "external_string_4",
        "external_string_5",
        "external_string_6",
        "external_string_7",
    };
    JS_STATIC_ASSERT(JS_ARRAY_LENGTH(GC_ARENA_NAMES) == FINALIZE_LIMIT + 1);

    fprintf(fp, "\nGC allocation statistics:\n\n");

#define UL(x)       ((unsigned long)(x))
#define ULSTAT(x)   UL(rt->gcStats.x)
#define PERCENT(x,y)  (100.0 * (double) (x) / (double) (y))

    size_t sumArenas = 0;
    size_t sumTotalArenas = 0;
    size_t sumThings = 0;
    size_t sumMaxThings = 0;
    size_t sumThingSize = 0;
    size_t sumTotalThingSize = 0;
    size_t sumArenaCapacity = 0;
    size_t sumTotalArenaCapacity = 0;
    size_t sumAlloc = 0;
    size_t sumLocalAlloc = 0;
    size_t sumFail = 0;
    size_t sumRetry = 0;
    for (int i = -1; i < (int) FINALIZE_LIMIT; i++) {
        size_t thingSize, thingsPerArena;
        JSGCArenaStats *st;
        if (i == -1) {
            thingSize = sizeof(jsdouble);
            thingsPerArena = DOUBLES_PER_ARENA;
            st = &rt->gcStats.doubleArenaStats;
        } else {
            thingSize = rt->gcArenaList[i].thingSize;
            thingsPerArena = ThingsPerArena(thingSize);
            st = &rt->gcStats.arenaStats[i];
        }
        if (st->maxarenas == 0)
            continue;
        fprintf(fp,
                "%s arenas (thing size %lu, %lu things per arena):",
                GC_ARENA_NAMES[i + 1], UL(thingSize), UL(thingsPerArena));
        putc('\n', fp);
        fprintf(fp, "           arenas before GC: %lu\n", UL(st->narenas));
        fprintf(fp, "       new arenas before GC: %lu (%.1f%%)\n",
                UL(st->newarenas), PERCENT(st->newarenas, st->narenas));
        fprintf(fp, "            arenas after GC: %lu (%.1f%%)\n",
                UL(st->livearenas), PERCENT(st->livearenas, st->narenas));
        fprintf(fp, "                 max arenas: %lu\n", UL(st->maxarenas));
        fprintf(fp, "                     things: %lu\n", UL(st->nthings));
        fprintf(fp, "        GC cell utilization: %.1f%%\n",
                PERCENT(st->nthings, thingsPerArena * st->narenas));
        fprintf(fp, "   average cell utilization: %.1f%%\n",
                PERCENT(st->totalthings, thingsPerArena * st->totalarenas));
        fprintf(fp, "                 max things: %lu\n", UL(st->maxthings));
        fprintf(fp, "             alloc attempts: %lu\n", UL(st->alloc));
        fprintf(fp, "        alloc without locks: %lu  (%.1f%%)\n",
                UL(st->localalloc), PERCENT(st->localalloc, st->alloc));
        sumArenas += st->narenas;
        sumTotalArenas += st->totalarenas;
        sumThings += st->nthings;
        sumMaxThings += st->maxthings;
        sumThingSize += thingSize * st->nthings;
        sumTotalThingSize += size_t(thingSize * st->totalthings);
        sumArenaCapacity += thingSize * thingsPerArena * st->narenas;
        sumTotalArenaCapacity += thingSize * thingsPerArena * st->totalarenas;
        sumAlloc += st->alloc;
        sumLocalAlloc += st->localalloc;
        sumFail += st->fail;
        sumRetry += st->retry;
        putc('\n', fp);
    }

    fputs("Never used arenas:\n", fp);
    for (int i = -1; i < (int) FINALIZE_LIMIT; i++) {
        size_t thingSize, thingsPerArena;
        JSGCArenaStats *st;
        if (i == -1) {
            thingSize = sizeof(jsdouble);
            thingsPerArena = DOUBLES_PER_ARENA;
            st = &rt->gcStats.doubleArenaStats;
        } else {
            thingSize = rt->gcArenaList[i].thingSize;
            thingsPerArena = ThingsPerArena(thingSize);
            st = &rt->gcStats.arenaStats[i];
        }
        if (st->maxarenas != 0)
            continue;
        fprintf(fp,
                "%s (thing size %lu, %lu things per arena)\n",
                GC_ARENA_NAMES[i + 1], UL(thingSize), UL(thingsPerArena));
    }
    fprintf(fp, "\nTOTAL STATS:\n");
    fprintf(fp, "            bytes allocated: %lu\n", UL(rt->gcBytes));
    fprintf(fp, "            total GC arenas: %lu\n", UL(sumArenas));
    fprintf(fp, "            total GC things: %lu\n", UL(sumThings));
    fprintf(fp, "        max total GC things: %lu\n", UL(sumMaxThings));
    fprintf(fp, "        GC cell utilization: %.1f%%\n",
            PERCENT(sumThingSize, sumArenaCapacity));
    fprintf(fp, "   average cell utilization: %.1f%%\n",
            PERCENT(sumTotalThingSize, sumTotalArenaCapacity));
    fprintf(fp, "allocation retries after GC: %lu\n", UL(sumRetry));
    fprintf(fp, "             alloc attempts: %lu\n", UL(sumAlloc));
    fprintf(fp, "        alloc without locks: %lu  (%.1f%%)\n",
            UL(sumLocalAlloc), PERCENT(sumLocalAlloc, sumAlloc));
    fprintf(fp, "        allocation failures: %lu\n", UL(sumFail));
    fprintf(fp, "         things born locked: %lu\n", ULSTAT(lockborn));
    fprintf(fp, "           valid lock calls: %lu\n", ULSTAT(lock));
    fprintf(fp, "         valid unlock calls: %lu\n", ULSTAT(unlock));
    fprintf(fp, "       mark recursion depth: %lu\n", ULSTAT(depth));
    fprintf(fp, "     maximum mark recursion: %lu\n", ULSTAT(maxdepth));
    fprintf(fp, "     mark C recursion depth: %lu\n", ULSTAT(cdepth));
    fprintf(fp, "   maximum mark C recursion: %lu\n", ULSTAT(maxcdepth));
    fprintf(fp, "      delayed tracing calls: %lu\n", ULSTAT(unmarked));
#ifdef DEBUG
    fprintf(fp, "      max trace later count: %lu\n", ULSTAT(maxunmarked));
#endif
    fprintf(fp, "   maximum GC nesting level: %lu\n", ULSTAT(maxlevel));
    fprintf(fp, "potentially useful GC calls: %lu\n", ULSTAT(poke));
    fprintf(fp, "  thing arenas freed so far: %lu\n", ULSTAT(afree));
    fprintf(fp, "     stack segments scanned: %lu\n", ULSTAT(stackseg));
    fprintf(fp, "stack segment slots scanned: %lu\n", ULSTAT(segslots));
    fprintf(fp, "reachable closeable objects: %lu\n", ULSTAT(nclose));
    fprintf(fp, "    max reachable closeable: %lu\n", ULSTAT(maxnclose));
    fprintf(fp, "      scheduled close hooks: %lu\n", ULSTAT(closelater));
    fprintf(fp, "  max scheduled close hooks: %lu\n", ULSTAT(maxcloselater));

#undef UL
#undef ULSTAT
#undef PERCENT
}
#endif

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

#ifdef JS_THREADSAFE
    rt->gcHelperThread.cancel();
#endif
    FinishGCArenaLists(rt);

    if (rt->gcRootsHash.initialized()) {
#ifdef DEBUG
        CheckLeakedRoots(rt);
#endif
    }
    if (rt->gcLocksHash.ops) {
        JS_DHashTableFinish(&rt->gcLocksHash);
        rt->gcLocksHash.ops = NULL;
    }
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

JSBool
js_AddRootRT(JSRuntime *rt, Value *vp, const char *name)
{
    






    AutoLockGC lock(rt);
    js_WaitForGC(rt);

    void *key = vp;
    return !!rt->gcRootsHash.put(key, RootInfo(name, JS_GC_ROOT_VALUE_PTR));
}

JSBool
js_AddRootRT(JSRuntime *rt, void **rp, const char *name)
{
    






    AutoLockGC lock(rt);
    js_WaitForGC(rt);

    void *key = rp;
    return !!rt->gcRootsHash.put(key, RootInfo(name, JS_GC_ROOT_GCTHING_PTR));
}

JSBool
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
}

void
JSRuntime::setGCLastBytes(size_t lastBytes)
{
    gcLastBytes = lastBytes;
    uint64 triggerBytes = uint64(lastBytes) * uint64(gcTriggerFactor / 100);
    if (triggerBytes != size_t(triggerBytes))
        triggerBytes = size_t(-1);
    gcTriggerBytes = size_t(triggerBytes);
}

void
JSGCFreeLists::purge()
{
    



    for (JSGCThing **p = finalizables; p != JS_ARRAY_END(finalizables); ++p) {
        JSGCThing *freeListHead = *p;
        if (freeListHead) {
            JSGCArenaInfo *ainfo = JSGCArenaInfo::fromGCThing(freeListHead);
            JS_ASSERT(!ainfo->freeList);
            ainfo->freeList = freeListHead;
            *p = NULL;
        }
    }
    doubles = NULL;
}

void
JSGCFreeLists::moveTo(JSGCFreeLists *another)
{
    *another = *this;
    doubles = NULL;
    PodArrayZero(finalizables);
    JS_ASSERT(isEmpty());
}

static inline bool
IsGCThresholdReached(JSRuntime *rt)
{
#ifdef JS_GC_ZEAL
    if (rt->gcZeal >= 1)
        return true;
#endif

    




    return rt->isGCMallocLimitReached() || rt->gcBytes >= rt->gcTriggerBytes;
}

static inline JSGCFreeLists *
GetGCFreeLists(JSContext *cx)
{
    JSThreadData *td = JS_THREAD_DATA(cx);
    if (!td->localRootStack)
        return &td->gcFreeLists;
    JS_ASSERT(td->gcFreeLists.isEmpty());
    return &td->localRootStack->gcFreeLists;
}

static void
LastDitchGC(JSContext *cx)
{
    JS_ASSERT(!JS_ON_TRACE(cx));

    
    AutoPreserveWeakRoots save(cx);
    AutoKeepAtoms keep(cx->runtime);

    






    js_GC(cx, GC_LOCK_HELD);
}

static JSGCThing *
RefillFinalizableFreeList(JSContext *cx, unsigned thingKind)
{
    JS_ASSERT(!GetGCFreeLists(cx)->finalizables[thingKind]);
    JSRuntime *rt = cx->runtime;
    JSGCArenaList *arenaList;
    JSGCArena *a;

    {
        AutoLockGC lock(rt);
        JS_ASSERT(!rt->gcRunning);
        if (rt->gcRunning) {
            METER(rt->gcStats.finalfail++);
            return NULL;
        }

        bool canGC = !JS_ON_TRACE(cx) && !JS_THREAD_DATA(cx)->waiveGCQuota;
        bool doGC = canGC && IsGCThresholdReached(rt);
        arenaList = &rt->gcArenaList[thingKind];
        for (;;) {
            if (doGC) {
                LastDitchGC(cx);
                METER(cx->runtime->gcStats.arenaStats[thingKind].retry++);
                canGC = false;

                




                JSGCThing *freeList = GetGCFreeLists(cx)->finalizables[thingKind];
                if (freeList)
                    return freeList;
            }

            while ((a = arenaList->cursor) != NULL) {
                JSGCArenaInfo *ainfo = a->getInfo();
                arenaList->cursor = ainfo->prev;
                JSGCThing *freeList = ainfo->freeList;
                if (freeList) {
                    ainfo->freeList = NULL;
                    return freeList;
                }
            }

            a = NewGCArena(cx);
            if (a)
                break;
            if (!canGC) {
                METER(cx->runtime->gcStats.arenaStats[thingKind].fail++);
                return NULL;
            }
            doGC = true;
        }

        




        JSGCArenaInfo *ainfo = a->getInfo();
        ainfo->list = arenaList;
        ainfo->prev = arenaList->head;
        ainfo->freeList = NULL;
        arenaList->head = a;
    }

    a->clearMarkBitmap();
    JSGCMarkingDelay *markingDelay = a->getMarkingDelay();
    markingDelay->link = NULL;
    markingDelay->unmarkedChildren = 0;

    return MakeNewArenaFreeList(a, arenaList->thingSize);
}

static inline void
CheckGCFreeListLink(JSGCThing *thing)
{
    



    JS_ASSERT_IF(thing->link,
                 JSGCArena::fromGCThing(thing) ==
                 JSGCArena::fromGCThing(thing->link));
    JS_ASSERT_IF(thing->link, thing < thing->link);
}

void *
js_NewFinalizableGCThing(JSContext *cx, unsigned thingKind)
{
    JS_ASSERT(thingKind < FINALIZE_LIMIT);
#ifdef JS_THREADSAFE
    JS_ASSERT(cx->thread);
#endif

    
    METER(cx->runtime->gcStats.arenaStats[thingKind].alloc++);

    JSGCThing **freeListp =
        JS_THREAD_DATA(cx)->gcFreeLists.finalizables + thingKind;
    JSGCThing *thing = *freeListp;
    if (thing) {
        JS_ASSERT(!JS_THREAD_DATA(cx)->localRootStack);
        *freeListp = thing->link;
        cx->weakRoots.finalizableNewborns[thingKind] = thing;
        CheckGCFreeListLink(thing);
        METER(cx->runtime->gcStats.arenaStats[thingKind].localalloc++);
        return thing;
    }

    





    JSLocalRootStack *lrs = JS_THREAD_DATA(cx)->localRootStack;
    for (;;) {
        if (lrs) {
            freeListp = lrs->gcFreeLists.finalizables + thingKind;
            thing = *freeListp;
            if (thing) {
                *freeListp = thing->link;
                METER(cx->runtime->gcStats.arenaStats[thingKind].localalloc++);
                break;
            }
        }

        thing = RefillFinalizableFreeList(cx, thingKind);
        if (thing) {
            



            JS_ASSERT(!*freeListp || *freeListp == thing);
            *freeListp = thing->link;
            break;
        }

        js_ReportOutOfMemory(cx);
        return NULL;
    }

    CheckGCFreeListLink(thing);
    if (lrs) {
        






        if (js_PushLocalRoot(cx, lrs, thing) < 0) {
            JS_ASSERT(thing->link == *freeListp);
            *freeListp = thing;
            return NULL;
        }
    } else {
        



        cx->weakRoots.finalizableNewborns[thingKind] = thing;
    }

    return thing;
}

static JSGCThing *
TurnUsedArenaIntoDoubleList(JSGCArena *a)
{
    JSGCThing *head;
    JSGCThing **tailp = &head;
    jsuword thing = a->toPageStart();
    jsbitmap *markBitmap = a->getMarkBitmap();
    jsbitmap *lastMarkWord = markBitmap + GC_MARK_BITMAP_WORDS - 1;

    for (jsbitmap *m = markBitmap; m <= lastMarkWord; ++m) {
        JS_ASSERT(thing < a->toPageStart() + GC_ARENA_SIZE);
        JS_ASSERT((thing - a->toPageStart()) %
                  (JS_BITS_PER_WORD * sizeof(jsdouble)) == 0);

        jsbitmap bits = *m;
        if (bits == jsbitmap(-1)) {
            thing += JS_BITS_PER_WORD * sizeof(jsdouble);
        } else {
            



            const unsigned unroll = 4;
            const jsbitmap unrollMask = (jsbitmap(1) << unroll) - 1;
            JS_STATIC_ASSERT((JS_BITS_PER_WORD & unrollMask) == 0);

            for (unsigned n = 0; n != JS_BITS_PER_WORD; n += unroll) {
                jsbitmap bitsChunk = bits & unrollMask;
                bits >>= unroll;
                if (bitsChunk == unrollMask) {
                    thing += unroll * sizeof(jsdouble);
                } else {
#define DO_BIT(bit)                                                           \
                    if (!(bitsChunk & (jsbitmap(1) << (bit)))) {              \
                        JS_ASSERT(thing - a->toPageStart() <=                 \
                                  (DOUBLES_PER_ARENA - 1) * sizeof(jsdouble));\
                        JSGCThing *t = reinterpret_cast<JSGCThing *>(thing);  \
                        *tailp = t;                                           \
                        tailp = &t->link;                                     \
                    }                                                         \
                    thing += sizeof(jsdouble);
                    DO_BIT(0);
                    DO_BIT(1);
                    DO_BIT(2);
                    DO_BIT(3);
#undef DO_BIT
                }
            }
        }
    }
    *tailp = NULL;
    return head;
}

static JSGCThing *
RefillDoubleFreeList(JSContext *cx)
{
    JS_ASSERT(!GetGCFreeLists(cx)->doubles);

    JSRuntime *rt = cx->runtime;
    JS_ASSERT(!rt->gcRunning);

    JS_LOCK_GC(rt);

    bool canGC = !JS_ON_TRACE(cx) && !JS_THREAD_DATA(cx)->waiveGCQuota;
    bool doGC = canGC && IsGCThresholdReached(rt);
    JSGCArena *a;
    for (;;) {
        if (doGC) {
            LastDitchGC(cx);
            METER(rt->gcStats.doubleArenaStats.retry++);
            canGC = false;

            
            JSGCThing *freeList = GetGCFreeLists(cx)->doubles;
            if (freeList) {
                JS_UNLOCK_GC(rt);
                return freeList;
            }
        }

        




        while (!!(a = rt->gcDoubleArenaList.cursor)) {
            rt->gcDoubleArenaList.cursor = a->getInfo()->prev;
            JS_UNLOCK_GC(rt);
            JSGCThing *list = TurnUsedArenaIntoDoubleList(a);
            if (list)
                return list;
            JS_LOCK_GC(rt);
        }
        a = NewGCArena(cx);
        if (a)
            break;
        if (!canGC) {
            METER(rt->gcStats.doubleArenaStats.fail++);
            JS_UNLOCK_GC(rt);
            return NULL;
        }
        doGC = true;
    }

    JSGCArenaInfo *ainfo = a->getInfo();
    ainfo->list = NULL;
    ainfo->freeList = NULL;
    ainfo->prev = rt->gcDoubleArenaList.head;
    rt->gcDoubleArenaList.head = a;
    JS_UNLOCK_GC(rt);

    ainfo->hasMarkedDoubles = false;
    return MakeNewArenaFreeList(a, sizeof(jsdouble));
}

jsdouble *
js_NewWeaklyRootedDoubleAtom(JSContext *cx, jsdouble d)
{
    
    METER(cx->runtime->gcStats.doubleArenaStats.alloc++);

    JSGCThing **freeListp = &JS_THREAD_DATA(cx)->gcFreeLists.doubles;
    JSGCThing *thing = *freeListp;
    if (thing) {
        METER(cx->runtime->gcStats.doubleArenaStats.localalloc++);
        JS_ASSERT(!JS_THREAD_DATA(cx)->localRootStack);
        CheckGCFreeListLink(thing);
        *freeListp = thing->link;

        jsdouble *dp = reinterpret_cast<jsdouble *>(thing);
        *dp = d;
        cx->weakRoots.newbornDouble = dp;
        return dp;
    }

    JSLocalRootStack *lrs = JS_THREAD_DATA(cx)->localRootStack;
    for (;;) {
        if (lrs) {
            freeListp = &lrs->gcFreeLists.doubles;
            thing = *freeListp;
            if (thing) {
                METER(cx->runtime->gcStats.doubleArenaStats.localalloc++);
                break;
            }
        }
        thing = RefillDoubleFreeList(cx);
        if (thing) {
            JS_ASSERT(!*freeListp || *freeListp == thing);
            break;
        }

        if (!JS_ON_TRACE(cx)) {
            
            js_ReportOutOfMemory(cx);
            METER(cx->runtime->gcStats.doubleArenaStats.fail++);
        }
        return NULL;
    }

    CheckGCFreeListLink(thing);
    *freeListp = thing->link;

    jsdouble *dp = reinterpret_cast<jsdouble *>(thing);
    *dp = d;
    cx->weakRoots.newbornDouble = dp;
    



    return dp;
}

JSBool
js_LockGCThingRT(JSRuntime *rt, void *thing)
{
    if (!thing)
        return true;

    AutoLockGC lock(rt);
    JSGCLockHashEntry *lhe = (JSGCLockHashEntry *)
                             JS_DHashTableOperate(&rt->gcLocksHash, thing,
                                                  JS_DHASH_ADD);
    bool ok = !!lhe;
    if (ok) {
        if (!lhe->thing) {
            lhe->thing = thing;
            lhe->count = 1;
        } else {
            JS_ASSERT(lhe->count >= 1);
            lhe->count++;
        }
        METER(rt->gcStats.lock++);
    }
    return ok;
}

void
js_UnlockGCThingRT(JSRuntime *rt, void *thing)
{
    if (!thing)
        return;

    AutoLockGC lock(rt);
    JSGCLockHashEntry *lhe = (JSGCLockHashEntry *)
                             JS_DHashTableOperate(&rt->gcLocksHash, thing,
                                                  JS_DHASH_LOOKUP);
    if (JS_DHASH_ENTRY_IS_BUSY(lhe)) {
        rt->gcPoke = true;
        if (--lhe->count == 0)
            JS_DHashTableOperate(&rt->gcLocksHash, thing, JS_DHASH_REMOVE);
        METER(rt->gcStats.unlock++);
    }
}

JS_PUBLIC_API(void)
JS_TraceChildren(JSTracer *trc, void *thing, uint32 kind)
{
    switch (kind) {
      case JSTRACE_OBJECT: {
        
        JSObject *obj = (JSObject *) thing;
        if (!obj->map)
            break;
        obj->map->ops->trace(trc, obj);
        break;
      }

      case JSTRACE_STRING: {
        JSString *str = (JSString *) thing;
        if (str->isDependent())
            JS_CALL_STRING_TRACER(trc, str->dependentBase(), "base");
        break;
      }

#if JS_HAS_XML_SUPPORT
      case JSTRACE_XML:
        js_TraceXML(trc, (JSXML *)thing);
        break;
#endif
    }
}



































inline unsigned
ThingsPerUnmarkedBit(unsigned thingSize)
{
    return JS_HOWMANY(ThingsPerArena(thingSize), JS_BITS_PER_WORD);
}

static void
DelayMarkingChildren(JSRuntime *rt, void *thing)
{
    JS_ASSERT(IsMarkedGCThing(thing));
    METER(rt->gcStats.unmarked++);

    JSGCArena *a = JSGCArena::fromGCThing(thing);
    JSGCArenaInfo *ainfo = a->getInfo();
    JSGCMarkingDelay *markingDelay = a->getMarkingDelay();

    size_t thingArenaIndex = GCThingToArenaIndex(thing);
    size_t unmarkedBitIndex = thingArenaIndex /
                              ThingsPerUnmarkedBit(ainfo->list->thingSize);
    JS_ASSERT(unmarkedBitIndex < JS_BITS_PER_WORD);

    jsuword bit = jsuword(1) << unmarkedBitIndex;
    if (markingDelay->unmarkedChildren != 0) {
        JS_ASSERT(rt->gcUnmarkedArenaStackTop);
        if (markingDelay->unmarkedChildren & bit) {
            
            return;
        }
        markingDelay->unmarkedChildren |= bit;
    } else {
        











        markingDelay->unmarkedChildren = bit;
        if (!markingDelay->link) {
            if (!rt->gcUnmarkedArenaStackTop) {
                
                markingDelay->link = a;
            } else {
                JS_ASSERT(rt->gcUnmarkedArenaStackTop->getMarkingDelay()->link);
                markingDelay->link = rt->gcUnmarkedArenaStackTop;
            }
            rt->gcUnmarkedArenaStackTop = a;
        }
        JS_ASSERT(rt->gcUnmarkedArenaStackTop);
    }
#ifdef DEBUG
    rt->gcMarkLaterCount += ThingsPerUnmarkedBit(ainfo->list->thingSize);
    METER_UPDATE_MAX(rt->gcStats.maxunmarked, rt->gcMarkLaterCount);
#endif
}

static void
MarkDelayedChildren(JSTracer *trc)
{
    JSRuntime *rt;
    JSGCArena *a, *aprev;
    unsigned thingSize, traceKind;
    unsigned thingsPerUnmarkedBit;
    unsigned unmarkedBitIndex, thingIndex, indexLimit, endIndex;

    rt = trc->context->runtime;
    a = rt->gcUnmarkedArenaStackTop;
    if (!a) {
        JS_ASSERT(rt->gcMarkLaterCount == 0);
        return;
    }

    for (;;) {
        





        JSGCArenaInfo *ainfo = a->getInfo();
        JSGCMarkingDelay *markingDelay = a->getMarkingDelay();
        JS_ASSERT(markingDelay->link);
        JS_ASSERT(rt->gcUnmarkedArenaStackTop->getMarkingDelay()->link);
        thingSize = ainfo->list->thingSize;
        traceKind = GetFinalizableArenaTraceKind(ainfo);
        indexLimit = ThingsPerArena(thingSize);
        thingsPerUnmarkedBit = ThingsPerUnmarkedBit(thingSize);

        




        while (markingDelay->unmarkedChildren != 0) {
            unmarkedBitIndex = JS_FLOOR_LOG2W(markingDelay->unmarkedChildren);
            markingDelay->unmarkedChildren &= ~(jsuword(1) << unmarkedBitIndex);
#ifdef DEBUG
            JS_ASSERT(rt->gcMarkLaterCount >= thingsPerUnmarkedBit);
            rt->gcMarkLaterCount -= thingsPerUnmarkedBit;
#endif
            thingIndex = unmarkedBitIndex * thingsPerUnmarkedBit;
            endIndex = thingIndex + thingsPerUnmarkedBit;

            



            if (endIndex > indexLimit)
                endIndex = indexLimit;
            uint8 *thing = GCArenaIndexToThing(a, ainfo, thingIndex);
            uint8 *end = GCArenaIndexToThing(a, ainfo, endIndex);
            do {
                JS_ASSERT(thing < end);
                if (IsMarkedGCThing(thing))
                    JS_TraceChildren(trc, thing, traceKind);
                thing += thingSize;
            } while (thing != end);
        }

        








        if (a == rt->gcUnmarkedArenaStackTop) {
            aprev = markingDelay->link;
            markingDelay->link = NULL;
            if (a == aprev) {
                



                break;
            }
            rt->gcUnmarkedArenaStackTop = a = aprev;
        } else {
            a = rt->gcUnmarkedArenaStackTop;
        }
    }
    JS_ASSERT(rt->gcUnmarkedArenaStackTop);
    JS_ASSERT(!rt->gcUnmarkedArenaStackTop->getMarkingDelay()->link);
    rt->gcUnmarkedArenaStackTop = NULL;
    JS_ASSERT(rt->gcMarkLaterCount == 0);
}

namespace js {

void
CallGCMarker(JSTracer *trc, void *thing, uint32 kind)
{
    JSContext *cx;
    JSRuntime *rt;

    JS_ASSERT(thing);
    JS_ASSERT(JS_IS_VALID_TRACE_KIND(kind));
    JS_ASSERT(trc->debugPrinter || trc->debugPrintArg);

    if (!IS_GC_MARKING_TRACER(trc)) {
        trc->callback(trc, thing, kind);
        goto out;
    }

    cx = trc->context;
    rt = cx->runtime;
    JS_ASSERT(rt->gcMarkingTracer == trc);
    JS_ASSERT(rt->gcLevel > 0);

    



    switch (kind) {
      case JSTRACE_DOUBLE: {
        JSGCArenaInfo *ainfo = JSGCArenaInfo::fromGCThing(thing);
        JS_ASSERT(!ainfo->list);
        if (!ainfo->hasMarkedDoubles) {
            ainfo->hasMarkedDoubles = true;
            JSGCArena::fromGCThing(thing)->clearMarkBitmap();
        }
        MarkIfUnmarkedGCThing(thing);
        goto out;
      }

      case JSTRACE_STRING:
        for (;;) {
            if (JSString::isStatic(thing))
                goto out;
            JS_ASSERT(kind == GetFinalizableThingTraceKind(thing));
            if (!MarkIfUnmarkedGCThing(thing))
                goto out;
            if (!((JSString *) thing)->isDependent())
                goto out;
            thing = ((JSString *) thing)->dependentBase();
        }
        
    }

    JS_ASSERT(kind == GetFinalizableThingTraceKind(thing));
    if (!MarkIfUnmarkedGCThing(thing))
        goto out;

    if (!cx->insideGCMarkCallback) {
        




#ifdef JS_GC_ASSUME_LOW_C_STACK
# define RECURSION_TOO_DEEP() JS_TRUE
#else
        int stackDummy;
# define RECURSION_TOO_DEEP() (!JS_CHECK_STACK_SIZE(cx, stackDummy))
#endif
        if (RECURSION_TOO_DEEP())
            DelayMarkingChildren(rt, thing);
        else
            JS_TraceChildren(trc, thing, kind);
    } else {
        














        cx->insideGCMarkCallback = false;
        JS_TraceChildren(trc, thing, kind);
        MarkDelayedChildren(trc);
        cx->insideGCMarkCallback = true;
    }

  out:
#ifdef DEBUG
    trc->debugPrinter = NULL;
    trc->debugPrintArg = NULL;
#endif
    return;     
}

void
CallGCMarkerForGCThing(JSTracer *trc, void *thing)
{
#ifdef DEBUG
    



    jsboxedword w = (jsboxedword)thing;
    JS_ASSERT(JSBOXEDWORD_IS_OBJECT(w));
#endif
    
    if (!thing)
        return;

    uint32 kind = js_GetGCThingTraceKind(thing);
    CallGCMarker(trc, thing, kind);
}

} 

static void
gc_root_traversal(JSTracer *trc, const RootEntry &entry)
{
#ifdef DEBUG
    void *ptr;
    if (entry.value.type == JS_GC_ROOT_GCTHING_PTR) {
        ptr = entry.key;
    } else {
        Value *vp = static_cast<Value *>(entry.key);
        ptr = vp->isGCThing() ? vp->asGCThing() : NULL;
    }

    if (ptr) {
        if (!JSString::isStatic(ptr)) {
            bool root_points_to_gcArenaList = false;
            jsuword thing = (jsuword) ptr;
            JSRuntime *rt = trc->context->runtime;
            for (unsigned i = 0; i != FINALIZE_LIMIT; i++) {
                JSGCArenaList *arenaList = &rt->gcArenaList[i];
                size_t thingSize = arenaList->thingSize;
                size_t limit = ThingsPerArena(thingSize) * thingSize;
                for (JSGCArena *a = arenaList->head;
                     a;
                     a = a->getInfo()->prev) {
                    if (thing - a->toPageStart() < limit) {
                        root_points_to_gcArenaList = true;
                        break;
                    }
                }
            }
            if (!root_points_to_gcArenaList) {
                for (JSGCArena *a = rt->gcDoubleArenaList.head;
                     a;
                     a = a->getInfo()->prev) {
                    if (thing - a->toPageStart() <
                        DOUBLES_PER_ARENA * sizeof(jsdouble)) {
                        root_points_to_gcArenaList = true;
                        break;
                    }
                }
            }
            if (!root_points_to_gcArenaList && entry.value.name) {
                fprintf(stderr,
"JS API usage error: the address passed to JS_AddNamedRoot currently holds an\n"
"invalid gcthing.  This is usually caused by a missing call to JS_RemoveRoot.\n"
"The root's name is \"%s\".\n",
                        entry.value.name);
            }
            JS_ASSERT(root_points_to_gcArenaList);
        }
    }
#endif

    JS_SET_TRACING_NAME(trc, entry.value.name ? entry.value.name : "root");
    if (entry.value.type == JS_GC_ROOT_GCTHING_PTR)
        CallGCMarkerForGCThing(trc, entry.key);
    else
        CallGCMarkerIfGCThing(trc, *static_cast<Value *>(entry.key));
}

static JSDHashOperator
gc_lock_traversal(JSDHashTable *table, JSDHashEntryHdr *hdr, uint32 num,
                  void *arg)
{
    JSGCLockHashEntry *lhe = (JSGCLockHashEntry *)hdr;
    void *thing = (void *)lhe->thing;
    JSTracer *trc = (JSTracer *)arg;
    uint32 traceKind;

    JS_ASSERT(lhe->count >= 1);
    traceKind = js_GetGCThingTraceKind(thing);
    JS_CALL_TRACER(trc, thing, traceKind, "locked object");
    return JS_DHASH_NEXT;
}

namespace js {

void
TraceObjectVector(JSTracer *trc, JSObject **vec, uint32 len)
{
    for (uint32 i = 0; i < len; i++) {
        if (JSObject *obj = vec[i]) {
            JS_SET_TRACING_INDEX(trc, "vector", i);
            CallGCMarker(trc, obj, JSTRACE_OBJECT);
        }
    }
}

}

void
js_TraceStackFrame(JSTracer *trc, JSStackFrame *fp)
{

    if (fp->callobj)
        JS_CALL_OBJECT_TRACER(trc, fp->callobj, "call");
    if (fp->argsobj)
        JS_CALL_OBJECT_TRACER(trc, fp->argsobj, "arguments");
    if (fp->script)
        js_TraceScript(trc, fp->script);

    
    CallGCMarkerIfGCThing(trc, fp->thisv, "this");
    CallGCMarkerIfGCThing(trc, fp->rval, "rval");
    if (fp->scopeChain)
        JS_CALL_OBJECT_TRACER(trc, fp->scopeChain, "scope chain");
}

void
JSWeakRoots::mark(JSTracer *trc)
{
#ifdef DEBUG
    const char * const newbornNames[] = {
        "newborn_object",             
        "newborn_function",           
#if JS_HAS_XML_SUPPORT
        "newborn_xml",                
#endif
        "newborn_string",             
        "newborn_external_string0",   
        "newborn_external_string1",   
        "newborn_external_string2",   
        "newborn_external_string3",   
        "newborn_external_string4",   
        "newborn_external_string5",   
        "newborn_external_string6",   
        "newborn_external_string7",   
    };
#endif
    for (size_t i = 0; i != JS_ARRAY_LENGTH(finalizableNewborns); ++i) {
        void *newborn = finalizableNewborns[i];
        if (newborn) {
            JS_CALL_TRACER(trc, newborn, GetFinalizableTraceKind(i),
                           newbornNames[i]);
        }
    }
    if (newbornDouble)
        JS_CALL_DOUBLE_TRACER(trc, newbornDouble, "newborn_double");
    CallGCMarkerIfGCThing(trc, ATOM_TO_JSID(lastAtom), "lastAtom");
    CallGCMarkerForGCThing(trc, lastInternalResult, "lastInternalResult");
}

void
js_TraceContext(JSTracer *trc, JSContext *acx)
{
    

    
    if (acx->globalObject && !JS_HAS_OPTION(acx, JSOPTION_UNROOTED_GLOBAL))
        JS_CALL_OBJECT_TRACER(trc, acx->globalObject, "global object");
    acx->weakRoots.mark(trc);
    if (acx->throwing) {
        CallGCMarkerIfGCThing(trc, acx->exception, "exception");
    } else {
        
        acx->exception.setNull();
    }

    for (js::AutoGCRooter *gcr = acx->autoGCRooters; gcr; gcr = gcr->down)
        gcr->trace(trc);

    if (acx->sharpObjectMap.depth > 0)
        js_TraceSharpMap(trc, &acx->sharpObjectMap);

    js_TraceRegExpStatics(trc, acx);

    CallGCMarkerIfGCThing(trc, acx->iterValue, "iterValue");

#ifdef JS_TRACER
    TracerState* state = acx->tracerState;
    while (state) {
        if (state->nativeVp)
            TraceValues(trc, state->nativeVpLen, state->nativeVp, "nativeVp");
        state = state->prev;
    }
#endif
}

JS_REQUIRES_STACK void
js_TraceRuntime(JSTracer *trc)
{
    JSRuntime *rt = trc->context->runtime;
    JSContext *iter, *acx;

    for (RootRange r = rt->gcRootsHash.all(); !r.empty(); r.popFront())
        gc_root_traversal(trc, r.front());

    JS_DHashTableEnumerate(&rt->gcLocksHash, gc_lock_traversal, trc);
    js_TraceAtomState(trc);
    js_TraceRuntimeNumberState(trc);
    js_MarkTraps(trc);

    iter = NULL;
    while ((acx = js_ContextIterator(rt, JS_TRUE, &iter)) != NULL)
        js_TraceContext(trc, acx);

    js_TraceThreads(rt, trc);

    if (rt->gcExtraRootsTraceOp)
        rt->gcExtraRootsTraceOp(trc, rt->gcExtraRootsData);
}

void
js_TriggerGC(JSContext *cx, JSBool gcLocked)
{
    JSRuntime *rt = cx->runtime;

#ifdef JS_THREADSAFE
    JS_ASSERT(cx->requestDepth > 0);
#endif
    JS_ASSERT(!rt->gcRunning);
    if (rt->gcIsNeeded)
        return;

    



    rt->gcIsNeeded = JS_TRUE;
    js_TriggerAllOperationCallbacks(rt, gcLocked);
}

static void
ProcessSetSlotRequest(JSContext *cx, JSSetSlotRequest *ssr)
{
    JSObject *obj = ssr->obj;
    JSObject *pobj = ssr->pobj;
    uint32 slot = ssr->slot;

    while (pobj) {
        pobj = js_GetWrappedObject(cx, pobj);
        if (pobj == obj) {
            ssr->cycle = true;
            return;
        }
        pobj = pobj->getSlot(slot).asObjectOrNull();
    }

    pobj = ssr->pobj;
    if (slot == JSSLOT_PROTO) {
        obj->setProto(ObjectOrNullTag(pobj));
    } else {
        JS_ASSERT(slot == JSSLOT_PARENT);
        obj->setParent(pobj);
    }
}

void
js_DestroyScriptsToGC(JSContext *cx, JSThreadData *data)
{
    JSScript **listp, *script;

    for (size_t i = 0; i != JS_ARRAY_LENGTH(data->scriptsToGC); ++i) {
        listp = &data->scriptsToGC[i];
        while ((script = *listp) != NULL) {
            *listp = script->u.nextToGC;
            script->u.nextToGC = NULL;
            js_DestroyScript(cx, script);
        }
    }
}

inline void
FinalizeObject(JSContext *cx, JSObject *obj, unsigned thingKind)
{
    JS_ASSERT(thingKind == FINALIZE_OBJECT ||
              thingKind == FINALIZE_FUNCTION);

    
    if (!obj->map)
        return;

    
    Class *clasp = obj->getClass();
    if (clasp->finalize)
        clasp->finalize(cx, obj);

    DTrace::finalizeObject(obj);

    if (JS_LIKELY(obj->isNative())) {
        JSScope *scope = obj->scope();
        if (scope->isSharedEmpty())
            static_cast<JSEmptyScope *>(scope)->dropFromGC(cx);
        else
            scope->destroy(cx);
    }
    if (obj->hasSlotsArray())
        obj->freeSlotsArray(cx);
}

inline void
FinalizeFunction(JSContext *cx, JSFunction *fun, unsigned thingKind)
{
    FinalizeObject(cx, FUN_OBJECT(fun), thingKind);
}

inline void
FinalizeHookedObject(JSContext *cx, JSObject *obj, unsigned thingKind)
{
    if (!obj->map)
        return;

    if (cx->debugHooks->objectHook) {
        cx->debugHooks->objectHook(cx, obj, JS_FALSE,
                                   cx->debugHooks->objectHookData);
    }
    FinalizeObject(cx, obj, thingKind);
}

inline void
FinalizeHookedFunction(JSContext *cx, JSFunction *fun, unsigned thingKind)
{
    FinalizeHookedObject(cx, FUN_OBJECT(fun), thingKind);
}

#if JS_HAS_XML_SUPPORT
inline void
FinalizeXML(JSContext *cx, JSXML *xml, unsigned thingKind)
{
    js_FinalizeXML(cx, xml);
}
#endif

JS_STATIC_ASSERT(JS_EXTERNAL_STRING_LIMIT == 8);
static JSStringFinalizeOp str_finalizers[JS_EXTERNAL_STRING_LIMIT] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

intN
js_ChangeExternalStringFinalizer(JSStringFinalizeOp oldop,
                                 JSStringFinalizeOp newop)
{
    for (uintN i = 0; i != JS_ARRAY_LENGTH(str_finalizers); i++) {
        if (str_finalizers[i] == oldop) {
            str_finalizers[i] = newop;
            return intN(i);
        }
    }
    return -1;
}

inline void
FinalizeString(JSContext *cx, JSString *str, unsigned thingKind)
{
    JS_ASSERT(FINALIZE_STRING == thingKind);
    JS_ASSERT(!JSString::isStatic(str));
    JS_RUNTIME_UNMETER(cx->runtime, liveStrings);
    if (str->isDependent()) {
        JS_ASSERT(str->dependentBase());
        JS_RUNTIME_UNMETER(cx->runtime, liveDependentStrings);
    } else {
        



        cx->free(str->flatChars());
    }
}

inline void
FinalizeExternalString(JSContext *cx, JSString *str, unsigned thingKind)
{
    unsigned type = thingKind - FINALIZE_EXTERNAL_STRING0;
    JS_ASSERT(type < JS_ARRAY_LENGTH(str_finalizers));
    JS_ASSERT(!JSString::isStatic(str));
    JS_ASSERT(!str->isDependent());

    JS_RUNTIME_UNMETER(cx->runtime, liveStrings);

    
    jschar *chars = str->flatChars();
    if (!chars)
        return;
    JSStringFinalizeOp finalizer = str_finalizers[type];
    if (finalizer)
        finalizer(cx, str);
}





void
js_FinalizeStringRT(JSRuntime *rt, JSString *str)
{
    JS_RUNTIME_UNMETER(rt, liveStrings);
    JS_ASSERT(!JSString::isStatic(str));

    if (str->isDependent()) {
        
        JS_ASSERT(JSGCArenaInfo::fromGCThing(str)->list->thingKind ==
                  FINALIZE_STRING);
        JS_ASSERT(str->dependentBase());
        JS_RUNTIME_UNMETER(rt, liveDependentStrings);
    } else {
        unsigned thingKind = JSGCArenaInfo::fromGCThing(str)->list->thingKind;
        JS_ASSERT(IsFinalizableStringKind(thingKind));

        
        jschar *chars = str->flatChars();
        if (!chars)
            return;
        if (thingKind == FINALIZE_STRING) {
            rt->free(chars);
        } else {
            unsigned type = thingKind - FINALIZE_EXTERNAL_STRING0;
            JS_ASSERT(type < JS_ARRAY_LENGTH(str_finalizers));
            JSStringFinalizeOp finalizer = str_finalizers[type];
            if (finalizer) {
                



                finalizer(NULL, str);
            }
        }
    }
}

template<typename T,
         void finalizer(JSContext *cx, T *thing, unsigned thingKind)>
static void
FinalizeArenaList(JSContext *cx, unsigned thingKind)
{
    JS_STATIC_ASSERT(!(sizeof(T) & GC_CELL_MASK));
    JSGCArenaList *arenaList = &cx->runtime->gcArenaList[thingKind];
    JS_ASSERT(sizeof(T) == arenaList->thingSize);

    JSGCArena **ap = &arenaList->head;
    JSGCArena *a = *ap;
    if (!a)
        return;

#ifdef JS_GCMETER
    uint32 nlivearenas = 0, nkilledarenas = 0, nthings = 0;
#endif
    for (;;) {
        JSGCArenaInfo *ainfo = a->getInfo();
        JS_ASSERT(ainfo->list == arenaList);
        JS_ASSERT(!a->getMarkingDelay()->link);
        JS_ASSERT(a->getMarkingDelay()->unmarkedChildren == 0);

        JSGCThing *freeList = NULL;
        JSGCThing **tailp = &freeList;
        bool allClear = true;

        jsuword thing = a->toPageStart();
        jsuword thingsEnd = thing + GC_ARENA_SIZE / sizeof(T) * sizeof(T);

        jsuword nextFree = reinterpret_cast<jsuword>(ainfo->freeList);
        if (!nextFree) {
            nextFree = thingsEnd;
        } else {
            JS_ASSERT(thing <= nextFree);
            JS_ASSERT(nextFree < thingsEnd);
        }

        jsuword gcCellIndex = 0;
        jsbitmap *bitmap = a->getMarkBitmap();
        for (;; thing += sizeof(T), gcCellIndex += sizeof(T) >> GC_CELL_SHIFT) {
            if (thing == nextFree) {
                if (thing == thingsEnd)
                    break;
                nextFree = reinterpret_cast<jsuword>(
                    reinterpret_cast<JSGCThing *>(nextFree)->link);
                if (!nextFree) {
                    nextFree = thingsEnd;
                } else {
                    JS_ASSERT(thing < nextFree);
                    JS_ASSERT(nextFree < thingsEnd);
                }
            } else if (JS_TEST_BIT(bitmap, gcCellIndex)) {
                allClear = false;
                METER(nthings++);
                continue;
            } else {
                T *t = reinterpret_cast<T *>(thing);
                finalizer(cx, t, thingKind);
#ifdef DEBUG
                memset(t, JS_FREE_PATTERN, sizeof(T));
#endif
            }
            JSGCThing *t = reinterpret_cast<JSGCThing *>(thing);
            *tailp = t;
            tailp = &t->link;
        }

#ifdef DEBUG
        
        unsigned nfree = 0;
        if (freeList) {
            JS_ASSERT(tailp != &freeList);
            JSGCThing *t = freeList;
            for (;;) {
                ++nfree;
                if (&t->link == tailp)
                    break;
                JS_ASSERT(t < t->link);
                t = t->link;
            }
        }
#endif
        if (allClear) {
            



            JS_ASSERT(nfree == ThingsPerArena(sizeof(T)));
            *ap = ainfo->prev;
            ReleaseGCArena(cx->runtime, a);
            METER(nkilledarenas++);
        } else {
            JS_ASSERT(nfree < ThingsPerArena(sizeof(T)));
            a->clearMarkBitmap();
            *tailp = NULL;
            ainfo->freeList = freeList;
            ap = &ainfo->prev;
            METER(nlivearenas++);
        }
        if (!(a = *ap))
            break;
    }
    arenaList->cursor = arenaList->head;

    METER(UpdateArenaStats(&cx->runtime->gcStats.arenaStats[thingKind],
                           nlivearenas, nkilledarenas, nthings));
}

#ifdef MOZ_GCTIMER
struct GCTimer {
    uint64 enter;
    uint64 startMark;
    uint64 startSweep;
    uint64 sweepObjectEnd;
    uint64 sweepStringEnd;
    uint64 sweepDoubleEnd;
    uint64 sweepDestroyEnd;
    uint64 end;

    GCTimer() {
        getFirstEnter();
        memset(this, 0, sizeof(GCTimer));
        enter = rdtsc();
    }

    static uint64 getFirstEnter() {
        static uint64 firstEnter = rdtsc();
        return firstEnter;
    }

    void finish(bool lastGC) {
        end = rdtsc();

        if (startMark > 0) {
            static FILE *gcFile;

            if (!gcFile) {
                gcFile = fopen("gcTimer.dat", "w");
        
                fprintf(gcFile, "     AppTime,  Total,   Mark,  Sweep, FinObj, ");
                fprintf(gcFile, "FinStr, FinDbl, Destroy,  newChunks, destoyChunks\n");
            }
            JS_ASSERT(gcFile);
            fprintf(gcFile, "%12.1f, %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %7.1f, ",
                    (double)(enter - getFirstEnter()) / 1e6,
                    (double)(end - enter) / 1e6,
                    (double)(startSweep - startMark) / 1e6,
                    (double)(sweepDestroyEnd - startSweep) / 1e6,
                    (double)(sweepObjectEnd - startSweep) / 1e6,
                    (double)(sweepStringEnd - sweepObjectEnd) / 1e6,
                    (double)(sweepDoubleEnd - sweepStringEnd) / 1e6,
                    (double)(sweepDestroyEnd - sweepDoubleEnd) / 1e6);
            fprintf(gcFile, "%10d, %10d \n", newChunkCount, destroyChunkCount);
            fflush(gcFile);

            if (lastGC) {
                fclose(gcFile);
                gcFile = NULL;
            }
        }
        newChunkCount = 0;
        destroyChunkCount = 0;
    }
};

# define GCTIMER_PARAM      , GCTimer &gcTimer
# define GCTIMER_ARG        , gcTimer
# define TIMESTAMP(x)       (gcTimer.x = rdtsc())
# define GCTIMER_BEGIN()    GCTimer gcTimer
# define GCTIMER_END(last)  (gcTimer.finish(last))
#else
# define GCTIMER_PARAM
# define GCTIMER_ARG
# define TIMESTAMP(x)       ((void) 0)
# define GCTIMER_BEGIN()    ((void) 0)
# define GCTIMER_END(last)  ((void) 0)
#endif

static void
SweepDoubles(JSRuntime *rt)
{
#ifdef JS_GCMETER
    uint32 nlivearenas = 0, nkilledarenas = 0, nthings = 0;
#endif
    JSGCArena **ap = &rt->gcDoubleArenaList.head;
    while (JSGCArena *a = *ap) {
        JSGCArenaInfo *ainfo = a->getInfo();
        if (!ainfo->hasMarkedDoubles) {
            
            *ap = ainfo->prev;
            ReleaseGCArena(rt, a);
            METER(nkilledarenas++);
        } else {
#ifdef JS_GCMETER
            jsdouble *thing = reinterpret_cast<jsdouble *>(a->toPageStart());
            jsdouble *end = thing + DOUBLES_PER_ARENA;
            for (; thing != end; ++thing) {
                if (IsMarkedGCThing(thing))
                    METER(nthings++);
            }
            METER(nlivearenas++);
#endif
            ainfo->hasMarkedDoubles = false;
            ap = &ainfo->prev;
        }
    }
    METER(UpdateArenaStats(&rt->gcStats.doubleArenaStats,
                           nlivearenas, nkilledarenas, nthings));
    rt->gcDoubleArenaList.cursor = rt->gcDoubleArenaList.head;
}

#ifdef JS_THREADSAFE

namespace js {

JS_FRIEND_API(void)
BackgroundSweepTask::replenishAndFreeLater(void *ptr)
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

void
BackgroundSweepTask::run()
{
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
}

}

#endif 





static void
PreGCCleanup(JSContext *cx, JSGCInvocationKind gckind)
{
    JSRuntime *rt = cx->runtime;

    
    rt->gcIsNeeded = JS_FALSE;

    
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
        rt->gcRegenShapesScopeFlag ^= JSScope::SHAPE_REGEN;
        rt->shapeGen = 0;
        rt->protoHazardShape = 0;
    }

    js_PurgeThreads(cx);
    {
        JSContext *iter = NULL;
        while (JSContext *acx = js_ContextIterator(rt, JS_TRUE, &iter))
            acx->purge();
    }

    JS_CLEAR_WEAK_ROOTS(&cx->weakRoots);
}








static void
GC(JSContext *cx  GCTIMER_PARAM)
{
    JSRuntime *rt = cx->runtime;
    rt->gcNumber++;
    JS_ASSERT(!rt->gcUnmarkedArenaStackTop);
    JS_ASSERT(rt->gcMarkLaterCount == 0);

    


    JSTracer trc;
    JS_TRACER_INIT(&trc, cx, NULL);
    rt->gcMarkingTracer = &trc;
    JS_ASSERT(IS_GC_MARKING_TRACER(&trc));

    for (JSGCArena *a = rt->gcDoubleArenaList.head; a;) {
        JSGCArenaInfo *ainfo = a->getInfo();
        JS_ASSERT(!ainfo->hasMarkedDoubles);
        a = ainfo->prev;
    }

    js_TraceRuntime(&trc);
    js_MarkScriptFilenames(rt);

    



    MarkDelayedChildren(&trc);

    JS_ASSERT(!cx->insideGCMarkCallback);
    if (rt->gcCallback) {
        cx->insideGCMarkCallback = JS_TRUE;
        (void) rt->gcCallback(cx, JSGC_MARK_END);
        JS_ASSERT(cx->insideGCMarkCallback);
        cx->insideGCMarkCallback = JS_FALSE;
    }
    JS_ASSERT(rt->gcMarkLaterCount == 0);

    rt->gcMarkingTracer = NULL;

#ifdef JS_THREADSAFE
    JS_ASSERT(!cx->gcSweepTask);
    if (!rt->gcHelperThread.busy())
        cx->gcSweepTask = new js::BackgroundSweepTask();
#endif

    













    TIMESTAMP(startSweep);
    js_SweepAtomState(cx);

    
    js_SweepWatchPoints(cx);

#ifdef DEBUG
    
    rt->liveScopePropsPreSweep = rt->liveScopeProps;
#endif

    










    JS_ASSERT(!rt->gcEmptyArenaList);
    if (!cx->debugHooks->objectHook) {
        FinalizeArenaList<JSObject, FinalizeObject>(cx, FINALIZE_OBJECT);
        FinalizeArenaList<JSFunction, FinalizeFunction>(cx, FINALIZE_FUNCTION);
    } else {
        FinalizeArenaList<JSObject, FinalizeHookedObject>(cx, FINALIZE_OBJECT);
        FinalizeArenaList<JSFunction, FinalizeHookedFunction>(cx, FINALIZE_FUNCTION);
    }
#if JS_HAS_XML_SUPPORT
    FinalizeArenaList<JSXML, FinalizeXML>(cx, FINALIZE_XML);
#endif
    TIMESTAMP(sweepObjectEnd);

    



    rt->deflatedStringCache->sweep(cx);

    FinalizeArenaList<JSString, FinalizeString>(cx, FINALIZE_STRING);
    for (unsigned i = FINALIZE_EXTERNAL_STRING0;
         i <= FINALIZE_EXTERNAL_STRING_LAST;
         ++i) {
        FinalizeArenaList<JSString, FinalizeExternalString>(cx, i);
    }
    TIMESTAMP(sweepStringEnd);

    SweepDoubles(rt);
    TIMESTAMP(sweepDoubleEnd);

    



    js::SweepScopeProperties(cx);

    





    js_SweepScriptFilenames(rt);

    



    FreeGCChunks(rt);
    TIMESTAMP(sweepDestroyEnd);

#ifdef JS_THREADSAFE
    if (cx->gcSweepTask) {
        rt->gcHelperThread.schedule(cx->gcSweepTask);
        cx->gcSweepTask = NULL;
    }
#endif

    if (rt->gcCallback)
        (void) rt->gcCallback(cx, JSGC_FINALIZE_END);
#ifdef DEBUG_srcnotesize
  { extern void DumpSrcNoteSizeHist();
    DumpSrcNoteSizeHist();
    printf("GC HEAP SIZE %lu\n", (unsigned long)rt->gcBytes);
  }
#endif

#ifdef JS_SCOPE_DEPTH_METER
  { static FILE *fp;
    if (!fp)
        fp = fopen("/tmp/scopedepth.stats", "w");

    if (fp) {
        JS_DumpBasicStats(&rt->protoLookupDepthStats, "proto-lookup depth", fp);
        JS_DumpBasicStats(&rt->scopeSearchDepthStats, "scope-search depth", fp);
        JS_DumpBasicStats(&rt->hostenvScopeDepthStats, "hostenv scope depth", fp);
        JS_DumpBasicStats(&rt->lexicalScopeDepthStats, "lexical scope depth", fp);

        putc('\n', fp);
        fflush(fp);
    }
  }
#endif 

#ifdef JS_DUMP_LOOP_STATS
  { static FILE *lsfp;
    if (!lsfp)
        lsfp = fopen("/tmp/loopstats", "w");
    if (lsfp) {
        JS_DumpBasicStats(&rt->loopStats, "loops", lsfp);
        fflush(lsfp);
    }
  }
#endif 
}





static void
GCUntilDone(JSContext *cx, JSGCInvocationKind gckind  GCTIMER_PARAM)
{
    JS_ASSERT_NOT_ON_TRACE(cx);
    JSRuntime *rt = cx->runtime;
    bool firstRun = true;

    do {
        rt->gcLevel = 1;
        rt->gcPoke = JS_FALSE;

        AutoUnlockGC unlock(rt);
        if (firstRun) {
            PreGCCleanup(cx, gckind);
            TIMESTAMP(startMark);
            firstRun = false;
        }
        GC(cx  GCTIMER_ARG);

        
        
        
        
    } while (rt->gcLevel > 1 || rt->gcPoke);
}

#ifdef JS_THREADSAFE






static void
DelegateGC(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    JS_ASSERT(rt->gcThread);

    
    rt->gcLevel++;
    METER_UPDATE_MAX(rt->gcStats.maxlevel, rt->gcLevel);

    




    if (rt->gcThread == cx->thread)
        return;

    



    size_t requestDebit = js_CountThreadRequests(cx);
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

        
        cx->thread->gcWaiting = true;
        js_ShareWaitingTitles(cx);

        



        JS_ASSERT(rt->gcLevel > 0);
        do {
            JS_AWAIT_GC_DONE(rt);
        } while (rt->gcLevel > 0);

        cx->thread->gcWaiting = false;
        rt->requestCount += requestDebit;
    }
}

#endif















static bool
BeginGCSession(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    METER(rt->gcStats.poke++);
    rt->gcPoke = JS_FALSE;

#ifdef JS_THREADSAFE
    



    if (rt->gcLevel > 0) {
        DelegateGC(cx);
        return false;
    }

    
    rt->gcLevel = 1;
    rt->gcThread = cx->thread;

    





    js_NudgeOtherContexts(cx);

    





    size_t requestDebit = js_CountThreadRequests(cx);
    JS_ASSERT_IF(cx->requestDepth != 0, requestDebit >= 1);
    JS_ASSERT(requestDebit <= rt->requestCount);
    if (requestDebit != rt->requestCount) {
        rt->requestCount -= requestDebit;

        





        cx->thread->gcWaiting = true;
        js_ShareWaitingTitles(cx);
        do {
            JS_AWAIT_REQUEST_DONE(rt);
        } while (rt->requestCount > 0);
        cx->thread->gcWaiting = false;
        rt->requestCount += requestDebit;
    }

#else  

    
    rt->gcLevel++;
    METER_UPDATE_MAX(rt->gcStats.maxlevel, rt->gcLevel);
    if (rt->gcLevel > 1)
        return false;

#endif 

    







    rt->gcRunning = JS_TRUE;
    return true;
}


static void
EndGCSession(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    rt->gcLevel = 0;
    rt->gcRunning = rt->gcRegenShapes = false;
#ifdef JS_THREADSAFE
    JS_ASSERT(rt->gcThread == cx->thread);
    rt->gcThread = NULL;
    JS_NOTIFY_GC_DONE(rt);
#endif
}





static bool
FireGCBegin(JSContext *cx, JSGCInvocationKind gckind)
{
    JSRuntime *rt = cx->runtime;
    JSGCCallback callback = rt->gcCallback;

    





    if (gckind != GC_SET_SLOT_REQUEST && callback) {
        Conditionally<AutoUnlockGC> unlockIf(!!(gckind & GC_LOCK_HELD), rt);
        return callback(cx, JSGC_BEGIN) || gckind == GC_LAST_CONTEXT;
    }
    return true;
}





static bool
FireGCEnd(JSContext *cx, JSGCInvocationKind gckind)
{
    JSRuntime *rt = cx->runtime;
    JSGCCallback callback = rt->gcCallback;

    




    if (gckind != GC_SET_SLOT_REQUEST && callback) {
        Conditionally<AutoUnlockGC> unlockIf(!!(gckind & GC_LOCK_HELD), rt);

        (void) callback(cx, JSGC_END);

        



        if (gckind == GC_LAST_CONTEXT && rt->gcPoke)
            return false;
    }
    return true;
}





static bool
ProcessAllSetSlotRequests(JSContext *cx, JSGCInvocationKind *gckindp)
{
    JSRuntime *rt = cx->runtime;

    while (JSSetSlotRequest *ssr = rt->setSlotRequests) {
        rt->setSlotRequests = ssr->next;
        AutoUnlockGC unlock(rt);
        ssr->next = NULL;
        ProcessSetSlotRequest(cx, ssr);
    }

    












    if (rt->gcLevel > 1 || rt->gcPoke || rt->gcIsNeeded) {
        rt->gcLevel = 0;
        rt->gcPoke = JS_FALSE;
        rt->gcRunning = JS_FALSE;
#ifdef JS_THREADSAFE
        rt->gcThread = NULL;
#endif
        *gckindp = GC_LOCK_HELD;
        if (!FireGCBegin(cx, *gckindp)) {  
            JS_NOTIFY_GC_DONE(rt);
            return false;
        }
        if (!BeginGCSession(cx))  
            return false;
    }
    return true;
}





void
js_GC(JSContext *cx, JSGCInvocationKind gckind)
{
    JSRuntime *rt = cx->runtime;

#ifdef JS_THREADSAFE
    JS_ASSERT(CURRENT_THREAD_IS_ME(cx->thread));
    JS_ASSERT(!JS_IS_RUNTIME_LOCKED(rt));
#endif

    





    if (rt->state != JSRTS_UP && gckind != GC_LAST_CONTEXT)
        return;

    GCTIMER_BEGIN();

    for (;;) {
        if (!FireGCBegin(cx, gckind))
            return;

        {
            
            Conditionally<AutoLockGC> lockIf(!(gckind & GC_LOCK_HELD), rt);

            if (!BeginGCSession(cx)) {
                
                return;
            }

            if (gckind == GC_SET_SLOT_REQUEST && !ProcessAllSetSlotRequests(cx, &gckind))
                return;

            if (gckind != GC_SET_SLOT_REQUEST) {
                if (!JS_ON_TRACE(cx))
                    GCUntilDone(cx, gckind  GCTIMER_ARG);
                rt->setGCLastBytes(rt->gcBytes);
            }

            EndGCSession(cx);
        }

        if (FireGCEnd(cx, gckind))
            break;
    }

    GCTIMER_END(gckind == GC_LAST_CONTEXT);
}
