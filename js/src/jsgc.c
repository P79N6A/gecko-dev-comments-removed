

















































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
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif










#if JS_BYTES_PER_WORD == 8
# define GC_THINGS_SHIFT 14     /* 16KB for things on Alpha, etc. */
#else
# define GC_THINGS_SHIFT 13     /* 8KB for things on most platforms */
#endif
#define GC_THINGS_SIZE  JS_BIT(GC_THINGS_SHIFT)
#define GC_FLAGS_SIZE   (GC_THINGS_SIZE / sizeof(JSGCThing))












































































#define GC_PAGE_SHIFT   10
#define GC_PAGE_MASK    ((jsuword) JS_BITMASK(GC_PAGE_SHIFT))
#define GC_PAGE_SIZE    JS_BIT(GC_PAGE_SHIFT)
#define GC_PAGE_COUNT   (1 << (GC_THINGS_SHIFT - GC_PAGE_SHIFT))

typedef struct JSGCPageInfo {
    jsuword     offsetInArena;          
    jsuword     unscannedBitmap;        

} JSGCPageInfo;

struct JSGCArena {
    JSGCArenaList       *list;          
    JSGCArena           *prev;          
    JSGCArena           *prevUnscanned; 


    jsuword             unscannedPages; 


    uint8               base[1];        
};

#define GC_ARENA_SIZE                                                         \
    (offsetof(JSGCArena, base) + GC_THINGS_SIZE + GC_FLAGS_SIZE)

#define FIRST_THING_PAGE(a)                                                   \
    (((jsuword)(a)->base + GC_FLAGS_SIZE - 1) & ~GC_PAGE_MASK)

#define PAGE_TO_ARENA(pi)                                                     \
    ((JSGCArena *)((jsuword)(pi) - (pi)->offsetInArena                        \
                   - offsetof(JSGCArena, base)))

#define PAGE_INDEX(pi)                                                        \
    ((size_t)((pi)->offsetInArena >> GC_PAGE_SHIFT))

#define THING_TO_PAGE(thing)                                                  \
    ((JSGCPageInfo *)((jsuword)(thing) & ~GC_PAGE_MASK))











#define PAGE_THING_GAP(n) (((n) & ((n) - 1)) ? (GC_PAGE_SIZE % (n)) : (n))

#ifdef JS_THREADSAFE







#define MAX_THREAD_LOCAL_THINGS 8

#endif

JS_STATIC_ASSERT(sizeof(JSGCThing) == sizeof(JSGCPageInfo));
JS_STATIC_ASSERT(GC_FLAGS_SIZE >= GC_PAGE_SIZE);
JS_STATIC_ASSERT(sizeof(JSStackHeader) >= 2 * sizeof(jsval));

JS_STATIC_ASSERT(sizeof(JSGCThing) >= sizeof(JSString));
JS_STATIC_ASSERT(sizeof(JSGCThing) >= sizeof(jsdouble));


JS_STATIC_ASSERT(sizeof(JSObject) % sizeof(JSGCThing) == 0);






JS_STATIC_ASSERT(GC_FREELIST_INDEX(sizeof(JSFunction)) !=
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
# define METER(x) x
#else
# define METER(x) ((void) 0)
#endif

static JSBool
NewGCArena(JSRuntime *rt, JSGCArenaList *arenaList)
{
    JSGCArena *a;
    jsuword offset;
    JSGCPageInfo *pi;

    
    if (rt->gcBytes >= rt->gcMaxBytes)
        return JS_FALSE;
    a = (JSGCArena *)malloc(GC_ARENA_SIZE);
    if (!a)
        return JS_FALSE;

    
    offset = (GC_PAGE_SIZE - ((jsuword)a->base & GC_PAGE_MASK)) & GC_PAGE_MASK;
    JS_ASSERT((jsuword)a->base + offset == FIRST_THING_PAGE(a));
    do {
        pi = (JSGCPageInfo *) (a->base + offset);
        pi->offsetInArena = offset;
        pi->unscannedBitmap = 0;
        offset += GC_PAGE_SIZE;
    } while (offset < GC_THINGS_SIZE);

    METER(++arenaList->stats.narenas);
    METER(arenaList->stats.maxarenas
          = JS_MAX(arenaList->stats.maxarenas, arenaList->stats.narenas));

    a->list = arenaList;
    a->prev = arenaList->last;
    a->prevUnscanned = NULL;
    a->unscannedPages = 0;
    arenaList->last = a;
    arenaList->lastLimit = 0;
    rt->gcBytes += GC_ARENA_SIZE;
    return JS_TRUE;
}

static void
DestroyGCArena(JSRuntime *rt, JSGCArenaList *arenaList, JSGCArena **ap)
{
    JSGCArena *a;

    a = *ap;
    JS_ASSERT(a);
    JS_ASSERT(rt->gcBytes >= GC_ARENA_SIZE);
    rt->gcBytes -= GC_ARENA_SIZE;
    METER(rt->gcStats.afree++);
    METER(--arenaList->stats.narenas);
    if (a == arenaList->last)
        arenaList->lastLimit = (uint16)(a->prev ? GC_THINGS_SIZE : 0);
    *ap = a->prev;

#ifdef DEBUG
    memset(a, JS_FREE_PATTERN, GC_ARENA_SIZE);
#endif
    free(a);
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
        arenaList->lastLimit = 0;
        arenaList->thingSize = (uint16)thingSize;
        arenaList->freeList = NULL;
        METER(memset(&arenaList->stats, 0, sizeof arenaList->stats));
    }
}

static void
FinishGCArenaLists(JSRuntime *rt)
{
    uintN i;
    JSGCArenaList *arenaList;

    for (i = 0; i < GC_NUM_FREELISTS; i++) {
        arenaList = &rt->gcArenaList[i];
        while (arenaList->last)
            DestroyGCArena(rt, arenaList, &arenaList->last);
        arenaList->freeList = NULL;
    }
}

uint8 *
js_GetGCThingFlags(void *thing)
{
    JSGCPageInfo *pi;
    jsuword offsetInArena, thingIndex;

    pi = THING_TO_PAGE(thing);
    offsetInArena = pi->offsetInArena;
    JS_ASSERT(offsetInArena < GC_THINGS_SIZE);
    thingIndex = ((offsetInArena & ~GC_PAGE_MASK) |
                  ((jsuword)thing & GC_PAGE_MASK)) / sizeof(JSGCThing);
    JS_ASSERT(thingIndex < GC_PAGE_SIZE);
    if (thingIndex >= (offsetInArena & GC_PAGE_MASK))
        thingIndex += GC_THINGS_SIZE;
    return (uint8 *)pi - offsetInArena + thingIndex;
}

JSRuntime*
js_GetGCStringRuntime(JSString *str)
{
    JSGCPageInfo *pi;
    JSGCArenaList *list;

    pi = THING_TO_PAGE(str);
    list = PAGE_TO_ARENA(pi)->list;

    JS_ASSERT(list->thingSize == sizeof(JSGCThing));
    JS_ASSERT(GC_FREELIST_INDEX(sizeof(JSGCThing)) == 0);

    return (JSRuntime *)((uint8 *)list - offsetof(JSRuntime, gcArenaList));
}

JSBool
js_IsAboutToBeFinalized(JSContext *cx, void *thing)
{
    uint8 flags = *js_GetGCThingFlags(thing);

    return !(flags & (GCF_MARK | GCF_LOCK | GCF_FINAL));
}

typedef void (*GCFinalizeOp)(JSContext *cx, JSGCThing *thing);

#ifndef DEBUG
# define js_FinalizeDouble       NULL
#endif

#if !JS_HAS_XML_SUPPORT
# define js_FinalizeXMLNamespace NULL
# define js_FinalizeXMLQName     NULL
# define js_FinalizeXML          NULL
#endif

static GCFinalizeOp gc_finalizers[GCX_NTYPES] = {
    (GCFinalizeOp) js_FinalizeObject,           
    (GCFinalizeOp) js_FinalizeString,           
    (GCFinalizeOp) js_FinalizeDouble,           
    (GCFinalizeOp) js_FinalizeString,           
    NULL,                                       
    (GCFinalizeOp) js_FinalizeXMLNamespace,     
    (GCFinalizeOp) js_FinalizeXMLQName,         
    (GCFinalizeOp) js_FinalizeXML,              
    NULL,                                       
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

#ifdef GC_MARK_DEBUG
static const char newborn_external_string[] = "newborn external string";

static const char *gc_typenames[GCX_NTYPES] = {
    "newborn object",
    "newborn string",
    "newborn double",
    "newborn mutable string",
    "newborn private",
    "newborn Namespace",
    "newborn QName",
    "newborn XML",
    newborn_external_string,
    newborn_external_string,
    newborn_external_string,
    newborn_external_string,
    newborn_external_string,
    newborn_external_string,
    newborn_external_string,
    newborn_external_string
};
#endif

intN
js_ChangeExternalStringFinalizer(JSStringFinalizeOp oldop,
                                 JSStringFinalizeOp newop)
{
    uintN i;

    for (i = GCX_EXTERNAL_STRING; i < GCX_NTYPES; i++) {
        if (gc_finalizers[i] == (GCFinalizeOp) oldop) {
            gc_finalizers[i] = (GCFinalizeOp) newop;
            return (intN) i;
        }
    }
    return -1;
}


typedef struct JSGCRootHashEntry {
    JSDHashEntryHdr hdr;
    void            *root;
    const char      *name;
} JSGCRootHashEntry;


#define GC_ROOTS_SIZE   256
#define GC_FINALIZE_LEN 1024

JSBool
js_InitGC(JSRuntime *rt, uint32 maxbytes)
{
    InitGCArenaLists(rt);
    if (!JS_DHashTableInit(&rt->gcRootsHash, JS_DHashGetStubOps(), NULL,
                           sizeof(JSGCRootHashEntry), GC_ROOTS_SIZE)) {
        rt->gcRootsHash.ops = NULL;
        return JS_FALSE;
    }
    rt->gcLocksHash = NULL;     

    



    rt->gcMaxBytes = rt->gcMaxMallocBytes = maxbytes;

    return JS_TRUE;
}

#ifdef JS_GCMETER
JS_FRIEND_API(void)
js_DumpGCStats(JSRuntime *rt, FILE *fp)
{
    uintN i;
    size_t totalThings, totalMaxThings, totalBytes;
    size_t sumArenas, sumTotalArenas;
    size_t sumFreeSize, sumTotalFreeSize;

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
        JSGCArenaList *list = &rt->gcArenaList[i];
        JSGCArenaStats *stats = &list->stats;
        if (stats->maxarenas == 0) {
            fprintf(fp, "ARENA LIST %u (thing size %lu): NEVER USED\n",
                    i, UL(GC_FREELIST_NBYTES(i)));
            continue;
        }
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
                : (100.0 * list->thingSize * (jsdouble)stats->freelen /
                   (GC_THINGS_SIZE * (jsdouble)stats->narenas)));
        fprintf(fp, "  average free list density: %.1f%%\n",
                stats->totalarenas == 0
                ? 0.0
                : (100.0 * list->thingSize * (jsdouble)stats->totalfreelen /
                   (GC_THINGS_SIZE * (jsdouble)stats->totalarenas)));
        fprintf(fp, "                   recycles: %lu\n", UL(stats->recycle));
        fprintf(fp, "        recycle/alloc ratio: %.2f\n",
                (jsdouble)stats->recycle /
                (jsdouble)(stats->totalnew - stats->recycle));
        totalThings += stats->nthings;
        totalMaxThings += stats->maxthings;
        totalBytes += GC_FREELIST_NBYTES(i) * stats->nthings;
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
            : 100.0 * sumFreeSize / (GC_THINGS_SIZE * (jsdouble)sumArenas));
    fprintf(fp, "  average free list density: %.1f%%\n",
            sumTotalFreeSize == 0
            ? 0.0
            : 100.0 * sumTotalFreeSize / (GC_THINGS_SIZE * sumTotalArenas));
    fprintf(fp, "allocation retries after GC: %lu\n", ULSTAT(retry));
    fprintf(fp, "        allocation failures: %lu\n", ULSTAT(fail));
    fprintf(fp, "         things born locked: %lu\n", ULSTAT(lockborn));
    fprintf(fp, "           valid lock calls: %lu\n", ULSTAT(lock));
    fprintf(fp, "         valid unlock calls: %lu\n", ULSTAT(unlock));
    fprintf(fp, "       mark recursion depth: %lu\n", ULSTAT(depth));
    fprintf(fp, "     maximum mark recursion: %lu\n", ULSTAT(maxdepth));
    fprintf(fp, "     mark C recursion depth: %lu\n", ULSTAT(cdepth));
    fprintf(fp, "   maximum mark C recursion: %lu\n", ULSTAT(maxcdepth));
    fprintf(fp, "      delayed scan bag adds: %lu\n", ULSTAT(unscanned));
#ifdef DEBUG
    fprintf(fp, "  max delayed scan bag size: %lu\n", ULSTAT(maxunscanned));
#endif
    fprintf(fp, "   maximum GC nesting level: %lu\n", ULSTAT(maxlevel));
    fprintf(fp, "potentially useful GC calls: %lu\n", ULSTAT(poke));
    fprintf(fp, "           useless GC calls: %lu\n", ULSTAT(nopoke));
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
#if JS_HAS_GENERATORS
    rt->gcCloseState.reachableList = NULL;
    METER(rt->gcStats.nclose = 0);
    rt->gcCloseState.todoQueue = NULL;
#endif
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
    JSDHashOperator op;

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

    return op;
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
CloseIteratorStates(JSContext *cx)
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
            js_CloseIteratorState(cx, obj);
        else
            array[newCount++] = obj;
    }
    ShrinkPtrTable(&rt->gcIteratorTable, &iteratorTableInfo, newCount);
}

#if JS_HAS_GENERATORS

void
js_RegisterGenerator(JSContext *cx, JSGenerator *gen)
{
    JSRuntime *rt;

    rt = cx->runtime;
    JS_ASSERT(!rt->gcRunning);
    JS_ASSERT(rt->state != JSRTS_LANDING);
    JS_ASSERT(gen->state == JSGEN_NEWBORN);

    JS_LOCK_GC(rt);
    gen->next = rt->gcCloseState.reachableList;
    rt->gcCloseState.reachableList = gen;
    METER(rt->gcStats.nclose++);
    METER(rt->gcStats.maxnclose = JS_MAX(rt->gcStats.maxnclose,
                                         rt->gcStats.nclose));
    JS_UNLOCK_GC(rt);
}








static JSBool
CanScheduleCloseHook(JSGenerator *gen)
{
    JSObject *parent;
    JSBool canSchedule;

    
    parent = STOBJ_GET_PARENT(gen->obj);
    canSchedule = *js_GetGCThingFlags(parent) & GCF_MARK;
#ifdef DEBUG_igor
    if (!canSchedule) {
        fprintf(stderr, "GEN: Kill without schedule, gen=%p parent=%p\n",
                (void *)gen, (void *)parent);
    }
#endif
    return canSchedule;
}












static JSBool
ShouldDeferCloseHook(JSContext *cx, JSGenerator *gen, JSBool *defer)
{
    JSObject *parent, *obj;
    JSClass *clasp;
    JSExtendedClass *xclasp;

    



    *defer = JS_FALSE;
    parent = OBJ_GET_PARENT(cx, gen->obj);
    clasp = OBJ_GET_CLASS(cx, parent);
    if (clasp->flags & JSCLASS_IS_EXTENDED) {
        xclasp = (JSExtendedClass *)clasp;
        if (xclasp->outerObject) {
            obj = xclasp->outerObject(cx, parent);
            if (!obj)
                return JS_FALSE;
            OBJ_TO_INNER_OBJECT(cx, obj);
            if (!obj)
                return JS_FALSE;
            *defer = obj != parent;
        }
    }
#ifdef DEBUG_igor
    if (*defer) {
        fprintf(stderr, "GEN: deferring, gen=%p parent=%p\n",
                (void *)gen, (void *)parent);
    }
#endif
    return JS_TRUE;
}







static void
FindAndMarkObjectsToClose(JSContext *cx, JSGCInvocationKind gckind,
                          JSGenerator **todoQueueTail)
{
    JSRuntime *rt;
    JSGenerator *todo, **genp, *gen;

    rt = cx->runtime;
    todo = NULL;
    genp = &rt->gcCloseState.reachableList;
    while ((gen = *genp) != NULL) {
        if (*js_GetGCThingFlags(gen->obj) & GCF_MARK) {
            genp = &gen->next;
        } else {
            
            JS_ASSERT(gen->state == JSGEN_NEWBORN ||
                      gen->state == JSGEN_OPEN ||
                      gen->state == JSGEN_CLOSED);

            *genp = gen->next;
            if (gen->state == JSGEN_OPEN &&
                js_FindFinallyHandler(gen->frame.script, gen->frame.pc) &&
                CanScheduleCloseHook(gen)) {
                







                gen->next = NULL;
                *todoQueueTail = gen;
                todoQueueTail = &gen->next;
                if (!todo)
                    todo = gen;
                METER(JS_ASSERT(rt->gcStats.nclose));
                METER(rt->gcStats.nclose--);
                METER(rt->gcStats.closelater++);
                METER(rt->gcStats.maxcloselater
                      = JS_MAX(rt->gcStats.maxcloselater,
                               rt->gcStats.closelater));
            }
        }
    }

    if (gckind == GC_LAST_CONTEXT) {
        



        rt->gcCloseState.todoQueue = NULL;
    } else {
        




        for (gen = todo; gen; gen = gen->next)
            GC_MARK(cx, gen->obj, "newly scheduled generator");
    }
}





static JSGenerator **
MarkScheduledGenerators(JSContext *cx)
{
    JSRuntime *rt;
    JSGenerator **genp, *gen;

    rt = cx->runtime;
    genp = &rt->gcCloseState.todoQueue;
    while ((gen = *genp) != NULL) {
        if (CanScheduleCloseHook(gen)) {
            GC_MARK(cx, gen->obj, "scheduled generator");
            genp = &gen->next;
        } else {
            
            *genp = gen->next;
            METER(JS_ASSERT(rt->gcStats.closelater > 0));
            METER(rt->gcStats.closelater--);
        }
    }
    return genp;
}

#ifdef JS_THREADSAFE
# define GC_RUNNING_CLOSE_HOOKS_PTR(cx)                                       \
    (&(cx)->thread->gcRunningCloseHooks)
#else
# define GC_RUNNING_CLOSE_HOOKS_PTR(cx)                                       \
    (&(cx)->runtime->gcCloseState.runningCloseHook)
#endif

typedef struct JSTempCloseList {
    JSTempValueRooter   tvr;
    JSGenerator         *head;
} JSTempCloseList;

JS_STATIC_DLL_CALLBACK(void)
mark_temp_close_list(JSContext *cx, JSTempValueRooter *tvr)
{
    JSTempCloseList *list = (JSTempCloseList *)tvr;
    JSGenerator *gen;

    for (gen = list->head; gen; gen = gen->next)
        GC_MARK(cx, gen->obj, "temp list generator");
}

#define JS_PUSH_TEMP_CLOSE_LIST(cx, tempList)                                 \
    JS_PUSH_TEMP_ROOT_MARKER(cx, mark_temp_close_list, &(tempList)->tvr)

#define JS_POP_TEMP_CLOSE_LIST(cx, tempList)                                  \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT((tempList)->tvr.u.marker == mark_temp_close_list);          \
        JS_POP_TEMP_ROOT(cx, &(tempList)->tvr);                               \
    JS_END_MACRO

JSBool
js_RunCloseHooks(JSContext *cx)
{
    JSRuntime *rt;
    JSTempCloseList tempList;
    JSStackFrame *fp;
    JSGenerator **genp, *gen;
    JSBool ok, defer;
#if JS_GCMETER
    uint32 deferCount;
#endif

    rt = cx->runtime;

    




    if (!rt->gcCloseState.todoQueue)
        return JS_TRUE;

    





    if (*GC_RUNNING_CLOSE_HOOKS_PTR(cx))
        return JS_TRUE;

    *GC_RUNNING_CLOSE_HOOKS_PTR(cx) = JS_TRUE;

    JS_LOCK_GC(rt);
    tempList.head = rt->gcCloseState.todoQueue;
    JS_PUSH_TEMP_CLOSE_LIST(cx, &tempList);
    rt->gcCloseState.todoQueue = NULL;
    METER(rt->gcStats.closelater = 0);
    rt->gcPoke = JS_TRUE;
    JS_UNLOCK_GC(rt);

    





    fp = cx->fp;
    if (fp) {
        JS_ASSERT(!fp->dormantNext);
        fp->dormantNext = cx->dormantFrameChain;
        cx->dormantFrameChain = fp;
    }
    cx->fp = NULL;

    genp = &tempList.head;
    ok = JS_TRUE;
    while ((gen = *genp) != NULL) {
        ok = ShouldDeferCloseHook(cx, gen, &defer);
        if (!ok) {
            
            *genp = gen->next;
            break;
        }
        if (defer) {
            genp = &gen->next;
            METER(deferCount++);
            continue;
        }
        ok = js_CloseGeneratorObject(cx, gen);

        



        *genp = gen->next;

        if (cx->throwing) {
            



            if (!js_ReportUncaughtException(cx))
                JS_ClearPendingException(cx);
            ok = JS_TRUE;
        } else if (!ok) {
            




            break;
        }
    }

    cx->fp = fp;
    if (fp) {
        JS_ASSERT(cx->dormantFrameChain == fp);
        cx->dormantFrameChain = fp->dormantNext;
        fp->dormantNext = NULL;
    }

    if (tempList.head) {
        



        while ((gen = *genp) != NULL) {
            genp = &gen->next;
            METER(deferCount++);
        }

        
        JS_LOCK_GC(rt);
        *genp = rt->gcCloseState.todoQueue;
        rt->gcCloseState.todoQueue = tempList.head;
        METER(rt->gcStats.closelater += deferCount);
        METER(rt->gcStats.maxcloselater
              = JS_MAX(rt->gcStats.maxcloselater, rt->gcStats.closelater));
        JS_UNLOCK_GC(rt);
    }

    JS_POP_TEMP_CLOSE_LIST(cx, &tempList);
    *GC_RUNNING_CLOSE_HOOKS_PTR(cx) = JS_FALSE;

    return ok;
}

#endif 

#if defined(DEBUG_brendan) || defined(DEBUG_timeless)
#define DEBUG_gchist
#endif

#ifdef DEBUG_gchist
#define NGCHIST 64

static struct GCHist {
    JSBool      lastDitch;
    JSGCThing   *freeList;
} gchist[NGCHIST];

unsigned gchpos;
#endif

void *
js_NewGCThing(JSContext *cx, uintN flags, size_t nbytes)
{
    JSRuntime *rt;
    uintN flindex;
    JSBool doGC;
    JSGCThing *thing;
    uint8 *flagp, *firstPage;
    JSGCArenaList *arenaList;
    jsuword offset;
    JSGCArena *a;
    JSLocalRootStack *lrs;
#ifdef JS_THREADSAFE
    JSBool gcLocked;
    uintN localMallocBytes;
    JSGCThing **flbase, **lastptr;
    JSGCThing *tmpthing;
    uint8 *tmpflagp;
    uintN maxFreeThings;         
    METER(size_t nfree);
#endif

    rt = cx->runtime;
    METER(rt->gcStats.alloc++);        
    nbytes = JS_ROUNDUP(nbytes, sizeof(JSGCThing));
    flindex = GC_FREELIST_INDEX(nbytes);

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

#ifdef TOO_MUCH_GC
#ifdef WAY_TOO_MUCH_GC
    rt->gcPoke = JS_TRUE;
#endif
    doGC = JS_TRUE;
#else
    doGC = (rt->gcMallocBytes >= rt->gcMaxMallocBytes);
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
            METER(arenaList->stats.freelen--);
            METER(arenaList->stats.recycle++);

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

        
        if ((arenaList->last && arenaList->lastLimit != GC_THINGS_SIZE) ||
            NewGCArena(rt, arenaList)) {

            offset = arenaList->lastLimit;
            if ((offset & GC_PAGE_MASK) == 0) {
                


                offset += PAGE_THING_GAP(nbytes);
            }
            JS_ASSERT(offset + nbytes <= GC_THINGS_SIZE);
            arenaList->lastLimit = (uint16)(offset + nbytes);
            a = arenaList->last;
            firstPage = (uint8 *)FIRST_THING_PAGE(a);
            thing = (JSGCThing *)(firstPage + offset);
            flagp = a->base + offset / sizeof(JSGCThing);
            if (flagp >= firstPage)
                flagp += GC_THINGS_SIZE;

#ifdef JS_THREADSAFE
            




            if (rt->gcMallocBytes >= rt->gcMaxMallocBytes || flbase[flindex])
                break;
            METER(nfree = 0);
            lastptr = &flbase[flindex];
            maxFreeThings = MAX_THREAD_LOCAL_THINGS;
            for (offset = arenaList->lastLimit;
                 offset != GC_THINGS_SIZE && maxFreeThings-- != 0;
                 offset += nbytes) {
                if ((offset & GC_PAGE_MASK) == 0)
                    offset += PAGE_THING_GAP(nbytes);
                JS_ASSERT(offset + nbytes <= GC_THINGS_SIZE);
                tmpflagp = a->base + offset / sizeof(JSGCThing);
                if (tmpflagp >= firstPage)
                    tmpflagp += GC_THINGS_SIZE;

                tmpthing = (JSGCThing *)(firstPage + offset);
                tmpthing->flagp = tmpflagp;
                *tmpflagp = GCF_FINAL;    

                *lastptr = tmpthing;
                lastptr = &tmpthing->next;
                METER(++nfree);
            }
            arenaList->lastLimit = offset;
            *lastptr = NULL;
            METER(arenaList->stats.freelen += nfree);
#endif
            break;
        }

        
        if (doGC)
            goto fail;
        rt->gcPoke = JS_TRUE;
        doGC = JS_TRUE;
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

    



    thing->next = NULL;
    thing->flagp = NULL;
#ifdef DEBUG_gchist
    gchist[gchpos].lastDitch = doGC;
    gchist[gchpos].freeList = rt->gcArenaList[flindex].freeList;
    if (++gchpos == NGCHIST)
        gchpos = 0;
#endif
#ifdef JS_GCMETER
    {
        JSGCArenaStats *stats = &rt->gcArenaList[flindex].stats;

        
        if (flags & GCF_LOCK)
            rt->gcStats.lockborn++;
        stats->totalnew++;
        stats->nthings++;
        if (stats->nthings > stats->maxthings)
            stats->maxthings = stats->nthings;
    }
#endif
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

    flagp = js_GetGCThingFlags(thing);

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
            lhe->thing = thing;
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

    flagp = js_GetGCThingFlags(thing);
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

#ifdef GC_MARK_DEBUG

#include <stdio.h>
#include "jsprf.h"

typedef struct GCMarkNode GCMarkNode;

struct GCMarkNode {
    void        *thing;
    const char  *name;
    GCMarkNode  *next;
    GCMarkNode  *prev;
};

JS_FRIEND_DATA(FILE *) js_DumpGCHeap;
JS_EXPORT_DATA(void *) js_LiveThingToFind;

#ifdef HAVE_XPCONNECT
#include "dump_xpc.h"
#endif

static void
GetObjSlotName(JSObject *obj, uint32 slot, char *buf, size_t bufsize)
{
    JSScope *scope;
    jsval nval;
    JSScopeProperty *sprop;
    JSClass *clasp;
    uint32 key;
    const char *slotname;

    scope = OBJ_IS_NATIVE(obj) ? OBJ_SCOPE(obj) : NULL;
    if (!scope) {
        JS_snprintf(buf, bufsize, "**UNKNOWN OBJECT MAP ENTRY**");
        return;
    }

    sprop = SCOPE_LAST_PROP(scope);
    while (sprop && sprop->slot != slot)
        sprop = sprop->parent;

    if (!sprop) {
        switch (slot) {
          case JSSLOT_PROTO:
            JS_snprintf(buf, bufsize, "__proto__");
            break;
          case JSSLOT_PARENT:
            JS_snprintf(buf, bufsize, "__parent__");
            break;
          default:
            slotname = NULL;
            clasp = LOCKED_OBJ_GET_CLASS(obj);
            if (clasp->flags & JSCLASS_IS_GLOBAL) {
                key = slot - JSSLOT_START(clasp);
#define JS_PROTO(name,code,init) \
    if ((code) == key) { slotname = js_##name##_str; goto found; }
#include "jsproto.tbl"
#undef JS_PROTO
            }
          found:
            if (slotname)
                JS_snprintf(buf, bufsize, "CLASS_OBJECT(%s)", slotname);
            else
                JS_snprintf(buf, bufsize, "**UNKNOWN SLOT %ld**", (long)slot);
            break;
        }
    } else {
        nval = ID_TO_VALUE(sprop->id);
        if (JSVAL_IS_INT(nval)) {
            JS_snprintf(buf, bufsize, "%ld", (long)JSVAL_TO_INT(nval));
        } else if (JSVAL_IS_STRING(nval)) {
            js_PutEscapedString(buf, bufsize, JSVAL_TO_STRING(nval), 0);
        } else {
            JS_snprintf(buf, bufsize, "**FINALIZED ATOM KEY**");
        }
    }
}

static const char *
gc_object_class_name(void* thing)
{
    uint8 *flagp = js_GetGCThingFlags(thing);
    const char *className = "";
    static char depbuf[32];

    switch (*flagp & GCF_TYPEMASK) {
      case GCX_OBJECT: {
        JSObject  *obj = (JSObject *)thing;
        JSClass   *clasp = STOBJ_GET_CLASS(obj);
        className = clasp->name;
#ifdef HAVE_XPCONNECT
        if (clasp->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS) {
            jsval privateValue = STOBJ_GET_SLOT(obj, JSSLOT_PRIVATE);

            JS_ASSERT(clasp->flags & JSCLASS_HAS_PRIVATE);
            if (!JSVAL_IS_VOID(privateValue)) {
                void  *privateThing = JSVAL_TO_PRIVATE(privateValue);
                const char *xpcClassName = GetXPCObjectClassName(privateThing);

                if (xpcClassName)
                    className = xpcClassName;
            }
        }
#endif
        break;
      }

      case GCX_STRING:
      case GCX_MUTABLE_STRING: {
        JSString *str = (JSString *)thing;
        if (JSSTRING_IS_DEPENDENT(str)) {
            JS_snprintf(depbuf, sizeof depbuf, "start:%u, length:%u",
                        JSSTRDEP_START(str), JSSTRDEP_LENGTH(str));
            className = depbuf;
        } else {
            className = "string";
        }
        break;
      }

      case GCX_DOUBLE:
        className = "double";
        break;
    }

    return className;
}

static void
gc_dump_thing(JSContext *cx, JSGCThing *thing, FILE *fp)
{
    GCMarkNode *prev = (GCMarkNode *)cx->gcCurrentMarkNode;
    GCMarkNode *next = NULL;
    char *path = NULL;

    while (prev) {
        next = prev;
        prev = prev->prev;
    }
    while (next) {
        uint8 nextFlags = *js_GetGCThingFlags(next->thing);
        if ((nextFlags & GCF_TYPEMASK) == GCX_OBJECT) {
            path = JS_sprintf_append(path, "%s(%s @ 0x%08p).",
                                     next->name,
                                     gc_object_class_name(next->thing),
                                     (JSObject*)next->thing);
        } else {
            path = JS_sprintf_append(path, "%s(%s).",
                                     next->name,
                                     gc_object_class_name(next->thing));
        }
        next = next->next;
    }
    if (!path)
        return;

    fprintf(fp, "%08lx ", (long)thing);
    switch (*js_GetGCThingFlags(thing) & GCF_TYPEMASK) {
      case GCX_OBJECT:
      {
        JSObject  *obj = (JSObject *)thing;
        jsval     privateValue = STOBJ_GET_SLOT(obj, JSSLOT_PRIVATE);
        void      *privateThing = JSVAL_IS_VOID(privateValue)
                                  ? NULL
                                  : JSVAL_TO_PRIVATE(privateValue);
        const char *className = gc_object_class_name(thing);
        fprintf(fp, "object %8p %s", privateThing, className);
        break;
      }
#if JS_HAS_XML_SUPPORT
      case GCX_NAMESPACE:
      {
        JSXMLNamespace *ns = (JSXMLNamespace *)thing;

        fputs("namespace ", fp);
        if (ns->prefix)
            js_FileEscapedString(fp, ns->prefix, 0);
        fputc(':', fp);
        js_FileEscapedString(fp, ns->uri, 0);
        break;
      }
      case GCX_QNAME:
      {
        JSXMLQName *qn = (JSXMLQName *)thing;

        fputs("qname ", fp);
        if (qn->prefix)
            js_FileEscapedString(fp, qn->prefix, 0);
        fputc('(', fp);
        js_FileEscapedString(fp, qn->uri, 0);
        fputs("):", fp);
        js_FileEscapedString(fp, qn->localName, 0);
        break;
      }
      case GCX_XML:
      {
        extern const char *js_xml_class_str[];
        JSXML *xml = (JSXML *)thing;
        fprintf(fp, "xml %8p %s", xml, js_xml_class_str[xml->xml_class]);
        break;
      }
#endif
      case GCX_DOUBLE:
        fprintf(fp, "double %g", *(jsdouble *)thing);
        break;
      case GCX_PRIVATE:
        fprintf(fp, "private %8p", (void *)thing);
        break;
      default:
        fputs("string ", fp);
        js_FileEscapedString(fp, (JSString *)thing, 0);
        break;
    }
    fprintf(fp, " via %s\n", path);
    free(path);
}

void
js_MarkNamedGCThing(JSContext *cx, void *thing, const char *name)
{
    GCMarkNode markNode;

    if (!thing)
        return;

    markNode.thing = thing;
    markNode.name  = name;
    markNode.next  = NULL;
    markNode.prev  = (GCMarkNode *)cx->gcCurrentMarkNode;
    if (markNode.prev)
        markNode.prev->next = &markNode;
    cx->gcCurrentMarkNode = &markNode;

    if (thing == js_LiveThingToFind) {
        



        gc_dump_thing(cx, thing, stderr);
    }

    js_MarkGCThing(cx, thing);

    if (markNode.prev)
        markNode.prev->next = NULL;
    cx->gcCurrentMarkNode = markNode.prev;
}

#endif 

static void
gc_mark_atom_key_thing(void *thing, void *arg)
{
    JSContext *cx = (JSContext *) arg;

    GC_MARK(cx, thing, "atom");
}

void
js_MarkAtom(JSContext *cx, JSAtom *atom)
{
    jsval key;
    void *thing;
    uint8 *flagp;

    if (atom->flags & ATOM_MARK) {
        if (ATOM_IS_OBJECT(atom) && cx->runtime->gcThingCallback) {
            thing = ATOM_TO_OBJECT(atom);
            flagp = js_GetGCThingFlags(thing);
            cx->runtime->gcThingCallback(thing, *flagp,
                                         cx->runtime->gcThingCallbackClosure);
        }

        return;
    }

    atom->flags |= ATOM_MARK;
    key = ATOM_KEY(atom);
    if (JSVAL_IS_GCTHING(key)) {
#ifdef GC_MARK_DEBUG
        char name[32];

        if (JSVAL_IS_STRING(key)) {
            js_PutEscapedString(name, sizeof name, JSVAL_TO_STRING(key), '\'');
        } else {
            JS_snprintf(name, sizeof name, "<%x>", key);
        }
#endif
        GC_MARK(cx, JSVAL_TO_GCTHING(key), name);
    }
    if (atom->flags & ATOM_HIDDEN)
        js_MarkAtom(cx, atom->entry.value);
}

static void
AddThingToUnscannedBag(JSRuntime *rt, void *thing, uint8 *flagp);

static void
MarkGCThingChildren(JSContext *cx, void *thing, uint8 *flagp,
                    JSBool shouldCheckRecursion)
{
    JSRuntime *rt;
    JSObject *obj;
    jsval v;
    uint32 i, end;
#ifndef GC_MARK_DEBUG
    void *next_thing;
    uint8 *next_flagp;
#endif
    JSString *str;
#ifdef JS_GCMETER
    uint32 tailCallNesting;
#endif

    




#ifdef JS_GC_ASSUME_LOW_C_STACK
# define RECURSION_TOO_DEEP() shouldCheckRecursion
#else
    int stackDummy;
# define RECURSION_TOO_DEEP() (shouldCheckRecursion &&                        \
                               !JS_CHECK_STACK_SIZE(cx, stackDummy))
#endif

    rt = cx->runtime;
    METER(tailCallNesting = 0);
    METER(if (++rt->gcStats.cdepth > rt->gcStats.maxcdepth)
              rt->gcStats.maxcdepth = rt->gcStats.cdepth);

#ifndef GC_MARK_DEBUG
  start:
#endif
    JS_ASSERT(flagp);
    JS_ASSERT(*flagp & GCF_MARK); 
    METER(if (++rt->gcStats.depth > rt->gcStats.maxdepth)
              rt->gcStats.maxdepth = rt->gcStats.depth);
#ifdef GC_MARK_DEBUG
    if (js_DumpGCHeap)
        gc_dump_thing(cx, thing, js_DumpGCHeap);
#endif

    switch (*flagp & GCF_TYPEMASK) {
      case GCX_OBJECT:
        if (RECURSION_TOO_DEEP())
            goto add_to_unscanned_bag;

        
        obj = (JSObject *) thing;
        if (!obj->map)
            break;

        
        end = (obj->map->ops->mark)
              ? obj->map->ops->mark(cx, obj, NULL)
              : LOCKED_OBJ_NSLOTS(obj);
        thing = NULL;
        flagp = NULL;
        for (i = 0; i != end; ++i) {
            v = STOBJ_GET_SLOT(obj, i);
            if (!JSVAL_IS_GCTHING(v) || v == JSVAL_NULL)
                continue;
#ifdef GC_MARK_DEBUG
            




            {
                char name[32];
                GetObjSlotName(obj, i, name, sizeof name);
                GC_MARK(cx, JSVAL_TO_GCTHING(v), name);
            }
#else            
            next_thing = JSVAL_TO_GCTHING(v);
            next_flagp = js_GetGCThingFlags(next_thing);
            if (rt->gcThingCallback) {
                rt->gcThingCallback(next_thing, *next_flagp,
                                    rt->gcThingCallbackClosure);
            }
            if (next_thing == thing)
                continue;
            if (*next_flagp & GCF_MARK)
                continue;
            JS_ASSERT(*next_flagp != GCF_FINAL);
            if (thing) {
                *flagp |= GCF_MARK;
                MarkGCThingChildren(cx, thing, flagp, JS_TRUE);
                if (*next_flagp & GCF_MARK) {
                    



                    thing = NULL;
                    continue;
                }
            }
            thing = next_thing;
            flagp = next_flagp;
        }
        if (thing) {
            







            shouldCheckRecursion = JS_FALSE;
            goto on_tail_recursion;
#endif
        }
        break;

#ifdef DEBUG
      case GCX_STRING:
        str = (JSString *)thing;
        JS_ASSERT(!JSSTRING_IS_DEPENDENT(str));
        break;
#endif

      case GCX_MUTABLE_STRING:
        str = (JSString *)thing;
        if (!JSSTRING_IS_DEPENDENT(str))
            break;
        thing = JSSTRDEP_BASE(str);
#ifdef GC_MARK_DEBUG
        GC_MARK(cx, thing, "base");
        break;
#else
        flagp = js_GetGCThingFlags(thing);
        if (rt->gcThingCallback) {
            rt->gcThingCallback(thing, *flagp,
                                rt->gcThingCallbackClosure);
        }
        if (*flagp & GCF_MARK)
            break;

        

      on_tail_recursion:
        
        JS_ASSERT(*flagp != GCF_FINAL);
        METER(++tailCallNesting);
        *flagp |= GCF_MARK;
        goto start;
#endif

#if JS_HAS_XML_SUPPORT
      case GCX_NAMESPACE:
        if (RECURSION_TOO_DEEP())
            goto add_to_unscanned_bag;
        js_MarkXMLNamespace(cx, (JSXMLNamespace *)thing);
        break;

      case GCX_QNAME:
        if (RECURSION_TOO_DEEP())
            goto add_to_unscanned_bag;
        js_MarkXMLQName(cx, (JSXMLQName *)thing);
        break;

      case GCX_XML:
        if (RECURSION_TOO_DEEP())
            goto add_to_unscanned_bag;
        js_MarkXML(cx, (JSXML *)thing);
        break;
#endif
      add_to_unscanned_bag:
        AddThingToUnscannedBag(cx->runtime, thing, flagp);
        break;
    }

#undef RECURSION_TOO_DEEP

    METER(rt->gcStats.depth -= 1 + tailCallNesting);
    METER(rt->gcStats.cdepth--);
}





#define GET_GAP_AND_CHUNK_SPAN(thingSize, thingsPerUnscannedChunk, pageGap)   \
    JS_BEGIN_MACRO                                                            \
        if (0 == ((thingSize) & ((thingSize) - 1))) {                         \
            pageGap = (thingSize);                                            \
            thingsPerUnscannedChunk = ((GC_PAGE_SIZE / (thingSize))           \
                                       + JS_BITS_PER_WORD - 1)                \
                                      >> JS_BITS_PER_WORD_LOG2;               \
        } else {                                                              \
            pageGap = GC_PAGE_SIZE % (thingSize);                             \
            thingsPerUnscannedChunk = JS_HOWMANY(GC_PAGE_SIZE / (thingSize),  \
                                                 JS_BITS_PER_WORD);           \
        }                                                                     \
    JS_END_MACRO

static void
AddThingToUnscannedBag(JSRuntime *rt, void *thing, uint8 *flagp)
{
    JSGCPageInfo *pi;
    JSGCArena *arena;
    size_t thingSize;
    size_t thingsPerUnscannedChunk;
    size_t pageGap;
    size_t chunkIndex;
    jsuword bit;

    
    JS_ASSERT((*flagp & (GCF_MARK | GCF_FINAL)) == GCF_MARK);
    *flagp |= GCF_FINAL;

    METER(rt->gcStats.unscanned++);
#ifdef DEBUG
    ++rt->gcUnscannedBagSize;
    METER(if (rt->gcUnscannedBagSize > rt->gcStats.maxunscanned)
              rt->gcStats.maxunscanned = rt->gcUnscannedBagSize);
#endif

    pi = THING_TO_PAGE(thing);
    arena = PAGE_TO_ARENA(pi);
    thingSize = arena->list->thingSize;
    GET_GAP_AND_CHUNK_SPAN(thingSize, thingsPerUnscannedChunk, pageGap);
    chunkIndex = (((jsuword)thing & GC_PAGE_MASK) - pageGap) /
                 (thingSize * thingsPerUnscannedChunk);
    JS_ASSERT(chunkIndex < JS_BITS_PER_WORD);
    bit = (jsuword)1 << chunkIndex;
    if (pi->unscannedBitmap != 0) {
        JS_ASSERT(rt->gcUnscannedArenaStackTop);
        if (thingsPerUnscannedChunk != 1) {
            if (pi->unscannedBitmap & bit) {
                
                return;
            }
        } else {
            



            JS_ASSERT(!(pi->unscannedBitmap & bit));
        }
        pi->unscannedBitmap |= bit;
        JS_ASSERT(arena->unscannedPages & ((size_t)1 << PAGE_INDEX(pi)));
    } else {
        



        pi->unscannedBitmap = bit;
        JS_ASSERT(PAGE_INDEX(pi) < JS_BITS_PER_WORD);
        bit = (jsuword)1 << PAGE_INDEX(pi);
        JS_ASSERT(!(arena->unscannedPages & bit));
        if (arena->unscannedPages != 0) {
            arena->unscannedPages |= bit;
            JS_ASSERT(arena->prevUnscanned);
            JS_ASSERT(rt->gcUnscannedArenaStackTop);
        } else {
            










            arena->unscannedPages = bit;
            if (!arena->prevUnscanned) {
                if (!rt->gcUnscannedArenaStackTop) {
                    
                    arena->prevUnscanned = arena;
                } else {
                    JS_ASSERT(rt->gcUnscannedArenaStackTop->prevUnscanned);
                    arena->prevUnscanned = rt->gcUnscannedArenaStackTop;
                }
                rt->gcUnscannedArenaStackTop = arena;
            }
         }
     }
    JS_ASSERT(rt->gcUnscannedArenaStackTop);
}

static void
ScanDelayedChildren(JSContext *cx)
{
    JSRuntime *rt;
    JSGCArena *arena;
    size_t thingSize;
    size_t thingsPerUnscannedChunk;
    size_t pageGap;
    size_t pageIndex;
    JSGCPageInfo *pi;
    size_t chunkIndex;
    size_t thingOffset, thingLimit;
    JSGCThing *thing;
    uint8 *flagp;
    JSGCArena *prevArena;

    rt = cx->runtime;
    arena = rt->gcUnscannedArenaStackTop;
    if (!arena) {
        JS_ASSERT(rt->gcUnscannedBagSize == 0);
        return;
    }

  init_size:
    thingSize = arena->list->thingSize;
    GET_GAP_AND_CHUNK_SPAN(thingSize, thingsPerUnscannedChunk, pageGap);
    for (;;) {
        




        JS_ASSERT(arena->prevUnscanned);
        JS_ASSERT(rt->gcUnscannedArenaStackTop->prevUnscanned);
        while (arena->unscannedPages != 0) {
            pageIndex = JS_FLOOR_LOG2W(arena->unscannedPages);
            JS_ASSERT(pageIndex < GC_PAGE_COUNT);
            pi = (JSGCPageInfo *)(FIRST_THING_PAGE(arena) +
                                  pageIndex * GC_PAGE_SIZE);
            JS_ASSERT(pi->unscannedBitmap);
            chunkIndex = JS_FLOOR_LOG2W(pi->unscannedBitmap);
            pi->unscannedBitmap &= ~((jsuword)1 << chunkIndex);
            if (pi->unscannedBitmap == 0)
                arena->unscannedPages &= ~((jsuword)1 << pageIndex);
            thingOffset = (pageGap
                           + chunkIndex * thingsPerUnscannedChunk * thingSize);
            JS_ASSERT(thingOffset >= sizeof(JSGCPageInfo));
            thingLimit = thingOffset + thingsPerUnscannedChunk * thingSize;
            if (thingsPerUnscannedChunk != 1) {
                



                if (arena->list->last == arena &&
                    arena->list->lastLimit < (pageIndex * GC_PAGE_SIZE +
                                              thingLimit)) {
                    thingLimit = (arena->list->lastLimit -
                                  pageIndex * GC_PAGE_SIZE);
                } else if (thingLimit > GC_PAGE_SIZE) {
                    thingLimit = GC_PAGE_SIZE;
                }
                JS_ASSERT(thingLimit > thingOffset);
            }
            JS_ASSERT(arena->list->last != arena ||
                      arena->list->lastLimit >= (pageIndex * GC_PAGE_SIZE +
                                                 thingLimit));
            JS_ASSERT(thingLimit <= GC_PAGE_SIZE);

            for (; thingOffset != thingLimit; thingOffset += thingSize) {
                



                thing = (void *)((jsuword)pi + thingOffset);
                flagp = js_GetGCThingFlags(thing);
                if (thingsPerUnscannedChunk != 1) {
                    



                    if ((*flagp & (GCF_MARK|GCF_FINAL)) != (GCF_MARK|GCF_FINAL))
                        continue;
                }
                JS_ASSERT((*flagp & (GCF_MARK|GCF_FINAL))
                              == (GCF_MARK|GCF_FINAL));
                *flagp &= ~GCF_FINAL;
#ifdef DEBUG
                JS_ASSERT(rt->gcUnscannedBagSize != 0);
                --rt->gcUnscannedBagSize;

                



                switch (*flagp & GCF_TYPEMASK) {
                  case GCX_OBJECT:
# if JS_HAS_XML_SUPPORT
                  case GCX_NAMESPACE:
                  case GCX_QNAME:
                  case GCX_XML:
# endif
                    break;
                  default:
                    JS_ASSERT(0);
                }
#endif
                MarkGCThingChildren(cx, thing, flagp, JS_FALSE);
            }
        }
        








        if (arena == rt->gcUnscannedArenaStackTop) {
            prevArena = arena->prevUnscanned;
            arena->prevUnscanned = NULL;
            if (arena == prevArena) {
                



                break;
            }
            rt->gcUnscannedArenaStackTop = arena = prevArena;
        } else {
            arena = rt->gcUnscannedArenaStackTop;
        }
        if (arena->list->thingSize != thingSize)
            goto init_size;
    }
    JS_ASSERT(rt->gcUnscannedArenaStackTop);
    JS_ASSERT(!rt->gcUnscannedArenaStackTop->prevUnscanned);
    rt->gcUnscannedArenaStackTop = NULL;
    JS_ASSERT(rt->gcUnscannedBagSize == 0);
}

void
js_MarkGCThing(JSContext *cx, void *thing)
{
    uint8 *flagp;

    if (!thing)
        return;

    flagp = js_GetGCThingFlags(thing);
    JS_ASSERT(*flagp != GCF_FINAL);

    if (cx->runtime->gcThingCallback) {
        cx->runtime->gcThingCallback(thing, *flagp,
                                     cx->runtime->gcThingCallbackClosure);
    }

    if (*flagp & GCF_MARK)
        return;
    *flagp |= GCF_MARK;

    if (!cx->insideGCMarkCallback) {
        MarkGCThingChildren(cx, thing, flagp, JS_TRUE);
    } else {
        















        cx->insideGCMarkCallback = JS_FALSE;
        MarkGCThingChildren(cx, thing, flagp, JS_FALSE);
        ScanDelayedChildren(cx);
        cx->insideGCMarkCallback = JS_TRUE;
    }
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
gc_root_marker(JSDHashTable *table, JSDHashEntryHdr *hdr, uint32 num, void *arg)
{
    JSGCRootHashEntry *rhe = (JSGCRootHashEntry *)hdr;
    jsval *rp = (jsval *)rhe->root;
    jsval v = *rp;

    
    if (!JSVAL_IS_NULL(v) && JSVAL_IS_GCTHING(v)) {
        JSContext *cx = (JSContext *)arg;
#ifdef DEBUG
        JSBool root_points_to_gcArenaList = JS_FALSE;
        jsuword thing = (jsuword) JSVAL_TO_GCTHING(v);
        uintN i;
        JSGCArenaList *arenaList;
        JSGCArena *a;
        size_t limit;

        for (i = 0; i < GC_NUM_FREELISTS; i++) {
            arenaList = &cx->runtime->gcArenaList[i];
            limit = arenaList->lastLimit;
            for (a = arenaList->last; a; a = a->prev) {
                if (thing - FIRST_THING_PAGE(a) < limit) {
                    root_points_to_gcArenaList = JS_TRUE;
                    break;
                }
                limit = GC_THINGS_SIZE;
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

        GC_MARK(cx, JSVAL_TO_GCTHING(v), rhe->name ? rhe->name : "root");
    }
    return JS_DHASH_NEXT;
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
gc_lock_marker(JSDHashTable *table, JSDHashEntryHdr *hdr, uint32 num, void *arg)
{
    JSGCLockHashEntry *lhe = (JSGCLockHashEntry *)hdr;
    void *thing = (void *)lhe->thing;
    JSContext *cx = (JSContext *)arg;
    JSRuntime *rt = cx->runtime;
    uint32 i, end;
    uint8 *flagp;

    GC_MARK(cx, thing, "locked object");
    if (rt->gcThingCallback) {
        end = lhe->count - 1;
        flagp = js_GetGCThingFlags(thing);
        for (i = 0; i != end; ++i) {
            rt->gcThingCallback(thing, *flagp, rt->gcThingCallbackClosure);
        }
    }
    return JS_DHASH_NEXT;
}

#define GC_MARK_JSVALS(cx, len, vec, name)                                    \
    JS_BEGIN_MACRO                                                            \
        jsval _v, *_vp, *_end;                                                \
                                                                              \
        for (_vp = vec, _end = _vp + len; _vp < _end; _vp++) {                \
            _v = *_vp;                                                        \
            if (JSVAL_IS_GCTHING(_v))                                         \
                GC_MARK(cx, JSVAL_TO_GCTHING(_v), name);                      \
        }                                                                     \
    JS_END_MACRO

void
js_MarkStackFrame(JSContext *cx, JSStackFrame *fp)
{
    uintN depth, nslots;

    if (fp->callobj)
        GC_MARK(cx, fp->callobj, "call object");
    if (fp->argsobj)
        GC_MARK(cx, fp->argsobj, "arguments object");
    if (fp->varobj)
        GC_MARK(cx, fp->varobj, "variables object");
    if (fp->script) {
        js_MarkScript(cx, fp->script);
        if (fp->spbase) {
            



            depth = fp->script->depth;
            nslots = (JS_UPTRDIFF(fp->sp, fp->spbase)
                      < depth * sizeof(jsval))
                     ? (uintN)(fp->sp - fp->spbase)
                     : depth;
            GC_MARK_JSVALS(cx, nslots, fp->spbase, "operand");
        }
    }

    
    JS_ASSERT(JSVAL_IS_OBJECT((jsval)fp->thisp) ||
              (fp->fun && JSFUN_THISP_FLAGS(fp->fun->flags)));
    if (JSVAL_IS_GCTHING((jsval)fp->thisp))
        GC_MARK(cx, JSVAL_TO_GCTHING((jsval)fp->thisp), "this");

    

























    if (fp->argv) {
        nslots = fp->argc;
        if (fp->fun) {
            if (fp->fun->nargs > nslots)
                nslots = fp->fun->nargs;
            if (!FUN_INTERPRETED(fp->fun))
                nslots += fp->fun->u.n.extra;
        }
        GC_MARK_JSVALS(cx, nslots, fp->argv, "arg");
    }
    if (JSVAL_IS_GCTHING(fp->rval))
        GC_MARK(cx, JSVAL_TO_GCTHING(fp->rval), "rval");
    if (fp->vars)
        GC_MARK_JSVALS(cx, fp->nvars, fp->vars, "var");
    GC_MARK(cx, fp->scopeChain, "scope chain");
    if (fp->sharpArray)
        GC_MARK(cx, fp->sharpArray, "sharp array");

    if (fp->xmlNamespace)
        GC_MARK(cx, fp->xmlNamespace, "xmlNamespace");
}

static void
MarkWeakRoots(JSContext *cx, JSWeakRoots *wr)
{
    uintN i;
    void *thing;

    for (i = 0; i < GCX_NTYPES; i++)
        GC_MARK(cx, wr->newborn[i], gc_typenames[i]);
    if (wr->lastAtom)
        GC_MARK_ATOM(cx, wr->lastAtom);
    if (JSVAL_IS_GCTHING(wr->lastInternalResult)) {
        thing = JSVAL_TO_GCTHING(wr->lastInternalResult);
        if (thing)
            GC_MARK(cx, thing, "lastInternalResult");
    }
}





void
js_GC(JSContext *cx, JSGCInvocationKind gckind)
{
    JSRuntime *rt;
    JSBool keepAtoms;
    uintN i, type;
    JSContext *iter, *acx;
#if JS_HAS_GENERATORS
    JSGenerator **genTodoTail;
#endif
    JSStackFrame *fp, *chain;
    JSStackHeader *sh;
    JSTempValueRooter *tvr;
    size_t nbytes, limit, offset;
    JSGCArena *a, **ap;
    uint8 flags, *flagp, *firstPage;
    JSGCThing *thing, *freeList;
    JSGCArenaList *arenaList;
    GCFinalizeOp finalizer;
    JSBool allClear;
#ifdef JS_THREADSAFE
    uint32 requestDebit;
#endif

    rt = cx->runtime;
#ifdef JS_THREADSAFE
    
    JS_ASSERT(!JS_IS_RUNTIME_LOCKED(rt));
#endif

    if (gckind == GC_LAST_DITCH) {
        
        keepAtoms = JS_TRUE;
    } else {
        JS_CLEAR_WEAK_ROOTS(&cx->weakRoots);
        rt->gcPoke = JS_TRUE;

        
        keepAtoms = (rt->gcKeepAtoms != 0);
    }

    





    if (rt->state != JSRTS_UP && gckind != GC_LAST_CONTEXT)
        return;

  restart_after_callback:
    



    if (rt->gcCallback &&
        !rt->gcCallback(cx, JSGC_BEGIN) &&
        gckind != GC_LAST_CONTEXT) {
        return;
    }

    
    if (gckind != GC_LAST_DITCH)
        JS_LOCK_GC(rt);

    
    if (!rt->gcPoke) {
        METER(rt->gcStats.nopoke++);
        if (gckind != GC_LAST_DITCH)
            JS_UNLOCK_GC(rt);
        return;
    }
    METER(rt->gcStats.poke++);
    rt->gcPoke = JS_FALSE;

#ifdef JS_THREADSAFE
    JS_ASSERT(cx->thread->id == js_CurrentThreadId());

    
    if (rt->gcThread == cx->thread) {
        JS_ASSERT(rt->gcLevel > 0);
        rt->gcLevel++;
        METER(if (rt->gcLevel > rt->gcStats.maxlevel)
                  rt->gcStats.maxlevel = rt->gcLevel);
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
        METER(if (rt->gcLevel > rt->gcStats.maxlevel)
                  rt->gcStats.maxlevel = rt->gcLevel);

        
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
    METER(if (rt->gcLevel > rt->gcStats.maxlevel)
              rt->gcStats.maxlevel = rt->gcLevel);
    if (rt->gcLevel > 1)
        return;

#endif 

    







    rt->gcRunning = JS_TRUE;
    JS_UNLOCK_GC(rt);

    
    rt->gcMallocBytes = 0;

#ifdef DEBUG_scopemeters
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
    JS_ASSERT(!rt->gcUnscannedArenaStackTop);
    JS_ASSERT(rt->gcUnscannedBagSize == 0);

    


    JS_DHashTableEnumerate(&rt->gcRootsHash, gc_root_marker, cx);
    if (rt->gcLocksHash)
        JS_DHashTableEnumerate(rt->gcLocksHash, gc_lock_marker, cx);
    js_MarkAtomState(&rt->atomState, keepAtoms, gc_mark_atom_key_thing, cx);
    js_MarkWatchPoints(cx);
    js_MarkScriptFilenames(rt, keepAtoms);
    js_MarkNativeIteratorStates(cx);

#if JS_HAS_GENERATORS
    genTodoTail = MarkScheduledGenerators(cx);
    JS_ASSERT(!*genTodoTail);
#endif

    iter = NULL;
    while ((acx = js_ContextIterator(rt, JS_TRUE, &iter)) != NULL) {
        





        chain = acx->fp;
        if (chain) {
            JS_ASSERT(!chain->dormantNext);
            chain->dormantNext = acx->dormantFrameChain;
        } else {
            chain = acx->dormantFrameChain;
        }

        for (fp = chain; fp; fp = chain = chain->dormantNext) {
            do {
                js_MarkStackFrame(cx, fp);
            } while ((fp = fp->down) != NULL);
        }

        
        if (acx->fp)
            acx->fp->dormantNext = NULL;

        
        GC_MARK(cx, acx->globalObject, "global object");
        MarkWeakRoots(cx, &acx->weakRoots);
        if (acx->throwing) {
            if (JSVAL_IS_GCTHING(acx->exception))
                GC_MARK(cx, JSVAL_TO_GCTHING(acx->exception), "exception");
        } else {
            
            acx->exception = JSVAL_NULL;
        }
#if JS_HAS_LVALUE_RETURN
        if (acx->rval2set && JSVAL_IS_GCTHING(acx->rval2))
            GC_MARK(cx, JSVAL_TO_GCTHING(acx->rval2), "rval2");
#endif

        for (sh = acx->stackHeaders; sh; sh = sh->down) {
            METER(rt->gcStats.stackseg++);
            METER(rt->gcStats.segslots += sh->nslots);
            GC_MARK_JSVALS(cx, sh->nslots, JS_STACK_SEGMENT(sh), "stack");
        }

        if (acx->localRootStack)
            js_MarkLocalRoots(cx, acx->localRootStack);

        for (tvr = acx->tempValueRooters; tvr; tvr = tvr->down) {
            switch (tvr->count) {
              case JSTVU_SINGLE:
                if (JSVAL_IS_GCTHING(tvr->u.value)) {
                    GC_MARK(cx, JSVAL_TO_GCTHING(tvr->u.value),
                            "tvr->u.value");
                }
                break;
              case JSTVU_MARKER:
                tvr->u.marker(cx, tvr);
                break;
              case JSTVU_SPROP:
                MARK_SCOPE_PROPERTY(cx, tvr->u.sprop);
                break;
              case JSTVU_WEAK_ROOTS:
                MarkWeakRoots(cx, tvr->u.weakRoots);
                break;
              default:
                JS_ASSERT(tvr->count >= 0);
                GC_MARK_JSVALS(cx, tvr->count, tvr->u.array, "tvr->u.array");
            }
        }

        if (acx->sharpObjectMap.depth > 0)
            js_GCMarkSharpMap(cx, &acx->sharpObjectMap);
    }

    



    ScanDelayedChildren(cx);

#if JS_HAS_GENERATORS
    



    FindAndMarkObjectsToClose(cx, gckind, genTodoTail);

    



    ScanDelayedChildren(cx);
#endif

    JS_ASSERT(!cx->insideGCMarkCallback);
    if (rt->gcCallback) {
        cx->insideGCMarkCallback = JS_TRUE;
        (void) rt->gcCallback(cx, JSGC_MARK_END);
        JS_ASSERT(cx->insideGCMarkCallback);
        cx->insideGCMarkCallback = JS_FALSE;
    }
    JS_ASSERT(rt->gcUnscannedBagSize == 0);

    
    CloseIteratorStates(cx);

#ifdef DUMP_CALL_TABLE
    



    js_DumpCallTable(cx);
#endif

    













    for (i = 0; i < GC_NUM_FREELISTS; i++) {
        arenaList = &rt->gcArenaList[i == 0
                                     ? GC_FREELIST_INDEX(sizeof(JSObject))
                                     : i == GC_FREELIST_INDEX(sizeof(JSObject))
                                     ? 0
                                     : i];
        nbytes = arenaList->thingSize;
        limit = arenaList->lastLimit;
        for (a = arenaList->last; a; a = a->prev) {
            JS_ASSERT(!a->prevUnscanned);
            JS_ASSERT(a->unscannedPages == 0);
            firstPage = (uint8 *) FIRST_THING_PAGE(a);
            for (offset = 0; offset != limit; offset += nbytes) {
                if ((offset & GC_PAGE_MASK) == 0) {
                    JS_ASSERT(((JSGCPageInfo *)(firstPage + offset))->
                              unscannedBitmap == 0);
                    offset += PAGE_THING_GAP(nbytes);
                }
                JS_ASSERT(offset < limit);
                flagp = a->base + offset / sizeof(JSGCThing);
                if (flagp >= firstPage)
                    flagp += GC_THINGS_SIZE;
                flags = *flagp;
                if (flags & GCF_MARK) {
                    *flagp &= ~GCF_MARK;
                } else if (!(flags & (GCF_LOCK | GCF_FINAL))) {
                    
                    type = flags & GCF_TYPEMASK;
                    finalizer = gc_finalizers[type];
                    if (finalizer) {
                        thing = (JSGCThing *)(firstPage + offset);
                        *flagp = (uint8)(flags | GCF_FINAL);
                        if (type >= GCX_EXTERNAL_STRING)
                            js_PurgeDeflatedStringCache(rt, (JSString *)thing);
                        finalizer(cx, thing);
                    }

                    
                    *flagp = GCF_FINAL;
                }
            }
            limit = GC_THINGS_SIZE;
        }
    }

    




    js_SweepScopeProperties(cx);
    js_SweepAtomState(&rt->atomState);

    





    js_SweepScriptFilenames(rt);

    



    for (i = 0; i < GC_NUM_FREELISTS; i++) {
        arenaList = &rt->gcArenaList[i];
        ap = &arenaList->last;
        a = *ap;
        if (!a)
            continue;

        allClear = JS_TRUE;
        arenaList->freeList = NULL;
        freeList = NULL;
        METER(arenaList->stats.nthings = 0);
        METER(arenaList->stats.freelen = 0);

        nbytes = GC_FREELIST_NBYTES(i);
        limit = arenaList->lastLimit;
        do {
            METER(size_t nfree = 0);
            firstPage = (uint8 *) FIRST_THING_PAGE(a);
            for (offset = 0; offset != limit; offset += nbytes) {
                if ((offset & GC_PAGE_MASK) == 0)
                    offset += PAGE_THING_GAP(nbytes);
                JS_ASSERT(offset < limit);
                flagp = a->base + offset / sizeof(JSGCThing);
                if (flagp >= firstPage)
                    flagp += GC_THINGS_SIZE;

                if (*flagp != GCF_FINAL) {
                    allClear = JS_FALSE;
                    METER(++arenaList->stats.nthings);
                } else {
                    thing = (JSGCThing *)(firstPage + offset);
                    thing->flagp = flagp;
                    thing->next = freeList;
                    freeList = thing;
                    METER(++nfree);
                }
            }
            if (allClear) {
                



                freeList = arenaList->freeList;
                DestroyGCArena(rt, arenaList, ap);
            } else {
                allClear = JS_TRUE;
                arenaList->freeList = freeList;
                ap = &a->prev;
                METER(arenaList->stats.freelen += nfree);
                METER(arenaList->stats.totalfreelen += nfree);
                METER(++arenaList->stats.totalarenas);
            }
            limit = GC_THINGS_SIZE;
        } while ((a = *ap) != NULL);
    }

    if (rt->gcCallback)
        (void) rt->gcCallback(cx, JSGC_FINALIZE_END);
#ifdef DEBUG_srcnotesize
  { extern void DumpSrcNoteSizeHist();
    DumpSrcNoteSizeHist();
    printf("GC HEAP SIZE %lu\n", (unsigned long)rt->gcBytes);
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

    
    if (rt->gcCallback) {
        JSWeakRoots savedWeakRoots;
        JSTempValueRooter tvr;

        if (gckind == GC_LAST_DITCH) {
            




            savedWeakRoots = cx->weakRoots;
            JS_PUSH_TEMP_ROOT_WEAK_COPY(cx, &savedWeakRoots, &tvr);
            JS_KEEP_ATOMS(rt);
            JS_UNLOCK_GC(rt);
        }

        (void) rt->gcCallback(cx, JSGC_END);

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
