








































#ifndef jsion_ion_alloc_policy_h__
#define jsion_ion_alloc_policy_h__

#include "jscntxt.h"
#include "jsarena.h"

#include "Ion.h"
#include "InlineList.h"

namespace js {
namespace ion {

class IonAllocPolicy
{
  public:
    void *malloc_(size_t bytes) {
        JSContext *cx = GetIonContext()->cx;
        void *p;
        JS_ARENA_ALLOCATE(p, &cx->tempPool, bytes);
        return p;
    }
    void *realloc_(void *p, size_t oldBytes, size_t bytes) {
        void *n = malloc_(bytes);
        if (!n)
            return n;
        memcpy(n, p, Min(oldBytes, bytes));
        return n;
    }
    void free_(void *p) {
    }
    void reportAllocOverflow() const {
    }
};

struct TempAllocator
{
    JSArenaPool *arena;

    TempAllocator(JSArenaPool *arena)
      : arena(arena),
        mark(JS_ARENA_MARK(arena))
    { }

    ~TempAllocator()
    {
        JS_ARENA_RELEASE(arena, mark);
    }

    void *allocate(size_t bytes)
    {
        void *p;
        JS_ARENA_ALLOCATE(p, arena, bytes);
        if (!ensureBallast())
            return NULL;
        return p;
    }

    bool ensureBallast() {
        return true;
    }

  private:
    void *mark;
};

struct TempObject
{
    inline void *operator new(size_t nbytes) {
        return GetIonContext()->temp->allocate(nbytes);
    }
public:
    inline void *operator new(size_t nbytes, void *pos) {
        return pos;
    }
};

template <typename T>
class TempObjectPool
{
    InlineForwardList<T> freed_;

  public:
    T *allocate() {
        if (freed_.empty())
            return new T();
        return freed_.popFront();
    }
    void free(T *obj) {
        freed_.pushFront(obj);
    }
};

} 
} 

#endif 

