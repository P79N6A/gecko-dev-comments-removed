






































#ifndef mozilla_AutoRestore_h_
#define mozilla_AutoRestore_h_

#include "mozilla/GuardObjects.h"

namespace mozilla {

  











  template <class T>
  class NS_STACK_CLASS AutoRestore
  {
  private:
    T& mLocation;
    T mValue;
    MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER
  public:
    AutoRestore(T& aValue MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM)
      : mLocation(aValue), mValue(aValue)
    {
      MOZILLA_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoRestore() { mLocation = mValue; }
  };

} 

#endif 
