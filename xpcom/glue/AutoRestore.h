






#ifndef mozilla_AutoRestore_h_
#define mozilla_AutoRestore_h_

#include "mozilla/Attributes.h" 
#include "mozilla/GuardObjects.h"

namespace mozilla {













template<class T>
class MOZ_STACK_CLASS AutoRestore
{
private:
  T& mLocation;
  T mValue;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
public:
  AutoRestore(T& aValue MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mLocation(aValue)
    , mValue(aValue)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  }
  ~AutoRestore()
  {
    mLocation = mValue;
  }
};

} 

#endif 
