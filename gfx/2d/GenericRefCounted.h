








#ifndef MOZILLA_GENERICREFCOUNTED_H_
#define MOZILLA_GENERICREFCOUNTED_H_

#include "mozilla/RefPtr.h"

namespace mozilla {








class GenericRefCountedBase
{
  protected:
    virtual ~GenericRefCountedBase() {};

  public:
    
    virtual void AddRef() = 0;

    virtual void Release() = 0;

    
    
    
    void ref() { AddRef(); }
    void deref() { Release(); }

#ifdef MOZ_REFCOUNTED_LEAK_CHECKING
    virtual const char* typeName() const = 0;
    virtual size_t typeSize() const = 0;
#endif
};

namespace detail {

template<RefCountAtomicity Atomicity>
class GenericRefCounted : public GenericRefCountedBase
{
  protected:
    GenericRefCounted() : refCnt(0) { }

    virtual ~GenericRefCounted() {
      MOZ_ASSERT(refCnt == detail::DEAD);
    }

  public:
    virtual void AddRef() {
      MOZ_ASSERT(int32_t(refCnt) >= 0);
      ++refCnt;
#ifdef MOZ_REFCOUNTED_LEAK_CHECKING
      detail::RefCountLogger::logAddRef(this, refCnt, typeName(), typeSize());
#endif
    }

    virtual void Release() {
      MOZ_ASSERT(int32_t(refCnt) > 0);
      --refCnt;
#ifdef MOZ_REFCOUNTED_LEAK_CHECKING
      detail::RefCountLogger::logRelease(this, refCnt, typeName());
#endif
      if (0 == refCnt) {
#ifdef DEBUG
        refCnt = detail::DEAD;
#endif
        delete this;
      }
    }

    MozRefCountType refCount() const { return refCnt; }
    bool hasOneRef() const {
      MOZ_ASSERT(refCnt > 0);
      return refCnt == 1;
    }

  private:
    typename Conditional<Atomicity == AtomicRefCount, Atomic<MozRefCountType>, MozRefCountType>::Type refCnt;
};

} 







class GenericRefCounted : public detail::GenericRefCounted<detail::NonAtomicRefCount>
{
};





class GenericAtomicRefCounted : public detail::GenericRefCounted<detail::AtomicRefCount>
{
};

} 

#endif
