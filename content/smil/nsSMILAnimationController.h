





































#ifndef NS_SMILANIMATIONCONTROLLER_H_
#define NS_SMILANIMATIONCONTROLLER_H_

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsSMILTimeContainer.h"
#include "nsSMILCompositorTable.h"
#include "nsSMILMilestone.h"
#include "nsRefreshDriver.h"

struct nsSMILTargetIdentifier;
class nsISMILAnimationElement;
class nsIDocument;














class nsSMILAnimationController : public nsSMILTimeContainer,
                                  public nsARefreshObserver
{
public:
  nsSMILAnimationController(nsIDocument* aDoc);
  ~nsSMILAnimationController();

  
  void Disconnect();

  
  virtual void Pause(PRUint32 aType);
  virtual void Resume(PRUint32 aType);
  virtual nsSMILTime GetParentTime() const;

  
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

  virtual void WillRefresh(mozilla::TimeStamp aTime);

  
  void RegisterAnimationElement(nsISMILAnimationElement* aAnimationElement);
  void UnregisterAnimationElement(nsISMILAnimationElement* aAnimationElement);

  
  
  
  
  void Resample() { DoSample(false); }

  void SetResampleNeeded()
  {
    if (!mRunningSample) {
      if (!mResampleNeeded) {
        FlagDocumentNeedsFlush();
      }
      mResampleNeeded = true;
    }
  }

  
  void FlushResampleRequests()
  {
    if (!mResampleNeeded)
      return;

    Resample();
  }

  
  void OnPageShow();
  void OnPageHide();

  
  void Traverse(nsCycleCollectionTraversalCallback* aCallback);
  void Unlink();

  
  void NotifyRefreshDriverCreated(nsRefreshDriver* aRefreshDriver);
  void NotifyRefreshDriverDestroying(nsRefreshDriver* aRefreshDriver);

  
  bool HasRegisteredAnimations()
  { return mAnimationElementTable.Count() != 0; }

protected:
  
  typedef nsPtrHashKey<nsSMILTimeContainer> TimeContainerPtrKey;
  typedef nsTHashtable<TimeContainerPtrKey> TimeContainerHashtable;
  typedef nsPtrHashKey<nsISMILAnimationElement> AnimationElementPtrKey;
  typedef nsTHashtable<AnimationElementPtrKey> AnimationElementHashtable;

  struct SampleTimeContainerParams
  {
    TimeContainerHashtable* mActiveContainers;
    bool                    mSkipUnchangedContainers;
  };

  struct SampleAnimationParams
  {
    TimeContainerHashtable* mActiveContainers;
    nsSMILCompositorTable*  mCompositorTable;
  };

  struct GetMilestoneElementsParams
  {
    nsTArray<nsRefPtr<nsISMILAnimationElement> > mElements;
    nsSMILMilestone                              mMilestone;
  };

  
  PR_STATIC_CALLBACK(PLDHashOperator) CompositorTableEntryTraverse(
      nsSMILCompositor* aCompositor, void* aArg);

  
  nsRefreshDriver* GetRefreshDriver();

  
  void StartSampling(nsRefreshDriver* aRefreshDriver);
  void StopSampling(nsRefreshDriver* aRefreshDriver);

  
  void MaybeStartSampling(nsRefreshDriver* aRefreshDriver);

  
  virtual void DoSample();
  void DoSample(bool aSkipUnchangedContainers);

  void RewindElements();
  PR_STATIC_CALLBACK(PLDHashOperator) RewindNeeded(
      TimeContainerPtrKey* aKey, void* aData);
  PR_STATIC_CALLBACK(PLDHashOperator) RewindAnimation(
      AnimationElementPtrKey* aKey, void* aData);
  PR_STATIC_CALLBACK(PLDHashOperator) ClearRewindNeeded(
      TimeContainerPtrKey* aKey, void* aData);

  void DoMilestoneSamples();
  PR_STATIC_CALLBACK(PLDHashOperator) GetNextMilestone(
      TimeContainerPtrKey* aKey, void* aData);
  PR_STATIC_CALLBACK(PLDHashOperator) GetMilestoneElements(
      TimeContainerPtrKey* aKey, void* aData);

  PR_STATIC_CALLBACK(PLDHashOperator) SampleTimeContainer(
      TimeContainerPtrKey* aKey, void* aData);
  PR_STATIC_CALLBACK(PLDHashOperator) SampleAnimation(
      AnimationElementPtrKey* aKey, void* aData);
  static void SampleTimedElement(nsISMILAnimationElement* aElement,
                                 TimeContainerHashtable* aActiveContainers);
  static void AddAnimationToCompositorTable(
    nsISMILAnimationElement* aElement, nsSMILCompositorTable* aCompositorTable);
  static bool GetTargetIdentifierForAnimation(
      nsISMILAnimationElement* aAnimElem, nsSMILTargetIdentifier& aResult);

  
  virtual nsresult AddChild(nsSMILTimeContainer& aChild);
  virtual void     RemoveChild(nsSMILTimeContainer& aChild);

  void FlagDocumentNeedsFlush();

  
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

  AnimationElementHashtable  mAnimationElementTable;
  TimeContainerHashtable     mChildContainerTable;
  mozilla::TimeStamp         mCurrentSampleTime;
  mozilla::TimeStamp         mStartTime;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsSMILTime                 mAvgTimeBetweenSamples;

  bool                       mResampleNeeded;
  
  
  
  bool                       mDeferredStartSampling;
  bool                       mRunningSample;

  
  
  nsIDocument* mDocument;

  
  
  
  
  nsAutoPtr<nsSMILCompositorTable> mLastCompositorTable;
};

#endif 
