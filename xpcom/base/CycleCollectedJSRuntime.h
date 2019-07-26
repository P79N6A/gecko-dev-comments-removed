





#ifndef mozilla_CycleCollectedJSRuntime_h__
#define mozilla_CycleCollectedJSRuntime_h__

#include "jsapi.h"

#include "nsDataHashtable.h"
#include "nsHashKeys.h"

class nsCycleCollectionNoteRootCallback;
class nsScriptObjectTracer;

namespace mozilla {

class CycleCollectedJSRuntime
{
protected:
  CycleCollectedJSRuntime(uint32_t aMaxbytes,
                          JSUseHelperThreads aUseHelperThreads,
                          bool aExpectRootedGlobals);
  virtual ~CycleCollectedJSRuntime();

  JSRuntime* Runtime() const
  {
    MOZ_ASSERT(mJSRuntime);
    return mJSRuntime;
  }

  void MaybeTraceGlobals(JSTracer* aTracer) const;
  virtual void TraverseAdditionalNativeRoots(nsCycleCollectionNoteRootCallback& aCb) = 0;
private:
  void MaybeTraverseGlobals(nsCycleCollectionNoteRootCallback& aCb) const;

  void TraverseNativeRoots(nsCycleCollectionNoteRootCallback& aCb);

public:
  void AddJSHolder(void* aHolder, nsScriptObjectTracer* aTracer);
  void RemoveJSHolder(void* aHolder);
#ifdef DEBUG
  bool TestJSHolder(void* aHolder);
  void SetObjectToUnlink(void* aObject) { mObjectToUnlink = aObject; }
  void AssertNoObjectsToTrace(void* aPossibleJSHolder);
#endif

  
  static nsCycleCollectionParticipant *JSContextParticipant();

  bool NotifyLeaveMainThread() const;
  void NotifyEnterCycleCollectionThread() const;
  void NotifyLeaveCycleCollectionThread() const;
  void NotifyEnterMainThread() const;
  nsresult BeginCycleCollection(nsCycleCollectionNoteRootCallback &aCb);
  void FixWeakMappingGrayBits() const;
  bool NeedCollect() const;


protected:
  nsDataHashtable<nsPtrHashKey<void>, nsScriptObjectTracer*> mJSHolders;

private:
  JSRuntime* mJSRuntime;

#ifdef DEBUG
  void* mObjectToUnlink;
  bool mExpectUnrootedGlobals;
#endif
};

} 

#endif 
