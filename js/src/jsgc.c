

















































#include "jsstddef.h"
#include <stdlib.h>     
#include <string.h>     
#include "jstypes.h"
#include "jsutil.h" 
#include "jshash.h" 
#include "jsapi.h"
#include "jsatom.h"
#include "jsbit.h"
#include "jsclist.h"
#include "jscntxt.h"
#include "jsconfig.h"
#include "jsdbgapi.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsparse.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif




#ifndef JS_GC_USE_MMAP
# if defined(XP_WIN)
#  define JS_GC_USE_MMAP 1
# elif defined(XP_UNIX) || defined(XP_BEOS)
#  include <unistd.h>
#  if defined(_POSIX_MAPPED_FILES) && _POSIX_MAPPED_FILES > 0
#   define JS_GC_USE_MMAP 1
#  endif
# endif
#endif

#ifndef JS_GC_USE_MMAP
# define JS_GC_USE_MMAP 0
#endif

#if JS_GC_USE_MMAP
# if defined(XP_WIN)
#  include <windows.h>
# elif defined(XP_UNIX) || defined(XP_BEOS)
#  include <sys/mman.h>


#  if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#   define MAP_ANONYMOUS MAP_ANON
#  endif

# endif
#endif

















































#if JS_GC_USE_MMAP
# define GC_ARENA_SHIFT              12
#else
# define GC_ARENA_SHIFT              10
#endif

#define GC_ARENA_MASK               ((jsuword) JS_BITMASK(GC_ARENA_SHIFT))
#define GC_ARENA_SIZE               JS_BIT(GC_ARENA_SHIFT)

struct JSGCArenaInfo {
    


    JSGCArenaList   *list;

    





    JSGCArenaInfo   *prev;

    




    jsuword         prevUntracedPage :  JS_BITS_PER_WORD - GC_ARENA_SHIFT;

    







    jsuword         arenaIndex :        GC_ARENA_SHIFT - 1;

    


    jsuword         firstArena :        1;

    


    jsuword         untracedThings;
};







JS_STATIC_ASSERT(sizeof(JSGCArenaInfo) == 4 * sizeof(jsuword));

#define NO_FREE_ARENAS              JS_BITMASK(GC_ARENA_SHIFT - 1)













struct JSGCChunkInfo {
    JSGCChunkInfo   **prevp;
    JSGCChunkInfo   *next;
    JSGCArenaInfo   *lastFreeArena;
    uint32          numFreeArenas;
};





#if JS_GC_USE_MMAP
static uint32 js_gcArenasPerChunk = 0;
static JSBool js_gcUseMmap = JS_FALSE;
#else
# define js_gcArenasPerChunk 31
#endif





#define ARENA_INFO_OFFSET (GC_ARENA_SIZE - (uint32) sizeof(JSGCArenaInfo))

#define IS_ARENA_INFO_ADDRESS(arena)                                          \
    (((jsuword) (arena) & GC_ARENA_MASK) == ARENA_INFO_OFFSET)

#define ARENA_START_TO_INFO(arenaStart)                                       \
    (JS_ASSERT(((arenaStart) & (jsuword) GC_ARENA_MASK) == 0),                \
     (JSGCArenaInfo *) ((arenaStart) + (jsuword) ARENA_INFO_OFFSET))

#define ARENA_INFO_TO_START(arena)                                            \
    (JS_ASSERT(IS_ARENA_INFO_ADDRESS(arena)),                                 \
     (jsuword) (arena) & ~(jsuword) GC_ARENA_MASK)

#define ARENA_PAGE_TO_INFO(arenaPage)                                         \
    (JS_ASSERT(arenaPage != 0),                                               \
     JS_ASSERT(!((jsuword)(arenaPage) >> (JS_BITS_PER_WORD-GC_ARENA_SHIFT))), \
     ARENA_START_TO_INFO((arenaPage) << GC_ARENA_SHIFT))

#define ARENA_INFO_TO_PAGE(arena)                                             \
    (JS_ASSERT(IS_ARENA_INFO_ADDRESS(arena)),                                 \
     ((jsuword) (arena) >> GC_ARENA_SHIFT))

#define GET_ARENA_INFO(chunk, index)                                          \
    (JS_ASSERT((index) < js_gcArenasPerChunk),                                \
     ARENA_START_TO_INFO(chunk + ((index) << GC_ARENA_SHIFT)))




#define GET_ARENA_CHUNK(arena, index)                                         \
    (JS_ASSERT(GET_ARENA_INDEX(arena) == index),                              \
     ARENA_INFO_TO_START(arena) - ((index) << GC_ARENA_SHIFT))

#define GET_ARENA_INDEX(arena)                                                \
    ((arena)->firstArena ? 0 : (uint32) (arena)->arenaIndex)

#define GET_CHUNK_INFO_INDEX(chunk)                                           \
    ((uint32) ARENA_START_TO_INFO(chunk)->arenaIndex)

#define SET_CHUNK_INFO_INDEX(chunk, index)                                    \
    (JS_ASSERT((index) < js_gcArenasPerChunk || (index) == NO_FREE_ARENAS),   \
     (void) (ARENA_START_TO_INFO(chunk)->arenaIndex = (jsuword) (index)))

#define GET_CHUNK_INFO(chunk, infoIndex)                                      \
    (JS_ASSERT(GET_CHUNK_INFO_INDEX(chunk) == (infoIndex)),                   \
     JS_ASSERT((uint32) (infoIndex) < js_gcArenasPerChunk),                   \
     (JSGCChunkInfo *) ((chunk) + ((infoIndex) << GC_ARENA_SHIFT)))

#define CHUNK_INFO_TO_INDEX(ci)                                               \
    GET_ARENA_INDEX(ARENA_START_TO_INFO((jsuword)ci))




#define THINGS_PER_ARENA(thingSize)                                           \
    ((GC_ARENA_SIZE - (uint32) sizeof(JSGCArenaInfo)) / ((thingSize) + 1U))

#define THING_TO_ARENA(thing)                                                 \
    ((JSGCArenaInfo *)(((jsuword) (thing) | GC_ARENA_MASK) +                  \
                       1 - sizeof(JSGCArenaInfo)))

#define THING_TO_INDEX(thing, thingSize)                                      \
    ((uint32) ((jsuword) (thing) & GC_ARENA_MASK) / (uint32) (thingSize))

#define THING_FLAGS_END(arena) ((uint8 *)(arena))

#define THING_FLAGP(arena, thingIndex)                                        \
    (JS_ASSERT((jsuword) (thingIndex)                                         \
               < (jsuword) THINGS_PER_ARENA((arena)->list->thingSize)),       \
     (uint8 *)(arena) - 1 - (thingIndex))

#define THING_TO_FLAGP(thing, thingSize)                                      \
    THING_FLAGP(THING_TO_ARENA(thing), THING_TO_INDEX(thing, thingSize))

#define FLAGP_TO_ARENA(flagp) THING_TO_ARENA(flagp)

#define FLAGP_TO_INDEX(flagp)                                                 \
    (JS_ASSERT(((jsuword) (flagp) & GC_ARENA_MASK) < ARENA_INFO_OFFSET),      \
     (ARENA_INFO_OFFSET - 1 - (uint32) ((jsuword) (flagp) & GC_ARENA_MASK)))

#define FLAGP_TO_THING(flagp, thingSize)                                      \
    (JS_ASSERT(((jsuword) (flagp) & GC_ARENA_MASK) >=                         \
               (ARENA_INFO_OFFSET - THINGS_PER_ARENA(thingSize))),            \
     (JSGCThing *)(((jsuword) (flagp) & ~GC_ARENA_MASK) +                     \
                   (thingSize) * FLAGP_TO_INDEX(flagp)))

#ifdef JS_THREADSAFE







#define MAX_THREAD_LOCAL_THINGS 8

#endif

JS_STATIC_ASSERT(sizeof(JSStackHeader) >= 2 * sizeof(jsval));

JS_STATIC_ASSERT(sizeof(JSGCThing) >= sizeof(JSString));
JS_STATIC_ASSERT(sizeof(JSGCThing) >= sizeof(jsdouble));


JS_STATIC_ASSERT(sizeof(JSObject) % sizeof(JSGCThing) == 0);






JS_STATIC_ASSERT(GC_FREELIST_INDEX(sizeof(JSString)) !=
                 GC_FREELIST_INDEX(sizeof(JSObject)));
JS_STATIC_ASSERT(GC_FREELIST_INDEX(sizeof(jsdouble)) !=
                 GC_FREELIST_INDEX(sizeof(JSObject)));






typedef struct JSPtrTableInfo {
    uint16      minCapacity;
    uint16      linearGrowthThreshold;
} JSPtrTableInfo;

#define GC_ITERATOR_TABLE_MIN     4
#define GC_ITERATOR_TABLE_LINEAR  1024

static const JSPtrTableInfo iteratorTableInfo = {
    GC_ITERATOR_TABLE_MIN,
    GC_ITERATOR_TABLE_LINEAR
};


static size_t
PtrTableCapacity(size_t count, const JSPtrTableInfo *info)
{
    size_t linear, log, capacity;

    linear = info->linearGrowthThreshold;
    JS_ASSERT(info->minCapacity <= linear);

    if (count == 0) {
        capacity = 0;
    } else if (count < linear) {
        log = JS_CEILING_LOG2W(count);
        JS_ASSERT(log != JS_BITS_PER_WORD);
        capacity = (size_t)1 << log;
        if (capacity < info->minCapacity)
            capacity = info->minCapacity;
    } else {
        capacity = JS_ROUNDUP(count, linear);
    }

    JS_ASSERT(capacity >= count);
    return capacity;
}

static void
FreePtrTable(JSPtrTable *table, const JSPtrTableInfo *info)
{
    if (table->array) {
        JS_ASSERT(table->count > 0);
        free(table->array);
        table->array = NULL;
        table->count = 0;
    }
    JS_ASSERT(table->count == 0);
}

static JSBool
AddToPtrTable(JSContext *cx, JSPtrTable *table, const JSPtrTableInfo *info,
              void *ptr)
{
    size_t count, capacity;
    void **array;

    count = table->count;
    capacity = PtrTableCapacity(count, info);

    if (count == capacity) {
        if (capacity < info->minCapacity) {
            JS_ASSERT(capacity == 0);
            JS_ASSERT(!table->array);
            capacity = info->minCapacity;
        } else {
            



            JS_STATIC_ASSERT(2 <= sizeof table->array[0]);
            capacity = (capacity < info->linearGrowthThreshold)
                       ? 2 * capacity
                       : capacity + info->linearGrowthThreshold;
            if (capacity > (size_t)-1 / sizeof table->array[0])
                goto bad;
        }
        array = (void **) realloc(table->array,
                                  capacity * sizeof table->array[0]);
        if (!array)
            goto bad;
#ifdef DEBUG
        memset(array + count, JS_FREE_PATTERN,
               (capacity - count) * sizeof table->array[0]);
#endif
        table->array = array;
    }

    table->array[count] = ptr;
    table->count = count + 1;

    return JS_TRUE;

  bad:
    JS_ReportOutOfMemory(cx);
    return JS_FALSE;
}

static void
ShrinkPtrTable(JSPtrTable *table, const JSPtrTableInfo *info,
               size_t newCount)
{
    size_t oldCapacity, capacity;
    void **array;

    JS_ASSERT(newCount <= table->count);
    if (newCount == table->count)
        return;

    oldCapacity = PtrTableCapacity(table->count, info);
    table->count = newCount;
    capacity = PtrTableCapacity(newCount, info);

    if (oldCapacity != capacity) {
        array = table->array;
        JS_ASSERT(array);
        if (capacity == 0) {
            free(array);
            table->array = NULL;
            return;
        }
        array = (void **) realloc(array, capacity * sizeof array[0]);
        if (array)
            table->array = array;
    }
#ifdef DEBUG
    memset(table->array + newCount, JS_FREE_PATTERN,
           (capacity - newCount) * sizeof table->array[0]);
#endif
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





static uint32 *
GetMallocedChunkGapPtr(jsuword chunk)
{
    JS_ASSERT((chunk & GC_ARENA_MASK) == 0);

    
    return (uint32 *) (chunk + (js_gcArenasPerChunk << GC_ARENA_SHIFT));
}

static jsuword
NewGCChunk()
{
    void *p;
    jsuword chunk;

#if JS_GC_USE_MMAP
    if (js_gcUseMmap) {
# if defined(XP_WIN)
        p = VirtualAlloc(NULL, js_gcArenasPerChunk << GC_ARENA_SHIFT,
                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        return (jsuword) p;
# elif defined(XP_UNIX) || defined(XP_BEOS)
        p = mmap(NULL, js_gcArenasPerChunk << GC_ARENA_SHIFT,
                 PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        return (p == MAP_FAILED) ? 0 : (jsuword) p;
# else
#  error "Not implemented"
# endif
    }
#endif

    












    p = malloc((js_gcArenasPerChunk + 1) << GC_ARENA_SHIFT);
    if (!p)
        return 0;
    chunk = ((jsuword) p + GC_ARENA_MASK) & ~GC_ARENA_MASK;
    *GetMallocedChunkGapPtr(chunk) = (uint32) (chunk - (jsuword) p);
    return chunk;
}

static void
DestroyGCChunk(jsuword chunk)
{
    JS_ASSERT((chunk & GC_ARENA_MASK) == 0);
#if JS_GC_USE_MMAP
    if (js_gcUseMmap) {
# if defined(XP_WIN)
        VirtualFree((void *) chunk, 0, MEM_RELEASE);
# elif defined(XP_UNIX) || defined(XP_BEOS)
        munmap((void *) chunk, js_gcArenasPerChunk << GC_ARENA_SHIFT);
# else
#  error "Not implemented"
# endif
        return;
    }
#endif

    
    JS_ASSERT(*GetMallocedChunkGapPtr(chunk) < GC_ARENA_SIZE);
    free((void *) (chunk - *GetMallocedChunkGapPtr(chunk)));
}

static void
AddChunkToList(JSRuntime *rt, JSGCChunkInfo *ci)
{
    ci->prevp = &rt->gcChunkList;
    ci->next = rt->gcChunkList;
    if (rt->gcChunkList) {
        JS_ASSERT(rt->gcChunkList->prevp == &rt->gcChunkList);
        rt->gcChunkList->prevp = &ci->next;
    }
    rt->gcChunkList = ci;
}

static void
RemoveChunkFromList(JSRuntime *rt, JSGCChunkInfo *ci)
{
    *ci->prevp = ci->next;
    if (ci->next) {
        JS_ASSERT(ci->next->prevp == &ci->next);
        ci->next->prevp = ci->prevp;
    }
}

static JSGCArenaInfo *
NewGCArena(JSRuntime *rt)
{
    jsuword chunk;
    JSGCChunkInfo *ci;
    uint32 i;
    JSGCArenaInfo *a, *aprev;

    if (js_gcArenasPerChunk == 1) {
        chunk = NewGCChunk();
        return (chunk == 0) ? NULL : ARENA_START_TO_INFO(chunk);
    }

    ci = rt->gcChunkList;
    if (!ci) {
        chunk = NewGCChunk();
        if (chunk == 0)
            return NULL;
        JS_ASSERT((chunk & GC_ARENA_MASK) == 0);
        a = GET_ARENA_INFO(chunk, 0);
        a->firstArena = JS_TRUE;
        a->arenaIndex = 0;
        aprev = NULL;
        i = 0;
        do {
            a->prev = aprev;
            aprev = a;
            ++i;
            a = GET_ARENA_INFO(chunk, i);
            a->firstArena = JS_FALSE;
            a->arenaIndex = i;
        } while (i != js_gcArenasPerChunk - 1);
        ci = GET_CHUNK_INFO(chunk, 0);
        ci->lastFreeArena = aprev;
        ci->numFreeArenas = js_gcArenasPerChunk - 1;
        AddChunkToList(rt, ci);
    } else {
        JS_ASSERT(ci->prevp == &rt->gcChunkList);
        a = ci->lastFreeArena;
        aprev = a->prev;
        if (!aprev) {
            JS_ASSERT(ci->numFreeArenas == 1);
            JS_ASSERT(ARENA_INFO_TO_START(a) == (jsuword) ci);
            RemoveChunkFromList(rt, ci);
            chunk = GET_ARENA_CHUNK(a, GET_ARENA_INDEX(a));
            SET_CHUNK_INFO_INDEX(chunk, NO_FREE_ARENAS);
        } else {
            JS_ASSERT(ci->numFreeArenas >= 2);
            JS_ASSERT(ARENA_INFO_TO_START(a) != (jsuword) ci);
            ci->lastFreeArena = aprev;
            ci->numFreeArenas--;
        }
    }

    return a;
}

static void
DestroyGCArena(JSRuntime *rt, JSGCArenaInfo *a)
{
    uint32 arenaIndex;
    jsuword chunk;
    uint32 chunkInfoIndex;
    JSGCChunkInfo *ci;

    METER(rt->gcStats.afree++);

    if (js_gcArenasPerChunk == 1) {
        DestroyGCChunk(ARENA_INFO_TO_START(a));
        return;
    }

#ifdef DEBUG
    {
        jsuword firstArena, arenaIndex;

        firstArena = a->firstArena;
        arenaIndex = a->arenaIndex;
        memset((void *) ARENA_INFO_TO_START(a), JS_FREE_PATTERN,
               GC_ARENA_SIZE);
        a->firstArena = firstArena;
        a->arenaIndex = arenaIndex;
    }
#endif

    arenaIndex = GET_ARENA_INDEX(a);
    chunk = GET_ARENA_CHUNK(a, arenaIndex);
    chunkInfoIndex = GET_CHUNK_INFO_INDEX(chunk);
    if (chunkInfoIndex == NO_FREE_ARENAS) {
        chunkInfoIndex = arenaIndex;
        SET_CHUNK_INFO_INDEX(chunk, arenaIndex);
        ci = GET_CHUNK_INFO(chunk, chunkInfoIndex);
        a->prev = NULL;
        ci->lastFreeArena = a;
        ci->numFreeArenas = 1;
        AddChunkToList(rt, ci);
    } else {
        JS_ASSERT(chunkInfoIndex != arenaIndex);
        ci = GET_CHUNK_INFO(chunk, chunkInfoIndex);
        JS_ASSERT(ci->numFreeArenas != 0);
        JS_ASSERT(ci->lastFreeArena);
        JS_ASSERT(a != ci->lastFreeArena);
        if (ci->numFreeArenas == js_gcArenasPerChunk - 1) {
            RemoveChunkFromList(rt, ci);
            DestroyGCChunk(chunk);
        } else {
            ++ci->numFreeArenas;
            a->prev = ci->lastFreeArena;
            ci->lastFreeArena = a;
        }
    }
}

static void
InitGCArenaLists(JSRuntime *rt)
{
    uintN i, thingSize;
    JSGCArenaList *arenaList;

    for (i = 0; i < GC_NUM_FREELISTS; i++) {
        arenaList = &rt->gcArenaList[i];
        thingSize = GC_FREELIST_NBYTES(i);
        JS_ASSERT((size_t)(uint16)thingSize == thingSize);
        arenaList->last = NULL;
        arenaList->lastCount = THINGS_PER_ARENA(thingSize);
        arenaList->thingSize = (uint16)thingSize;
        arenaList->freeList = NULL;
    }
}

static void
FinishGCArenaLists(JSRuntime *rt)
{
    uintN i;
    JSGCArenaList *arenaList;
    JSGCArenaInfo *a, *aprev;

    for (i = 0; i < GC_NUM_FREELISTS; i++) {
        arenaList = &rt->gcArenaList[i];

        for (a = arenaList->last; a; a = aprev) {
            aprev = a->prev;
            DestroyGCArena(rt, a);
        }
        arenaList->last = NULL;
        arenaList->lastCount = THINGS_PER_ARENA(arenaList->thingSize);
        arenaList->freeList = NULL;
        METER(rt->gcStats.arenas[i].narenas = 0);
    }
    rt->gcBytes = 0;
    JS_ASSERT(rt->gcChunkList == 0);
}

static uint8 *
GetGCThingFlags(void *thing)
{
    JSGCArenaInfo *a;
    uint32 index;

    a = THING_TO_ARENA(thing);
    index = THING_TO_INDEX(thing, a->list->thingSize);
    return THING_FLAGP(a, index);
}

intN
js_GetExternalStringGCType(JSString *str)
{
    uintN type;

    type = (uintN) *GetGCThingFlags(str) & GCF_TYPEMASK;
    JS_ASSERT(type == GCX_STRING || type >= GCX_EXTERNAL_STRING);
    return (type == GCX_STRING) ? -1 : (intN) (type - GCX_EXTERNAL_STRING);
}

static uint32
MapGCFlagsToTraceKind(uintN flags)
{
    uint32 type;

    type = flags & GCF_TYPEMASK;
    JS_ASSERT(type < GCX_NTYPES);
    return (type < GCX_EXTERNAL_STRING) ? type : JSTRACE_STRING;
}

JS_FRIEND_API(uint32)
js_GetGCThingTraceKind(void *thing)
{
    return MapGCFlagsToTraceKind(*GetGCThingFlags(thing));
}

JSRuntime*
js_GetGCStringRuntime(JSString *str)
{
    JSGCArenaList *list;

    list = THING_TO_ARENA(str)->list;

    JS_ASSERT(list->thingSize == sizeof(JSGCThing));
    JS_ASSERT(GC_FREELIST_INDEX(sizeof(JSGCThing)) == 0);

    return (JSRuntime *)((uint8 *)list - offsetof(JSRuntime, gcArenaList));
}

JSBool
js_IsAboutToBeFinalized(JSContext *cx, void *thing)
{
    uint8 flags = *GetGCThingFlags(thing);

    return !(flags & (GCF_MARK | GCF_LOCK | GCF_FINAL));
}


typedef struct JSGCRootHashEntry {
    JSDHashEntryHdr hdr;
    void            *root;
    const char      *name;
} JSGCRootHashEntry;


#define GC_ROOTS_SIZE   256





#define GC_ARENAS_PER_CPU_PAGE_LIMIT JS_BIT(18 - GC_ARENA_SHIFT)

JS_STATIC_ASSERT(GC_ARENAS_PER_CPU_PAGE_LIMIT <= NO_FREE_ARENAS);

JSBool
js_InitGC(JSRuntime *rt, uint32 maxbytes)
{
#if JS_GC_USE_MMAP
    if (js_gcArenasPerChunk == 0) {
        size_t cpuPageSize, arenasPerPage;
# if defined(XP_WIN)
        SYSTEM_INFO si;

        GetSystemInfo(&si);
        cpuPageSize = si.dwPageSize;

# elif defined(XP_UNIX) || defined(XP_BEOS)
        cpuPageSize = (size_t) sysconf(_SC_PAGESIZE);
# else
#  error "Not implemented"
# endif
        
        JS_ASSERT((cpuPageSize & (cpuPageSize - 1)) == 0);
        arenasPerPage = cpuPageSize >> GC_ARENA_SHIFT;
#ifdef DEBUG
        if (arenasPerPage == 0) {
            fprintf(stderr,
"JS engine warning: the size of the CPU page, %u bytes, is too low to use\n"
"paged allocation for the garbage collector. Please report this.\n",
                    (unsigned) cpuPageSize);
        }
#endif
        if (arenasPerPage - 1 <= (size_t) (GC_ARENAS_PER_CPU_PAGE_LIMIT - 1)) {
            



            js_gcUseMmap = JS_TRUE;
            js_gcArenasPerChunk = JS_MAX((uint32) arenasPerPage, 4);
        } else {
            js_gcUseMmap = JS_FALSE;
            js_gcArenasPerChunk = 7;
        }
    }
#endif
    JS_ASSERT(1 <= js_gcArenasPerChunk &&
              js_gcArenasPerChunk <= NO_FREE_ARENAS);

    InitGCArenaLists(rt);
    if (!JS_DHashTableInit(&rt->gcRootsHash, JS_DHashGetStubOps(), NULL,
                           sizeof(JSGCRootHashEntry), GC_ROOTS_SIZE)) {
        rt->gcRootsHash.ops = NULL;
        return JS_FALSE;
    }
    rt->gcLocksHash = NULL;     

    



    rt->gcMaxBytes = rt->gcMaxMallocBytes = maxbytes;
    rt->gcStackPoolLifespan = 30000;

    METER(memset(&rt->gcStats, 0, sizeof rt->gcStats));
    return JS_TRUE;
}

#ifdef JS_GCMETER
JS_FRIEND_API(void)
js_DumpGCStats(JSRuntime *rt, FILE *fp)
{
    uintN i;
    size_t thingsPerArena;
    size_t totalThings, totalMaxThings, totalBytes;
    size_t sumArenas, sumTotalArenas;
    size_t sumFreeSize, sumTotalFreeSize;
    JSGCArenaList *list;
    JSGCArenaStats *stats;

    fprintf(fp, "\nGC allocation statistics:\n");

#define UL(x)       ((unsigned long)(x))
#define ULSTAT(x)   UL(rt->gcStats.x)
    totalThings = 0;

    totalMaxThings = 0;
    totalBytes = 0;
    sumArenas = 0;
    sumTotalArenas = 0;
    sumFreeSize = 0;
    sumTotalFreeSize = 0;
    for (i = 0; i < GC_NUM_FREELISTS; i++) {
        list = &rt->gcArenaList[i];
        stats = &rt->gcStats.arenas[i];
        if (stats->maxarenas == 0) {
            fprintf(fp, "ARENA LIST %u (thing size %lu): NEVER USED\n",
                    i, UL(GC_FREELIST_NBYTES(i)));
            continue;
        }
        thingsPerArena = THINGS_PER_ARENA(list->thingSize);
        fprintf(fp, "ARENA LIST %u (thing size %lu):\n",
                i, UL(GC_FREELIST_NBYTES(i)));
        fprintf(fp, "                     arenas: %lu\n", UL(stats->narenas));
        fprintf(fp, "                 max arenas: %lu\n", UL(stats->maxarenas));
        fprintf(fp, "                     things: %lu\n", UL(stats->nthings));
        fprintf(fp, "                 max things: %lu\n", UL(stats->maxthings));
        fprintf(fp, "                  free list: %lu\n", UL(stats->freelen));
        fprintf(fp, "          free list density: %.1f%%\n",
                stats->narenas == 0
                ? 0.0
                : 100.0 * stats->freelen / (thingsPerArena * stats->narenas));
        fprintf(fp, "  average free list density: %.1f%%\n",
                stats->totalarenas == 0
                ? 0.0
                : 100.0 * stats->totalfreelen /
                  (thingsPerArena * stats->totalarenas));
        fprintf(fp, "                   recycles: %lu\n", UL(stats->recycle));
        fprintf(fp, "        recycle/alloc ratio: %.2f\n",
                (double) stats->recycle / (stats->totalnew - stats->recycle));
        totalThings += stats->nthings;
        totalMaxThings += stats->maxthings;
        totalBytes += list->thingSize * stats->nthings;
        sumArenas += stats->narenas;
        sumTotalArenas += stats->totalarenas;
        sumFreeSize += list->thingSize * stats->freelen;
        sumTotalFreeSize += list->thingSize * stats->totalfreelen;
    }
    fprintf(fp, "TOTAL STATS:\n");
    fprintf(fp, "            bytes allocated: %lu\n", UL(rt->gcBytes));
    fprintf(fp, "             alloc attempts: %lu\n", ULSTAT(alloc));
#ifdef JS_THREADSAFE
    fprintf(fp, "        alloc without locks: %1u\n", ULSTAT(localalloc));
#endif
    fprintf(fp, "            total GC things: %lu\n", UL(totalThings));
    fprintf(fp, "        max total GC things: %lu\n", UL(totalMaxThings));
    fprintf(fp, "             GC things size: %lu\n", UL(totalBytes));
    fprintf(fp, "            total GC arenas: %lu\n", UL(sumArenas));
    fprintf(fp, "    total free list density: %.1f%%\n",
            sumArenas == 0
            ? 0.0
            : 100.0 * sumFreeSize / (sumArenas << GC_ARENA_SHIFT));
    fprintf(fp, "  average free list density: %.1f%%\n",
            sumTotalFreeSize == 0
            ? 0.0
            : 100.0 * sumTotalFreeSize / (sumTotalArenas << GC_ARENA_SHIFT));
    fprintf(fp, "allocation retries after GC: %lu\n", ULSTAT(retry));
    fprintf(fp, "        allocation failures: %lu\n", ULSTAT(fail));
    fprintf(fp, "         things born locked: %lu\n", ULSTAT(lockborn));
    fprintf(fp, "           valid lock calls: %lu\n", ULSTAT(lock));
    fprintf(fp, "         valid unlock calls: %lu\n", ULSTAT(unlock));
    fprintf(fp, "       mark recursion depth: %lu\n", ULSTAT(depth));
    fprintf(fp, "     maximum mark recursion: %lu\n", ULSTAT(maxdepth));
    fprintf(fp, "     mark C recursion depth: %lu\n", ULSTAT(cdepth));
    fprintf(fp, "   maximum mark C recursion: %lu\n", ULSTAT(maxcdepth));
    fprintf(fp, "      delayed tracing calls: %lu\n", ULSTAT(untraced));
#ifdef DEBUG
    fprintf(fp, "      max trace later count: %lu\n", ULSTAT(maxuntraced));
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
#undef US

#ifdef JS_ARENAMETER
    JS_DumpArenaStats(fp);
#endif
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
    js_DumpGCStats(rt, stdout);
#endif

    FreePtrTable(&rt->gcIteratorTable, &iteratorTableInfo);
    FinishGCArenaLists(rt);

    if (rt->gcRootsHash.ops) {
#ifdef DEBUG
        CheckLeakedRoots(rt);
#endif
        JS_DHashTableFinish(&rt->gcRootsHash);
        rt->gcRootsHash.ops = NULL;
    }
    if (rt->gcLocksHash) {
        JS_DHashTableDestroy(rt->gcLocksHash);
        rt->gcLocksHash = NULL;
    }
}

JSBool
js_AddRoot(JSContext *cx, void *rp, const char *name)
{
    JSBool ok = js_AddRootRT(cx->runtime, rp, name);
    if (!ok)
        JS_ReportOutOfMemory(cx);
    return ok;
}

JSBool
js_AddRootRT(JSRuntime *rt, void *rp, const char *name)
{
    JSBool ok;
    JSGCRootHashEntry *rhe;

    












    JS_LOCK_GC(rt);
#ifdef JS_THREADSAFE
    JS_ASSERT(!rt->gcRunning || rt->gcLevel > 0);
    if (rt->gcRunning && rt->gcThread->id != js_CurrentThreadId()) {
        do {
            JS_AWAIT_GC_DONE(rt);
        } while (rt->gcLevel > 0);
    }
#endif
    rhe = (JSGCRootHashEntry *)
          JS_DHashTableOperate(&rt->gcRootsHash, rp, JS_DHASH_ADD);
    if (rhe) {
        rhe->root = rp;
        rhe->name = name;
        ok = JS_TRUE;
    } else {
        ok = JS_FALSE;
    }
    JS_UNLOCK_GC(rt);
    return ok;
}

JSBool
js_RemoveRoot(JSRuntime *rt, void *rp)
{
    



    JS_LOCK_GC(rt);
#ifdef JS_THREADSAFE
    JS_ASSERT(!rt->gcRunning || rt->gcLevel > 0);
    if (rt->gcRunning && rt->gcThread->id != js_CurrentThreadId()) {
        do {
            JS_AWAIT_GC_DONE(rt);
        } while (rt->gcLevel > 0);
    }
#endif
    (void) JS_DHashTableOperate(&rt->gcRootsHash, rp, JS_DHASH_REMOVE);
    rt->gcPoke = JS_TRUE;
    JS_UNLOCK_GC(rt);
    return JS_TRUE;
}

#ifdef DEBUG

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
js_root_printer(JSDHashTable *table, JSDHashEntryHdr *hdr, uint32 i, void *arg)
{
    uint32 *leakedroots = (uint32 *)arg;
    JSGCRootHashEntry *rhe = (JSGCRootHashEntry *)hdr;

    (*leakedroots)++;
    fprintf(stderr,
            "JS engine warning: leaking GC root \'%s\' at %p\n",
            rhe->name ? (char *)rhe->name : "", rhe->root);

    return JS_DHASH_NEXT;
}

static void
CheckLeakedRoots(JSRuntime *rt)
{
    uint32 leakedroots = 0;

    
    JS_DHashTableEnumerate(&rt->gcRootsHash, js_root_printer,
                           &leakedroots);
    if (leakedroots > 0) {
        if (leakedroots == 1) {
            fprintf(stderr,
"JS engine warning: 1 GC root remains after destroying the JSRuntime.\n"
"                   This root may point to freed memory. Objects reachable\n"
"                   through it have not been finalized.\n");
        } else {
            fprintf(stderr,
"JS engine warning: %lu GC roots remain after destroying the JSRuntime.\n"
"                   These roots may point to freed memory. Objects reachable\n"
"                   through them have not been finalized.\n",
                        (unsigned long) leakedroots);
        }
    }
}

typedef struct NamedRootDumpArgs {
    void (*dump)(const char *name, void *rp, void *data);
    void *data;
} NamedRootDumpArgs;

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
js_named_root_dumper(JSDHashTable *table, JSDHashEntryHdr *hdr, uint32 number,
                     void *arg)
{
    NamedRootDumpArgs *args = (NamedRootDumpArgs *) arg;
    JSGCRootHashEntry *rhe = (JSGCRootHashEntry *)hdr;

    if (rhe->name)
        args->dump(rhe->name, rhe->root, args->data);
    return JS_DHASH_NEXT;
}

void
js_DumpNamedRoots(JSRuntime *rt,
                  void (*dump)(const char *name, void *rp, void *data),
                  void *data)
{
    NamedRootDumpArgs args;

    args.dump = dump;
    args.data = data;
    JS_DHashTableEnumerate(&rt->gcRootsHash, js_named_root_dumper, &args);
}

#endif 

typedef struct GCRootMapArgs {
    JSGCRootMapFun map;
    void *data;
} GCRootMapArgs;

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
js_gcroot_mapper(JSDHashTable *table, JSDHashEntryHdr *hdr, uint32 number,
                 void *arg)
{
    GCRootMapArgs *args = (GCRootMapArgs *) arg;
    JSGCRootHashEntry *rhe = (JSGCRootHashEntry *)hdr;
    intN mapflags;
    int op;

    mapflags = args->map(rhe->root, rhe->name, args->data);

#if JS_MAP_GCROOT_NEXT == JS_DHASH_NEXT &&                                     \
    JS_MAP_GCROOT_STOP == JS_DHASH_STOP &&                                     \
    JS_MAP_GCROOT_REMOVE == JS_DHASH_REMOVE
    op = (JSDHashOperator)mapflags;
#else
    op = JS_DHASH_NEXT;
    if (mapflags & JS_MAP_GCROOT_STOP)
        op |= JS_DHASH_STOP;
    if (mapflags & JS_MAP_GCROOT_REMOVE)
        op |= JS_DHASH_REMOVE;
#endif

    return (JSDHashOperator) op;
}

uint32
js_MapGCRoots(JSRuntime *rt, JSGCRootMapFun map, void *data)
{
    GCRootMapArgs args;
    uint32 rv;

    args.map = map;
    args.data = data;
    JS_LOCK_GC(rt);
    rv = JS_DHashTableEnumerate(&rt->gcRootsHash, js_gcroot_mapper, &args);
    JS_UNLOCK_GC(rt);
    return rv;
}

JSBool
js_RegisterCloseableIterator(JSContext *cx, JSObject *obj)
{
    JSRuntime *rt;
    JSBool ok;

    rt = cx->runtime;
    JS_ASSERT(!rt->gcRunning);

    JS_LOCK_GC(rt);
    ok = AddToPtrTable(cx, &rt->gcIteratorTable, &iteratorTableInfo, obj);
    JS_UNLOCK_GC(rt);
    return ok;
}

static void
CloseNativeIterators(JSContext *cx)
{
    JSRuntime *rt;
    size_t count, newCount, i;
    void **array;
    JSObject *obj;

    rt = cx->runtime;
    count = rt->gcIteratorTable.count;
    array = rt->gcIteratorTable.array;

    newCount = 0;
    for (i = 0; i != count; ++i) {
        obj = (JSObject *)array[i];
        if (js_IsAboutToBeFinalized(cx, obj))
            js_CloseNativeIterator(cx, obj);
        else
            array[newCount++] = obj;
    }
    ShrinkPtrTable(&rt->gcIteratorTable, &iteratorTableInfo, newCount);
}

#if defined(DEBUG_brendan) || defined(DEBUG_timeless)
#define DEBUG_gchist
#endif

#ifdef DEBUG_gchist
#define NGCHIST 64

static struct GCHist {
    JSBool      lastDitch;
    JSGCThing   *freeList;
} gchist[NGCHIST];

unsigned gchpos = 0;
#endif

void *
js_NewGCThing(JSContext *cx, uintN flags, size_t nbytes)
{
    JSRuntime *rt;
    uintN flindex;
    JSBool doGC;
    JSGCThing *thing;
    uint8 *flagp;
    JSGCArenaList *arenaList;
#ifdef JS_GCMETER
    JSGCArenaStats *listStats;
#endif
    JSGCArenaInfo *a;
    uintN thingsLimit;
    JSLocalRootStack *lrs;
#ifdef JS_THREADSAFE
    JSBool gcLocked;
    uintN localMallocBytes;
    JSGCThing **flbase, **lastptr;
    JSGCThing *tmpthing;
    uint8 *tmpflagp;
    uintN maxFreeThings;         
#endif

    rt = cx->runtime;
    METER(rt->gcStats.alloc++);        
    nbytes = JS_ROUNDUP(nbytes, sizeof(JSGCThing));
    flindex = GC_FREELIST_INDEX(nbytes);
    METER(listStats = &rt->gcStats.arenas[flindex]);

#ifdef JS_THREADSAFE
    gcLocked = JS_FALSE;
    JS_ASSERT(cx->thread);
    flbase = cx->thread->gcFreeLists;
    JS_ASSERT(flbase);
    thing = flbase[flindex];
    localMallocBytes = cx->thread->gcMallocBytes;
    if (thing && rt->gcMaxMallocBytes - rt->gcMallocBytes > localMallocBytes) {
        flagp = thing->flagp;
        flbase[flindex] = thing->next;
        METER(rt->gcStats.localalloc++);  
        goto success;
    }

    JS_LOCK_GC(rt);
    gcLocked = JS_TRUE;

    
    if (localMallocBytes != 0) {
        cx->thread->gcMallocBytes = 0;
        if (rt->gcMaxMallocBytes - rt->gcMallocBytes < localMallocBytes)
            rt->gcMallocBytes = rt->gcMaxMallocBytes;
        else
            rt->gcMallocBytes += localMallocBytes;
    }
#endif
    JS_ASSERT(!rt->gcRunning);
    if (rt->gcRunning) {
        METER(rt->gcStats.finalfail++);
        JS_UNLOCK_GC(rt);
        return NULL;
    }

    doGC = (rt->gcMallocBytes >= rt->gcMaxMallocBytes && rt->gcPoke);
#ifdef JS_GC_ZEAL
    doGC = doGC || rt->gcZeal >= 2 || (rt->gcZeal >= 1 && rt->gcPoke);
#endif

    arenaList = &rt->gcArenaList[flindex];
    for (;;) {
        if (doGC) {
            







            js_GC(cx, GC_LAST_DITCH);
            METER(rt->gcStats.retry++);
        }

        
        thing = arenaList->freeList;
        if (thing) {
            arenaList->freeList = thing->next;
            flagp = thing->flagp;
            JS_ASSERT(*flagp & GCF_FINAL);
            METER(listStats->freelen--);
            METER(listStats->recycle++);

#ifdef JS_THREADSAFE
            







            if (rt->gcMallocBytes >= rt->gcMaxMallocBytes || flbase[flindex])
                break;
            tmpthing = arenaList->freeList;
            if (tmpthing) {
                maxFreeThings = MAX_THREAD_LOCAL_THINGS;
                do {
                    if (!tmpthing->next)
                        break;
                    tmpthing = tmpthing->next;
                } while (--maxFreeThings != 0);

                flbase[flindex] = arenaList->freeList;
                arenaList->freeList = tmpthing->next;
                tmpthing->next = NULL;
            }
#endif
            break;
        }

        




        thingsLimit = THINGS_PER_ARENA(nbytes);
        if (arenaList->lastCount != thingsLimit) {
            JS_ASSERT(arenaList->lastCount < thingsLimit);
            a = arenaList->last;
        } else {
            if (rt->gcBytes >= rt->gcMaxBytes || !(a = NewGCArena(rt))) {
                if (doGC)
                    goto fail;
                doGC = JS_TRUE;
                continue;
            }

            rt->gcBytes += GC_ARENA_SIZE;
            METER(listStats->narenas++);
            METER_UPDATE_MAX(listStats->maxarenas, listStats->narenas);

            a->list = arenaList;
            a->prev = arenaList->last;
            a->prevUntracedPage = 0;
            a->untracedThings = 0;
            arenaList->last = a;
            arenaList->lastCount = 0;
        }

        flagp = THING_FLAGP(a, arenaList->lastCount);
        thing = FLAGP_TO_THING(flagp, nbytes);
        arenaList->lastCount++;

#ifdef JS_THREADSAFE
        




        if (rt->gcMallocBytes >= rt->gcMaxMallocBytes || flbase[flindex])
            break;
        lastptr = &flbase[flindex];
        maxFreeThings = thingsLimit - arenaList->lastCount;
        if (maxFreeThings > MAX_THREAD_LOCAL_THINGS)
            maxFreeThings = MAX_THREAD_LOCAL_THINGS;
        METER(listStats->freelen += maxFreeThings);
        while (maxFreeThings != 0) {
            --maxFreeThings;

            tmpflagp = THING_FLAGP(a, arenaList->lastCount);
            tmpthing = FLAGP_TO_THING(tmpflagp, nbytes);
            arenaList->lastCount++;
            tmpthing->flagp = tmpflagp;
            *tmpflagp = GCF_FINAL;    

            *lastptr = tmpthing;
            lastptr = &tmpthing->next;
        }
        *lastptr = NULL;
#endif
        break;
    }

    
#ifdef JS_THREADSAFE
  success:
#endif
    lrs = cx->localRootStack;
    if (lrs) {
        






        if (js_PushLocalRoot(cx, lrs, (jsval) thing) < 0) {
            





            *flagp = GCF_FINAL;
            goto fail;
        }
    } else {
        



        cx->weakRoots.newborn[flags & GCF_TYPEMASK] = thing;
    }

    
    *flagp = (uint8)flags;

#ifdef DEBUG_gchist
    gchist[gchpos].lastDitch = doGC;
    gchist[gchpos].freeList = rt->gcArenaList[flindex].freeList;
    if (++gchpos == NGCHIST)
        gchpos = 0;
#endif

    
    METER_IF(flags & GCF_LOCK, rt->gcStats.lockborn++);
    METER(listStats->totalnew++);
    METER(listStats->nthings++);
    METER_UPDATE_MAX(listStats->maxthings, listStats->nthings);

#ifdef JS_THREADSAFE
    if (gcLocked)
        JS_UNLOCK_GC(rt);
#endif
    JS_COUNT_OPERATION(cx, JSOW_ALLOCATION);
    return thing;

fail:
#ifdef JS_THREADSAFE
    if (gcLocked)
        JS_UNLOCK_GC(rt);
#endif
    METER(rt->gcStats.fail++);
    JS_ReportOutOfMemory(cx);
    return NULL;
}

JSBool
js_LockGCThing(JSContext *cx, void *thing)
{
    JSBool ok = js_LockGCThingRT(cx->runtime, thing);
    if (!ok)
        JS_ReportOutOfMemory(cx);
    return ok;
}








#define GC_TYPE_IS_STRING(t)    ((t) == GCX_STRING ||                         \
                                 (t) >= GCX_EXTERNAL_STRING)
#define GC_TYPE_IS_XML(t)       ((unsigned)((t) - GCX_NAMESPACE) <=           \
                                 (unsigned)(GCX_XML - GCX_NAMESPACE))
#define GC_TYPE_IS_DEEP(t)      ((t) == GCX_OBJECT || GC_TYPE_IS_XML(t))

#define IS_DEEP_STRING(t,o)     (GC_TYPE_IS_STRING(t) &&                      \
                                 JSSTRING_IS_DEPENDENT((JSString *)(o)))

#define GC_THING_IS_DEEP(t,o)   (GC_TYPE_IS_DEEP(t) || IS_DEEP_STRING(t, o))


typedef struct JSGCLockHashEntry {
    JSDHashEntryHdr hdr;
    const JSGCThing *thing;
    uint32          count;
} JSGCLockHashEntry;

JSBool
js_LockGCThingRT(JSRuntime *rt, void *thing)
{
    JSBool ok, deep;
    uint8 *flagp;
    uintN flags, lock, type;
    JSGCLockHashEntry *lhe;

    ok = JS_TRUE;
    if (!thing)
        return ok;

    flagp = GetGCThingFlags(thing);

    JS_LOCK_GC(rt);
    flags = *flagp;
    lock = (flags & GCF_LOCK);
    type = (flags & GCF_TYPEMASK);
    deep = GC_THING_IS_DEEP(type, thing);

    



    if (lock || deep) {
        if (!rt->gcLocksHash) {
            rt->gcLocksHash =
                JS_NewDHashTable(JS_DHashGetStubOps(), NULL,
                                 sizeof(JSGCLockHashEntry),
                                 GC_ROOTS_SIZE);
            if (!rt->gcLocksHash) {
                ok = JS_FALSE;
                goto done;
            }
        } else if (lock == 0) {
#ifdef DEBUG
            JSDHashEntryHdr *hdr =
                JS_DHashTableOperate(rt->gcLocksHash, thing,
                                     JS_DHASH_LOOKUP);
            JS_ASSERT(JS_DHASH_ENTRY_IS_FREE(hdr));
#endif
        }

        lhe = (JSGCLockHashEntry *)
              JS_DHashTableOperate(rt->gcLocksHash, thing, JS_DHASH_ADD);
        if (!lhe) {
            ok = JS_FALSE;
            goto done;
        }
        if (!lhe->thing) {
            lhe->thing = (JSGCThing *) thing;
            lhe->count = deep ? 1 : 2;
        } else {
            JS_ASSERT(lhe->count >= 1);
            lhe->count++;
        }
    }

    *flagp = (uint8)(flags | GCF_LOCK);
    METER(rt->gcStats.lock++);
    ok = JS_TRUE;
done:
    JS_UNLOCK_GC(rt);
    return ok;
}

JSBool
js_UnlockGCThingRT(JSRuntime *rt, void *thing)
{
    uint8 *flagp, flags;
    JSGCLockHashEntry *lhe;

    if (!thing)
        return JS_TRUE;

    flagp = GetGCThingFlags(thing);
    JS_LOCK_GC(rt);
    flags = *flagp;

    if (flags & GCF_LOCK) {
        if (!rt->gcLocksHash ||
            (lhe = (JSGCLockHashEntry *)
                   JS_DHashTableOperate(rt->gcLocksHash, thing,
                                        JS_DHASH_LOOKUP),
             JS_DHASH_ENTRY_IS_FREE(&lhe->hdr))) {
            
            JS_ASSERT(!GC_THING_IS_DEEP(flags & GCF_TYPEMASK, thing));
        } else {
            
            if (--lhe->count != 0)
                goto out;
            JS_DHashTableOperate(rt->gcLocksHash, thing, JS_DHASH_REMOVE);
        }
        *flagp = (uint8)(flags & ~GCF_LOCK);
    }

    rt->gcPoke = JS_TRUE;
out:
    METER(rt->gcStats.unlock++);
    JS_UNLOCK_GC(rt);
    return JS_TRUE;
}

JS_PUBLIC_API(void)
JS_TraceChildren(JSTracer *trc, void *thing, uint32 kind)
{
    JSObject *obj;
    size_t nslots, i;
    jsval v;
    JSString *str;

    switch (kind) {
      case JSTRACE_OBJECT:
        
        obj = (JSObject *) thing;
        if (!obj->map)
            break;
        if (obj->map->ops->trace) {
            obj->map->ops->trace(trc, obj);
        } else {
            nslots = STOBJ_NSLOTS(obj);
            for (i = 0; i != nslots; ++i) {
                v = STOBJ_GET_SLOT(obj, i);
                if (JSVAL_IS_TRACEABLE(v)) {
                    JS_SET_TRACING_INDEX(trc, "slot", i);
                    JS_CallTracer(trc, JSVAL_TO_TRACEABLE(v),
                                  JSVAL_TRACE_KIND(v));
                }
            }
        }
        break;

      case JSTRACE_STRING:
        str = (JSString *)thing;
        if (JSSTRING_IS_DEPENDENT(str))
            JS_CALL_STRING_TRACER(trc, JSSTRDEP_BASE(str), "base");
        break;

      case JSTRACE_FUNCTION:
        js_TraceFunction(trc, (JSFunction *)thing);
        break;

#if JS_HAS_XML_SUPPORT
      case JSTRACE_NAMESPACE:
        js_TraceXMLNamespace(trc, (JSXMLNamespace *)thing);
        break;

      case JSTRACE_QNAME:
        js_TraceXMLQName(trc, (JSXMLQName *)thing);
        break;

      case JSTRACE_XML:
        js_TraceXML(trc, (JSXML *)thing);
        break;
#endif
    }
}




#define THINGS_PER_UNTRACED_BIT(thingSize)                                    \
    JS_HOWMANY(THINGS_PER_ARENA(thingSize), JS_BITS_PER_WORD)

static void
DelayTracingChildren(JSRuntime *rt, uint8 *flagp)
{
    JSGCArenaInfo *a;
    uint32 untracedBitIndex;
    jsuword bit;

    



    JS_ASSERT((*flagp & (GCF_MARK | GCF_FINAL)) == GCF_MARK);
    *flagp |= GCF_FINAL;

    METER(rt->gcStats.untraced++);
#ifdef DEBUG
    ++rt->gcTraceLaterCount;
    METER_UPDATE_MAX(rt->gcStats.maxuntraced, rt->gcTraceLaterCount);
#endif

    a = FLAGP_TO_ARENA(flagp);
    untracedBitIndex = FLAGP_TO_INDEX(flagp) /
                       THINGS_PER_UNTRACED_BIT(a->list->thingSize);
    JS_ASSERT(untracedBitIndex < JS_BITS_PER_WORD);
    bit = (jsuword)1 << untracedBitIndex;
    if (a->untracedThings != 0) {
        JS_ASSERT(rt->gcUntracedArenaStackTop);
        if (a->untracedThings & bit) {
            
            return;
        }
        a->untracedThings |= bit;
    } else {
        











        a->untracedThings = bit;
        if (a->prevUntracedPage == 0) {
            if (!rt->gcUntracedArenaStackTop) {
                
                a->prevUntracedPage = ARENA_INFO_TO_PAGE(a);
            } else {
                JS_ASSERT(rt->gcUntracedArenaStackTop->prevUntracedPage != 0);
                a->prevUntracedPage =
                    ARENA_INFO_TO_PAGE(rt->gcUntracedArenaStackTop);
            }
            rt->gcUntracedArenaStackTop = a;
        }
    }
    JS_ASSERT(rt->gcUntracedArenaStackTop);
}

static void
TraceDelayedChildren(JSTracer *trc)
{
    JSRuntime *rt;
    JSGCArenaInfo *a, *aprev;
    uint32 thingSize;
    uint32 thingsPerUntracedBit;
    uint32 untracedBitIndex, thingIndex, indexLimit, endIndex;
    JSGCThing *thing;
    uint8 *flagp;

    rt = trc->context->runtime;
    a = rt->gcUntracedArenaStackTop;
    if (!a) {
        JS_ASSERT(rt->gcTraceLaterCount == 0);
        return;
    }

    for (;;) {
        




        JS_ASSERT(a->prevUntracedPage != 0);
        JS_ASSERT(rt->gcUntracedArenaStackTop->prevUntracedPage != 0);
        thingSize = a->list->thingSize;
        indexLimit = (a == a->list->last)
                     ? a->list->lastCount
                     : THINGS_PER_ARENA(thingSize);
        thingsPerUntracedBit = THINGS_PER_UNTRACED_BIT(thingSize);

        




        while (a->untracedThings != 0) {
            untracedBitIndex = JS_FLOOR_LOG2W(a->untracedThings);
            a->untracedThings &= ~((jsuword)1 << untracedBitIndex);
            thingIndex = untracedBitIndex * thingsPerUntracedBit;
            endIndex = thingIndex + thingsPerUntracedBit;

            



            if (endIndex > indexLimit)
                endIndex = indexLimit;
            JS_ASSERT(thingIndex < indexLimit);

            do {
                



                flagp = THING_FLAGP(a, thingIndex);
                if ((*flagp & (GCF_MARK|GCF_FINAL)) != (GCF_MARK|GCF_FINAL))
                    continue;
                *flagp &= ~GCF_FINAL;
#ifdef DEBUG
                JS_ASSERT(rt->gcTraceLaterCount != 0);
                --rt->gcTraceLaterCount;
#endif
                thing = FLAGP_TO_THING(flagp, thingSize);
                JS_TraceChildren(trc, thing, MapGCFlagsToTraceKind(*flagp));
            } while (++thingIndex != endIndex);
        }

        








        if (a == rt->gcUntracedArenaStackTop) {
            aprev = ARENA_PAGE_TO_INFO(a->prevUntracedPage);
            a->prevUntracedPage = 0;
            if (a == aprev) {
                



                break;
            }
            rt->gcUntracedArenaStackTop = a = aprev;
        } else {
            a = rt->gcUntracedArenaStackTop;
        }
    }
    JS_ASSERT(rt->gcUntracedArenaStackTop);
    JS_ASSERT(rt->gcUntracedArenaStackTop->prevUntracedPage == 0);
    rt->gcUntracedArenaStackTop = NULL;
    JS_ASSERT(rt->gcTraceLaterCount == 0);
}

JS_PUBLIC_API(void)
JS_CallTracer(JSTracer *trc, void *thing, uint32 kind)
{
    JSContext *cx;
    JSRuntime *rt;
    uint8 *flagp;

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
      case JSTRACE_DOUBLE:
        flagp = THING_TO_FLAGP(thing, sizeof(JSGCThing));
        JS_ASSERT((*flagp & GCF_FINAL) == 0);
        JS_ASSERT(kind == MapGCFlagsToTraceKind(*flagp));
        *flagp |= GCF_MARK;
        goto out;

      case JSTRACE_STRING:
        for (;;) {
            flagp = THING_TO_FLAGP(thing, sizeof(JSGCThing));
            JS_ASSERT((*flagp & GCF_FINAL) == 0);
            JS_ASSERT(kind == MapGCFlagsToTraceKind(*flagp));
            if (!JSSTRING_IS_DEPENDENT((JSString *) thing)) {
                *flagp |= GCF_MARK;
                goto out;
            }
            if (*flagp & GCF_MARK)
                goto out;
            *flagp |= GCF_MARK;
            thing = JSSTRDEP_BASE((JSString *) thing);
        }
        
    }

    flagp = GetGCThingFlags(thing);
    JS_ASSERT(kind == MapGCFlagsToTraceKind(*flagp));
    if (*flagp & GCF_MARK)
        goto out;

    



    JS_ASSERT(*flagp != GCF_FINAL);
    *flagp |= GCF_MARK;
    if (!cx->insideGCMarkCallback) {
        




#ifdef JS_GC_ASSUME_LOW_C_STACK
# define RECURSION_TOO_DEEP() JS_TRUE
#else
        int stackDummy;
# define RECURSION_TOO_DEEP() (!JS_CHECK_STACK_SIZE(cx, stackDummy))
#endif
        if (RECURSION_TOO_DEEP())
            DelayTracingChildren(rt, flagp);
        else
            JS_TraceChildren(trc, thing, kind);
    } else {
        














        cx->insideGCMarkCallback = JS_FALSE;
        JS_TraceChildren(trc, thing, kind);
        TraceDelayedChildren(trc);
        cx->insideGCMarkCallback = JS_TRUE;
    }

  out:
#ifdef DEBUG
    trc->debugPrinter = NULL;
    trc->debugPrintArg = NULL;
#endif
    return;     
}

void
js_CallValueTracerIfGCThing(JSTracer *trc, jsval v)
{
    void *thing;
    uint32 kind;

    if (JSVAL_IS_DOUBLE(v) || JSVAL_IS_STRING(v)) {
        thing = JSVAL_TO_TRACEABLE(v);
        kind = JSVAL_TRACE_KIND(v);
        JS_ASSERT(kind == js_GetGCThingTraceKind(JSVAL_TO_GCTHING(v)));
    } else if (JSVAL_IS_OBJECT(v) && v != JSVAL_NULL) {
        
        thing = JSVAL_TO_OBJECT(v);
        kind = js_GetGCThingTraceKind(thing);
    } else {
        return;
    }
    JS_CallTracer(trc, thing, kind);
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
gc_root_traversal(JSDHashTable *table, JSDHashEntryHdr *hdr, uint32 num,
                  void *arg)
{
    JSGCRootHashEntry *rhe = (JSGCRootHashEntry *)hdr;
    JSTracer *trc = (JSTracer *)arg;
    jsval *rp = (jsval *)rhe->root;
    jsval v = *rp;

    
    if (!JSVAL_IS_NULL(v) && JSVAL_IS_GCTHING(v)) {
#ifdef DEBUG
        JSBool root_points_to_gcArenaList = JS_FALSE;
        jsuword thing = (jsuword) JSVAL_TO_GCTHING(v);
        uintN i;
        JSGCArenaList *arenaList;
        uint32 thingSize;
        JSGCArenaInfo *a;
        size_t limit;

        for (i = 0; i < GC_NUM_FREELISTS; i++) {
            arenaList = &trc->context->runtime->gcArenaList[i];
            thingSize = arenaList->thingSize;
            limit = (size_t) arenaList->lastCount * thingSize;
            for (a = arenaList->last; a; a = a->prev) {
                if (thing - ARENA_INFO_TO_START(a) < limit) {
                    root_points_to_gcArenaList = JS_TRUE;
                    break;
                }
                limit = (size_t) THINGS_PER_ARENA(thingSize) * thingSize;
            }
        }
        if (!root_points_to_gcArenaList && rhe->name) {
            fprintf(stderr,
"JS API usage error: the address passed to JS_AddNamedRoot currently holds an\n"
"invalid jsval.  This is usually caused by a missing call to JS_RemoveRoot.\n"
"The root's name is \"%s\".\n",
                    rhe->name);
        }
        JS_ASSERT(root_points_to_gcArenaList);
#endif
        JS_SET_TRACING_NAME(trc, rhe->name ? rhe->name : "root");
        js_CallValueTracerIfGCThing(trc, v);
    }

    return JS_DHASH_NEXT;
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
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

#define TRACE_JSVALS(trc, len, vec, name)                                     \
    JS_BEGIN_MACRO                                                            \
    jsval _v, *_vp, *_end;                                                    \
                                                                              \
        for (_vp = vec, _end = _vp + len; _vp < _end; _vp++) {                \
            _v = *_vp;                                                        \
            if (JSVAL_IS_TRACEABLE(_v)) {                                     \
                JS_SET_TRACING_INDEX(trc, name, _vp - (vec));                 \
                JS_CallTracer(trc, JSVAL_TO_TRACEABLE(_v),                    \
                              JSVAL_TRACE_KIND(_v));                          \
            }                                                                 \
        }                                                                     \
    JS_END_MACRO

void
js_TraceStackFrame(JSTracer *trc, JSStackFrame *fp)
{
    uintN nslots, minargs, skip;

    if (fp->callobj)
        JS_CALL_OBJECT_TRACER(trc, fp->callobj, "call");
    if (fp->argsobj)
        JS_CALL_OBJECT_TRACER(trc, fp->argsobj, "arguments");
    if (fp->varobj)
        JS_CALL_OBJECT_TRACER(trc, fp->varobj, "variables");
    if (fp->script) {
        js_TraceScript(trc, fp->script);
        if (fp->spbase) {
            



            JS_ASSERT(JS_UPTRDIFF(fp->sp, fp->spbase) <=
                      fp->script->depth * sizeof(jsval));
            nslots = (uintN) (fp->sp - fp->spbase);
            TRACE_JSVALS(trc, nslots, fp->spbase, "operand");
        }
    }

    
    JS_ASSERT(JSVAL_IS_OBJECT((jsval)fp->thisp) ||
              (fp->fun && JSFUN_THISP_FLAGS(fp->fun->flags)));
    JS_CALL_VALUE_TRACER(trc, (jsval)fp->thisp, "this");

    if (fp->callee)
        JS_CALL_OBJECT_TRACER(trc, fp->callee, "callee");

    if (fp->argv) {
        nslots = fp->argc;
        skip = 0;
        if (fp->fun) {
            minargs = FUN_MINARGS(fp->fun);
            if (minargs > nslots)
                nslots = minargs;
            if (!FUN_INTERPRETED(fp->fun)) {
                JS_ASSERT(!(fp->fun->flags & JSFUN_FAST_NATIVE));
                nslots += fp->fun->u.n.extra;
            }
            if (fp->fun->flags & JSFRAME_ROOTED_ARGV)
                skip = 2 + fp->argc;
        }
        TRACE_JSVALS(trc, 2 + nslots - skip, fp->argv - 2 + skip, "operand");
    }
    JS_CALL_VALUE_TRACER(trc, fp->rval, "rval");
    if (fp->vars)
        TRACE_JSVALS(trc, fp->nvars, fp->vars, "var");
    if (fp->scopeChain)
        JS_CALL_OBJECT_TRACER(trc, fp->scopeChain, "scope chain");
    if (fp->sharpArray)
        JS_CALL_OBJECT_TRACER(trc, fp->sharpArray, "sharp array");

    if (fp->xmlNamespace)
        JS_CALL_OBJECT_TRACER(trc, fp->xmlNamespace, "xmlNamespace");
}

static void
TraceWeakRoots(JSTracer *trc, JSWeakRoots *wr)
{
    uint32 i;
    void *thing;

#ifdef DEBUG
    static const char *weakRootNames[JSTRACE_LIMIT] = {
        "newborn object",
        "newborn double",
        "newborn string",
        "newborn function",
        "newborn namespace",
        "newborn qname",
        "newborn xml"
    };
#endif

    for (i = 0; i != JSTRACE_LIMIT; i++) {
        thing = wr->newborn[i];
        if (thing)
            JS_CALL_TRACER(trc, thing, i, weakRootNames[i]);
    }
    JS_ASSERT(i == GCX_EXTERNAL_STRING);
    for (; i != GCX_NTYPES; ++i) {
        thing = wr->newborn[i];
        if (thing) {
            JS_SET_TRACING_INDEX(trc, "newborn external string",
                                 i - GCX_EXTERNAL_STRING);
            JS_CallTracer(trc, thing, JSTRACE_STRING);
        }
    }

    JS_CALL_VALUE_TRACER(trc, wr->lastAtom, "lastAtom");
    JS_SET_TRACING_NAME(trc, "lastInternalResult");
    js_CallValueTracerIfGCThing(trc, wr->lastInternalResult);
}

JS_FRIEND_API(void)
js_TraceContext(JSTracer *trc, JSContext *acx)
{
    JSArena *a;
    int64 *timestamp;
    JSStackFrame *fp, *nextChain;
    JSStackHeader *sh;
    JSTempValueRooter *tvr;

    



    a = acx->stackPool.current;
    if (a == acx->stackPool.first.next &&
        a->avail == a->base + sizeof(int64)) {
        timestamp = (int64 *) a->base;
        if (JS_Now() - *timestamp > acx->runtime->gcStackPoolLifespan * 1000)
            JS_FinishArenaPool(&acx->stackPool);
    }

    




    fp = acx->fp;
    nextChain = acx->dormantFrameChain;
    if (!fp)
        goto next_chain;

    
    JS_ASSERT(!fp->dormantNext);
    for (;;) {
        do {
            js_TraceStackFrame(trc, fp);
        } while ((fp = fp->down) != NULL);

      next_chain:
        if (!nextChain)
            break;
        fp = nextChain;
        nextChain = nextChain->dormantNext;
    }

    
    if (acx->globalObject)
        JS_CALL_OBJECT_TRACER(trc, acx->globalObject, "global object");
    TraceWeakRoots(trc, &acx->weakRoots);
    if (acx->throwing) {
        JS_CALL_VALUE_TRACER(trc, acx->exception, "exception");
    } else {
        
        acx->exception = JSVAL_NULL;
    }
#if JS_HAS_LVALUE_RETURN
    if (acx->rval2set)
        JS_CALL_VALUE_TRACER(trc, acx->rval2, "rval2");
#endif

    for (sh = acx->stackHeaders; sh; sh = sh->down) {
        METER(trc->context->runtime->gcStats.stackseg++);
        METER(trc->context->runtime->gcStats.segslots += sh->nslots);
        TRACE_JSVALS(trc, sh->nslots, JS_STACK_SEGMENT(sh), "stack");
    }

    if (acx->localRootStack)
        js_TraceLocalRoots(trc, acx->localRootStack);

    for (tvr = acx->tempValueRooters; tvr; tvr = tvr->down) {
        switch (tvr->count) {
          case JSTVU_SINGLE:
            JS_SET_TRACING_NAME(trc, "tvr->u.value");
            js_CallValueTracerIfGCThing(trc, tvr->u.value);
            break;
          case JSTVU_TRACE:
            tvr->u.trace(trc, tvr);
            break;
          case JSTVU_SPROP:
            TRACE_SCOPE_PROPERTY(trc, tvr->u.sprop);
            break;
          case JSTVU_WEAK_ROOTS:
            TraceWeakRoots(trc, tvr->u.weakRoots);
            break;
          case JSTVU_PARSE_CONTEXT:
            js_TraceParseContext(trc, tvr->u.parseContext);
            break;
          case JSTVU_SCRIPT:
            js_TraceScript(trc, tvr->u.script);
            break;
          default:
            JS_ASSERT(tvr->count >= 0);
            TRACE_JSVALS(trc, tvr->count, tvr->u.array, "tvr->u.array");
        }
    }

    if (acx->sharpObjectMap.depth > 0)
        js_TraceSharpMap(trc, &acx->sharpObjectMap);
}

void
js_TraceRuntime(JSTracer *trc, JSBool allAtoms)
{
    JSRuntime *rt = trc->context->runtime;
    JSContext *iter, *acx;

    JS_DHashTableEnumerate(&rt->gcRootsHash, gc_root_traversal, trc);
    if (rt->gcLocksHash)
        JS_DHashTableEnumerate(rt->gcLocksHash, gc_lock_traversal, trc);
    js_TraceAtomState(trc, allAtoms);
    js_TraceNativeIteratorStates(trc);

    iter = NULL;
    while ((acx = js_ContextIterator(rt, JS_TRUE, &iter)) != NULL)
        js_TraceContext(trc, acx);

    if (rt->gcExtraRootsTraceOp)
        rt->gcExtraRootsTraceOp(trc, rt->gcExtraRootsData);
}





void
js_GC(JSContext *cx, JSGCInvocationKind gckind)
{
    JSRuntime *rt;
    JSBool keepAtoms;
    JSGCCallback callback;
    uintN i, type;
    JSTracer trc;
    uint32 thingSize, indexLimit;
    JSGCArenaInfo *a, **ap, *emptyArenas;
    uint8 flags, *flagp;
    JSGCThing *thing, *freeList;
    JSGCArenaList *arenaList;
    JSBool allClear;
#ifdef JS_THREADSAFE
    uint32 requestDebit;
    JSContext *acx, *iter;
#endif
#ifdef JS_GCMETER
    JSGCArenaStats *listStats;
    size_t nfree;
#endif

    rt = cx->runtime;
#ifdef JS_THREADSAFE
    
    JS_ASSERT(!JS_IS_RUNTIME_LOCKED(rt));
#endif

    if (gckind == GC_LAST_DITCH) {
        
        keepAtoms = JS_TRUE;
    } else {
        
        keepAtoms = (rt->gcKeepAtoms != 0);
        JS_CLEAR_WEAK_ROOTS(&cx->weakRoots);
    }

    





    if (rt->state != JSRTS_UP && gckind != GC_LAST_CONTEXT)
        return;

  restart_after_callback:
    





    callback = rt->gcCallback;
    if (callback) {
        JSBool ok;

        if (gckind != GC_LAST_DITCH)
            JS_UNLOCK_GC(rt);
        ok = callback(cx, JSGC_BEGIN);
        if (gckind != GC_LAST_DITCH)
            JS_LOCK_GC(rt);
        if (!ok && gckind != GC_LAST_CONTEXT)
            return;
    }

    
    if (gckind != GC_LAST_DITCH)
        JS_LOCK_GC(rt);

    METER(rt->gcStats.poke++);
    rt->gcPoke = JS_FALSE;

#ifdef JS_THREADSAFE
    JS_ASSERT(cx->thread->id == js_CurrentThreadId());

    
    if (rt->gcThread == cx->thread) {
        JS_ASSERT(rt->gcLevel > 0);
        rt->gcLevel++;
        METER_UPDATE_MAX(rt->gcStats.maxlevel, rt->gcLevel);
        if (gckind != GC_LAST_DITCH)
            JS_UNLOCK_GC(rt);
        return;
    }

    





    requestDebit = 0;
    if (cx->thread) {
        JSCList *head, *link;

        



        head = &cx->thread->contextList;
        for (link = head->next; link != head; link = link->next) {
            acx = CX_FROM_THREAD_LINKS(link);
            JS_ASSERT(acx->thread == cx->thread);
            if (acx->requestDepth)
                requestDebit++;
        }
    } else {
        






        JS_ASSERT(cx->requestDepth == 0);
        if (cx->requestDepth)
            requestDebit = 1;
    }
    if (requestDebit) {
        JS_ASSERT(requestDebit <= rt->requestCount);
        rt->requestCount -= requestDebit;
        if (rt->requestCount == 0)
            JS_NOTIFY_REQUEST_DONE(rt);
    }

    
    if (rt->gcLevel > 0) {
        
        rt->gcLevel++;
        METER_UPDATE_MAX(rt->gcStats.maxlevel, rt->gcLevel);

        
        while (rt->gcLevel > 0)
            JS_AWAIT_GC_DONE(rt);
        if (requestDebit)
            rt->requestCount += requestDebit;
        if (gckind != GC_LAST_DITCH)
            JS_UNLOCK_GC(rt);
        return;
    }

    
    rt->gcLevel = 1;
    rt->gcThread = cx->thread;

    
    while (rt->requestCount > 0)
        JS_AWAIT_REQUEST_DONE(rt);

#else  

    
    rt->gcLevel++;
    METER_UPDATE_MAX(rt->gcStats.maxlevel, rt->gcLevel);
    if (rt->gcLevel > 1)
        return;

#endif 

    







    rt->gcRunning = JS_TRUE;
    JS_UNLOCK_GC(rt);

    
    rt->gcMallocBytes = 0;

#ifdef JS_DUMP_SCOPE_METERS
  { extern void js_DumpScopeMeters(JSRuntime *rt);
    js_DumpScopeMeters(rt);
  }
#endif

#ifdef JS_THREADSAFE
    










    memset(cx->thread->gcFreeLists, 0, sizeof cx->thread->gcFreeLists);
    iter = NULL;
    while ((acx = js_ContextIterator(rt, JS_FALSE, &iter)) != NULL) {
        if (!acx->thread || acx->thread == cx->thread)
            continue;
        memset(acx->thread->gcFreeLists, 0, sizeof acx->thread->gcFreeLists);
        GSN_CACHE_CLEAR(&acx->thread->gsnCache);
    }
#else
    
    GSN_CACHE_CLEAR(&rt->gsnCache);
#endif

restart:
    rt->gcNumber++;
    JS_ASSERT(!rt->gcUntracedArenaStackTop);
    JS_ASSERT(rt->gcTraceLaterCount == 0);

    


    JS_TRACER_INIT(&trc, cx, NULL);
    rt->gcMarkingTracer = &trc;
    JS_ASSERT(IS_GC_MARKING_TRACER(&trc));
    js_TraceRuntime(&trc, keepAtoms);
    js_MarkScriptFilenames(rt, keepAtoms);

    



    TraceDelayedChildren(&trc);

    JS_ASSERT(!cx->insideGCMarkCallback);
    if (rt->gcCallback) {
        cx->insideGCMarkCallback = JS_TRUE;
        (void) rt->gcCallback(cx, JSGC_MARK_END);
        JS_ASSERT(cx->insideGCMarkCallback);
        cx->insideGCMarkCallback = JS_FALSE;
    }
    JS_ASSERT(rt->gcTraceLaterCount == 0);

    rt->gcMarkingTracer = NULL;

    













    js_SweepAtomState(cx);

    
    CloseNativeIterators(cx);

    
    js_SweepWatchPoints(cx);

#ifdef DEBUG
    
    rt->liveScopePropsPreSweep = rt->liveScopeProps;
#endif

    







    emptyArenas = NULL;
    for (i = 0; i < GC_NUM_FREELISTS; i++) {
        arenaList = &rt->gcArenaList[i == 0
                                     ? GC_FREELIST_INDEX(sizeof(JSObject))
                                     : i == GC_FREELIST_INDEX(sizeof(JSObject))
                                     ? 0
                                     : i];
        ap = &arenaList->last;
        if (!(a = *ap))
            continue;

        JS_ASSERT(arenaList->lastCount > 0);
        arenaList->freeList = NULL;
        freeList = NULL;

        
        METER(listStats = &rt->gcStats.arenas[arenaList - &rt->gcArenaList[0]]);
        METER(listStats->nthings = 0);
        METER(listStats->freelen = 0);
        thingSize = arenaList->thingSize;
        indexLimit = THINGS_PER_ARENA(thingSize);
        flagp = THING_FLAGP(a, arenaList->lastCount - 1);
        for (;;) {
            JS_ASSERT(a->prevUntracedPage == 0);
            JS_ASSERT(a->untracedThings == 0);
            allClear = JS_TRUE;
            METER(nfree = 0);
            do {
                flags = *flagp;
                if (flags & (GCF_MARK | GCF_LOCK)) {
                    *flagp &= ~GCF_MARK;
                    allClear = JS_FALSE;
                    METER(listStats->nthings++);
                } else {
                    thing = FLAGP_TO_THING(flagp, thingSize);
                    if (!(flags & GCF_FINAL)) {
                        


                        *flagp = (uint8)(flags | GCF_FINAL);
                        type = flags & GCF_TYPEMASK;
                        switch (type) {
                          case GCX_OBJECT:
                            js_FinalizeObject(cx, (JSObject *) thing);
                            break;
                          case GCX_DOUBLE:
                            
                            break;
                          case GCX_FUNCTION:
                            js_FinalizeFunction(cx, (JSFunction *) thing);
                            break;
#if JS_HAS_XML_SUPPORT
                          case GCX_NAMESPACE:
                            js_FinalizeXMLNamespace(cx,
                                                    (JSXMLNamespace *) thing);
                            break;
                          case GCX_QNAME:
                            js_FinalizeXMLQName(cx, (JSXMLQName *) thing);
                            break;
                          case GCX_XML:
                            js_FinalizeXML(cx, (JSXML *) thing);
                            break;
#endif
                          default:
                            JS_ASSERT(type == GCX_STRING ||
                                      type - GCX_EXTERNAL_STRING <
                                      GCX_NTYPES - GCX_EXTERNAL_STRING);
                            js_FinalizeStringRT(rt, (JSString *) thing,
                                                (intN) (type -
                                                        GCX_EXTERNAL_STRING),
                                                cx);
                            break;
                        }
                    }
                    thing->flagp = flagp;
                    thing->next = freeList;
                    freeList = thing;
                    METER(++nfree);
                }
            } while (++flagp != THING_FLAGS_END(a));

            if (allClear) {
                



                freeList = arenaList->freeList;
                if (a == arenaList->last)
                    arenaList->lastCount = indexLimit;
                *ap = a->prev;
                JS_ASSERT(rt->gcBytes >= GC_ARENA_SIZE);
                rt->gcBytes -= GC_ARENA_SIZE;
                a->prev = emptyArenas;
                emptyArenas = a;
            } else {
                arenaList->freeList = freeList;
                ap = &a->prev;
                METER(listStats->freelen += nfree);
                METER(listStats->totalfreelen += nfree);
                METER(listStats->totalarenas++);
            }
            if (!(a = *ap))
                break;
            flagp = THING_FLAGP(a, indexLimit - 1);
        }
    }

    



    js_SweepScopeProperties(cx);

    





    js_SweepScriptFilenames(rt);

    



    while (emptyArenas) {
        a = emptyArenas;
        emptyArenas = emptyArenas->prev;
        DestroyGCArena(rt, a);
    }

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

    JS_LOCK_GC(rt);

    



    if (rt->gcLevel > 1 || rt->gcPoke) {
        rt->gcLevel = 1;
        rt->gcPoke = JS_FALSE;
        JS_UNLOCK_GC(rt);
        goto restart;
    }
    rt->gcLevel = 0;
    rt->gcLastBytes = rt->gcBytes;
    rt->gcRunning = JS_FALSE;

#ifdef JS_THREADSAFE
    
    if (requestDebit)
        rt->requestCount += requestDebit;
    rt->gcThread = NULL;
    JS_NOTIFY_GC_DONE(rt);

    


    if (gckind != GC_LAST_DITCH)
        JS_UNLOCK_GC(rt);
#endif

    




    callback = rt->gcCallback;
    if (callback) {
        JSWeakRoots savedWeakRoots;
        JSTempValueRooter tvr;

        if (gckind == GC_LAST_DITCH) {
            




            savedWeakRoots = cx->weakRoots;
            JS_PUSH_TEMP_ROOT_WEAK_COPY(cx, &savedWeakRoots, &tvr);
            JS_KEEP_ATOMS(rt);
            JS_UNLOCK_GC(rt);
        }

        (void) callback(cx, JSGC_END);

        if (gckind == GC_LAST_DITCH) {
            JS_LOCK_GC(rt);
            JS_UNKEEP_ATOMS(rt);
            JS_POP_TEMP_ROOT(cx, &tvr);
        } else if (gckind == GC_LAST_CONTEXT && rt->gcPoke) {
            



            goto restart_after_callback;
        }
    }
}

void
js_UpdateMallocCounter(JSContext *cx, size_t nbytes)
{
    uint32 *pbytes, bytes;

#ifdef JS_THREADSAFE
    pbytes = &cx->thread->gcMallocBytes;
#else
    pbytes = &cx->runtime->gcMallocBytes;
#endif
    bytes = *pbytes;
    *pbytes = ((uint32)-1 - bytes <= nbytes) ? (uint32)-1 : bytes + nbytes;
}
