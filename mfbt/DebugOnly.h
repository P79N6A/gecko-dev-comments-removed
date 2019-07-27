










#ifndef mozilla_DebugOnly_h
#define mozilla_DebugOnly_h

#include "mozilla/Attributes.h"

namespace mozilla {


















template<typename T>
class DebugOnly
{
public:
#ifdef DEBUG
  T value;

  DebugOnly() { }
  MOZ_IMPLICIT DebugOnly(const T& aOther) : value(aOther) { }
  DebugOnly(const DebugOnly& aOther) : value(aOther.value) { }
  DebugOnly& operator=(const T& aRhs) {
    value = aRhs;
    return *this;
  }

  void operator++(int) { value++; }
  void operator--(int) { value--; }

  
  
  

  T* operator&() { return &value; }

  operator T&() { return value; }
  operator const T&() const { return value; }

  T& operator->() { return value; }
  const T& operator->() const { return value; }

#else
  DebugOnly() { }
  MOZ_IMPLICIT DebugOnly(const T&) { }
  DebugOnly(const DebugOnly&) { }
  DebugOnly& operator=(const T&) { return *this; }
  void operator++(int) { }
  void operator--(int) { }
  DebugOnly& operator+=(const T&) { return *this; }
  DebugOnly& operator-=(const T&) { return *this; }
#endif

  




  ~DebugOnly() {}
};

} 

#endif 
