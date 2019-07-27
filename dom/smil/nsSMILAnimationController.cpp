




#include "nsSMILAnimationController.h"
#include "nsSMILCompositor.h"
#include "nsSMILCSSProperty.h"
#include "nsCSSProps.h"
#include "nsITimer.h"
#include "mozilla/dom/Element.h"
#include "nsIDocument.h"
#include "mozilla/dom/SVGAnimationElement.h"
#include "nsSMILTimedElement.h"
#include <algorithm>
#include "mozilla/AutoRestore.h"
#include "RestyleTracker.h"

using namespace mozilla;
using namespace mozilla::dom;







nsSMILAnimationController::nsSMILAnimationController(nsIDocument* aDoc)
  : mAvgTimeBetweenSamples(0),
    mResampleNeeded(false),
    mDeferredStartSampling(false),
    mRunningSample(false),
    mRegisteredWithRefreshDriver(false),
    mDocument(aDoc)
{
  MOZ_ASSERT(aDoc, "need a non-null document");

  nsRefreshDriver* refreshDriver = GetRefreshDriver();
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
  NS_ASSERTION(!mRegisteredWithRefreshDriver,
               "Leaving stale entry in refresh driver's observer list");
}

void
nsSMILAnimationController::Disconnect()
{
  MOZ_ASSERT(mDocument, "disconnecting when we weren't connected...?");
  MOZ_ASSERT(mRefCnt.get() == 1,
             "Expecting to disconnect when doc is sole remaining owner");
  NS_ASSERTION(mPauseState & nsSMILTimeContainer::PAUSE_PAGEHIDE,
               "Expecting to be paused for pagehide before disconnect");

  StopSampling(GetRefreshDriver());

  mDocument = nullptr; 
}




void
nsSMILAnimationController::Pause(uint32_t aType)
{
  nsSMILTimeContainer::Pause(aType);

  if (mPauseState) {
    mDeferredStartSampling = false;
    StopSampling(GetRefreshDriver());
  }
}

void
nsSMILAnimationController::Resume(uint32_t aType)
{
  bool wasPaused = (mPauseState != 0);
  
  
  mCurrentSampleTime = mozilla::TimeStamp::Now();

  nsSMILTimeContainer::Resume(aType);

  if (wasPaused && !mPauseState && mChildContainerTable.Count()) {
    MaybeStartSampling(GetRefreshDriver());
    Sample(); 
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
  
  
  
  
  
  
  aTime = std::max(mCurrentSampleTime, aTime);

  
  
  
  
  

  
  
  
  static const double SAMPLE_DUR_WEIGHTING = 0.2;
  
  
  static const double SAMPLE_DEV_THRESHOLD = 200.0;

  nsSMILTime elapsedTime =
    (nsSMILTime)(aTime - mCurrentSampleTime).ToMilliseconds();
  if (mAvgTimeBetweenSamples == 0) {
    
    mAvgTimeBetweenSamples = elapsedTime;
  } else {
    if (elapsedTime > SAMPLE_DEV_THRESHOLD * mAvgTimeBetweenSamples) {
      
      NS_WARNING("Detected really long delay between samples, continuing from "
                 "previous sample");
      mParentOffset += elapsedTime - mAvgTimeBetweenSamples;
    }
    
    
    mAvgTimeBetweenSamples =
      (nsSMILTime)(elapsedTime * SAMPLE_DUR_WEIGHTING +
      mAvgTimeBetweenSamples * (1.0 - SAMPLE_DUR_WEIGHTING));
  }
  mCurrentSampleTime = aTime;

  Sample();
}




void
nsSMILAnimationController::RegisterAnimationElement(
                                  SVGAnimationElement* aAnimationElement)
{
  mAnimationElementTable.PutEntry(aAnimationElement);
  if (mDeferredStartSampling) {
    mDeferredStartSampling = false;
    if (mChildContainerTable.Count()) {
      
      MOZ_ASSERT(mAnimationElementTable.Count() == 1,
                 "we shouldn't have deferred sampling if we already had "
                 "animations registered");
      StartSampling(GetRefreshDriver());
      Sample(); 
    } 
  }
}

void
nsSMILAnimationController::UnregisterAnimationElement(
                                  SVGAnimationElement* aAnimationElement)
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

 PLDHashOperator
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
  mLastCompositorTable = nullptr;
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
    MOZ_ASSERT(!mRegisteredWithRefreshDriver,
               "Redundantly registering with refresh driver");
    MOZ_ASSERT(!GetRefreshDriver() || aRefreshDriver == GetRefreshDriver(),
               "Starting sampling with wrong refresh driver");
    
    
    mCurrentSampleTime = mozilla::TimeStamp::Now();
    aRefreshDriver->AddRefreshObserver(this, Flush_Style);
    mRegisteredWithRefreshDriver = true;
  }
}

void
nsSMILAnimationController::StopSampling(nsRefreshDriver* aRefreshDriver)
{
  if (aRefreshDriver && mRegisteredWithRefreshDriver) {
    
    
    MOZ_ASSERT(!GetRefreshDriver() || aRefreshDriver == GetRefreshDriver(),
               "Stopping sampling with wrong refresh driver");
    aRefreshDriver->RemoveRefreshObserver(this, Flush_Style);
    mRegisteredWithRefreshDriver = false;
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
    mDeferredStartSampling = true;
  }
}




PLDHashOperator
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

PLDHashOperator
RemoveCompositorFromTable(nsSMILCompositor* aCompositor,
                          void* aData)
{
  nsSMILCompositorTable* lastCompositorTable =
    static_cast<nsSMILCompositorTable*>(aData);
  lastCompositorTable->RemoveEntry(aCompositor->GetKey());
  return PL_DHASH_NEXT;
}

PLDHashOperator
DoClearAnimationEffects(nsSMILCompositor* aCompositor,
                        void* )
{
  aCompositor->ClearAnimationEffects();
  return PL_DHASH_NEXT;
}

PLDHashOperator
DoComposeAttribute(nsSMILCompositor* aCompositor,
                   void* )
{
  aCompositor->ComposeAttribute();
  return PL_DHASH_NEXT;
}

void
nsSMILAnimationController::DoSample()
{
  DoSample(true); 
}

void
nsSMILAnimationController::DoSample(bool aSkipUnchangedContainers)
{
  if (!mDocument) {
    NS_ERROR("Shouldn't be sampling after document has disconnected");
    return;
  }
  if (mRunningSample) {
    NS_ERROR("Shouldn't be recursively sampling");
    return;
  }

  mResampleNeeded = false;
  
  
  AutoRestore<bool> autoRestoreRunningSample(mRunningSample);
  mRunningSample = true;
  
  
  
  
  RewindElements();
  DoMilestoneSamples();

  
  
  
  
  TimeContainerHashtable activeContainers(mChildContainerTable.Count());
  SampleTimeContainerParams tcParams = { &activeContainers,
                                         aSkipUnchangedContainers };
  mChildContainerTable.EnumerateEntries(SampleTimeContainer, &tcParams);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  nsAutoPtr<nsSMILCompositorTable>
    currentCompositorTable(new nsSMILCompositorTable(0));

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

    
    
    mLastCompositorTable->EnumerateEntries(DoClearAnimationEffects, nullptr);
  }

  
  if (currentCompositorTable->Count() == 0) {
    mLastCompositorTable = nullptr;
    return;
  }

  nsCOMPtr<nsIDocument> kungFuDeathGrip(mDocument);  
  mDocument->FlushPendingNotifications(Flush_Style);

  
  
  
  

  
  
  
  
  
  currentCompositorTable->EnumerateEntries(DoComposeAttribute, nullptr);

  
  mLastCompositorTable = currentCompositorTable.forget();

  NS_ASSERTION(!mResampleNeeded, "Resample dirty flag set during sample!");
}

void
nsSMILAnimationController::RewindElements()
{
  bool rewindNeeded = false;
  mChildContainerTable.EnumerateEntries(RewindNeeded, &rewindNeeded);
  if (!rewindNeeded)
    return;

  mAnimationElementTable.EnumerateEntries(RewindAnimation, nullptr);
  mChildContainerTable.EnumerateEntries(ClearRewindNeeded, nullptr);
}

 PLDHashOperator
nsSMILAnimationController::RewindNeeded(TimeContainerPtrKey* aKey,
                                        void* aData)
{
  MOZ_ASSERT(aData,
             "Null data pointer during time container enumeration");
  bool* rewindNeeded = static_cast<bool*>(aData);

  nsSMILTimeContainer* container = aKey->GetKey();
  if (container->NeedsRewind()) {
    *rewindNeeded = true;
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

 PLDHashOperator
nsSMILAnimationController::RewindAnimation(AnimationElementPtrKey* aKey,
                                           void* aData)
{
  SVGAnimationElement* animElem = aKey->GetKey();
  nsSMILTimeContainer* timeContainer = animElem->GetTimeContainer();
  if (timeContainer && timeContainer->NeedsRewind()) {
    animElem->TimedElement().Rewind();
  }

  return PL_DHASH_NEXT;
}

 PLDHashOperator
nsSMILAnimationController::ClearRewindNeeded(TimeContainerPtrKey* aKey,
                                             void* aData)
{
  aKey->GetKey()->ClearNeedsRewind();
  return PL_DHASH_NEXT;
}

void
nsSMILAnimationController::DoMilestoneSamples()
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsSMILTime sampleTime = INT64_MIN;

  while (true) {
    
    
    
    
    
    nsSMILMilestone nextMilestone(GetCurrentTime() + 1, true);
    mChildContainerTable.EnumerateEntries(GetNextMilestone, &nextMilestone);

    if (nextMilestone.mTime > GetCurrentTime()) {
      break;
    }

    GetMilestoneElementsParams params;
    params.mMilestone = nextMilestone;
    mChildContainerTable.EnumerateEntries(GetMilestoneElements, &params);
    uint32_t length = params.mElements.Length();

    
    
    
    
    
    
    
    
    
    sampleTime = std::max(nextMilestone.mTime, sampleTime);

    for (uint32_t i = 0; i < length; ++i) {
      SVGAnimationElement* elem = params.mElements[i].get();
      MOZ_ASSERT(elem, "nullptr animation element in list");
      nsSMILTimeContainer* container = elem->GetTimeContainer();
      if (!container)
        
        
        continue;

      nsSMILTimeValue containerTimeValue =
        container->ParentToContainerTime(sampleTime);
      if (!containerTimeValue.IsDefinite())
        continue;

      
      nsSMILTime containerTime = std::max<nsSMILTime>(0, containerTimeValue.GetMillis());

      if (nextMilestone.mIsEnd) {
        elem->TimedElement().SampleEndAt(containerTime);
      } else {
        elem->TimedElement().SampleAt(containerTime);
      }
    }
  }
}

 PLDHashOperator
nsSMILAnimationController::GetNextMilestone(TimeContainerPtrKey* aKey,
                                            void* aData)
{
  MOZ_ASSERT(aKey, "Null hash key for time container hash table");
  MOZ_ASSERT(aKey->GetKey(), "Null time container key in hash table");
  MOZ_ASSERT(aData,
             "Null data pointer during time container enumeration");

  nsSMILMilestone* nextMilestone = static_cast<nsSMILMilestone*>(aData);

  nsSMILTimeContainer* container = aKey->GetKey();
  if (container->IsPausedByType(nsSMILTimeContainer::PAUSE_BEGIN))
    return PL_DHASH_NEXT;

  nsSMILMilestone thisMilestone;
  bool didGetMilestone =
    container->GetNextMilestoneInParentTime(thisMilestone);
  if (didGetMilestone && thisMilestone < *nextMilestone) {
    *nextMilestone = thisMilestone;
  }

  return PL_DHASH_NEXT;
}

 PLDHashOperator
nsSMILAnimationController::GetMilestoneElements(TimeContainerPtrKey* aKey,
                                                void* aData)
{
  MOZ_ASSERT(aKey, "Null hash key for time container hash table");
  MOZ_ASSERT(aKey->GetKey(), "Null time container key in hash table");
  MOZ_ASSERT(aData,
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

 PLDHashOperator
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

 PLDHashOperator
nsSMILAnimationController::SampleAnimation(AnimationElementPtrKey* aKey,
                                           void* aData)
{
  NS_ENSURE_TRUE(aKey, PL_DHASH_NEXT);
  NS_ENSURE_TRUE(aKey->GetKey(), PL_DHASH_NEXT);
  NS_ENSURE_TRUE(aData, PL_DHASH_NEXT);

  SVGAnimationElement* animElem = aKey->GetKey();
  SampleAnimationParams* params = static_cast<SampleAnimationParams*>(aData);

  SampleTimedElement(animElem, params->mActiveContainers);
  AddAnimationToCompositorTable(animElem, params->mCompositorTable);

  return PL_DHASH_NEXT;
}

 void
nsSMILAnimationController::SampleTimedElement(
  SVGAnimationElement* aElement, TimeContainerHashtable* aActiveContainers)
{
  nsSMILTimeContainer* timeContainer = aElement->GetTimeContainer();
  if (!timeContainer)
    return;

  
  
  
  
  
  
  
  
  
  if (!aActiveContainers->GetEntry(timeContainer))
    return;

  nsSMILTime containerTime = timeContainer->GetCurrentTime();

  MOZ_ASSERT(!timeContainer->IsSeeking(),
             "Doing a regular sample but the time container is still seeking");
  aElement->TimedElement().SampleAt(containerTime);
}

 void
nsSMILAnimationController::AddAnimationToCompositorTable(
  SVGAnimationElement* aElement, nsSMILCompositorTable* aCompositorTable)
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

static inline bool
IsTransformAttribute(int32_t aNamespaceID, nsIAtom *aAttributeName)
{
  return aNamespaceID == kNameSpaceID_None &&
         (aAttributeName == nsGkAtoms::transform ||
          aAttributeName == nsGkAtoms::patternTransform ||
          aAttributeName == nsGkAtoms::gradientTransform);
}




 bool
nsSMILAnimationController::GetTargetIdentifierForAnimation(
    SVGAnimationElement* aAnimElem, nsSMILTargetIdentifier& aResult)
{
  
  Element* targetElem = aAnimElem->GetTargetElementContent();
  if (!targetElem)
    
    return false;

  
  
  
  nsCOMPtr<nsIAtom> attributeName;
  int32_t attributeNamespaceID;
  if (!aAnimElem->GetTargetAttributeName(&attributeNamespaceID,
                                         getter_AddRefs(attributeName)))
    
    return false;

  
  
  if (IsTransformAttribute(attributeNamespaceID, attributeName) !=
      (aAnimElem->Tag() == nsGkAtoms::animateTransform))
    return false;

  
  nsSMILTargetAttrType attributeType = aAnimElem->GetTargetAttributeType();

  
  
  
  bool isCSS = false;
  if (attributeType == eSMILTargetAttrType_auto) {
    if (attributeNamespaceID == kNameSpaceID_None) {
      
      
      if (attributeName == nsGkAtoms::width ||
          attributeName == nsGkAtoms::height) {
        isCSS = targetElem->GetNameSpaceID() != kNameSpaceID_SVG;
      } else {
        nsCSSProperty prop =
          nsCSSProps::LookupProperty(nsDependentAtomString(attributeName),
                                     nsCSSProps::eEnabledForAllContent);
        isCSS = nsSMILCSSProperty::IsPropertyAnimatable(prop);
      }
    }
  } else {
    isCSS = (attributeType == eSMILTargetAttrType_CSS);
  }

  
  aResult.mElement = targetElem;
  aResult.mAttributeName = attributeName;
  aResult.mAttributeNamespaceID = attributeNamespaceID;
  aResult.mIsCSS = isCSS;

  return true;
}

 PLDHashOperator
nsSMILAnimationController::AddStyleUpdate(AnimationElementPtrKey* aKey,
                                          void* aData)
{
  SVGAnimationElement* animElement = aKey->GetKey();
  RestyleTracker* restyleTracker = static_cast<RestyleTracker*>(aData);

  nsSMILTargetIdentifier key;
  if (!GetTargetIdentifierForAnimation(animElement, key)) {
    
    return PL_DHASH_NEXT;
  }

  
  
  
  
  nsRestyleHint rshint = key.mIsCSS ? eRestyle_StyleAttribute
                                    : eRestyle_SVGAttrAnimations;
  restyleTracker->AddPendingRestyle(key.mElement, rshint, nsChangeHint(0));

  return PL_DHASH_NEXT;
}

void
nsSMILAnimationController::AddStyleUpdatesTo(RestyleTracker& aTracker)
{
  mAnimationElementTable.EnumerateEntries(AddStyleUpdate, &aTracker);
}




nsresult
nsSMILAnimationController::AddChild(nsSMILTimeContainer& aChild)
{
  TimeContainerPtrKey* key = mChildContainerTable.PutEntry(&aChild);
  NS_ENSURE_TRUE(key, NS_ERROR_OUT_OF_MEMORY);

  if (!mPauseState && mChildContainerTable.Count() == 1) {
    MaybeStartSampling(GetRefreshDriver());
    Sample(); 
  }

  return NS_OK;
}

void
nsSMILAnimationController::RemoveChild(nsSMILTimeContainer& aChild)
{
  mChildContainerTable.RemoveEntry(&aChild);

  if (!mPauseState && mChildContainerTable.Count() == 0) {
    StopSampling(GetRefreshDriver());
  }
}


nsRefreshDriver*
nsSMILAnimationController::GetRefreshDriver()
{
  if (!mDocument) {
    NS_ERROR("Requesting refresh driver after document has disconnected!");
    return nullptr;
  }

  nsIPresShell* shell = mDocument->GetShell();
  if (!shell) {
    return nullptr;
  }

  nsPresContext* context = shell->GetPresContext();
  return context ? context->RefreshDriver() : nullptr;
}

void
nsSMILAnimationController::FlagDocumentNeedsFlush()
{
  mDocument->SetNeedStyleFlush();
}
