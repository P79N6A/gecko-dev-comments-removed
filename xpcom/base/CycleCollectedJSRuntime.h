





#ifndef mozilla_CycleCollectedJSRuntime_h__
#define mozilla_CycleCollectedJSRuntime_h__

#include "mozilla/MemoryReporting.h"
#include "jsapi.h"
#include "js/Class.h"

#include "nsCycleCollector.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"

class nsCycleCollectionNoteRootCallback;
class nsScriptObjectTracer;
class nsIException;

namespace mozilla {

class JSGCThingParticipant: public nsCycleCollectionParticipant
{
public:
  NS_IMETHOD Root(void *n)
  {
    return NS_OK;
  }

  NS_IMETHOD Unlink(void *n)
  {
    return NS_OK;
  }

  NS_IMETHOD Unroot(void *n)
  {
    return NS_OK;
  }

  NS_IMETHOD_(void) DeleteCycleCollectable(void *n)
  {
  }

  NS_IMETHOD Traverse(void *n, nsCycleCollectionTraversalCallback &cb);
};

class JSZoneParticipant : public nsCycleCollectionParticipant
{
public:
  MOZ_CONSTEXPR JSZoneParticipant(): nsCycleCollectionParticipant() {}

  NS_IMETHOD Root(void *p)
  {
    return NS_OK;
  }

  NS_IMETHOD Unlink(void *p)
  {
    return NS_OK;
  }

  NS_IMETHOD Unroot(void *p)
  {
    return NS_OK;
  }

  NS_IMETHOD_(void) DeleteCycleCollectable(void *n)
  {
  }

  NS_IMETHOD Traverse(void *p, nsCycleCollectionTraversalCallback &cb);
};

class IncrementalFinalizeRunnable;

class CycleCollectedJSRuntime
{
  friend class JSGCThingParticipant;
  friend class JSZoneParticipant;
  friend class IncrementalFinalizeRunnable;
protected:
  CycleCollectedJSRuntime(uint32_t aMaxbytes,
                          JSUseHelperThreads aUseHelperThreads);
  virtual ~CycleCollectedJSRuntime();

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
  void UnmarkSkippableJSHolders();

  virtual void TraverseAdditionalNativeRoots(nsCycleCollectionNoteRootCallback& aCb) {}
  virtual void TraceAdditionalNativeGrayRoots(JSTracer* aTracer) {}

  virtual void CustomGCCallback(JSGCStatus aStatus) {}
  virtual bool CustomContextCallback(JSContext* aCx, unsigned aOperation)
  {
    return true; 
  }

private:

  void
  DescribeGCThing(bool aIsMarked, void* aThing, JSGCTraceKind aTraceKind,
                  nsCycleCollectionTraversalCallback& aCb) const;

  virtual bool
  DescribeCustomObjects(JSObject* aObject, js::Class* aClasp,
                        char (&aName)[72]) const
  {
    return false; 
  }

  void
  NoteGCThingJSChildren(void* aThing, JSGCTraceKind aTraceKind,
                        nsCycleCollectionTraversalCallback& aCb) const;

  void
  NoteGCThingXPCOMChildren(js::Class* aClasp, JSObject* aObj,
                           nsCycleCollectionTraversalCallback& aCb) const;

  virtual bool
  NoteCustomGCThingXPCOMChildren(js::Class* aClasp, JSObject* aObj,
                                 nsCycleCollectionTraversalCallback& aCb) const
  {
    return false; 
  }

  enum TraverseSelect {
      TRAVERSE_CPP,
      TRAVERSE_FULL
  };

  void
  TraverseGCThing(TraverseSelect aTs, void* aThing,
                  JSGCTraceKind aTraceKind,
                  nsCycleCollectionTraversalCallback& aCb);

  void
  TraverseZone(JS::Zone* aZone, nsCycleCollectionTraversalCallback& aCb);

  static void
  TraverseObjectShim(void* aData, void* aThing);

  void TraverseNativeRoots(nsCycleCollectionNoteRootCallback& aCb);

  static void TraceBlackJS(JSTracer* aTracer, void* aData);
  static void TraceGrayJS(JSTracer* aTracer, void* aData);
  static void GCCallback(JSRuntime* aRuntime, JSGCStatus aStatus, void* aData);
  static bool ContextCallback(JSContext* aCx, unsigned aOperation,
                              void* aData);

  virtual void TraceNativeBlackRoots(JSTracer* aTracer) { };
  void TraceNativeGrayRoots(JSTracer* aTracer);

  enum DeferredFinalizeType {
    FinalizeIncrementally,
    FinalizeNow,
  };

  void FinalizeDeferredThings(DeferredFinalizeType aType);

  void OnGC(JSGCStatus aStatus);
  bool OnContext(JSContext* aCx, unsigned aOperation);

public:
  void AddJSHolder(void* aHolder, nsScriptObjectTracer* aTracer);
  void RemoveJSHolder(void* aHolder);
#ifdef DEBUG
  bool IsJSHolder(void* aHolder);
  void SetObjectToUnlink(void* aObject) { mObjectToUnlink = aObject; }
  void AssertNoObjectsToTrace(void* aPossibleJSHolder);
#endif

  already_AddRefed<nsIException> GetPendingException() const;
  void SetPendingException(nsIException* aException);

  nsCycleCollectionParticipant* GCThingParticipant();
  nsCycleCollectionParticipant* ZoneParticipant();

  nsresult BeginCycleCollection(nsCycleCollectionNoteRootCallback &aCb);
  bool UsefulToMergeZones() const;
  void FixWeakMappingGrayBits() const;
  bool NeedCollect() const;
  void Collect(uint32_t reason) const;

  virtual void PrepareForForgetSkippable() {}
  virtual void PrepareForCollection() {}

  void DeferredFinalize(DeferredFinalizeAppendFunction aAppendFunc,
                        DeferredFinalizeFunction aFunc,
                        void* aThing);
  void DeferredFinalize(nsISupports* aSupports);

  void DumpJSHeap(FILE* aFile);
  
  virtual void DispatchDeferredDeletion(bool aContinuation) = 0;

  JSRuntime* Runtime() const
  {
    MOZ_ASSERT(mJSRuntime);
    return mJSRuntime;
  }

  
  
  static CycleCollectedJSRuntime*
  Get();

private:
  JSGCThingParticipant mGCThingCycleCollectorGlobal;

  JSZoneParticipant mJSZoneCycleCollectorGlobal;

  JSRuntime* mJSRuntime;

  nsDataHashtable<nsPtrHashKey<void>, nsScriptObjectTracer*> mJSHolders;

  nsTArray<nsISupports*> mDeferredSupports;
  typedef nsDataHashtable<nsFuncPtrHashKey<DeferredFinalizeFunction>, void*>
    DeferredFinalizerTable;
  DeferredFinalizerTable mDeferredFinalizerTable;

  nsRefPtr<IncrementalFinalizeRunnable> mFinalizeRunnable;

  nsCOMPtr<nsIException> mPendingException;

#ifdef DEBUG
  void* mObjectToUnlink;
#endif
};

} 

#endif 
