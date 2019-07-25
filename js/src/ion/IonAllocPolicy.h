








































#ifndef jsion_ion_alloc_policy_h__
#define jsion_ion_alloc_policy_h__

#include "jscntxt.h"
#include "ds/LifoAlloc.h"

#include "Ion.h"
#include "InlineList.h"

namespace js {
namespace ion {

class IonAllocPolicy
{
  public:
    void *malloc_(size_t bytes) {
        JSContext *cx = GetIonContext()->cx;
        return cx->tempLifoAlloc().alloc(bytes);
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

    void *allocate(size_t bytes)
    {
        void *p = lifoAlloc_->alloc(bytes);
        if (!ensureBallast())
            return NULL;
        return p;
    }

    bool ensureBallast() {
        return true;
    }
};

class AutoIonContextAlloc
{
    TempAllocator tempAlloc;
    IonContext *icx;

  public:
    explicit AutoIonContextAlloc(JSContext *cx)
      : tempAlloc(&cx->tempLifoAlloc()) {
        icx = GetIonContext();
        JS_ASSERT(!icx->temp);
        icx->temp = &tempAlloc;
    }

    ~AutoIonContextAlloc() {
        JS_ASSERT(icx->temp == &tempAlloc);
        icx->temp = NULL;
    }
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

