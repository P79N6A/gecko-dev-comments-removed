




#ifndef NS_SMILANIMATIONCONTROLLER_H_
#define NS_SMILANIMATIONCONTROLLER_H_

#include "mozilla/Attributes.h"
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
class nsIDocument;

namespace mozilla {
namespace dom {
class SVGAnimationElement;
}
}














class nsSMILAnimationController MOZ_FINAL : public nsSMILTimeContainer,
                                            public nsARefreshObserver
{
public:
  explicit nsSMILAnimationController(nsIDocument* aDoc);

  
  void Disconnect();

  
  virtual void Pause(uint32_t aType) MOZ_OVERRIDE;
  virtual void Resume(uint32_t aType) MOZ_OVERRIDE;
  virtual nsSMILTime GetParentTime() const MOZ_OVERRIDE;

  
  NS_IMETHOD_(MozExternalRefCountType) AddRef() MOZ_OVERRIDE;
  NS_IMETHOD_(MozExternalRefCountType) Release() MOZ_OVERRIDE;

  virtual void WillRefresh(mozilla::TimeStamp aTime) MOZ_OVERRIDE;

  
  void RegisterAnimationElement(mozilla::dom::SVGAnimationElement* aAnimationElement);
  void UnregisterAnimationElement(mozilla::dom::SVGAnimationElement* aAnimationElement);

  
  
  
  
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
  ~nsSMILAnimationController();

  
  typedef nsPtrHashKey<nsSMILTimeContainer> TimeContainerPtrKey;
  typedef nsTHashtable<TimeContainerPtrKey> TimeContainerHashtable;
  typedef nsPtrHashKey<mozilla::dom::SVGAnimationElement> AnimationElementPtrKey;
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
    nsTArray<nsRefPtr<mozilla::dom::SVGAnimationElement> > mElements;
    nsSMILMilestone                                        mMilestone;
  };

  
  static PLDHashOperator CompositorTableEntryTraverse(
      nsSMILCompositor* aCompositor, void* aArg);

  
  nsRefreshDriver* GetRefreshDriver();

  
  void StartSampling(nsRefreshDriver* aRefreshDriver);
  void StopSampling(nsRefreshDriver* aRefreshDriver);

  
  void MaybeStartSampling(nsRefreshDriver* aRefreshDriver);

  
  virtual void DoSample() MOZ_OVERRIDE;
  void DoSample(bool aSkipUnchangedContainers);

  void RewindElements();
  static PLDHashOperator RewindNeeded(
      TimeContainerPtrKey* aKey, void* aData);
  static PLDHashOperator RewindAnimation(
      AnimationElementPtrKey* aKey, void* aData);
  static PLDHashOperator ClearRewindNeeded(
      TimeContainerPtrKey* aKey, void* aData);

  void DoMilestoneSamples();
  static PLDHashOperator GetNextMilestone(
      TimeContainerPtrKey* aKey, void* aData);
  static PLDHashOperator GetMilestoneElements(
      TimeContainerPtrKey* aKey, void* aData);

  static PLDHashOperator SampleTimeContainer(
      TimeContainerPtrKey* aKey, void* aData);
  static PLDHashOperator SampleAnimation(
      AnimationElementPtrKey* aKey, void* aData);
  static void SampleTimedElement(mozilla::dom::SVGAnimationElement* aElement,
                                 TimeContainerHashtable* aActiveContainers);
  static void AddAnimationToCompositorTable(
    mozilla::dom::SVGAnimationElement* aElement, nsSMILCompositorTable* aCompositorTable);
  static bool GetTargetIdentifierForAnimation(
      mozilla::dom::SVGAnimationElement* aAnimElem, nsSMILTargetIdentifier& aResult);

  
  virtual nsresult AddChild(nsSMILTimeContainer& aChild) MOZ_OVERRIDE;
  virtual void     RemoveChild(nsSMILTimeContainer& aChild) MOZ_OVERRIDE;

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

  
  bool                       mRegisteredWithRefreshDriver;

  
  
  nsIDocument* mDocument;

  
  
  
  
  nsAutoPtr<nsSMILCompositorTable> mLastCompositorTable;
};

#endif 
