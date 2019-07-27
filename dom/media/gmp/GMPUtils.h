




#ifndef GMPUtils_h_
#define GMPUtils_h_

#include "mozilla/UniquePtr.h"

namespace mozilla {

template<typename T>
struct DestroyPolicy
{
  void operator()(T* aGMPObject) const {
    aGMPObject->Destroy();
  }
};

template<typename T>
using GMPUniquePtr = mozilla::UniquePtr<T, DestroyPolicy<T>>;

} 

#endif
