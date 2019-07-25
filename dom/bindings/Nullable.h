





#ifndef mozilla_dom_bindings_Nullable_h
#define mozilla_dom_bindings_Nullable_h

#include "mozilla/Assertions.h"

namespace mozilla {
namespace dom {
namespace bindings {


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

  void SetNull() {
    mIsNull = true;
  }

  T Value() const {
    MOZ_ASSERT(!mIsNull);
    return mValue;
  }

  bool IsNull() const {
    return mIsNull;
  }
};

} 
} 
} 

#endif 
