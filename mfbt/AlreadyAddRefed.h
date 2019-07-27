







#ifndef AlreadyAddRefed_h
#define AlreadyAddRefed_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Move.h"

namespace mozilla {

struct unused_t;

} 










template<class T>
struct already_AddRefed
{
  

























  already_AddRefed() : mRawPtr(nullptr) {}

  
  
  typedef void (already_AddRefed::* MatchNullptr)(double, float);
  MOZ_IMPLICIT already_AddRefed(MatchNullptr aRawPtr) : mRawPtr(nullptr) {}

  explicit already_AddRefed(T* aRawPtr) : mRawPtr(aRawPtr) {}

  
  already_AddRefed(const already_AddRefed<T>& aOther) = delete;
  already_AddRefed<T>& operator=(const already_AddRefed<T>& aOther) = delete;

  already_AddRefed(already_AddRefed<T>&& aOther) : mRawPtr(aOther.take()) {}

  already_AddRefed<T>& operator=(already_AddRefed<T>&& aOther)
  {
    mRawPtr = aOther.take();
    return *this;
  }

  
















  template <typename U>
  already_AddRefed(already_AddRefed<U>&& aOther) : mRawPtr(aOther.take()) {}

  ~already_AddRefed() { MOZ_ASSERT(!mRawPtr); }

  
  
  
  
  friend void operator<<(const mozilla::unused_t& aUnused,
                         const already_AddRefed<T>& aRhs)
  {
    auto mutableAlreadyAddRefed = const_cast<already_AddRefed<T>*>(&aRhs);
    aUnused << mutableAlreadyAddRefed->take();
  }

  MOZ_WARN_UNUSED_RESULT T* take()
  {
    T* rawPtr = mRawPtr;
    mRawPtr = nullptr;
    return rawPtr;
  }

  













  template<class U>
  already_AddRefed<U> downcast()
  {
    U* tmp = static_cast<U*>(mRawPtr);
    mRawPtr = nullptr;
    return already_AddRefed<U>(tmp);
  }

private:
  T* MOZ_OWNING_REF mRawPtr;
};

#endif 
