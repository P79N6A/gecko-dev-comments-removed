





#ifndef mozilla_dom_Nullable_h
#define mozilla_dom_Nullable_h

#include "mozilla/Assertions.h"

namespace mozilla {
namespace dom {


template <typename T>
struct Nullable
{
private:
  T mValue;
  bool mIsNull;

public:
  Nullable()
    : mIsNull(true)
  {}

  Nullable(T aValue)
    : mValue(aValue)
    , mIsNull(false)
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

  bool IsNull() const {
    return mIsNull;
  }
};

} 
} 

#endif 
