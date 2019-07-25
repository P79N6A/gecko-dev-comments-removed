











































#include <stdlib.h>
#include <string.h>
#include "jsalloc.h"
#include "jstypes.h"
#include "jsstdint.h"
#include "jsbit.h"
#include "jsarena.h"
#include "jsprvtd.h"

using namespace js;

#define JS_ARENA_DEFAULT_ALIGN  sizeof(double)

JS_PUBLIC_API(void)
JS_InitArenaPool(JSArenaPool *pool, const char *name, size_t size,
                 size_t align)
{
    if (align == 0)
        align = JS_ARENA_DEFAULT_ALIGN;
    pool->mask = JS_BITMASK(JS_CeilingLog2(align));
    pool->first.next = NULL;
    pool->first.base = pool->first.avail = pool->first.limit =
        JS_ARENA_ALIGN(pool, &pool->first + 1);
    pool->current = &pool->first;
    pool->arenasize = size;
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
            b = (JSArena *) OffTheBooks::malloc_(gross);
            if (!b)
                return NULL;

            b->next = NULL;
            b->limit = (jsuword)b + gross;

            
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
    jsuword boff, aoff, extra, hdrsz, gross;

    



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
    a = (JSArena *) OffTheBooks::realloc_(a, gross);
    if (!a)
        return NULL;

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
        JS_CLEAR_ARENA(a);
        UnwantedForeground::free_(a);
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
}

JS_PUBLIC_API(void)
JS_FinishArenaPool(JSArenaPool *pool)
{
    FreeArenaList(pool, &pool->first);
}

JS_PUBLIC_API(void)
JS_ArenaFinish()
{
}

JS_PUBLIC_API(void)
JS_ArenaShutDown(void)
{
}
