










































#include <stdlib.h>
#include <string.h>
#include "plarena.h"
#include "prmem.h"
#include "prbit.h"
#include "prlog.h"
#include "prlock.h"
#include "prinit.h"

static PLArena *arena_freelist;

#ifdef PL_ARENAMETER
static PLArenaStats *arena_stats_list;

#define COUNT(pool,what)  (pool)->stats.what++
#else
#define COUNT(pool,what)
#endif

#define PL_ARENA_DEFAULT_ALIGN  sizeof(double)

static PRLock    *arenaLock;
static PRCallOnceType once;
static const PRCallOnceType pristineCallOnce;















static PRStatus InitializeArenas( void )
{
    PR_ASSERT( arenaLock == NULL );
    arenaLock = PR_NewLock();
    if ( arenaLock == NULL )
        return PR_FAILURE;
    else
        return PR_SUCCESS;
} 

static PRStatus LockArena( void )
{
    PRStatus rc = PR_CallOnce( &once, InitializeArenas );

    if ( PR_FAILURE != rc )
        PR_Lock( arenaLock );
    return(rc);
} 

static void UnlockArena( void )
{
    PR_Unlock( arenaLock );
    return;
} 

PR_IMPLEMENT(void) PL_InitArenaPool(
    PLArenaPool *pool, const char *name, PRUint32 size, PRUint32 align)
{
    



    static const PRUint8 pmasks[33] = {
         0,                                               
         0, 1, 3, 3, 7, 7, 7, 7,15,15,15,15,15,15,15,15,  
        31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31}; 

    if (align == 0)
        align = PL_ARENA_DEFAULT_ALIGN;

    if (align < sizeof(pmasks)/sizeof(pmasks[0]))
        pool->mask = pmasks[align];
    else
        pool->mask = PR_BITMASK(PR_CeilingLog2(align));

    pool->first.next = NULL;
    pool->first.base = pool->first.avail = pool->first.limit =
        (PRUword)PL_ARENA_ALIGN(pool, &pool->first + 1);
    pool->current = &pool->first;
    pool->arenasize = size;                                  
#ifdef PL_ARENAMETER
    memset(&pool->stats, 0, sizeof pool->stats);
    pool->stats.name = strdup(name);
    pool->stats.next = arena_stats_list;
    arena_stats_list = &pool->stats;
#endif
}




























PR_IMPLEMENT(void *) PL_ArenaAllocate(PLArenaPool *pool, PRUint32 nb)
{
    PLArena *a;   
    char *rp;     

    PR_ASSERT((nb & pool->mask) == 0);
    
    nb = (PRUword)PL_ARENA_ALIGN(pool, nb); 

    
    {
        a = pool->current;
        do {
            if ( a->avail +nb <= a->limit )  {
                pool->current = a;
                rp = (char *)a->avail;
                a->avail += nb;
                return rp;
            }
        } while( NULL != (a = a->next) );
    }

    
    {
        PLArena *p; 

        
        if ( PR_FAILURE == LockArena())
            return(0);

        for ( a = arena_freelist, p = NULL; a != NULL ; p = a, a = a->next ) {
            if ( a->base +nb <= a->limit )  {
                if ( p == NULL )
                    arena_freelist = a->next;
                else
                    p->next = a->next;
                UnlockArena();
                a->avail = a->base;
                rp = (char *)a->avail;
                a->avail += nb;
                

                a->next = pool->current->next;
                pool->current->next = a;
                pool->current = a;
                if ( NULL == pool->first.next )
                    pool->first.next = a;
                return(rp);
            }
        }
        UnlockArena();
    }

     
    {  
        PRUint32 sz = PR_MAX(pool->arenasize, nb);
        sz += sizeof *a + pool->mask;  
        a = (PLArena*)PR_MALLOC(sz);
        if ( NULL != a )  {
            a->limit = (PRUword)a + sz;
            a->base = a->avail = (PRUword)PL_ARENA_ALIGN(pool, a + 1);
            rp = (char *)a->avail;
            a->avail += nb;
            

            a->next = pool->current->next;
            pool->current->next = a;
            pool->current = a;
            if ( NULL == pool->first.next )
                pool->first.next = a;
            PL_COUNT_ARENA(pool,++);
            COUNT(pool, nmallocs);
            return(rp);
        }
    }

    
    return(NULL);
} 

PR_IMPLEMENT(void *) PL_ArenaGrow(
    PLArenaPool *pool, void *p, PRUint32 size, PRUint32 incr)
{
    void *newp;

    PL_ARENA_ALLOCATE(newp, pool, size + incr);
    if (newp)
        memcpy(newp, p, size);
    return newp;
}





static void FreeArenaList(PLArenaPool *pool, PLArena *head, PRBool reallyFree)
{
    PLArena **ap, *a;

    ap = &head->next;
    a = *ap;
    if (!a)
        return;

#ifdef DEBUG
    do {
        PR_ASSERT(a->base <= a->avail && a->avail <= a->limit);
        a->avail = a->base;
        PL_CLEAR_UNUSED(a);
    } while ((a = a->next) != 0);
    a = *ap;
#endif

    if (reallyFree) {
        do {
            *ap = a->next;
            PL_CLEAR_ARENA(a);
            PL_COUNT_ARENA(pool,--);
            PR_DELETE(a);
        } while ((a = *ap) != 0);
    } else {
        
        do {
            ap = &(*ap)->next;
        } while (*ap);
        LockArena();
        *ap = arena_freelist;
        arena_freelist = a;
        head->next = 0;
        UnlockArena();
    }

    pool->current = head;
}

PR_IMPLEMENT(void) PL_ArenaRelease(PLArenaPool *pool, char *mark)
{
    PLArena *a;

    for (a = pool->first.next; a; a = a->next) {
        if (PR_UPTRDIFF(mark, a->base) < PR_UPTRDIFF(a->avail, a->base)) {
            a->avail = (PRUword)PL_ARENA_ALIGN(pool, mark);
            FreeArenaList(pool, a, PR_FALSE);
            return;
        }
    }
}

PR_IMPLEMENT(void) PL_FreeArenaPool(PLArenaPool *pool)
{
    FreeArenaList(pool, &pool->first, PR_FALSE);
    COUNT(pool, ndeallocs);
}

PR_IMPLEMENT(void) PL_FinishArenaPool(PLArenaPool *pool)
{
    FreeArenaList(pool, &pool->first, PR_TRUE);
#ifdef PL_ARENAMETER
    {
        PLArenaStats *stats, **statsp;

        if (pool->stats.name)
            PR_DELETE(pool->stats.name);
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

PR_IMPLEMENT(void) PL_CompactArenaPool(PLArenaPool *ap)
{
}

PR_IMPLEMENT(void) PL_ArenaFinish(void)
{
    PLArena *a, *next;

    for (a = arena_freelist; a; a = next) {
        next = a->next;
        PR_DELETE(a);
    }
    arena_freelist = NULL;

    if (arenaLock) {
        PR_DestroyLock(arenaLock);
        arenaLock = NULL;
    }
    once = pristineCallOnce;
}

#ifdef PL_ARENAMETER
PR_IMPLEMENT(void) PL_ArenaCountAllocation(PLArenaPool *pool, PRUint32 nb)
{
    pool->stats.nallocs++;
    pool->stats.nbytes += nb;
    if (nb > pool->stats.maxalloc)
        pool->stats.maxalloc = nb;
    pool->stats.variance += nb * nb;
}

PR_IMPLEMENT(void) PL_ArenaCountInplaceGrowth(
    PLArenaPool *pool, PRUint32 size, PRUint32 incr)
{
    pool->stats.ninplace++;
}

PR_IMPLEMENT(void) PL_ArenaCountGrowth(
    PLArenaPool *pool, PRUint32 size, PRUint32 incr)
{
    pool->stats.ngrows++;
    pool->stats.nbytes += incr;
    pool->stats.variance -= size * size;
    size += incr;
    if (size > pool->stats.maxalloc)
        pool->stats.maxalloc = size;
    pool->stats.variance += size * size;
}

PR_IMPLEMENT(void) PL_ArenaCountRelease(PLArenaPool *pool, char *mark)
{
    pool->stats.nreleases++;
}

PR_IMPLEMENT(void) PL_ArenaCountRetract(PLArenaPool *pool, char *mark)
{
    pool->stats.nfastrels++;
}

#include <math.h>
#include <stdio.h>

PR_IMPLEMENT(void) PL_DumpArenaStats(FILE *fp)
{
    PLArenaStats *stats;
    double mean, variance;

    for (stats = arena_stats_list; stats; stats = stats->next) {
        if (stats->nallocs != 0) {
            mean = (double)stats->nbytes / stats->nallocs;
            variance = fabs(stats->variance / stats->nallocs - mean * mean);
        } else {
            mean = variance = 0;
        }

        fprintf(fp, "\n%s allocation statistics:\n", stats->name);
        fprintf(fp, "              number of arenas: %u\n", stats->narenas);
        fprintf(fp, "         number of allocations: %u\n", stats->nallocs);
        fprintf(fp, " number of free arena reclaims: %u\n", stats->nreclaims);
        fprintf(fp, "        number of malloc calls: %u\n", stats->nmallocs);
        fprintf(fp, "       number of deallocations: %u\n", stats->ndeallocs);
        fprintf(fp, "  number of allocation growths: %u\n", stats->ngrows);
        fprintf(fp, "    number of in-place growths: %u\n", stats->ninplace);
        fprintf(fp, "number of released allocations: %u\n", stats->nreleases);
        fprintf(fp, "       number of fast releases: %u\n", stats->nfastrels);
        fprintf(fp, "         total bytes allocated: %u\n", stats->nbytes);
        fprintf(fp, "          mean allocation size: %g\n", mean);
        fprintf(fp, "            standard deviation: %g\n", sqrt(variance));
        fprintf(fp, "       maximum allocation size: %u\n", stats->maxalloc);
    }
}
#endif 
