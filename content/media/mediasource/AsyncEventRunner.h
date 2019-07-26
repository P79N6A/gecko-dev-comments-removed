





#ifndef MOZILLA_ASYNCEVENTRUNNER_H_
#define MOZILLA_ASYNCEVENTRUNNER_H_

#include "nsThreadUtils.h"

namespace mozilla {

template <typename T>
class AsyncEventRunner : public nsRunnable
{
public:
  AsyncEventRunner(T* aTarget, const char* aName)
    : mTarget(aTarget)
    , mName(aName)
  {}

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    mTarget->DispatchSimpleEvent(mName);
    return NS_OK;
  }

private:
  nsRefPtr<T> mTarget;
  const char* mName;
};

} 
#endif 
