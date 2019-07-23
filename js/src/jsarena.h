






































#ifndef jsarena_h___
#define jsarena_h___







#include <stdlib.h>
#include "jstypes.h"
#include "jscompat.h"

JS_BEGIN_EXTERN_C

typedef struct JSArena JSArena;
typedef struct JSArenaPool JSArenaPool;

struct JSArena {
    JSArena     *next;          
    jsuword     base;           
    jsuword     limit;          
    jsuword     avail;          
};

#ifdef JS_ARENAMETER
typedef struct JSArenaStats JSArenaStats;

struct JSArenaStats {
    JSArenaStats *next;         
    char        *name;          
    uint32      narenas;        
    uint32      nallocs;        
    uint32      nmallocs;       
    uint32      ndeallocs;      
    uint32      ngrows;         
    uint32      ninplace;       
    uint32      nreallocs;      
    uint32      nreleases;      
    uint32      nfastrels;      
    size_t      nbytes;         
    size_t      maxalloc;       
    double      variance;       
};
#endif

struct JSArenaPool {
    JSArena     first;          
    JSArena     *current;       
    size_t      arenasize;      
    jsuword     mask;           
    size_t      *quotap;        

#ifdef JS_ARENAMETER
    JSArenaStats stats;
#endif
};

#define JS_ARENA_ALIGN(pool, n) (((jsuword)(n) + (pool)->mask) & ~(pool)->mask)

#define JS_ARENA_ALLOCATE(p, pool, nb)                                        \
    JS_ARENA_ALLOCATE_CAST(p, void *, pool, nb)

#define JS_ARENA_ALLOCATE_TYPE(p, type, pool)                                 \
    JS_ARENA_ALLOCATE_COMMON(p, type *, pool, sizeof(type), 0)

#define JS_ARENA_ALLOCATE_CAST(p, type, pool, nb)                             \
    JS_ARENA_ALLOCATE_COMMON(p, type, pool, nb, _nb > _a->limit)











#define JS_ARENA_ALLOCATE_COMMON(p, type, pool, nb, guard)                    \
    JS_BEGIN_MACRO                                                            \
        JSArena *_a = (pool)->current;                                        \
        size_t _nb = JS_ARENA_ALIGN(pool, nb);                                \
        jsuword _p = _a->avail;                                               \
        if ((guard) || _p > _a->limit - _nb)                                  \
            _p = (jsuword)JS_ArenaAllocate(pool, _nb);                        \
        else                                                                  \
            _a->avail = _p + _nb;                                             \
        p = (type) _p;                                                        \
        JS_ArenaCountAllocation(pool, nb);                                    \
    JS_END_MACRO

#define JS_ARENA_GROW(p, pool, size, incr)                                    \
    JS_ARENA_GROW_CAST(p, void *, pool, size, incr)

#define JS_ARENA_GROW_CAST(p, type, pool, size, incr)                         \
    JS_BEGIN_MACRO                                                            \
        JSArena *_a = (pool)->current;                                        \
        if (_a->avail == (jsuword)(p) + JS_ARENA_ALIGN(pool, size)) {         \
            size_t _nb = (size) + (incr);                                     \
            _nb = JS_ARENA_ALIGN(pool, _nb);                                  \
            if (_a->limit >= _nb && (jsuword)(p) <= _a->limit - _nb) {        \
                _a->avail = (jsuword)(p) + _nb;                               \
                JS_ArenaCountInplaceGrowth(pool, size, incr);                 \
            } else if ((jsuword)(p) == _a->base) {                            \
                p = (type) JS_ArenaRealloc(pool, p, size, incr);              \
            } else {                                                          \
                p = (type) JS_ArenaGrow(pool, p, size, incr);                 \
            }                                                                 \
        } else {                                                              \
            p = (type) JS_ArenaGrow(pool, p, size, incr);                     \
        }                                                                     \
        JS_ArenaCountGrowth(pool, size, incr);                                \
    JS_END_MACRO

#define JS_ARENA_MARK(pool)     ((void *) (pool)->current->avail)
#define JS_UPTRDIFF(p,q)        ((jsuword)(p) - (jsuword)(q))




#define JS_ARENA_MARK_MATCH(a, mark)                                          \
    (JS_UPTRDIFF(mark, (a)->base) <= JS_UPTRDIFF((a)->avail, (a)->base))

#ifdef DEBUG
#define JS_FREE_PATTERN         0xDA
#define JS_CLEAR_UNUSED(a)      (JS_ASSERT((a)->avail <= (a)->limit),         \
                                 memset((void*)(a)->avail, JS_FREE_PATTERN,   \
                                        (a)->limit - (a)->avail))
#define JS_CLEAR_ARENA(a)       memset((void*)(a), JS_FREE_PATTERN,           \
                                       (a)->limit - (jsuword)(a))
#else
#define JS_CLEAR_UNUSED(a)
#define JS_CLEAR_ARENA(a)
#endif

#define JS_ARENA_RELEASE(pool, mark)                                          \
    JS_BEGIN_MACRO                                                            \
        char *_m = (char *)(mark);                                            \
        JSArena *_a = (pool)->current;                                        \
        if (_a != &(pool)->first && JS_ARENA_MARK_MATCH(_a, _m)) {            \
            _a->avail = (jsuword)JS_ARENA_ALIGN(pool, _m);                    \
            JS_ASSERT(_a->avail <= _a->limit);                                \
            JS_CLEAR_UNUSED(_a);                                              \
            JS_ArenaCountRetract(pool, _m);                                   \
        } else {                                                              \
            JS_ArenaRelease(pool, _m);                                        \
        }                                                                     \
        JS_ArenaCountRelease(pool, _m);                                       \
    JS_END_MACRO

#ifdef JS_ARENAMETER
#define JS_COUNT_ARENA(pool,op) ((pool)->stats.narenas op)
#else
#define JS_COUNT_ARENA(pool,op)
#endif

#define JS_ARENA_DESTROY(pool, a, pnext)                                      \
    JS_BEGIN_MACRO                                                            \
        JS_COUNT_ARENA(pool,--);                                              \
        if ((pool)->current == (a)) (pool)->current = &(pool)->first;         \
        *(pnext) = (a)->next;                                                 \
        JS_CLEAR_ARENA(a);                                                    \
        free(a);                                                              \
        (a) = NULL;                                                           \
    JS_END_MACRO




extern JS_PUBLIC_API(void)
JS_InitArenaPool(JSArenaPool *pool, const char *name, size_t size,
                 size_t align, size_t *quotap);






extern JS_PUBLIC_API(void)
JS_FreeArenaPool(JSArenaPool *pool);




extern JS_PUBLIC_API(void)
JS_FinishArenaPool(JSArenaPool *pool);




extern JS_PUBLIC_API(void)
JS_ArenaFinish(void);




extern JS_PUBLIC_API(void)
JS_ArenaShutDown(void);




extern JS_PUBLIC_API(void *)
JS_ArenaAllocate(JSArenaPool *pool, size_t nb);

extern JS_PUBLIC_API(void *)
JS_ArenaRealloc(JSArenaPool *pool, void *p, size_t size, size_t incr);

extern JS_PUBLIC_API(void *)
JS_ArenaGrow(JSArenaPool *pool, void *p, size_t size, size_t incr);

extern JS_PUBLIC_API(void)
JS_ArenaRelease(JSArenaPool *pool, char *mark);

#ifdef JS_ARENAMETER

#include <stdio.h>

extern JS_PUBLIC_API(void)
JS_ArenaCountAllocation(JSArenaPool *pool, size_t nb);

extern JS_PUBLIC_API(void)
JS_ArenaCountInplaceGrowth(JSArenaPool *pool, size_t size, size_t incr);

extern JS_PUBLIC_API(void)
JS_ArenaCountGrowth(JSArenaPool *pool, size_t size, size_t incr);

extern JS_PUBLIC_API(void)
JS_ArenaCountRelease(JSArenaPool *pool, char *mark);

extern JS_PUBLIC_API(void)
JS_ArenaCountRetract(JSArenaPool *pool, char *mark);

extern JS_PUBLIC_API(void)
JS_DumpArenaStats(FILE *fp);

#else  

#define JS_ArenaCountAllocation(ap, nb)
#define JS_ArenaCountInplaceGrowth(ap, size, incr)
#define JS_ArenaCountGrowth(ap, size, incr)
#define JS_ArenaCountRelease(ap, mark)
#define JS_ArenaCountRetract(ap, mark)

#endif 

JS_END_EXTERN_C

#endif 
