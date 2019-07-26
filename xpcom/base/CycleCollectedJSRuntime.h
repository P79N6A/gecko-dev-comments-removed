





#ifndef mozilla_CycleCollectedJSRuntime_h__
#define mozilla_CycleCollectedJSRuntime_h__

#include "jsapi.h"

#include "nsDataHashtable.h"
#include "nsHashKeys.h"

class nsScriptObjectTracer;

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

public:
  void AddJSHolder(void* aHolder, nsScriptObjectTracer* aTracer);
  void RemoveJSHolder(void* aHolder);
#ifdef DEBUG
  bool TestJSHolder(void* aHolder);
  void SetObjectToUnlink(void* aObject) { mObjectToUnlink = aObject; }
  void AssertNoObjectsToTrace(void* aPossibleJSHolder);
#endif


protected:
  nsDataHashtable<nsPtrHashKey<void>, nsScriptObjectTracer*> mJSHolders;

private:
  JSRuntime* mJSRuntime;

#ifdef DEBUG
  void* mObjectToUnlink;
#endif
};

} 

#endif 
