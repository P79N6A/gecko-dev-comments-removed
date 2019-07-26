








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
      ++refCnt;
    }

    virtual void Release() {
      MOZ_ASSERT(refCnt > 0);
      if (0 == --refCnt) {
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
