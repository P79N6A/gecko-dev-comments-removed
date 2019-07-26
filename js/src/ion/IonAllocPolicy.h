






#ifndef jsion_ion_alloc_policy_h__
#define jsion_ion_alloc_policy_h__

#include "jscntxt.h"
#include "ds/LifoAlloc.h"

#include "Ion.h"
#include "InlineList.h"

namespace js {
namespace ion {

class TempAllocator
{
    LifoAlloc *lifoAlloc_;
    void *mark_;

  public:
    TempAllocator(LifoAlloc *lifoAlloc)
      : lifoAlloc_(lifoAlloc),
        mark_(lifoAlloc->mark())
    { }

    ~TempAllocator()
    {
        lifoAlloc_->release(mark_);
    }

    void *allocateInfallible(size_t bytes)
    {
        void *p = lifoAlloc_->allocInfallible(bytes);
        JS_ASSERT(p);
        return p;
    }

    void *allocate(size_t bytes)
    {
        void *p = lifoAlloc_->alloc(bytes);
        if (!ensureBallast())
            return NULL;
        return p;
    }

    LifoAlloc *lifoAlloc()
    {
        return lifoAlloc_;
    }

    bool ensureBallast() {
        
        
        return lifoAlloc_->ensureUnusedApproximate(16 * 1024);
    }
};

class IonAllocPolicy
{
  public:
    void *malloc_(size_t bytes) {
        return GetIonContext()->temp->allocate(bytes);
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

class AutoIonContextAlloc
{
    TempAllocator tempAlloc_;
    IonContext *icx_;
    TempAllocator *prevAlloc_;

  public:
    explicit AutoIonContextAlloc(JSContext *cx)
      : tempAlloc_(&cx->tempLifoAlloc()),
        icx_(GetIonContext()),
        prevAlloc_(icx_->temp)
    {
        icx_->temp = &tempAlloc_;
    }

    ~AutoIonContextAlloc() {
        JS_ASSERT(icx_->temp == &tempAlloc_);
        icx_->temp = prevAlloc_;
    }
};

struct TempObject
{
    inline void *operator new(size_t nbytes) {
        return GetIonContext()->temp->allocateInfallible(nbytes);
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

