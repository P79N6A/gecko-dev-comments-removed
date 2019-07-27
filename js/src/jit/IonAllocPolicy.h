





#ifndef jit_IonAllocPolicy_h
#define jit_IonAllocPolicy_h

#include "mozilla/GuardObjects.h"
#include "mozilla/TypeTraits.h"

#include "jscntxt.h"

#include "ds/LifoAlloc.h"
#include "jit/InlineList.h"
#include "jit/Ion.h"

namespace js {
namespace jit {

class TempAllocator
{
    LifoAllocScope lifoScope_;

    
    CompilerRootNode *rootList_;

  public:
    explicit TempAllocator(LifoAlloc *lifoAlloc)
      : lifoScope_(lifoAlloc),
        rootList_(nullptr)
    { }

    void *allocateInfallible(size_t bytes)
    {
        return lifoScope_.alloc().allocInfallible(bytes);
    }

    void *allocate(size_t bytes)
    {
        void *p = lifoScope_.alloc().alloc(bytes);
        if (!ensureBallast())
            return nullptr;
        return p;
    }

    template <size_t ElemSize>
    void *allocateArray(size_t n)
    {
        if (n & mozilla::tl::MulOverflowMask<ElemSize>::value)
            return nullptr;
        void *p = lifoScope_.alloc().alloc(n * ElemSize);
        if (!ensureBallast())
            return nullptr;
        return p;
    }

    LifoAlloc *lifoAlloc()
    {
        return &lifoScope_.alloc();
    }

    CompilerRootNode *&rootList()
    {
        return rootList_;
    }

    bool ensureBallast() {
        
        
        return lifoScope_.alloc().ensureUnusedApproximate(16 * 1024);
    }
};


class AutoTempAllocatorRooter : private JS::AutoGCRooter
{
  public:
    explicit AutoTempAllocatorRooter(JSContext *cx, TempAllocator *temp
                                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : JS::AutoGCRooter(cx, IONALLOC), temp(temp)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    friend void JS::AutoGCRooter::trace(JSTracer *trc);
    void trace(JSTracer *trc);

  private:
    TempAllocator *temp;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class IonAllocPolicy
{
    TempAllocator &alloc_;

  public:
    MOZ_IMPLICIT IonAllocPolicy(TempAllocator &alloc)
      : alloc_(alloc)
    {}
    void *malloc_(size_t bytes) {
        return alloc_.allocate(bytes);
    }
    template <typename T>
    T *pod_calloc(size_t numElems) {
        if (numElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value)
            return nullptr;
        T *p = (T *)alloc_.allocate(numElems * sizeof(T));
        if (p)
            memset(p, 0, numElems * sizeof(T));
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



class OldIonAllocPolicy
{
  public:
    OldIonAllocPolicy()
    {}
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
    inline void *operator new(size_t nbytes, TempAllocator &alloc) {
        return alloc.allocateInfallible(nbytes);
    }
    template <class T>
    inline void *operator new(size_t nbytes, T *pos) {
        static_assert(mozilla::IsConvertible<T*, TempObject*>::value,
                      "Placement new argument type must inherit from TempObject");
        return pos;
    }
};

template <typename T>
class TempObjectPool
{
    TempAllocator *alloc_;
    InlineForwardList<T> freed_;

  public:
    TempObjectPool()
      : alloc_(nullptr)
    {}
    void setAllocator(TempAllocator &alloc) {
        JS_ASSERT(freed_.empty());
        alloc_ = &alloc;
    }
    T *allocate() {
        JS_ASSERT(alloc_);
        if (freed_.empty())
            return new(*alloc_) T();
        return freed_.popFront();
    }
    void free(T *obj) {
        freed_.pushFront(obj);
    }
    void clear() {
        freed_.clear();
    }
};

} 
} 

#endif 
