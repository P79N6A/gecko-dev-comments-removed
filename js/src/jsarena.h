






































#ifndef jsarena_h___
#define jsarena_h___







#include <stdlib.h>
#include "jstypes.h"
#include "jscompat.h"
#include "jsstaticcheck.h"

JS_BEGIN_EXTERN_C

typedef struct JSArena JSArena;
typedef struct JSArenaPool JSArenaPool;

struct JSArena {
    JSArena     *next;          
    jsuword     base;           
    jsuword     limit;          
    jsuword     avail;          
};

struct JSArenaPool {
    JSArena     first;          
    JSArena     *current;       
    size_t      arenasize;      
    jsuword     mask;           
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
        STATIC_ASSUME(!p || ubound((char *)p) >= nb);                         \
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
            } else if ((jsuword)(p) == _a->base) {                            \
                p = (type) JS_ArenaRealloc(pool, p, size, incr);              \
            } else {                                                          \
                p = (type) JS_ArenaGrow(pool, p, size, incr);                 \
            }                                                                 \
        } else {                                                              \
            p = (type) JS_ArenaGrow(pool, p, size, incr);                     \
        }                                                                     \
        STATIC_ASSUME(!p || ubound((char *)p) >= size + incr);                \
    JS_END_MACRO

#define JS_ARENA_MARK(pool)     ((void *) (pool)->current->avail)
#define JS_UPTRDIFF(p,q)        ((jsuword)(p) - (jsuword)(q))




#define JS_ARENA_MARK_MATCH(a, mark)                                          \
    (JS_UPTRDIFF(mark, (a)->base) <= JS_UPTRDIFF((a)->avail, (a)->base))

#ifdef DEBUG
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
        } else {                                                              \
            JS_ArenaRelease(pool, _m);                                        \
        }                                                                     \
    JS_END_MACRO

#define JS_ARENA_DESTROY(pool, a, pnext)                                      \
    JS_BEGIN_MACRO                                                            \
        JS_COUNT_ARENA(pool,--);                                              \
        if ((pool)->current == (a)) (pool)->current = &(pool)->first;         \
        *(pnext) = (a)->next;                                                 \
        JS_CLEAR_ARENA(a);                                                    \
        js::UnwantedForeground::free_(a);                                      \
        (a) = NULL;                                                           \
    JS_END_MACRO




extern JS_PUBLIC_API(void)
JS_InitArenaPool(JSArenaPool *pool, const char *name, size_t size,
                 size_t align);






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

JS_END_EXTERN_C

#ifdef __cplusplus

namespace js {

template <typename T>
inline T *
ArenaArray(JSArenaPool &pool, unsigned count)
{
    void *v;
    JS_ARENA_ALLOCATE(v, &pool, count * sizeof(T));
    return (T *) v;
}

template <typename T>
inline T *
ArenaNew(JSArenaPool &pool)
{
    void *v;
    JS_ARENA_ALLOCATE(v, &pool, sizeof(T));
    return v ? new (v) T() : NULL;
}

template <typename T, typename A>
inline T *
ArenaNew(JSArenaPool &pool, const A &a)
{
    void *v;
    JS_ARENA_ALLOCATE(v, &pool, sizeof(T));
    return v ? new (v) T(a) : NULL;
}

template <typename T, typename A, typename B>
inline T *
ArenaNew(JSArenaPool &pool, const A &a, const B &b)
{
    void *v;
    JS_ARENA_ALLOCATE(v, &pool, sizeof(T));
    return v ? new (v) T(a, b) : NULL;
}

template <typename T, typename A, typename B, typename C>
inline T *
ArenaNew(JSArenaPool &pool, const A &a, const B &b, const C &c)
{
    void *v;
    JS_ARENA_ALLOCATE(v, &pool, sizeof(T));
    return v ? new (v) T(a, b, c) : NULL;
}

template <typename T, typename A, typename B, typename C, typename D>
inline T *
ArenaNew(JSArenaPool &pool, const A &a, const B &b, const C &c, const D &d)
{
    void *v;
    JS_ARENA_ALLOCATE(v, &pool, sizeof(T));
    return v ? new (v) T(a, b, c, d) : NULL;
}

template <typename T, typename A, typename B, typename C, typename D, typename E>
inline T *
ArenaNew(JSArenaPool &pool, const A &a, const B &b, const C &c, const D &d, const E &e)
{
    void *v;
    JS_ARENA_ALLOCATE(v, &pool, sizeof(T));
    return v ? new (v) T(a, b, c, d, e) : NULL;
}

inline uintN
ArenaAllocatedSize(const JSArenaPool &pool)
{
    uintN res = 0;
    const JSArena *a = &pool.first;
    while (a) {
        res += (a->limit - (jsuword)a);
        a = a->next;
    }
    return res;
}


inline void
MoveArenaPool(JSArenaPool *oldPool, JSArenaPool *newPool)
{
    *newPool = *oldPool;
    JS_InitArenaPool(oldPool, NULL, newPool->arenasize, newPool->mask + 1);
}

} 

#endif 

#endif 
