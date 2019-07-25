








































#ifndef jsion_temp_alloc_policy_h__
#define jsion_temp_alloc_policy_h__

namespace js {
namespace ion {

#include "jscntxt.h"
#include "jsarena.h"

class TempAllocPolicy
{
    JSContext *cx;

  public:
    TempAllocPolicy(JSContext *cx)
      : cx(cx)
    { }
    TempAllocPolicy(const TempAllocPolicy &policy)
      : cx(policy.cx)
    { }
    void *malloc_(size_t bytes) {
        void *p;
        JS_ARENA_ALLOCATE(p, &cx->tempPool, bytes);
        return p;
    }
    void *realloc_(void *p, size_t bytes) {
        return malloc_(bytes);
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
        return p;
    }

  private:
    void *mark;
};

struct TempObject
{
    void *operator new(size_t nbytes);
    inline void *operator new(size_t nbytes, TempAllocator &alloc) {
        return alloc.allocate(nbytes);
    }
};

} 
} 

#endif 

