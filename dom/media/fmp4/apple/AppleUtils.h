





#ifndef mozilla_AppleUtils_h
#define mozilla_AppleUtils_h

#include "mozilla/Attributes.h"

namespace mozilla {



template <class T>
class AutoCFRelease {
public:
  MOZ_IMPLICIT AutoCFRelease(T aRef)
    : mRef(aRef)
  {
  }
  ~AutoCFRelease()
  {
    if (mRef) {
      CFRelease(mRef);
    }
  }
  
  operator T()
  {
    return mRef;
  }
  
  T* receive()
  {
    return &mRef;
  }

private:
  
  AutoCFRelease<T>& operator=(const AutoCFRelease<T>&);
  T mRef;
};

} 

#endif 
