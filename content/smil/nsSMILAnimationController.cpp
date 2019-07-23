





































#include "nsSMILAnimationController.h"
#include "nsSMILCompositor.h"
#include "nsSMILCSSProperty.h"
#include "nsCSSProps.h"
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
               " elements when it dies");
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
  NS_ASSERTION(mPauseState == 0, "Starting timer but controller is paused");

  
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
TransferCachedBaseValue(nsSMILCompositor* aCompositor,
                        void* aData)
{
  nsSMILCompositorTable* lastCompositorTable =
    static_cast<nsSMILCompositorTable*>(aData);
  nsSMILCompositor* lastCompositor =
    lastCompositorTable->GetEntry(aCompositor->GetKey());

  if (lastCompositor) {
    aCompositor->StealCachedBaseValue(lastCompositor);
  }

  return PL_DHASH_NEXT;  
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

  
  DoMilestoneSamples();

  
  
  
  
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
  mAnimationElementTable.EnumerateEntries(SampleAnimation,
                                          &saParams);
  activeContainers.Clear();

  
  
  
  if (mLastCompositorTable) {
    
    currentCompositorTable->EnumerateEntries(TransferCachedBaseValue,
                                             mLastCompositorTable);

    
    
    
    currentCompositorTable->EnumerateEntries(RemoveCompositorFromTable,
                                             mLastCompositorTable);

    
    
    mLastCompositorTable->EnumerateEntries(DoClearAnimationEffects, nsnull);
  }

  
  
  
  
  
  currentCompositorTable->EnumerateEntries(DoComposeAttribute, nsnull);

  
  mLastCompositorTable = currentCompositorTable.forget();

  NS_ASSERTION(!mResampleNeeded, "Resample dirty flag set during sample!");
}

void
nsSMILAnimationController::DoMilestoneSamples()
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsSMILTime sampleTime = LL_MININT;

  while (PR_TRUE) {
    
    
    
    
    
    nsSMILMilestone nextMilestone(GetCurrentTime() + 1, PR_TRUE);
    mChildContainerTable.EnumerateEntries(GetNextMilestone, &nextMilestone);

    if (nextMilestone.mTime > GetCurrentTime()) {
      break;
    }

    GetMilestoneElementsParams params;
    params.mMilestone = nextMilestone;
    mChildContainerTable.EnumerateEntries(GetMilestoneElements, &params);
    PRUint32 length = params.mElements.Length();

    
    
    
    
    
    
    
    
    
    sampleTime = PR_MAX(nextMilestone.mTime, sampleTime);

    for (PRUint32 i = 0; i < length; ++i) {
      nsISMILAnimationElement* elem = params.mElements[i].get();
      NS_ABORT_IF_FALSE(elem, "NULL animation element in list");
      nsSMILTimeContainer* container = elem->GetTimeContainer();
      if (!container)
        
        
        continue;

      nsSMILTimeValue containerTimeValue =
        container->ParentToContainerTime(sampleTime);
      if (!containerTimeValue.IsResolved())
        continue;

      
      nsSMILTime containerTime = PR_MAX(0, containerTimeValue.GetMillis());

      if (nextMilestone.mIsEnd) {
        elem->TimedElement().SampleEndAt(containerTime);
      } else {
        elem->TimedElement().SampleAt(containerTime);
      }
    }
  }
}

 PR_CALLBACK PLDHashOperator
nsSMILAnimationController::GetNextMilestone(TimeContainerPtrKey* aKey,
                                            void* aData)
{
  NS_ABORT_IF_FALSE(aKey, "Null hash key for time container hash table");
  NS_ABORT_IF_FALSE(aKey->GetKey(), "Null time container key in hash table");
  NS_ABORT_IF_FALSE(aData,
      "Null data pointer during time container enumeration");

  nsSMILMilestone* nextMilestone = static_cast<nsSMILMilestone*>(aData);

  nsSMILTimeContainer* container = aKey->GetKey();
  if (container->IsPausedByType(nsSMILTimeContainer::PAUSE_BEGIN))
    return PL_DHASH_NEXT;

  nsSMILMilestone thisMilestone;
  PRBool didGetMilestone =
    container->GetNextMilestoneInParentTime(thisMilestone);
  if (didGetMilestone && thisMilestone < *nextMilestone) {
    *nextMilestone = thisMilestone;
  }

  return PL_DHASH_NEXT;
}

 PR_CALLBACK PLDHashOperator
nsSMILAnimationController::GetMilestoneElements(TimeContainerPtrKey* aKey,
                                                void* aData)
{
  NS_ABORT_IF_FALSE(aKey, "Null hash key for time container hash table");
  NS_ABORT_IF_FALSE(aKey->GetKey(), "Null time container key in hash table");
  NS_ABORT_IF_FALSE(aData,
      "Null data pointer during time container enumeration");

  GetMilestoneElementsParams* params =
    static_cast<GetMilestoneElementsParams*>(aData);

  nsSMILTimeContainer* container = aKey->GetKey();
  if (container->IsPausedByType(nsSMILTimeContainer::PAUSE_BEGIN))
    return PL_DHASH_NEXT;

  container->PopMilestoneElementsAtMilestone(params->mMilestone,
                                             params->mElements);

  return PL_DHASH_NEXT;
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
  if (!container->IsPausedByType(nsSMILTimeContainer::PAUSE_BEGIN) &&
      (container->NeedsSample() || !params->mSkipUnchangedContainers)) {
    container->ClearMilestones();
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
  
  nsSMILTargetIdentifier key;
  if (!GetTargetIdentifierForAnimation(aElement, key))
    
    return;

  nsSMILAnimationFunction& func = aElement->AnimationFunction();

  
  
  
  if (func.IsActiveOrFrozen()) {
    
    
    nsSMILCompositor* result = aCompositorTable->PutEntry(key);
    result->AddAnimationFunction(&func);

  } else if (func.HasChanged()) {
    
    
    
    
    
    nsSMILCompositor* result = aCompositorTable->PutEntry(key);
    result->ToggleForceCompositing();

    
    
    
    func.ClearHasChanged();
  }
}




 PRBool
nsSMILAnimationController::GetTargetIdentifierForAnimation(
    nsISMILAnimationElement* aAnimElem, nsSMILTargetIdentifier& aResult)
{
  
  nsIContent* targetElem = aAnimElem->GetTargetElementContent();
  if (!targetElem)
    
    return PR_FALSE;

  
  
  
  
  
  nsIAtom* attributeName = aAnimElem->GetTargetAttributeName();
  if (!attributeName)
    
    return PR_FALSE;

  
  nsSMILTargetAttrType attributeType = aAnimElem->GetTargetAttributeType();

  
  
  
  PRBool isCSS;
  if (attributeType == eSMILTargetAttrType_auto) {
    nsAutoString attributeNameStr;
    attributeName->ToString(attributeNameStr);
    nsCSSProperty prop = nsCSSProps::LookupProperty(attributeNameStr);
    isCSS = nsSMILCSSProperty::IsPropertyAnimatable(prop);
  } else {
    isCSS = (attributeType == eSMILTargetAttrType_CSS);
  }

  
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
