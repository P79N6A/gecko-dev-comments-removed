





#ifndef mozilla_CycleCollectedJSRuntime_h__
#define mozilla_CycleCollectedJSRuntime_h__

#include "jsprvtd.h"
#include "jsapi.h"

#include "nsCycleCollectionParticipant.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"

class nsCycleCollectionNoteRootCallback;
class nsScriptObjectTracer;

namespace mozilla {

class JSGCThingParticipant: public nsCycleCollectionParticipant
{
public:
  static NS_METHOD RootImpl(void *n)
  {
    return NS_OK;
  }

  static NS_METHOD UnlinkImpl(void *n)
  {
    return NS_OK;
  }

  static NS_METHOD UnrootImpl(void *n)
  {
    return NS_OK;
  }

  static NS_METHOD_(void) UnmarkIfPurpleImpl(void *n)
  {
  }

  static NS_METHOD TraverseImpl(JSGCThingParticipant *that, void *n,
                                nsCycleCollectionTraversalCallback &cb);
};

class JSZoneParticipant : public nsCycleCollectionParticipant
{
public:

  static NS_METHOD RootImpl(void *p)
  {
    return NS_OK;
  }

  static NS_METHOD UnlinkImpl(void *p)
  {
    return NS_OK;
  }

  static NS_METHOD UnrootImpl(void *p)
  {
    return NS_OK;
  }

  static NS_METHOD_(void) UnmarkIfPurpleImpl(void *n)
  {
  }

  static NS_METHOD TraverseImpl(JSZoneParticipant *that, void *p,
                                nsCycleCollectionTraversalCallback &cb);
};

class CycleCollectedJSRuntime
{
  friend class JSGCThingParticipant;
  friend class JSZoneParticipant;
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

  void
  DescribeGCThing(bool aIsMarked, void* aThing, JSGCTraceKind aTraceKind,
                  nsCycleCollectionTraversalCallback& aCb) const;

  virtual bool
  DescribeCustomObjects(JSObject* aObject, js::Class* aClasp,
                        char (&aName)[72]) const = 0;

  void
  NoteGCThingJSChildren(void* aThing, JSGCTraceKind aTraceKind,
                        nsCycleCollectionTraversalCallback& aCb) const;

  void
  NoteGCThingXPCOMChildren(js::Class* aClasp, JSObject* aObj,
                           nsCycleCollectionTraversalCallback& aCb) const;

  virtual bool
  NoteCustomGCThingXPCOMChildren(js::Class* aClasp, JSObject* aObj,
                                 nsCycleCollectionTraversalCallback& aCb) const = 0;


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

  
  static nsCycleCollectionParticipant* JSContextParticipant();

  nsCycleCollectionParticipant* GCThingParticipant() const;
  nsCycleCollectionParticipant* ZoneParticipant() const;

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
  typedef const CCParticipantVTable<JSGCThingParticipant>::Type GCThingParticipantVTable;
  const GCThingParticipantVTable mGCThingCycleCollectorGlobal;

  typedef const CCParticipantVTable<JSZoneParticipant>::Type JSZoneParticipantVTable;
  const JSZoneParticipantVTable mJSZoneCycleCollectorGlobal;

  JSRuntime* mJSRuntime;

#ifdef DEBUG
  void* mObjectToUnlink;
  bool mExpectUnrootedGlobals;
#endif
};

} 

#endif 
