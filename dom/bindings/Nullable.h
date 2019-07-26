





#ifndef mozilla_dom_Nullable_h
#define mozilla_dom_Nullable_h

#include "mozilla/Assertions.h"
#include "nsTArrayForwardDeclare.h"

namespace mozilla {
namespace dom {


template <typename T>
struct Nullable
{
private:
  
  
  bool mIsNull;
  T mValue;

public:
  Nullable()
    : mIsNull(true)
  {}

  explicit Nullable(T aValue)
    : mIsNull(false)
    , mValue(aValue)
  {}

  void SetValue(T aValue) {
    mValue = aValue;
    mIsNull = false;
  }

  
  
  
  T& SetValue() {
    mIsNull = false;
    return mValue;
  }

  void SetNull() {
    mIsNull = true;
  }

  const T& Value() const {
    MOZ_ASSERT(!mIsNull);
    return mValue;
  }

  T& Value() {
    MOZ_ASSERT(!mIsNull);
    return mValue;
  }

  bool IsNull() const {
    return mIsNull;
  }

  
  
  template<typename U>
  operator const Nullable< nsTArray<U> >&() const {
    
    const nsTArray<U>& arr = mValue;
    (void)arr;
    return *reinterpret_cast<const Nullable< nsTArray<U> >*>(this);
  }
  template<typename U>
  operator const Nullable< FallibleTArray<U> >&() const {
    
    const FallibleTArray<U>& arr = mValue;
    (void)arr;
    return *reinterpret_cast<const Nullable< FallibleTArray<U> >*>(this);
  }
};

} 
} 

#endif 
