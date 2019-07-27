





#ifndef mozilla_dom_Nullable_h
#define mozilla_dom_Nullable_h

#include "mozilla/Assertions.h"
#include "nsTArrayForwardDeclare.h"
#include "mozilla/Move.h"
#include "mozilla/Maybe.h"

class nsCycleCollectionTraversalCallback;

namespace mozilla {
namespace dom {


template <typename T>
struct Nullable
{
private:
  Maybe<T> mValue;

public:
  Nullable()
    : mValue()
  {}

  explicit Nullable(T aValue)
    : mValue()
  {
    mValue.emplace(aValue);
  }

  explicit Nullable(Nullable<T>&& aOther)
    : mValue(mozilla::Move(aOther.mValue))
  {}

  Nullable(const Nullable<T>& aOther)
    : mValue(aOther.mValue)
  {}

  void operator=(const Nullable<T>& aOther)
  {
    mValue = aOther.mValue;
  }

  void SetValue(T aValue) {
    mValue.reset();
    mValue.emplace(aValue);
  }

  
  
  
  T& SetValue() {
    if (mValue.isNothing()) {
      mValue.emplace();
    }
    return mValue.ref();
  }

  void SetNull() {
    mValue.reset();
  }

  const T& Value() const {
    return mValue.ref();
  }

  T& Value() {
    return mValue.ref();
  }

  bool IsNull() const {
    return mValue.isNothing();
  }

  bool Equals(const Nullable<T>& aOtherNullable) const
  {
    return mValue == aOtherNullable.mValue;
  }

  bool operator==(const Nullable<T>& aOtherNullable) const
  {
    return Equals(aOtherNullable);
  }

  bool operator!=(const Nullable<T>& aOtherNullable) const
  {
    return !Equals(aOtherNullable);
  }
};


template<typename T>
void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            Nullable<T>& aNullable,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  if (!aNullable.IsNull()) {
    ImplCycleCollectionTraverse(aCallback, aNullable.Value(), aName, aFlags);
  }
}

template<typename T>
void
ImplCycleCollectionUnlink(Nullable<T>& aNullable)
{
  if (!aNullable.IsNull()) {
    ImplCycleCollectionUnlink(aNullable.Value());
  }
}

} 
} 

#endif 
