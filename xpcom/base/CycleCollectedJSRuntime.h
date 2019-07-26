





#ifndef mozilla_CycleCollectedJSRuntime_h__
#define mozilla_CycleCollectedJSRuntime_h__

#include "jsapi.h"

namespace mozilla {

class CycleCollectedJSRuntime
{
protected:
  CycleCollectedJSRuntime(uint32_t aMaxbytes,
                          JSUseHelperThreads aUseHelperThreads);
  virtual ~CycleCollectedJSRuntime();

  JSRuntime* Runtime() const
  {
    MOZ_ASSERT(mJSRuntime);
    return mJSRuntime;
  }

private:
  JSRuntime* mJSRuntime;
};

} 

#endif 
