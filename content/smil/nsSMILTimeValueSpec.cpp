




#include "nsSMILTimeValueSpec.h"
#include "nsSMILInterval.h"
#include "nsSMILTimeContainer.h"
#include "nsSMILTimeValue.h"
#include "nsSMILTimedElement.h"
#include "nsSMILInstanceTime.h"
#include "nsSMILParserUtils.h"
#include "nsISMILAnimationElement.h"
#include "nsEventListenerManager.h"
#include "nsGUIEvent.h"
#include "nsIDOMTimeEvent.h"
#include "nsString.h"
#include <limits>

using namespace mozilla::dom;




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
                                         bool aIsBegin)
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
                             Element* aContextNode)
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

  
  if (mParams.mType == nsSMILTimeValueSpecParams::REPEAT) {
    mParams.mEventSymbol = nsGkAtoms::repeatEvent;
  } else if (mParams.mType == nsSMILTimeValueSpecParams::ACCESSKEY) {
    mParams.mEventSymbol = nsGkAtoms::keypress;
  }

  ResolveReferences(aContextNode);

  return rv;
}

void
nsSMILTimeValueSpec::ResolveReferences(nsIContent* aContextNode)
{
  if (mParams.mType != nsSMILTimeValueSpecParams::SYNCBASE && !IsEventBased())
    return;

  NS_ABORT_IF_FALSE(aContextNode,
      "null context node for resolving timing references against");

  
  
  if (!aContextNode->IsInDoc())
    return;

  
  
  
  nsRefPtr<Element> oldReferencedElement = mReferencedElement.get();

  if (mParams.mDependentElemID) {
    mReferencedElement.ResetWithID(aContextNode,
        nsDependentAtomString(mParams.mDependentElemID));
  } else if (mParams.mType == nsSMILTimeValueSpecParams::EVENT) {
    Element* target = mOwner->GetTargetElement();
    mReferencedElement.ResetWithElement(target);
  } else if (mParams.mType == nsSMILTimeValueSpecParams::ACCESSKEY) {
    nsIDocument* doc = aContextNode->GetCurrentDoc();
    NS_ABORT_IF_FALSE(doc, "We are in the document but current doc is null");
    mReferencedElement.ResetWithElement(doc->GetRootElement());
  } else {
    NS_ABORT_IF_FALSE(false, "Syncbase or repeat spec without ID");
  }
  UpdateReferencedElement(oldReferencedElement, mReferencedElement.get());
}

bool
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

  
  if (!ApplyOffset(newTime)) {
    NS_WARNING("New time overflows nsSMILTime, ignoring");
    return;
  }

  
  nsRefPtr<nsSMILInstanceTime> newInstance =
    new nsSMILInstanceTime(newTime, nsSMILInstanceTime::SOURCE_SYNCBASE, this,
                           &aInterval);
  mOwner->AddInstanceTime(newInstance, mIsBegin);
}

void
nsSMILTimeValueSpec::HandleTargetElementChange(Element* aNewTarget)
{
  if (!IsEventBased() || mParams.mDependentElemID)
    return;

  mReferencedElement.ResetWithElement(aNewTarget);
}

void
nsSMILTimeValueSpec::HandleChangedInstanceTime(
    const nsSMILInstanceTime& aBaseTime,
    const nsSMILTimeContainer* aSrcContainer,
    nsSMILInstanceTime& aInstanceTimeToUpdate,
    bool aObjectChanged)
{
  
  
  if (aInstanceTimeToUpdate.IsFixedTime())
    return;

  nsSMILTimeValue updatedTime =
    ConvertBetweenTimeContainers(aBaseTime.Time(), aSrcContainer);

  
  if (!ApplyOffset(updatedTime)) {
    NS_WARNING("Updated time overflows nsSMILTime, ignoring");
    return;
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

bool
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
nsSMILTimeValueSpec::UpdateReferencedElement(Element* aFrom, Element* aTo)
{
  if (aFrom == aTo)
    return;

  UnregisterFromReferencedElement(aFrom);

  switch (mParams.mType)
  {
  case nsSMILTimeValueSpecParams::SYNCBASE:
    {
      nsSMILTimedElement* to = GetTimedElement(aTo);
      if (to) {
        to->AddDependent(*this);
      }
    }
    break;

  case nsSMILTimeValueSpecParams::EVENT:
  case nsSMILTimeValueSpecParams::REPEAT:
  case nsSMILTimeValueSpecParams::ACCESSKEY:
    RegisterEventListener(aTo);
    break;

  default:
    
    break;
  }
}

void
nsSMILTimeValueSpec::UnregisterFromReferencedElement(Element* aElement)
{
  if (!aElement)
    return;

  if (mParams.mType == nsSMILTimeValueSpecParams::SYNCBASE) {
    nsSMILTimedElement* timedElement = GetTimedElement(aElement);
    if (timedElement) {
      timedElement->RemoveDependent(*this);
    }
    mOwner->RemoveInstanceTimesForCreator(this, mIsBegin);
  } else if (IsEventBased()) {
    UnregisterEventListener(aElement);
  }
}

nsSMILTimedElement*
nsSMILTimeValueSpec::GetTimedElement(Element* aElement)
{
  if (!aElement)
    return nsnull;

  nsCOMPtr<nsISMILAnimationElement> animElement = do_QueryInterface(aElement);
  if (!animElement)
    return nsnull;

  return &animElement->TimedElement();
}



bool
nsSMILTimeValueSpec::IsWhitelistedEvent()
{
  
  if (mParams.mType == nsSMILTimeValueSpecParams::REPEAT) {
    return true;
  }

  
  if (mParams.mType == nsSMILTimeValueSpecParams::EVENT &&
      (mParams.mEventSymbol == nsGkAtoms::repeat ||
       mParams.mEventSymbol == nsGkAtoms::repeatEvent ||
       mParams.mEventSymbol == nsGkAtoms::beginEvent ||
       mParams.mEventSymbol == nsGkAtoms::endEvent)) {
    return true;
  }

  return false;
}

void
nsSMILTimeValueSpec::RegisterEventListener(Element* aTarget)
{
  NS_ABORT_IF_FALSE(IsEventBased(),
    "Attempting to register event-listener for unexpected nsSMILTimeValueSpec"
    " type");
  NS_ABORT_IF_FALSE(mParams.mEventSymbol,
    "Attempting to register event-listener but there is no event name");

  if (!aTarget)
    return;

  
  if (!aTarget->GetOwnerDocument()->IsScriptEnabled() &&
      !IsWhitelistedEvent()) {
    return;
  }

  if (!mEventListener) {
    mEventListener = new EventListener(this);
  }

  nsEventListenerManager* elm = GetEventListenerManager(aTarget);
  if (!elm)
    return;

  elm->AddEventListenerByType(mEventListener,
                              nsDependentAtomString(mParams.mEventSymbol),
                              NS_EVENT_FLAG_BUBBLE |
                              NS_PRIV_EVENT_UNTRUSTED_PERMITTED |
                              NS_EVENT_FLAG_SYSTEM_EVENT);
}

void
nsSMILTimeValueSpec::UnregisterEventListener(Element* aTarget)
{
  if (!aTarget || !mEventListener)
    return;

  nsEventListenerManager* elm = GetEventListenerManager(aTarget);
  if (!elm)
    return;

  elm->RemoveEventListenerByType(mEventListener,
                                 nsDependentAtomString(mParams.mEventSymbol),
                                 NS_EVENT_FLAG_BUBBLE |
                                 NS_PRIV_EVENT_UNTRUSTED_PERMITTED |
                                 NS_EVENT_FLAG_SYSTEM_EVENT);
}

nsEventListenerManager*
nsSMILTimeValueSpec::GetEventListenerManager(Element* aTarget)
{
  NS_ABORT_IF_FALSE(aTarget, "null target; can't get EventListenerManager");

  nsCOMPtr<nsIDOMEventTarget> target;

  if (mParams.mType == nsSMILTimeValueSpecParams::ACCESSKEY) {
    nsIDocument* doc = aTarget->GetCurrentDoc();
    if (!doc)
      return nsnull;
    nsPIDOMWindow* win = doc->GetWindow();
    if (!win)
      return nsnull;
    target = do_QueryInterface(win);
  } else {
    target = aTarget;
  }
  if (!target)
    return nsnull;

  return target->GetListenerManager(true);
}

void
nsSMILTimeValueSpec::HandleEvent(nsIDOMEvent* aEvent)
{
  NS_ABORT_IF_FALSE(mEventListener, "Got event without an event listener");
  NS_ABORT_IF_FALSE(IsEventBased(),
                    "Got event for non-event nsSMILTimeValueSpec");
  NS_ABORT_IF_FALSE(aEvent, "No event supplied");

  
  
  
  nsSMILTimeContainer* container = mOwner->GetTimeContainer();
  if (!container)
    return;

  if (!CheckEventDetail(aEvent))
    return;

  nsSMILTime currentTime = container->GetCurrentTime();
  nsSMILTimeValue newTime(currentTime);
  if (!ApplyOffset(newTime)) {
    NS_WARNING("New time generated from event overflows nsSMILTime, ignoring");
    return;
  }

  nsRefPtr<nsSMILInstanceTime> newInstance =
    new nsSMILInstanceTime(newTime, nsSMILInstanceTime::SOURCE_EVENT);
  mOwner->AddInstanceTime(newInstance, mIsBegin);
}

bool
nsSMILTimeValueSpec::CheckEventDetail(nsIDOMEvent *aEvent)
{
  switch (mParams.mType)
  {
  case nsSMILTimeValueSpecParams::REPEAT:
    return CheckRepeatEventDetail(aEvent);

  case nsSMILTimeValueSpecParams::ACCESSKEY:
    return CheckAccessKeyEventDetail(aEvent);

  default:
    
    return true;
  }
}

bool
nsSMILTimeValueSpec::CheckRepeatEventDetail(nsIDOMEvent *aEvent)
{
  nsCOMPtr<nsIDOMTimeEvent> timeEvent = do_QueryInterface(aEvent);
  if (!timeEvent) {
    NS_WARNING("Received a repeat event that was not a DOMTimeEvent");
    return false;
  }

  PRInt32 detail;
  timeEvent->GetDetail(&detail);
  return detail > 0 && (PRUint32)detail == mParams.mRepeatIterationOrAccessKey;
}

bool
nsSMILTimeValueSpec::CheckAccessKeyEventDetail(nsIDOMEvent *aEvent)
{
  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aEvent);
  if (!keyEvent) {
    NS_WARNING("Received an accesskey event that was not a DOMKeyEvent");
    return false;
  }

  
  
  
  bool isCtrl;
  bool isMeta;
  keyEvent->GetCtrlKey(&isCtrl);
  keyEvent->GetMetaKey(&isMeta);
  if (isCtrl || isMeta)
    return false;

  PRUint32 code;
  keyEvent->GetCharCode(&code);
  if (code)
    return code == mParams.mRepeatIterationOrAccessKey;

  
  
  
  
  bool isAlt;
  bool isShift;
  keyEvent->GetAltKey(&isAlt);
  keyEvent->GetShiftKey(&isShift);
  if (isAlt || isShift)
    return false;

  keyEvent->GetKeyCode(&code);
  switch (code)
  {
  case nsIDOMKeyEvent::DOM_VK_BACK_SPACE:
    return mParams.mRepeatIterationOrAccessKey == 0x08;

  case nsIDOMKeyEvent::DOM_VK_RETURN:
  case nsIDOMKeyEvent::DOM_VK_ENTER:
    return mParams.mRepeatIterationOrAccessKey == 0x0A ||
           mParams.mRepeatIterationOrAccessKey == 0x0D;

  case nsIDOMKeyEvent::DOM_VK_ESCAPE:
    return mParams.mRepeatIterationOrAccessKey == 0x1B;

  case nsIDOMKeyEvent::DOM_VK_DELETE:
    return mParams.mRepeatIterationOrAccessKey == 0x7F;

  default:
    return false;
  }
}

nsSMILTimeValue
nsSMILTimeValueSpec::ConvertBetweenTimeContainers(
    const nsSMILTimeValue& aSrcTime,
    const nsSMILTimeContainer* aSrcContainer)
{
  
  
  if (!aSrcTime.IsDefinite())
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

  NS_ABORT_IF_FALSE(docTime.IsDefinite(),
    "ContainerToParentTime gave us an unresolved or indefinite time");

  return dstContainer->ParentToContainerTime(docTime.GetMillis());
}

bool
nsSMILTimeValueSpec::ApplyOffset(nsSMILTimeValue& aTime) const
{
  
  if (!aTime.IsDefinite()) {
    return true;
  }

  double resultAsDouble =
    (double)aTime.GetMillis() + mParams.mOffset.GetMillis();
  if (resultAsDouble > std::numeric_limits<nsSMILTime>::max() ||
      resultAsDouble < std::numeric_limits<nsSMILTime>::min()) {
    return false;
  }
  aTime.SetMillis(aTime.GetMillis() + mParams.mOffset.GetMillis());
  return true;
}
