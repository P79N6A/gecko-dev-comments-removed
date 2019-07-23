





































#ifndef plarena_h___
#define plarena_h___







#include "prtypes.h"
#include "plarenas.h"

PR_BEGIN_EXTERN_C

typedef struct PLArena          PLArena;

struct PLArena {
    PLArena     *next;          
    PRUword     base;           
    PRUword     limit;          
    PRUword     avail;          
};

#ifdef PL_ARENAMETER
typedef struct PLArenaStats PLArenaStats;

struct PLArenaStats {
    PLArenaStats  *next;        
    char          *name;        
    PRUint32      narenas;      
    PRUint32      nallocs;      
    PRUint32      nreclaims;    
    PRUint32      nmallocs;     
    PRUint32      ndeallocs;    
    PRUint32      ngrows;       
    PRUint32      ninplace;     
    PRUint32      nreleases;    
    PRUint32      nfastrels;    
    PRUint32      nbytes;       
    PRUint32      maxalloc;     
    PRFloat64     variance;     
};
#endif

struct PLArenaPool {
    PLArena     first;          
    PLArena     *current;       
    PRUint32    arenasize;      
    PRUword     mask;           
#ifdef PL_ARENAMETER
    PLArenaStats stats;
#endif
};






#ifdef PL_ARENA_CONST_ALIGN_MASK
#define PL_ARENA_ALIGN(pool, n) (((PRUword)(n) + PL_ARENA_CONST_ALIGN_MASK) \
                                & ~PL_ARENA_CONST_ALIGN_MASK)

#define PL_INIT_ARENA_POOL(pool, name, size) \
        PL_InitArenaPool(pool, name, size, PL_ARENA_CONST_ALIGN_MASK + 1)
#else
#define PL_ARENA_ALIGN(pool, n) (((PRUword)(n) + (pool)->mask) & ~(pool)->mask)
#endif

#define PL_ARENA_ALLOCATE(p, pool, nb) \
    PR_BEGIN_MACRO \
        PLArena *_a = (pool)->current; \
        PRUint32 _nb = PL_ARENA_ALIGN(pool, nb); \
        PRUword _p = _a->avail; \
        PRUword _q = _p + _nb; \
        if (_q > _a->limit) \
            _p = (PRUword)PL_ArenaAllocate(pool, _nb); \
        else \
            _a->avail = _q; \
        p = (void *)_p; \
        PL_ArenaCountAllocation(pool, nb); \
    PR_END_MACRO

#define PL_ARENA_GROW(p, pool, size, incr) \
    PR_BEGIN_MACRO \
        PLArena *_a = (pool)->current; \
        PRUint32 _incr = PL_ARENA_ALIGN(pool, incr); \
        PRUword _p = _a->avail; \
        PRUword _q = _p + _incr; \
        if (_p == (PRUword)(p) + PL_ARENA_ALIGN(pool, size) && \
            _q <= _a->limit) { \
            _a->avail = _q; \
            PL_ArenaCountInplaceGrowth(pool, size, incr); \
        } else { \
            p = PL_ArenaGrow(pool, p, size, incr); \
        } \
        PL_ArenaCountGrowth(pool, size, incr); \
    PR_END_MACRO

#define PL_ARENA_MARK(pool) ((void *) (pool)->current->avail)
#define PR_UPTRDIFF(p,q) ((PRUword)(p) - (PRUword)(q))

#define PL_CLEAR_UNUSED_PATTERN(a, pattern) \
	   (PR_ASSERT((a)->avail <= (a)->limit), \
	   memset((void*)(a)->avail, (pattern), (a)->limit - (a)->avail))
#ifdef DEBUG
#define PL_FREE_PATTERN 0xDA
#define PL_CLEAR_UNUSED(a) PL_CLEAR_UNUSED_PATTERN((a), PL_FREE_PATTERN)
#define PL_CLEAR_ARENA(a)  memset((void*)(a), PL_FREE_PATTERN, \
                           (a)->limit - (PRUword)(a))
#else
#define PL_CLEAR_UNUSED(a)
#define PL_CLEAR_ARENA(a)
#endif

#define PL_ARENA_RELEASE(pool, mark) \
    PR_BEGIN_MACRO \
        char *_m = (char *)(mark); \
        PLArena *_a = (pool)->current; \
        if (PR_UPTRDIFF(_m, _a->base) <= PR_UPTRDIFF(_a->avail, _a->base)) { \
            _a->avail = (PRUword)PL_ARENA_ALIGN(pool, _m); \
            PL_CLEAR_UNUSED(_a); \
            PL_ArenaCountRetract(pool, _m); \
        } else { \
            PL_ArenaRelease(pool, _m); \
        } \
        PL_ArenaCountRelease(pool, _m); \
    PR_END_MACRO

#ifdef PL_ARENAMETER
#define PL_COUNT_ARENA(pool,op) ((pool)->stats.narenas op)
#else
#define PL_COUNT_ARENA(pool,op)
#endif

#define PL_ARENA_DESTROY(pool, a, pnext) \
    PR_BEGIN_MACRO \
        PL_COUNT_ARENA(pool,--); \
        if ((pool)->current == (a)) (pool)->current = &(pool)->first; \
        *(pnext) = (a)->next; \
        PL_CLEAR_ARENA(a); \
        free(a); \
        (a) = 0; \
    PR_END_MACRO

#ifdef PL_ARENAMETER

#include <stdio.h>

PR_EXTERN(void) PL_ArenaCountAllocation(PLArenaPool *pool, PRUint32 nb);

PR_EXTERN(void) PL_ArenaCountInplaceGrowth(
    PLArenaPool *pool, PRUint32 size, PRUint32 incr);

PR_EXTERN(void) PL_ArenaCountGrowth(
    PLArenaPool *pool, PRUint32 size, PRUint32 incr);

PR_EXTERN(void) PL_ArenaCountRelease(PLArenaPool *pool, char *mark);

PR_EXTERN(void) PL_ArenaCountRetract(PLArenaPool *pool, char *mark);

PR_EXTERN(void) PL_DumpArenaStats(FILE *fp);

#else  

#define PL_ArenaCountAllocation(ap, nb)
#define PL_ArenaCountInplaceGrowth(ap, size, incr)
#define PL_ArenaCountGrowth(ap, size, incr)
#define PL_ArenaCountRelease(ap, mark)
#define PL_ArenaCountRetract(ap, mark)

#endif 

PR_END_EXTERN_C

#endif 
