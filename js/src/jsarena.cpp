











































#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsbit.h"
#include "jsarena.h" 
#include "jsutil.h" 

#ifdef JS_ARENAMETER
static JSArenaStats *arena_stats_list;

#define COUNT(pool,what)  (pool)->stats.what++
#else
#define COUNT(pool,what)
#endif

#define JS_ARENA_DEFAULT_ALIGN  sizeof(double)

JS_PUBLIC_API(void)
JS_InitArenaPool(JSArenaPool *pool, const char *name, size_t size,
                 size_t align, size_t *quotap)
{
    if (align == 0)
        align = JS_ARENA_DEFAULT_ALIGN;
    pool->mask = JS_BITMASK(JS_CeilingLog2(align));
    pool->first.next = NULL;
    pool->first.base = pool->first.avail = pool->first.limit =
        JS_ARENA_ALIGN(pool, &pool->first + 1);
    pool->current = &pool->first;
    pool->arenasize = size;
    pool->quotap = quotap;
#ifdef JS_ARENAMETER
    memset(&pool->stats, 0, sizeof pool->stats);
    pool->stats.name = strdup(name);
    pool->stats.next = arena_stats_list;
    arena_stats_list = &pool->stats;
#endif
}



































#define POINTER_MASK            ((jsuword)(JS_ALIGN_OF_POINTER - 1))
#define HEADER_SIZE(pool)       (sizeof(JSArena **)                           \
                                 + (((pool)->mask < POINTER_MASK)             \
                                    ? POINTER_MASK - (pool)->mask             \
                                    : 0))
#define HEADER_BASE_MASK(pool)  ((pool)->mask | POINTER_MASK)
#define PTR_TO_HEADER(pool,p)   (JS_ASSERT(((jsuword)(p)                      \
                                            & HEADER_BASE_MASK(pool))         \
                                           == 0),                             \
                                 (JSArena ***)(p) - 1)
#define GET_HEADER(pool,a)      (*PTR_TO_HEADER(pool, (a)->base))
#define SET_HEADER(pool,a,ap)   (*PTR_TO_HEADER(pool, (a)->base) = (ap))

JS_PUBLIC_API(void *)
JS_ArenaAllocate(JSArenaPool *pool, size_t nb)
{
    JSArena **ap, *a, *b;
    jsuword extra, hdrsz, gross;
    void *p;

    










    JS_ASSERT((nb & pool->mask) == 0);
    for (a = pool->current; nb > a->limit || a->avail > a->limit - nb;
         pool->current = a) {
        ap = &a->next;
        if (!*ap) {
            
            extra = (nb > pool->arenasize) ? HEADER_SIZE(pool) : 0;
            hdrsz = sizeof *a + extra + pool->mask;
            gross = hdrsz + JS_MAX(nb, pool->arenasize);
            if (gross < nb)
                return NULL;
            if (pool->quotap) {
                if (gross > *pool->quotap)
                    return NULL;
                b = (JSArena *) js_malloc(gross);
                if (!b)
                    return NULL;
                *pool->quotap -= gross;
            } else {
                b = (JSArena *) js_malloc(gross);
                if (!b)
                    return NULL;
            }

            b->next = NULL;
            b->limit = (jsuword)b + gross;
            JS_COUNT_ARENA(pool,++);
            COUNT(pool, nmallocs);

            
            *ap = a = b;
            JS_ASSERT(gross <= JS_UPTRDIFF(a->limit, a));
            if (extra) {
                a->base = a->avail =
                    ((jsuword)a + hdrsz) & ~HEADER_BASE_MASK(pool);
                SET_HEADER(pool, a, ap);
            } else {
                a->base = a->avail = JS_ARENA_ALIGN(pool, a + 1);
            }
            continue;
        }
        a = *ap;                                
    }

    p = (void *)a->avail;
    a->avail += nb;
    JS_ASSERT(a->base <= a->avail && a->avail <= a->limit);
    return p;
}

JS_PUBLIC_API(void *)
JS_ArenaRealloc(JSArenaPool *pool, void *p, size_t size, size_t incr)
{
    JSArena **ap, *a, *b;
    jsuword boff, aoff, extra, hdrsz, gross, growth;

    



    if (size > pool->arenasize) {
        ap = *PTR_TO_HEADER(pool, p);
        a = *ap;
    } else {
        ap = &pool->first.next;
        while ((a = *ap) != pool->current)
            ap = &a->next;
    }

    JS_ASSERT(a->base == (jsuword)p);
    boff = JS_UPTRDIFF(a->base, a);
    aoff = JS_ARENA_ALIGN(pool, size + incr);
    JS_ASSERT(aoff > pool->arenasize);
    extra = HEADER_SIZE(pool);                  
    hdrsz = sizeof *a + extra + pool->mask;     
    gross = hdrsz + aoff;
    JS_ASSERT(gross > aoff);
    if (pool->quotap) {
        growth = gross - (a->limit - (jsuword) a);
        if (growth > *pool->quotap)
            return NULL;
        a = (JSArena *) js_realloc(a, gross);
        if (!a)
            return NULL;
        *pool->quotap -= growth;
    } else {
        a = (JSArena *) js_realloc(a, gross);
        if (!a)
            return NULL;
    }
#ifdef JS_ARENAMETER
    pool->stats.nreallocs++;
#endif

    if (a != *ap) {
        
        if (pool->current == *ap)
            pool->current = a;
        b = a->next;
        if (b && b->avail - b->base > pool->arenasize) {
            JS_ASSERT(GET_HEADER(pool, b) == &(*ap)->next);
            SET_HEADER(pool, b, &a->next);
        }

        
        *ap = a;
    }

    a->base = ((jsuword)a + hdrsz) & ~HEADER_BASE_MASK(pool);
    a->limit = (jsuword)a + gross;
    a->avail = a->base + aoff;
    JS_ASSERT(a->base <= a->avail && a->avail <= a->limit);

    
    if (boff != JS_UPTRDIFF(a->base, a))
        memmove((void *)a->base, (char *)a + boff, size);

    
    SET_HEADER(pool, a, ap);
    return (void *)a->base;
}

JS_PUBLIC_API(void *)
JS_ArenaGrow(JSArenaPool *pool, void *p, size_t size, size_t incr)
{
    void *newp;

    



    if (size > pool->arenasize)
        return JS_ArenaRealloc(pool, p, size, incr);

    JS_ARENA_ALLOCATE(newp, pool, size + incr);
    if (newp)
        memcpy(newp, p, size);
    return newp;
}





static void
FreeArenaList(JSArenaPool *pool, JSArena *head)
{
    JSArena **ap, *a;

    ap = &head->next;
    a = *ap;
    if (!a)
        return;

#ifdef DEBUG
    do {
        JS_ASSERT(a->base <= a->avail && a->avail <= a->limit);
        a->avail = a->base;
        JS_CLEAR_UNUSED(a);
    } while ((a = a->next) != NULL);
    a = *ap;
#endif

    do {
        *ap = a->next;
        if (pool->quotap)
            *pool->quotap += a->limit - (jsuword) a;
        JS_CLEAR_ARENA(a);
        JS_COUNT_ARENA(pool,--);
        js_free(a);
    } while ((a = *ap) != NULL);

    pool->current = head;
}

JS_PUBLIC_API(void)
JS_ArenaRelease(JSArenaPool *pool, char *mark)
{
    JSArena *a;

    for (a = &pool->first; a; a = a->next) {
        JS_ASSERT(a->base <= a->avail && a->avail <= a->limit);

        if (JS_ARENA_MARK_MATCH(a, mark)) {
            a->avail = JS_ARENA_ALIGN(pool, mark);
            JS_ASSERT(a->avail <= a->limit);
            FreeArenaList(pool, a);
            return;
        }
    }
}

JS_PUBLIC_API(void)
JS_FreeArenaPool(JSArenaPool *pool)
{
    FreeArenaList(pool, &pool->first);
    COUNT(pool, ndeallocs);
}

JS_PUBLIC_API(void)
JS_FinishArenaPool(JSArenaPool *pool)
{
    FreeArenaList(pool, &pool->first);
#ifdef JS_ARENAMETER
    {
        JSArenaStats *stats, **statsp;

        if (pool->stats.name) {
            js_free(pool->stats.name);
            pool->stats.name = NULL;
        }
        for (statsp = &arena_stats_list; (stats = *statsp) != 0;
             statsp = &stats->next) {
            if (stats == &pool->stats) {
                *statsp = stats->next;
                return;
            }
        }
    }
#endif
}

JS_PUBLIC_API(void)
JS_ArenaFinish()
{
}

JS_PUBLIC_API(void)
JS_ArenaShutDown(void)
{
}

#ifdef JS_ARENAMETER
JS_PUBLIC_API(void)
JS_ArenaCountAllocation(JSArenaPool *pool, size_t nb)
{
    pool->stats.nallocs++;
    pool->stats.nbytes += nb;
    if (nb > pool->stats.maxalloc)
        pool->stats.maxalloc = nb;
    pool->stats.variance += nb * nb;
}

JS_PUBLIC_API(void)
JS_ArenaCountInplaceGrowth(JSArenaPool *pool, size_t size, size_t incr)
{
    pool->stats.ninplace++;
}

JS_PUBLIC_API(void)
JS_ArenaCountGrowth(JSArenaPool *pool, size_t size, size_t incr)
{
    pool->stats.ngrows++;
    pool->stats.nbytes += incr;
    pool->stats.variance -= size * size;
    size += incr;
    if (size > pool->stats.maxalloc)
        pool->stats.maxalloc = size;
    pool->stats.variance += size * size;
}

JS_PUBLIC_API(void)
JS_ArenaCountRelease(JSArenaPool *pool, char *mark)
{
    pool->stats.nreleases++;
}

JS_PUBLIC_API(void)
JS_ArenaCountRetract(JSArenaPool *pool, char *mark)
{
    pool->stats.nfastrels++;
}

#include <stdio.h>

JS_PUBLIC_API(void)
JS_DumpArenaStats(FILE *fp)
{
    JSArenaStats *stats;
    double mean, sigma;

    for (stats = arena_stats_list; stats; stats = stats->next) {
        mean = JS_MeanAndStdDev(stats->nallocs, stats->nbytes, stats->variance,
                                &sigma);

        fprintf(fp, "\n%s allocation statistics:\n", stats->name);
        fprintf(fp, "              number of arenas: %u\n", stats->narenas);
        fprintf(fp, "         number of allocations: %u\n", stats->nallocs);
        fprintf(fp, "        number of malloc calls: %u\n", stats->nmallocs);
        fprintf(fp, "       number of deallocations: %u\n", stats->ndeallocs);
        fprintf(fp, "  number of allocation growths: %u\n", stats->ngrows);
        fprintf(fp, "    number of in-place growths: %u\n", stats->ninplace);
        fprintf(fp, " number of realloc'ing growths: %u\n", stats->nreallocs);
        fprintf(fp, "number of released allocations: %u\n", stats->nreleases);
        fprintf(fp, "       number of fast releases: %u\n", stats->nfastrels);
        fprintf(fp, "         total bytes allocated: %u\n", stats->nbytes);
        fprintf(fp, "          mean allocation size: %g\n", mean);
        fprintf(fp, "            standard deviation: %g\n", sigma);
        fprintf(fp, "       maximum allocation size: %u\n", stats->maxalloc);
    }
}
#endif 
