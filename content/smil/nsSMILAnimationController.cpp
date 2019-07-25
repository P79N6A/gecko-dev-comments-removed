






































#include "nsSMILAnimationController.h"
#include "nsSMILCompositor.h"
#include "nsSMILCSSProperty.h"
#include "nsCSSProps.h"
#include "nsComponentManagerUtils.h"
#include "nsITimer.h"
#include "nsIContent.h"
#include "mozilla/dom/Element.h"
#include "nsIDocument.h"
#include "nsISMILAnimationElement.h"
#include "nsIDOMSVGAnimationElement.h"
#include "nsSMILTimedElement.h"

using namespace mozilla::dom;





static nsRefreshDriver*
GetRefreshDriverForDoc(nsIDocument* aDoc)
{
  if (!aDoc) {
    NS_ERROR("Requesting refresh driver after document has disconnected!");
    return nsnull;
  }

  nsIPresShell* shell = aDoc->GetShell();
  if (!shell) {
    return nsnull;
  }

  nsPresContext* context = shell->GetPresContext();
  return context ? context->RefreshDriver() : nsnull;
}




nsSMILAnimationController::nsSMILAnimationController(nsIDocument* aDoc)
  : mAvgTimeBetweenSamples(0),
    mResampleNeeded(PR_FALSE),
    mDeferredStartSampling(PR_FALSE),
    mRunningSample(PR_FALSE),
    mDocument(aDoc)
{
  NS_ABORT_IF_FALSE(aDoc, "need a non-null document");

  mAnimationElementTable.Init();
  mChildContainerTable.Init();

  nsRefreshDriver* refreshDriver = GetRefreshDriverForDoc(mDocument);
  if (refreshDriver) {
    mStartTime = refreshDriver->MostRecentRefresh();
  } else {
    mStartTime = mozilla::TimeStamp::Now();
  }
  mCurrentSampleTime = mStartTime;

  Begin();
}

nsSMILAnimationController::~nsSMILAnimationController()
{
  NS_ASSERTION(mAnimationElementTable.Count() == 0,
               "Animation controller shouldn't be tracking any animation"
               " elements when it dies");
}

void
nsSMILAnimationController::Disconnect()
{
  NS_ABORT_IF_FALSE(mDocument, "disconnecting when we weren't connected...?");
  NS_ABORT_IF_FALSE(mRefCnt.get() == 1,
                    "Expecting to disconnect when doc is sole remaining owner");

  StopSampling(GetRefreshDriverForDoc(mDocument));

  mDocument = nsnull; 
}




void
nsSMILAnimationController::Pause(PRUint32 aType)
{
  nsSMILTimeContainer::Pause(aType);

  if (mPauseState) {
    mDeferredStartSampling = PR_FALSE;
    StopSampling(GetRefreshDriverForDoc(mDocument));
  }
}

void
nsSMILAnimationController::Resume(PRUint32 aType)
{
  PRBool wasPaused = (mPauseState != 0);
  
  
  mCurrentSampleTime = mozilla::TimeStamp::Now();

  nsSMILTimeContainer::Resume(aType);

  if (wasPaused && !mPauseState && mChildContainerTable.Count()) {
    Sample(); 
    MaybeStartSampling(GetRefreshDriverForDoc(mDocument));
  }
}

nsSMILTime
nsSMILAnimationController::GetParentTime() const
{
  return (nsSMILTime)(mCurrentSampleTime - mStartTime).ToMilliseconds();
}



NS_IMPL_ADDREF(nsSMILAnimationController)
NS_IMPL_RELEASE(nsSMILAnimationController)


void
nsSMILAnimationController::WillRefresh(mozilla::TimeStamp aTime)
{
  
  
  
  
  
  
  aTime = NS_MAX(mCurrentSampleTime, aTime);

  
  
  
  
  

  
  
  
  static const double SAMPLE_DUR_WEIGHTING = 0.2;
  
  
  static const double SAMPLE_DEV_THRESHOLD = 200.0;

  nsSMILTime elapsedTime =
    (nsSMILTime)(aTime - mCurrentSampleTime).ToMilliseconds();
  
  if (mAvgTimeBetweenSamples == 0) {
    mAvgTimeBetweenSamples = elapsedTime;
  
  } else if (elapsedTime > SAMPLE_DEV_THRESHOLD * mAvgTimeBetweenSamples) {
    NS_WARNING("Detected really long delay between samples, continuing from "
               "previous sample");
    mParentOffset += elapsedTime - mAvgTimeBetweenSamples;
  
  } else {
    
    
    mAvgTimeBetweenSamples =
      (nsSMILTime)(elapsedTime * SAMPLE_DUR_WEIGHTING +
      mAvgTimeBetweenSamples * (1.0 - SAMPLE_DUR_WEIGHTING));
  }
  mCurrentSampleTime = aTime;

  Sample();
}




void
nsSMILAnimationController::RegisterAnimationElement(
                                  nsISMILAnimationElement* aAnimationElement)
{
  mAnimationElementTable.PutEntry(aAnimationElement);
  if (mDeferredStartSampling) {
    mDeferredStartSampling = PR_FALSE;
    if (mChildContainerTable.Count()) {
      
      NS_ABORT_IF_FALSE(mAnimationElementTable.Count() == 1,
                        "we shouldn't have deferred sampling if we already had "
                        "animations registered");
      StartSampling(GetRefreshDriverForDoc(mDocument));
    } 
  }
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
nsSMILAnimationController::NotifyRefreshDriverCreated(
    nsRefreshDriver* aRefreshDriver)
{
  if (!mPauseState) {
    MaybeStartSampling(aRefreshDriver);
  }
}

void
nsSMILAnimationController::NotifyRefreshDriverDestroying(
    nsRefreshDriver* aRefreshDriver)
{
  if (!mPauseState && !mDeferredStartSampling) {
    StopSampling(aRefreshDriver);
  }
}




void
nsSMILAnimationController::StartSampling(nsRefreshDriver* aRefreshDriver)
{
  NS_ASSERTION(mPauseState == 0, "Starting sampling but controller is paused");
  NS_ASSERTION(!mDeferredStartSampling,
               "Started sampling but the deferred start flag is still set");
  if (aRefreshDriver) {
    NS_ABORT_IF_FALSE(!GetRefreshDriverForDoc(mDocument) ||
                      aRefreshDriver == GetRefreshDriverForDoc(mDocument),
                      "Starting sampling with wrong refresh driver");
    
    
    mCurrentSampleTime = mozilla::TimeStamp::Now();
    aRefreshDriver->AddRefreshObserver(this, Flush_Style);
  }
}

void
nsSMILAnimationController::StopSampling(nsRefreshDriver* aRefreshDriver)
{
  if (aRefreshDriver) {
    
    
    NS_ABORT_IF_FALSE(!GetRefreshDriverForDoc(mDocument) ||
                      aRefreshDriver == GetRefreshDriverForDoc(mDocument),
                      "Stopping sampling with wrong refresh driver");
    aRefreshDriver->RemoveRefreshObserver(this, Flush_Style);
  }
}

void
nsSMILAnimationController::MaybeStartSampling(nsRefreshDriver* aRefreshDriver)
{
  if (mDeferredStartSampling) {
    
    
    return;
  }

  if (mAnimationElementTable.Count()) {
    StartSampling(aRefreshDriver);
  } else {
    mDeferredStartSampling = PR_TRUE;
  }
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
  if (!mDocument) {
    NS_ERROR("Shouldn't be sampling after document has disconnected");
    return;
  }

  mResampleNeeded = PR_FALSE;
  
  
  mRunningSample = PR_TRUE;
  mDocument->FlushPendingNotifications(Flush_Style);

  
  
  
  RewindElements();
  DoMilestoneSamples();

  
  
  
  
  TimeContainerHashtable activeContainers;
  activeContainers.Init(mChildContainerTable.Count());
  SampleTimeContainerParams tcParams = { &activeContainers,
                                         aSkipUnchangedContainers };
  mChildContainerTable.EnumerateEntries(SampleTimeContainer, &tcParams);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  nsAutoPtr<nsSMILCompositorTable>
    currentCompositorTable(new nsSMILCompositorTable());
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
  mRunningSample = PR_FALSE;

  
  mLastCompositorTable = currentCompositorTable.forget();

  NS_ASSERTION(!mResampleNeeded, "Resample dirty flag set during sample!");
}

void
nsSMILAnimationController::RewindElements()
{
  PRBool rewindNeeded = PR_FALSE;
  mChildContainerTable.EnumerateEntries(RewindNeeded, &rewindNeeded);
  if (!rewindNeeded)
    return;

  mAnimationElementTable.EnumerateEntries(RewindAnimation, nsnull);
  mChildContainerTable.EnumerateEntries(ClearRewindNeeded, nsnull);
}

 PR_CALLBACK PLDHashOperator
nsSMILAnimationController::RewindNeeded(TimeContainerPtrKey* aKey,
                                        void* aData)
{
  NS_ABORT_IF_FALSE(aData,
      "Null data pointer during time container enumeration");
  PRBool* rewindNeeded = static_cast<PRBool*>(aData);

  nsSMILTimeContainer* container = aKey->GetKey();
  if (container->NeedsRewind()) {
    *rewindNeeded = PR_TRUE;
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

 PR_CALLBACK PLDHashOperator
nsSMILAnimationController::RewindAnimation(AnimationElementPtrKey* aKey,
                                           void* aData)
{
  nsISMILAnimationElement* animElem = aKey->GetKey();
  nsSMILTimeContainer* timeContainer = animElem->GetTimeContainer();
  if (timeContainer && timeContainer->NeedsRewind()) {
    animElem->TimedElement().Rewind();
  }

  return PL_DHASH_NEXT;
}

 PR_CALLBACK PLDHashOperator
nsSMILAnimationController::ClearRewindNeeded(TimeContainerPtrKey* aKey,
                                             void* aData)
{
  aKey->GetKey()->ClearNeedsRewind();
  return PL_DHASH_NEXT;
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
    container->MarkSeekFinished();
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

  NS_ABORT_IF_FALSE(!timeContainer->IsSeeking(),
      "Doing a regular sample but the time container is still seeking");
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
  
  Element* targetElem = aAnimElem->GetTargetElementContent();
  if (!targetElem)
    
    return PR_FALSE;

  
  
  
  nsCOMPtr<nsIAtom> attributeName;
  PRInt32 attributeNamespaceID;
  if (!aAnimElem->GetTargetAttributeName(&attributeNamespaceID,
                                         getter_AddRefs(attributeName)))
    
    return PR_FALSE;

  
  nsSMILTargetAttrType attributeType = aAnimElem->GetTargetAttributeType();

  
  
  
  PRBool isCSS = PR_FALSE;
  if (attributeType == eSMILTargetAttrType_auto) {
    if (attributeNamespaceID == kNameSpaceID_None) {
      nsCSSProperty prop =
        nsCSSProps::LookupProperty(nsDependentAtomString(attributeName));
      isCSS = nsSMILCSSProperty::IsPropertyAnimatable(prop);
    }
  } else {
    isCSS = (attributeType == eSMILTargetAttrType_CSS);
  }

  
  aResult.mElement = targetElem;
  aResult.mAttributeName = attributeName;
  aResult.mAttributeNamespaceID = attributeNamespaceID;
  aResult.mIsCSS = isCSS;

  return PR_TRUE;
}




nsresult
nsSMILAnimationController::AddChild(nsSMILTimeContainer& aChild)
{
  TimeContainerPtrKey* key = mChildContainerTable.PutEntry(&aChild);
  NS_ENSURE_TRUE(key, NS_ERROR_OUT_OF_MEMORY);

  if (!mPauseState && mChildContainerTable.Count() == 1) {
    Sample(); 
    MaybeStartSampling(GetRefreshDriverForDoc(mDocument));
  }

  return NS_OK;
}

void
nsSMILAnimationController::RemoveChild(nsSMILTimeContainer& aChild)
{
  mChildContainerTable.RemoveEntry(&aChild);

  if (!mPauseState && mChildContainerTable.Count() == 0) {
    StopSampling(GetRefreshDriverForDoc(mDocument));
  }
}
