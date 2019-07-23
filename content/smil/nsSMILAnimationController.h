





































#ifndef NS_SMILANIMATIONCONTROLLER_H_
#define NS_SMILANIMATIONCONTROLLER_H_

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsThreadUtils.h" 
#include "nsSMILTimeContainer.h"
#include "nsSMILCompositorTable.h"

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

  
  nsresult OnForceSample();
  void     FireForceSampleEvent();

  
  void     OnPageShow();
  void     OnPageHide();

  
  void     Traverse(nsCycleCollectionTraversalCallback* aCallback);
  void     Unlink();

protected:
  
  typedef nsPtrHashKey<nsSMILTimeContainer> TimeContainerPtrKey;
  typedef nsTHashtable<TimeContainerPtrKey> TimeContainerHashtable;
  typedef nsPtrHashKey<nsISMILAnimationElement> AnimationElementPtrKey;
  typedef nsTHashtable<AnimationElementPtrKey> AnimationElementHashtable;

  
  class ForceSampleEvent : public nsRunnable {
  public:
    ForceSampleEvent(nsSMILAnimationController &aAnimationController)
      : mAnimationController(&aAnimationController) { }

    NS_IMETHOD Run() {
      if (!mAnimationController)
        return NS_OK;

      return mAnimationController->OnForceSample();
    }
    void Expire() { mAnimationController = nsnull; }

  private:
    nsSMILAnimationController* mAnimationController;
  };

  struct SampleAnimationParams
  {
    TimeContainerHashtable* mActiveContainers;
    nsSMILCompositorTable*  mCompositorTable;
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
  PR_STATIC_CALLBACK(PLDHashOperator) SampleTimeContainers(
      TimeContainerPtrKey* aKey, void* aData);
  PR_STATIC_CALLBACK(PLDHashOperator) SampleAnimation(
      AnimationElementPtrKey* aKey, void* aData);
  PR_STATIC_CALLBACK(PLDHashOperator) AddAnimationToCompositorTable(
      AnimationElementPtrKey* aKey, void* aData);
  static void SampleTimedElement(nsISMILAnimationElement* aElement,
                                 TimeContainerHashtable* aActiveContainers);
  static void AddAnimationToCompositorTable(
    nsISMILAnimationElement* aElement, nsSMILCompositorTable* aCompositorTable);
  static PRBool GetCompositorKeyForAnimation(nsISMILAnimationElement* aAnimElem,
                                             nsSMILCompositorKey& aResult);

  
  virtual nsresult AddChild(nsSMILTimeContainer& aChild);
  virtual void     RemoveChild(nsSMILTimeContainer& aChild);

  
  static const PRUint32      kTimerInterval;
  nsCOMPtr<nsITimer>         mTimer;
  AnimationElementHashtable  mAnimationElementTable;
  TimeContainerHashtable     mChildContainerTable;
  nsRefPtr<ForceSampleEvent> mForceSampleEvent;
  
  
  
  nsIDocument* mDocument;

  
  
  
  
  nsAutoPtr<nsSMILCompositorTable> mLastCompositorTable;
};

nsSMILAnimationController* NS_NewSMILAnimationController(nsIDocument *doc);

#endif 
