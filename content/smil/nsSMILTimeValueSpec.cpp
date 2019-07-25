




































#include "nsSMILTimeValueSpec.h"
#include "nsSMILInterval.h"
#include "nsSMILTimeContainer.h"
#include "nsSMILTimeValue.h"
#include "nsSMILTimedElement.h"
#include "nsSMILInstanceTime.h"
#include "nsSMILParserUtils.h"
#include "nsISMILAnimationElement.h"
#include "nsContentUtils.h"
#include "nsIEventListenerManager.h"
#include "nsIDOMEventGroup.h"
#include "nsGUIEvent.h"
#include "nsString.h"




NS_IMPL_ISUPPORTS1(nsSMILTimeValueSpec::EventListener, nsIDOMEventListener)

NS_IMETHODIMP
nsSMILTimeValueSpec::EventListener::HandleEvent(nsIDOMEvent* aEvent)
{
  if (mSpec) {
    mSpec->HandleEvent(aEvent);
  }
  return NS_OK;
}




#ifdef _MSC_VER



#pragma warning(push)
#pragma warning(disable:4355)
#endif
nsSMILTimeValueSpec::nsSMILTimeValueSpec(nsSMILTimedElement& aOwner,
                                         PRBool aIsBegin)
  : mOwner(&aOwner),
    mIsBegin(aIsBegin),
    mReferencedElement(this)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
{
}

nsSMILTimeValueSpec::~nsSMILTimeValueSpec()
{
  UnregisterFromReferencedElement(mReferencedElement.get());
  if (mEventListener) {
    mEventListener->Disconnect();
    mEventListener = nsnull;
  }
}

nsresult
nsSMILTimeValueSpec::SetSpec(const nsAString& aStringSpec,
                             nsIContent* aContextNode)
{
  nsSMILTimeValueSpecParams params;
  nsresult rv =
    nsSMILParserUtils::ParseTimeValueSpecParams(aStringSpec, params);

  if (NS_FAILED(rv))
    return rv;

  mParams = params;

  
  
  
  
  if (mParams.mType == nsSMILTimeValueSpecParams::OFFSET ||
      (!mIsBegin && mParams.mType == nsSMILTimeValueSpecParams::INDEFINITE)) {
    mOwner->AddInstanceTime(new nsSMILInstanceTime(mParams.mOffset), mIsBegin);
  }

  ResolveReferences(aContextNode);

  return rv;
}

void
nsSMILTimeValueSpec::ResolveReferences(nsIContent* aContextNode)
{
  if (mParams.mType != nsSMILTimeValueSpecParams::SYNCBASE &&
      mParams.mType != nsSMILTimeValueSpecParams::EVENT)
    return;

  NS_ABORT_IF_FALSE(aContextNode,
      "null context node for resolving timing references against");

  
  
  if (!aContextNode->IsInDoc())
    return;

  
  
  
  nsRefPtr<nsIContent> oldReferencedContent = mReferencedElement.get();

  
  NS_ABORT_IF_FALSE(mParams.mDependentElemID, "NULL dependent element id");
  nsString idStr;
  mParams.mDependentElemID->ToString(idStr);
  mReferencedElement.ResetWithID(aContextNode, idStr);
  UpdateReferencedElement(oldReferencedContent, mReferencedElement.get());
}

PRBool
nsSMILTimeValueSpec::IsEventBased() const
{
  return mParams.mType == nsSMILTimeValueSpecParams::EVENT ||
         mParams.mType == nsSMILTimeValueSpecParams::REPEAT ||
         mParams.mType == nsSMILTimeValueSpecParams::ACCESSKEY;
}

void
nsSMILTimeValueSpec::HandleNewInterval(nsSMILInterval& aInterval,
                                       const nsSMILTimeContainer* aSrcContainer)
{
  const nsSMILInstanceTime& baseInstance = mParams.mSyncBegin
    ? *aInterval.Begin() : *aInterval.End();
  nsSMILTimeValue newTime =
    ConvertBetweenTimeContainers(baseInstance.Time(), aSrcContainer);

  
  if (newTime.IsResolved()) {
    newTime.SetMillis(newTime.GetMillis() + mParams.mOffset.GetMillis());
  }

  
  nsRefPtr<nsSMILInstanceTime> newInstance =
    new nsSMILInstanceTime(newTime, nsSMILInstanceTime::SOURCE_SYNCBASE, this,
                           &aInterval);
  mOwner->AddInstanceTime(newInstance, mIsBegin);
}

void
nsSMILTimeValueSpec::HandleChangedInstanceTime(
    const nsSMILInstanceTime& aBaseTime,
    const nsSMILTimeContainer* aSrcContainer,
    nsSMILInstanceTime& aInstanceTimeToUpdate,
    PRBool aObjectChanged)
{
  
  
  if (aInstanceTimeToUpdate.IsFixedTime())
    return;

  nsSMILTimeValue updatedTime =
    ConvertBetweenTimeContainers(aBaseTime.Time(), aSrcContainer);

  
  if (updatedTime.IsResolved()) {
    updatedTime.SetMillis(updatedTime.GetMillis() +
                          mParams.mOffset.GetMillis());
  }

  
  
  if (aInstanceTimeToUpdate.Time() != updatedTime || aObjectChanged) {
    mOwner->UpdateInstanceTime(&aInstanceTimeToUpdate, updatedTime, mIsBegin);
  }
}

void
nsSMILTimeValueSpec::HandleDeletedInstanceTime(
    nsSMILInstanceTime &aInstanceTime)
{
  mOwner->RemoveInstanceTime(&aInstanceTime, mIsBegin);
}

PRBool
nsSMILTimeValueSpec::DependsOnBegin() const
{
  return mParams.mSyncBegin;
}

void
nsSMILTimeValueSpec::Traverse(nsCycleCollectionTraversalCallback* aCallback)
{
  mReferencedElement.Traverse(aCallback);
}

void
nsSMILTimeValueSpec::Unlink()
{
  UnregisterFromReferencedElement(mReferencedElement.get());
  mReferencedElement.Unlink();
}




void
nsSMILTimeValueSpec::UpdateReferencedElement(nsIContent* aFrom, nsIContent* aTo)
{
  if (aFrom == aTo)
    return;

  UnregisterFromReferencedElement(aFrom);

  if (mParams.mType == nsSMILTimeValueSpecParams::SYNCBASE) {
    nsSMILTimedElement* to = GetTimedElementFromContent(aTo);
    if (to) {
      to->AddDependent(*this);
    }
  } else if (mParams.mType == nsSMILTimeValueSpecParams::EVENT) {
    RegisterEventListener(aTo);
  }
}

void
nsSMILTimeValueSpec::UnregisterFromReferencedElement(nsIContent* aContent)
{
  if (!aContent)
    return;

  if (mParams.mType == nsSMILTimeValueSpecParams::SYNCBASE) {
    nsSMILTimedElement* timedElement = GetTimedElementFromContent(aContent);
    if (timedElement) {
      timedElement->RemoveDependent(*this);
    }
    mOwner->RemoveInstanceTimesForCreator(this, mIsBegin);
  } else if (mParams.mType == nsSMILTimeValueSpecParams::EVENT) {
    UnregisterEventListener(aContent);
  }
}

nsSMILTimedElement*
nsSMILTimeValueSpec::GetTimedElementFromContent(nsIContent* aContent)
{
  if (!aContent)
    return nsnull;

  nsCOMPtr<nsISMILAnimationElement> animElement = do_QueryInterface(aContent);
  if (!animElement)
    return nsnull;

  return &animElement->TimedElement();
}

void
nsSMILTimeValueSpec::RegisterEventListener(nsIContent* aTarget)
{
  NS_ABORT_IF_FALSE(mParams.mType == nsSMILTimeValueSpecParams::EVENT,
    "Attempting to register event-listener for non-event nsSMILTimeValueSpec");
  NS_ABORT_IF_FALSE(mParams.mEventSymbol,
    "Attempting to register event-listener but there is no event name");

  
  if (!aTarget)
    return;

  if (!mEventListener) {
    mEventListener = new EventListener(this);
  }

  nsCOMPtr<nsIDOMEventGroup> sysGroup;
  nsIEventListenerManager* elm =
    GetEventListenerManager(aTarget, getter_AddRefs(sysGroup));
  if (!elm)
    return;
  
  elm->AddEventListenerByType(mEventListener,
                              nsDependentAtomString(mParams.mEventSymbol),
                              NS_EVENT_FLAG_BUBBLE |
                              NS_PRIV_EVENT_UNTRUSTED_PERMITTED,
                              sysGroup);
}

void
nsSMILTimeValueSpec::UnregisterEventListener(nsIContent* aTarget)
{
  if (!aTarget || !mEventListener)
    return;

  nsCOMPtr<nsIDOMEventGroup> sysGroup;
  nsIEventListenerManager* elm =
    GetEventListenerManager(aTarget, getter_AddRefs(sysGroup));
  if (!elm)
    return;

  elm->RemoveEventListenerByType(mEventListener,
                                 nsDependentAtomString(mParams.mEventSymbol),
                                 NS_EVENT_FLAG_BUBBLE |
                                 NS_PRIV_EVENT_UNTRUSTED_PERMITTED,
                                 sysGroup);
}

nsIEventListenerManager*
nsSMILTimeValueSpec::GetEventListenerManager(nsIContent* aTarget,
                                             nsIDOMEventGroup** aSystemGroup)
{
  NS_ABORT_IF_FALSE(aTarget, "null target; can't get EventListenerManager");
  NS_ABORT_IF_FALSE(aSystemGroup && !*aSystemGroup,
      "Bad out param for system group");

  nsIEventListenerManager* elm = aTarget->GetListenerManager(PR_TRUE);
  if (!elm)
    return nsnull;

  aTarget->GetSystemEventGroup(aSystemGroup);
  if (!*aSystemGroup)
    return nsnull;

  return elm;
}

void
nsSMILTimeValueSpec::HandleEvent(nsIDOMEvent* aEvent)
{
  NS_ABORT_IF_FALSE(mEventListener, "Got event without an event listener");
  NS_ABORT_IF_FALSE(mParams.mType == nsSMILTimeValueSpecParams::EVENT,
    "Got event for non-event nsSMILTimeValueSpec");

  
  
  
  nsSMILTimeContainer* container = mOwner->GetTimeContainer();
  if (!container)
    return;

  nsSMILTime currentTime = container->GetCurrentTime();
  nsSMILTimeValue newTime(currentTime + mParams.mOffset.GetMillis());

  nsRefPtr<nsSMILInstanceTime> newInstance =
    new nsSMILInstanceTime(newTime, nsSMILInstanceTime::SOURCE_EVENT);
  mOwner->AddInstanceTime(newInstance, mIsBegin);
}

nsSMILTimeValue
nsSMILTimeValueSpec::ConvertBetweenTimeContainers(
    const nsSMILTimeValue& aSrcTime,
    const nsSMILTimeContainer* aSrcContainer)
{
  
  
  if (!aSrcTime.IsResolved())
    return aSrcTime;

  
  const nsSMILTimeContainer* dstContainer = mOwner->GetTimeContainer();
  if (dstContainer == aSrcContainer)
    return aSrcTime;

  
  
  if (!aSrcContainer || !dstContainer)
    return nsSMILTimeValue(); 

  nsSMILTimeValue docTime =
    aSrcContainer->ContainerToParentTime(aSrcTime.GetMillis());

  if (docTime.IsIndefinite())
    
    
    return docTime;

   NS_ABORT_IF_FALSE(docTime.IsResolved(),
       "ContainerToParentTime gave us an unresolved time");

  return dstContainer->ParentToContainerTime(docTime.GetMillis());
}
