











































#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsbit.h"
#include "jsarena.h" 
#include "jsutil.h" 

#ifdef DEBUG
static JSArenaStats *arena_stats_list;
#define COUNT(pool,what)  (pool)->stats.what++
#else
#define COUNT(pool,what)
#endif 

#define JS_ARENA_DEFAULT_ALIGN  sizeof(double)

void
JSArenaPool::init(const char *name, size_t size, size_t align, size_t *quotap)
{
    if (align == 0)
        align = JS_ARENA_DEFAULT_ALIGN;
    mask = JS_BITMASK(JS_CeilingLog2(align));
    first.next = NULL;
    first.base = first.avail = first.limit = this->align(jsuword(&first + 1));
    current = &first;
    arenasize = size;
    this->quotap = quotap;
#ifdef DEBUG
    stats.init(name, arena_stats_list);
    arena_stats_list = &stats;
#endif
}




























void *
JSArenaPool::allocate(size_t nb, bool limitCheck)
{
    countAllocation(nb);
    size_t alignedNB = align(nb);
    jsuword p = current->avail;
    



    if ((limitCheck && alignedNB > current->limit) || p > current->limit - alignedNB)
        p = jsuword(allocateInternal(alignedNB));
    else
        current->avail = p + alignedNB;
    return (void *) p;
}

void *
JSArenaPool::allocateInternal(size_t nb)
{
    










    JS_ASSERT((nb & mask) == 0);
    JSArena *a;
    for (a = current; nb > a->limit || a->avail > a->limit - nb; current = a) {
        JSArena **ap = &a->next;
        if (!*ap) {
            
            jsuword extra = (nb > arenasize) ? headerSize() : 0;
            jsuword hdrsz = sizeof *a + extra + mask;
            jsuword gross = hdrsz + JS_MAX(nb, arenasize);
            if (gross < nb)
                return NULL;
            JSArena *b;
            if (quotap) {
                if (gross > *quotap)
                    return NULL;
                b = (JSArena *) js_malloc(gross);
                if (!b)
                    return NULL;
                *quotap -= gross;
            } else {
                b = (JSArena *) js_malloc(gross);
                if (!b)
                    return NULL;
            }

            b->next = NULL;
            b->limit = (jsuword)b + gross;
            incArenaCount();
            COUNT(this, nmallocs);

            
            *ap = a = b;
            JS_ASSERT(gross <= JS_UPTRDIFF(a->limit, a));
            if (extra) {
                a->base = a->avail =
                    ((jsuword)a + hdrsz) & ~headerBaseMask();
                setHeader(a, ap);
            } else {
                a->base = a->avail = align(jsuword(a + 1));
            }
            continue;
        }
        a = *ap;                                
    }

    void *p = (void *) a->avail;
    a->avail += nb;
    JS_ASSERT(a->base <= a->avail && a->avail <= a->limit);
    return p;
}

void *
JSArenaPool::reallocInternal(void *p, size_t size, size_t incr)
{
    JSArena **ap, *a, *b;
    jsuword boff, aoff, extra, hdrsz, gross, growth;

    



    if (size > arenasize) {
        ap = *ptrToHeader(p);
        a = *ap;
    } else {
        ap = &first.next;
        while ((a = *ap) != current)
            ap = &a->next;
    }

    JS_ASSERT(a->base == (jsuword)p);
    boff = JS_UPTRDIFF(a->base, a);
    aoff = align(size + incr);
    JS_ASSERT(aoff > arenasize);
    extra = headerSize();                  
    hdrsz = sizeof *a + extra + mask;     
    gross = hdrsz + aoff;
    JS_ASSERT(gross > aoff);
    if (quotap) {
        growth = gross - (a->limit - (jsuword) a);
        if (growth > *quotap)
            return NULL;
        a = (JSArena *) js_realloc(a, gross);
        if (!a)
            return NULL;
        *quotap -= growth;
    } else {
        a = (JSArena *) js_realloc(a, gross);
        if (!a)
            return NULL;
    }
    incReallocCount();

    if (a != *ap) {
        
        if (current == *ap)
            current = a;
        b = a->next;
        if (b && b->avail - b->base > arenasize) {
            JS_ASSERT(getHeader(b) == &(*ap)->next);
            setHeader(b, &a->next);
        }

        
        *ap = a;
    }

    a->base = ((jsuword)a + hdrsz) & ~headerBaseMask();
    a->limit = (jsuword)a + gross;
    a->avail = a->base + aoff;
    JS_ASSERT(a->base <= a->avail && a->avail <= a->limit);

    
    if (boff != JS_UPTRDIFF(a->base, a))
        memmove((void *) a->base, (char *) a + boff, size);

    
    setHeader(a, ap);
    return (void *) a->base;
}

void
JSArenaPool::finish()
{
    freeArenaList(&first);
#ifdef DEBUG
    {
        JSArenaStats *stats, **statsp;

        this->stats.finish();
        for (statsp = &arena_stats_list; (stats = *statsp) != 0;
             statsp = &stats->next) {
            if (stats == &this->stats) {
                *statsp = stats->next;
                return;
            }
        }
    }
#endif
}

#ifdef DEBUG
void
JSArenaPool::countAllocation(size_t nb)
{
    stats.nallocs++;
    stats.nbytes += nb;
    if (nb > stats.maxalloc)
        stats.maxalloc = nb;
    stats.variance += nb * nb;
}

void
JSArenaPool::countGrowth(size_t size, size_t incr)
{
    stats.ngrows++;
    stats.nbytes += incr;
    stats.variance -= size * size;
    size += incr;
    if (size > stats.maxalloc)
        stats.maxalloc = size;
    stats.variance += size * size;
}

JS_FRIEND_API(void)
JS_DumpArenaStats()
{
    const char *filename = getenv("JS_ARENA_STATFILE");
    if (!filename)
        return;
    FILE *arenaStatFile = strcmp(filename, "stdout")
                          ? stdout : strcmp(filename, "stderr")
                          ? stderr : fopen(filename, "w");
    for (const JSArenaStats *stats = arena_stats_list; stats; stats = stats->getNext())
        stats->dump(arenaStatFile);
    fclose(arenaStatFile);
}

void
JSArenaStats::dump(FILE *fp) const
{
    double sigma;
    double mean = JS_MeanAndStdDev(nallocs, nbytes, variance, &sigma);

    fprintf(fp, "\n%s allocation statistics:\n", name);
    fprintf(fp, "              number of arenas: %u\n", narenas);
    fprintf(fp, "         number of allocations: %u\n", nallocs);
    fprintf(fp, "        number of malloc calls: %u\n", nmallocs);
    fprintf(fp, "       number of deallocations: %u\n", ndeallocs);
    fprintf(fp, "  number of allocation growths: %u\n", ngrows);
    fprintf(fp, "    number of in-place growths: %u\n", ninplace);
    fprintf(fp, " number of realloc'ing growths: %u\n", nreallocs);
    fprintf(fp, "number of released allocations: %u\n", nreleases);
    fprintf(fp, "       number of fast releases: %u\n", nfastrels);
    fprintf(fp, "         total bytes allocated: %u\n", unsigned(nbytes));
    fprintf(fp, "          mean allocation size: %g\n", mean);
    fprintf(fp, "            standard deviation: %g\n", sigma);
    fprintf(fp, "       maximum allocation size: %u\n", unsigned(maxalloc));
}
#endif 



JS_FRIEND_API(void *)
JS_ARENA_MARK(const JSArenaPool *pool)
{
    return pool->getMark();
}

JS_FRIEND_API(void)
JS_ARENA_RELEASE(JSArenaPool *pool, void *mark)
{
    pool->release(mark);
}

JS_FRIEND_API(void *)
JS_ARENA_ALLOCATE_COMMON_SANE(jsuword p, JSArenaPool *pool, size_t nb, bool limitCheck)
{
    return pool->allocate(nb, limitCheck);
}
