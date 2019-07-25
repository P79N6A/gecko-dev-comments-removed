






































#ifndef jsarena_h___
#define jsarena_h___







#include <stdlib.h>
#include <memory.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include "jstypes.h"
#include "jscompat.h"
#include "jsutil.h"

#ifdef DEBUG
const uint8_t JS_FREE_PATTERN = 0xDA;
#endif

#define JS_UPTRDIFF(p,q)        (jsuword(p) - jsuword(q))

struct JSArena {
    JSArena     *next;          
    jsuword     base;           
    jsuword     limit;          
    jsuword     avail;          

  public:
    jsuword getBase() const { return base; }
    jsuword getLimit() const { return limit; }
    jsuword getAvail() const { return avail; }
    void setAvail(jsuword newAvail) { avail = newAvail; }
    const JSArena *getNext() const { return next; }
    JSArena *getNext() { return next; }
    void munge(JSArena *next) { this-> next = next; }
    void munge(JSArena *next, jsuword base, jsuword avail, jsuword limit) {
        this->next = next;
        this->base = base;
        this->avail = avail;
        this->limit = limit;
    }

    


    bool markMatch(void *mark) const {
        return JS_UPTRDIFF(mark, base) <= JS_UPTRDIFF(avail, base);
    }

#ifdef DEBUG
    void clear() { memset(this, JS_FREE_PATTERN, limit - jsuword(this)); }

    void clearUnused() {
        JS_ASSERT(avail <= limit);
        memset((void *) avail, JS_FREE_PATTERN, limit - avail);
    }
#else
    void clear() {}
    void clearUnused() {}
#endif

    friend class JSArenaPool;
};

#ifdef DEBUG
class JSArenaStats {
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

  public:
    void init(const char *name, JSArenaStats *next) {
        memset(this, 0, sizeof *this);
        this->name = strdup(name);
        this->next = next;
    }
    void finish() { free(name); name = 0; }
    void dump(FILE *fp) const;
    const JSArenaStats *getNext() const { return next; }

    friend class JSArenaPool;
};

JS_FRIEND_API(void)
JS_DumpArenaStats();
#endif 

class JSArenaPool {
    JSArena     first;          
    JSArena     *current;       
    size_t      arenasize;      
    jsuword     mask;           
    size_t      *quotap;        

#ifdef DEBUG
    JSArenaStats stats;
#endif

  public:
    


    void init(const char *name, size_t size, size_t align, size_t *quotap);

    


    void finish();

    




    void free() {
        freeArenaList(&first);
        incDeallocCount();
    }

    void *getMark() const { return (void *) current->avail; }
    jsuword align(jsuword n) const { return (n + mask) & ~mask; }
    jsuword align(void *n) const { return align(jsuword(n)); }
    bool currentIsFirst() const { return current == &first; }
    const JSArena *getFirst() const { return &first; }
    const JSArena *getSecond() const { return first.next; }
    const JSArena *getCurrent() const { return current; }
    JSArena *getFirst() { return &first; }
    JSArena *getSecond() { return first.next; }
    JSArena *getCurrent() { return current; }

    void *allocate(size_t nb, bool limitCheck);

    template <typename T>
    void allocateCast(T &p, size_t nb) {
        p = (T) allocate(nb, true);
    }

    template <typename T>
    void allocateType(T *&p) {
        p = (T *) allocate(sizeof(T), false);
    }

    void allocate(void *&p, size_t nb) { allocateCast<void *>(p, nb); }

    void *grow(jsuword p, size_t size, size_t incr) {
        countGrowth(size, incr);
        if (current->avail == p + align(size)) {
            size_t nb = align(size + incr);
            if (current->limit >= nb && p <= current->limit - nb) {
                current->avail = p + nb;
                countInplaceGrowth(size, incr);
            } else if (p == current->base) {
                p = jsuword(reallocInternal((void *) p, size, incr));
            } else {
                p = jsuword(growInternal((void *) p, size, incr));
            }
        } else {
            p = jsuword(growInternal((void *) p, size, incr));
        }
        return (void *) p;
    }

    template <typename T>
    void growCast(T &p, size_t size, size_t incr) {
        p = (T) grow(jsuword(p), size, incr);
    }

    void release(void *mark) {
        if (current != &first && current->markMatch(mark)) {
            current->avail = jsuword(align(mark));
            JS_ASSERT(current->avail <= current->limit);
            current->clearUnused();
            countRetract(mark);
        } else {
            releaseInternal(mark);
        }
        countRelease(mark);
    }

    JSArena *destroy(JSArena *&a) {
        decArenaCount();
        if (current == a)
            current = &first;
        JSArena *next = a->next;
        a->clear();
        ::free(a);
        a = NULL;
        return next;
    }

    class ScopedExtension {
        JSArenaPool * const parent;
        JSArena     * const prevTop;
        JSArena     * const pushed;
      public:
        ScopedExtension(JSArenaPool *parent, JSArena *toPush)
          : parent(parent), prevTop(parent->getCurrent()), pushed(toPush) {
            JS_ASSERT(!prevTop->getNext());
            JS_ASSERT(!toPush->getNext());
            JS_ASSERT(prevTop != toPush);
            parent->current = prevTop->next = toPush;
        }
        ~ScopedExtension() {
            JS_ASSERT(!pushed->next);
            JS_ASSERT(prevTop->next == pushed);
            JS_ASSERT(parent->getCurrent() == pushed);
            parent->current = prevTop;
            prevTop->next = NULL;
        }
    };

  private:
    



#ifdef JS_ALIGN_OF_POINTER
    static const jsuword POINTER_MASK = jsuword(JS_ALIGN_OF_POINTER - 1);

    


    jsuword headerBaseMask() const { return mask | POINTER_MASK; }

    jsuword headerSize() const {
        return sizeof(JSArena **) + ((mask < POINTER_MASK) ? POINTER_MASK - mask : 0);
    }
#endif

    



    JSArena ***ptrToHeader(void *p) const {
        JS_ASSERT((jsuword(p) & headerBaseMask()) == 0);
        return (JSArena ***)(p) - 1;
    }
    JSArena ***ptrToHeader(jsuword p) const { return ptrToHeader((void *) p); }

    


    JSArena **getHeader(const JSArena *a) const { return *ptrToHeader(a->base); }
    JSArena **setHeader(JSArena *a, JSArena **ap) const { return *ptrToHeader(a->base) = ap; }

    void *allocateInternal(size_t nb);
    void *reallocInternal(void *p, size_t size, size_t incr);

    void releaseInternal(void *mark) {
        for (JSArena *a = &first; a; a = a->next) {
            JS_ASSERT(a->base <= a->avail && a->avail <= a->limit);

            if (a->markMatch(mark)) {
                a->avail = align(mark);
                JS_ASSERT(a->avail <= a->limit);
                freeArenaList(a);
                return;
            }
        }
    }

    void *growInternal(void *p, size_t size, size_t incr) {
        



        if (size > arenasize)
            return reallocInternal(p, size, incr);

        void *newp = allocate(size + incr, true);
        if (newp)
            memcpy(newp, p, size);
        return newp;
    }

    



    void freeArenaList(JSArena *head) {
        JSArena **ap = &head->next;
        JSArena *a = *ap;
        if (!a)
            return;

#ifdef DEBUG
        do {
            JS_ASSERT(a->base <= a->avail);
            JS_ASSERT(a->avail <= a->limit);
            a->avail = a->base;
            a->clearUnused();
        } while ((a = a->next) != NULL);
        a = *ap;
#endif

        do {
            *ap = a->next;
            if (quotap)
                *quotap += a->limit - (jsuword) a;
            a->clear();
            decArenaCount();
            js_free(a);
        } while ((a = *ap) != NULL);

        current = head;
    }

    
#ifdef DEBUG
    void countAllocation(size_t nb);
    void countInplaceGrowth(size_t size, size_t incr) { stats.ninplace++; }
    void countGrowth(size_t size, size_t incr);
    void countRelease(void *mark) { stats.nreleases++; }
    void countRetract(void *mark) { stats.nfastrels++; }
    void incArenaCount() { stats.narenas++; }
    void decArenaCount() { stats.narenas--; }
    void incDeallocCount() { stats.ndeallocs++; }
    void incReallocCount() { stats.nreallocs++; }
#else
    void countAllocation(size_t nb) {}
    void countInplaceGrowth(size_t size, size_t incr) {}
    void countGrowth(size_t size, size_t incr) {}
    void countRelease(void *mark) {}
    void countRetract(void *mark) {}
    void incArenaCount() {}
    void decArenaCount() {}
    void incDeallocCount() {}
    void incReallocCount() {}
#endif
};



JS_BEGIN_EXTERN_C

JS_FRIEND_API(void *)
JS_ARENA_MARK(const JSArenaPool *pool);

JS_FRIEND_API(void)
JS_ARENA_RELEASE(JSArenaPool *pool, void *mark);

JS_FRIEND_API(void *)
JS_ARENA_ALLOCATE_COMMON_SANE(jsuword p, JSArenaPool *pool, size_t nb, bool limitCheck);

#define JS_ARENA_ALLOCATE_CAST(p, type, pool, nb)                                   \
    JS_BEGIN_MACRO                                                                  \
        (p) = (type) JS_ARENA_ALLOCATE_COMMON_SANE(jsuword(p), (pool), (nb), true); \
    JS_END_MACRO
    

JS_END_EXTERN_C

#endif 
