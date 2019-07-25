











































#include <stdlib.h>
#include <string.h>
#include "jsalloc.h"
#include "jstypes.h"
#include "jsstdint.h"
#include "jsbit.h"
#include "jsarena.h"
#include "jsprvtd.h"

using namespace js;


JS_STATIC_ASSERT(sizeof(JSArena) % 8 == 0);

JS_PUBLIC_API(void)
JS_InitArenaPool(JSArenaPool *pool, const char *name, size_t size, size_t align)
{
    
    if (align == 1 || align == 2 || align == 4 || align == 8) {
        pool->mask = align - 1;
    } else {
        
        JS_NOT_REACHED("JS_InitArenaPool: bad align");
        pool->mask = 7;
    }
    pool->first.next = NULL;
    
    pool->first.base = pool->first.avail = pool->first.limit =
        JS_ARENA_ALIGN(pool, &pool->first + 1);
    pool->current = &pool->first;
    pool->arenasize = size;
}

JS_PUBLIC_API(void *)
JS_ArenaAllocate(JSArenaPool *pool, size_t nb)
{
    









    JS_ASSERT((nb & pool->mask) == 0);
    JSArena *a;
    



    for (a = pool->current; nb > a->limit || a->avail > a->limit - nb; pool->current = a) {
        JSArena **ap = &a->next;
        if (!*ap) {
            
            size_t gross = sizeof(JSArena) + JS_MAX(nb, pool->arenasize);
            a = (JSArena *) OffTheBooks::malloc_(gross);
            if (!a)
                return NULL;

            a->next = NULL;
            a->base = a->avail = jsuword(a) + sizeof(JSArena);
            




            JS_ASSERT(a->base == JS_ARENA_ALIGN(pool, a->base));
            a->limit = (jsuword)a + gross;

            *ap = a;
            continue;
        }
        a = *ap;        
    }

    void* p = (void *)a->avail;
    a->avail += nb;
    JS_ASSERT(a->base <= a->avail && a->avail <= a->limit);
    return p;
}

JS_PUBLIC_API(void *)
JS_ArenaRealloc(JSArenaPool *pool, void *p, size_t size, size_t incr)
{
    
    JS_ASSERT(size + incr > pool->arenasize);

    
    JSArena *a;
    JSArena **ap = &pool->first.next;
    while (true) {
        a = *ap;
        if (JS_IS_IN_ARENA(a, p))
            break;
        JS_ASSERT(a != pool->current);
        ap = &a->next;
    }
    
    JS_ASSERT(a->base == jsuword(p));

    size_t gross = sizeof(JSArena) + JS_ARENA_ALIGN(pool, size + incr);
    a = (JSArena *) OffTheBooks::realloc_(a, gross);
    if (!a)
        return NULL;

    a->base = jsuword(a) + sizeof(JSArena);
    a->avail = a->limit = jsuword(a) + gross;
    




    JS_ASSERT(a->base == JS_ARENA_ALIGN(pool, a->base));

    if (a != *ap) {
        
        if (pool->current == *ap)
            pool->current = a;
        *ap = a;
    }

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

        if (JS_IS_IN_ARENA(a, mark)) {
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
