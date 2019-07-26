







#ifndef mozilla_dom_UnionMember_h
#define mozilla_dom_UnionMember_h

#include "mozilla/Alignment.h"

namespace mozilla {
namespace dom {



template<class T>
class UnionMember
{
  AlignedStorage2<T> mStorage;

public:
  T& SetValue()
  {
    new (mStorage.addr()) T();
    return *mStorage.addr();
  }
  template <typename T1>
  T& SetValue(const T1& aValue)
  {
    new (mStorage.addr()) T(aValue);
    return *mStorage.addr();
  }
  template<typename T1, typename T2>
  T& SetValue(const T1& aValue1, const T2& aValue2)
  {
    new (mStorage.addr()) T(aValue1, aValue2);
    return *mStorage.addr();
  }
  T& Value()
  {
    return *mStorage.addr();
  }
  const T& Value() const
  {
    return *mStorage.addr();
  }
  void Destroy()
  {
    mStorage.addr()->~T();
  }
};

} 
} 

#endif 
