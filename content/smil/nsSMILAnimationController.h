





































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

struct nsSMILTargetIdentifier;
class nsISMILAnimationElement;
class nsIDocument;














class nsSMILAnimationController : public nsSMILTimeContainer
{
public:
  nsSMILAnimationController();
  ~nsSMILAnimationController();

  
  virtual void Pause(PRUint32 aType);
  virtual void Resume(PRUint32 aType);
  virtual nsSMILTime GetParentTime() const;

  
  void RegisterAnimationElement(nsISMILAnimationElement* aAnimationElement);
  void UnregisterAnimationElement(nsISMILAnimationElement* aAnimationElement);

  
  
  
  void Resample() { DoSample(PR_FALSE); }
  void SetResampleNeeded() { mResampleNeeded = PR_TRUE; }
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

protected:
  
  typedef nsPtrHashKey<nsSMILTimeContainer> TimeContainerPtrKey;
  typedef nsTHashtable<TimeContainerPtrKey> TimeContainerHashtable;
  typedef nsPtrHashKey<nsISMILAnimationElement> AnimationElementPtrKey;
  typedef nsTHashtable<AnimationElementPtrKey> AnimationElementHashtable;

  struct SampleTimeContainerParams
  {
    TimeContainerHashtable* mActiveContainers;
    PRBool                  mSkipUnchangedContainers;
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

  
  friend nsSMILAnimationController*
  NS_NewSMILAnimationController(nsIDocument* aDoc);
  nsresult    Init(nsIDocument* aDoc);

  
  PR_STATIC_CALLBACK(PLDHashOperator) CompositorTableEntryTraverse(
      nsSMILCompositor* aCompositor, void* aArg);

  
  static void Notify(nsITimer* aTimer, void* aClosure);
  nsresult    StartTimer();
  nsresult    StopTimer();

  
  virtual void DoSample();
  void DoSample(PRBool aSkipUnchangedContainers);
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
  static PRBool GetTargetIdentifierForAnimation(
      nsISMILAnimationElement* aAnimElem, nsSMILTargetIdentifier& aResult);

  
  virtual nsresult AddChild(nsSMILTimeContainer& aChild);
  virtual void     RemoveChild(nsSMILTimeContainer& aChild);

  
  static const PRUint32      kTimerInterval;
  nsCOMPtr<nsITimer>         mTimer;
  AnimationElementHashtable  mAnimationElementTable;
  TimeContainerHashtable     mChildContainerTable;
  PRPackedBool               mResampleNeeded;

  
  
  nsIDocument* mDocument;

  
  
  
  
  nsAutoPtr<nsSMILCompositorTable> mLastCompositorTable;
};

nsSMILAnimationController* NS_NewSMILAnimationController(nsIDocument *doc);

#endif 
