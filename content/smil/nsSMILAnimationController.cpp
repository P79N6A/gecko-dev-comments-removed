





































#include "nsSMILAnimationController.h"
#include "nsSMILCompositor.h"
#include "nsComponentManagerUtils.h"
#include "nsITimer.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsISMILAnimationElement.h"
#include "nsIDOMSVGAnimationElement.h"
#include "nsSMILTimedElement.h"













const PRUint32 nsSMILAnimationController::kTimerInterval = 22;




nsSMILAnimationController::nsSMILAnimationController()
  : mResampleNeeded(PR_FALSE),
    mDocument(nsnull)
{
  mAnimationElementTable.Init();
  mChildContainerTable.Init();
}

nsSMILAnimationController::~nsSMILAnimationController()
{
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nsnull;
  }

  NS_ASSERTION(mAnimationElementTable.Count() == 0,
               "Animation controller shouldn't be tracking any animation"
               " elements when it dies.");
}

nsSMILAnimationController* NS_NewSMILAnimationController(nsIDocument* aDoc)
{
  nsSMILAnimationController* animationController =
    new nsSMILAnimationController();
  NS_ENSURE_TRUE(animationController, nsnull);

  nsresult rv = animationController->Init(aDoc);
  if (NS_FAILED(rv)) {
    delete animationController;
    animationController = nsnull;
  }

  return animationController;
}

nsresult
nsSMILAnimationController::Init(nsIDocument* aDoc)
{
  NS_ENSURE_ARG_POINTER(aDoc);

  mTimer = do_CreateInstance("@mozilla.org/timer;1");
  NS_ENSURE_TRUE(mTimer, NS_ERROR_OUT_OF_MEMORY);

  
  mDocument = aDoc;

  Begin();

  return NS_OK;
}




void
nsSMILAnimationController::Pause(PRUint32 aType)
{
  nsSMILTimeContainer::Pause(aType);

  if (mPauseState) {
    StopTimer();
  }
}

void
nsSMILAnimationController::Resume(PRUint32 aType)
{
  PRBool wasPaused = (mPauseState != 0);

  nsSMILTimeContainer::Resume(aType);

  if (wasPaused && !mPauseState && mChildContainerTable.Count()) {
    StartTimer();
  }
}

nsSMILTime
nsSMILAnimationController::GetParentTime() const
{
  
  return PR_Now() / PR_USEC_PER_MSEC;
}




void
nsSMILAnimationController::RegisterAnimationElement(
                                  nsISMILAnimationElement* aAnimationElement)
{
  mAnimationElementTable.PutEntry(aAnimationElement);
}

void
nsSMILAnimationController::UnregisterAnimationElement(
                                  nsISMILAnimationElement* aAnimationElement)
{
  mAnimationElementTable.RemoveEntry(aAnimationElement);
}




void
nsSMILAnimationController::Resample()
{
  DoSample(PR_FALSE);
}




void
nsSMILAnimationController::OnPageShow()
{
  Resume(nsSMILTimeContainer::PAUSE_PAGEHIDE);
}

void
nsSMILAnimationController::OnPageHide()
{
  Pause(nsSMILTimeContainer::PAUSE_PAGEHIDE);
}




void
nsSMILAnimationController::Traverse(
    nsCycleCollectionTraversalCallback* aCallback)
{
  
  if (mLastCompositorTable) {
    mLastCompositorTable->EnumerateEntries(CompositorTableEntryTraverse,
                                           aCallback);
  }
}

 PR_CALLBACK PLDHashOperator
nsSMILAnimationController::CompositorTableEntryTraverse(
                                      nsSMILCompositor* aCompositor,
                                      void* aArg)
{
  nsCycleCollectionTraversalCallback* cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aArg);
  aCompositor->Traverse(cb);
  return PL_DHASH_NEXT;
}

void
nsSMILAnimationController::Unlink()
{
  mLastCompositorTable = nsnull;
}




 void
nsSMILAnimationController::Notify(nsITimer* timer, void* aClosure)
{
  nsSMILAnimationController* controller = (nsSMILAnimationController*)aClosure;

  NS_ASSERTION(controller->mTimer == timer,
               "nsSMILAnimationController::Notify called with incorrect timer");

  controller->Sample();
}

nsresult
nsSMILAnimationController::StartTimer()
{
  NS_ENSURE_TRUE(mTimer, NS_ERROR_FAILURE);
  NS_ASSERTION(mPauseState == 0, "Starting timer but controller is paused.");

  
  Sample();

  
  
  
  
  return mTimer->InitWithFuncCallback(nsSMILAnimationController::Notify,
                                      this,
                                      kTimerInterval,
                                      nsITimer::TYPE_REPEATING_SLACK);
}

nsresult
nsSMILAnimationController::StopTimer()
{
  NS_ENSURE_TRUE(mTimer, NS_ERROR_FAILURE);

  return mTimer->Cancel();
}




PR_CALLBACK PLDHashOperator
RemoveCompositorFromTable(nsSMILCompositor* aCompositor,
                          void* aData)
{
  nsSMILCompositorTable* lastCompositorTable =
    static_cast<nsSMILCompositorTable*>(aData);
  lastCompositorTable->RemoveEntry(aCompositor->GetKey());
  return PL_DHASH_NEXT;
}

PR_CALLBACK PLDHashOperator
DoClearAnimationEffects(nsSMILCompositor* aCompositor,
                        void* )
{
  aCompositor->ClearAnimationEffects();
  return PL_DHASH_NEXT;
}

PR_CALLBACK PLDHashOperator
DoComposeAttribute(nsSMILCompositor* aCompositor,
                   void* )
{
  aCompositor->ComposeAttribute();
  return PL_DHASH_NEXT;
}

void
nsSMILAnimationController::DoSample()
{
  DoSample(PR_TRUE); 
}

void
nsSMILAnimationController::DoSample(PRBool aSkipUnchangedContainers)
{
  
  mResampleNeeded = PR_FALSE;

  
  
  
  
  TimeContainerHashtable activeContainers;
  activeContainers.Init(mChildContainerTable.Count());
  SampleTimeContainerParams tcParams = { &activeContainers,
                                         aSkipUnchangedContainers };
  mChildContainerTable.EnumerateEntries(SampleTimeContainer, &tcParams);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  nsAutoPtr<nsSMILCompositorTable>
    currentCompositorTable(new nsSMILCompositorTable());
  if (!currentCompositorTable)
    return;
  currentCompositorTable->Init(0);

  SampleAnimationParams saParams = { &activeContainers,
                                     currentCompositorTable };
  nsresult rv = mAnimationElementTable.EnumerateEntries(SampleAnimation,
                                                        &saParams);
  if (NS_FAILED(rv)) {
    NS_WARNING("SampleAnimationParams failed");
  }
  activeContainers.Clear();

  
  if (mLastCompositorTable) {
    
    
    
    currentCompositorTable->EnumerateEntries(RemoveCompositorFromTable,
                                             mLastCompositorTable);

    
    
    mLastCompositorTable->EnumerateEntries(DoClearAnimationEffects, nsnull);
  }

  
  currentCompositorTable->EnumerateEntries(DoComposeAttribute, nsnull);

  
  mLastCompositorTable = currentCompositorTable.forget();

  NS_ASSERTION(!mResampleNeeded, "Resample dirty flag set during sample!");
}

 PR_CALLBACK PLDHashOperator
nsSMILAnimationController::SampleTimeContainer(TimeContainerPtrKey* aKey,
                                               void* aData)
{
  NS_ENSURE_TRUE(aKey, PL_DHASH_NEXT);
  NS_ENSURE_TRUE(aKey->GetKey(), PL_DHASH_NEXT);
  NS_ENSURE_TRUE(aData, PL_DHASH_NEXT);

  SampleTimeContainerParams* params =
    static_cast<SampleTimeContainerParams*>(aData);

  nsSMILTimeContainer* container = aKey->GetKey();
  if (container->NeedsSample() || !params->mSkipUnchangedContainers) {
    container->Sample();
    params->mActiveContainers->PutEntry(container);
  }

  return PL_DHASH_NEXT;
}

 PR_CALLBACK PLDHashOperator
nsSMILAnimationController::SampleAnimation(AnimationElementPtrKey* aKey,
                                           void* aData)
{
  NS_ENSURE_TRUE(aKey, PL_DHASH_NEXT);
  NS_ENSURE_TRUE(aKey->GetKey(), PL_DHASH_NEXT);
  NS_ENSURE_TRUE(aData, PL_DHASH_NEXT);

  nsISMILAnimationElement* animElem = aKey->GetKey();
  SampleAnimationParams* params = static_cast<SampleAnimationParams*>(aData);

  SampleTimedElement(animElem, params->mActiveContainers);
  AddAnimationToCompositorTable(animElem, params->mCompositorTable);

  return PL_DHASH_NEXT;
}

 void
nsSMILAnimationController::SampleTimedElement(
  nsISMILAnimationElement* aElement, TimeContainerHashtable* aActiveContainers)
{
  nsSMILTimeContainer* timeContainer = aElement->GetTimeContainer();
  if (!timeContainer)
    return;

  
  
  
  
  
  
  
  
  
  if (!aActiveContainers->GetEntry(timeContainer))
    return;

  nsSMILTime containerTime = timeContainer->GetCurrentTime();

  aElement->TimedElement().SampleAt(containerTime);
}

 void
nsSMILAnimationController::AddAnimationToCompositorTable(
  nsISMILAnimationElement* aElement, nsSMILCompositorTable* aCompositorTable)
{
  
  nsSMILCompositorKey key;
  if (!GetCompositorKeyForAnimation(aElement, key))
    
    return;

  nsSMILAnimationFunction& func = aElement->AnimationFunction();

  
  
  
  if (func.IsActiveOrFrozen()) {
    nsSMILCompositor* result = aCompositorTable->PutEntry(key);

    
    
    result->AddAnimationFunction(&func);
  }
}




 PRBool
nsSMILAnimationController::GetCompositorKeyForAnimation(
    nsISMILAnimationElement* aAnimElem, nsSMILCompositorKey& aResult)
{
  
  nsIContent* targetElem = aAnimElem->GetTargetElementContent();
  if (!targetElem)
    
    return PR_FALSE;

  
  
  
  
  
  nsIAtom* attributeName = aAnimElem->GetTargetAttributeName();
  if (!attributeName)
    
    return PR_FALSE;

  
  nsSMILTargetAttrType attributeType = aAnimElem->GetTargetAttributeType();

  
  
  
  
  
  
  if (attributeType == eSMILTargetAttrType_auto) {
    attributeType = (targetElem->IsAttributeMapped(attributeName))
                  ? eSMILTargetAttrType_CSS
                  : eSMILTargetAttrType_XML;
  }
  PRBool isCSS = (attributeType == eSMILTargetAttrType_CSS);

  
  aResult.mElement = targetElem;
  aResult.mAttributeName = attributeName;
  aResult.mIsCSS = isCSS;

  return PR_TRUE;
}




nsresult
nsSMILAnimationController::AddChild(nsSMILTimeContainer& aChild)
{
  TimeContainerPtrKey* key = mChildContainerTable.PutEntry(&aChild);
  NS_ENSURE_TRUE(key,NS_ERROR_OUT_OF_MEMORY);

  if (!mPauseState && mChildContainerTable.Count() == 1) {
    StartTimer();
  }

  return NS_OK;
}

void
nsSMILAnimationController::RemoveChild(nsSMILTimeContainer& aChild)
{
  mChildContainerTable.RemoveEntry(&aChild);

  if (!mPauseState && mChildContainerTable.Count() == 0) {
    StopTimer();
  }
}
