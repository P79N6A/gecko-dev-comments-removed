




































#include "nsSmartCardEvent.h"
#include "nsIDOMSmartCardEvent.h"
#include "nsIDOMClassInfo.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMEvent.h"
#include "nsXPCOM.h"


nsSmartCardEvent::nsSmartCardEvent(const nsAString &aTokenName) 
    : mInner(nsnull), mPrivate(nsnull), mTokenName(aTokenName)
{
}

nsSmartCardEvent::~nsSmartCardEvent()
{}




NS_INTERFACE_MAP_BEGIN(nsSmartCardEvent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMSmartCardEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSmartCardEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEvent)
  NS_INTERFACE_MAP_ENTRY(nsIPrivateDOMEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SmartCardEvent)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsSmartCardEvent)
NS_IMPL_RELEASE(nsSmartCardEvent)




NS_IMETHODIMP nsSmartCardEvent::Init(nsIDOMEvent * aInner)
{
  nsresult rv;

  NS_ASSERTION(aInner, "SmartCardEvent initialized with a null Event");
  mInner = aInner;
  mPrivate = do_QueryInterface(mInner, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }
  mNSEvent = do_QueryInterface(mInner, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }
  return mPrivate->SetTrusted(PR_TRUE);
}


NS_IMETHODIMP nsSmartCardEvent::GetTokenName(nsAString &aTokenName)
{
  aTokenName = mTokenName;
  return NS_OK;
}


NS_IMETHODIMP nsSmartCardEvent::DuplicatePrivateData(void)
{
  NS_ASSERTION(mPrivate, "SmartCardEvent called without Init");
  return mPrivate->DuplicatePrivateData();
}

NS_IMETHODIMP nsSmartCardEvent::SetTarget(nsIDOMEventTarget *aTarget)
{
  NS_ASSERTION(mPrivate, "SmartCardEvent called without Init");
  return mPrivate->SetTarget(aTarget);
}

NS_IMETHODIMP_(PRBool ) nsSmartCardEvent::IsDispatchStopped()
{
  PRBool  isDispatchPrevented = nsnull;
  PRBool * aIsDispatchPrevented = &isDispatchPrevented;
  NS_ASSERTION(mPrivate, "SmartCardEvent called without Init");
  return mPrivate->IsDispatchStopped();
}

NS_IMETHODIMP_(nsEvent*) nsSmartCardEvent::GetInternalNSEvent()
{
  nsEvent* nSEvent = nsnull;
  nsEvent** aNSEvent = &nSEvent;
  NS_ASSERTION(mPrivate, "SmartCardEvent called without Init");
  return mPrivate->GetInternalNSEvent();
}

NS_IMETHODIMP nsSmartCardEvent::SetTrusted(PRBool aResult)
{
  NS_ASSERTION(mPrivate, "SmartCardEvent called without Init");
  return mPrivate->SetTrusted(aResult);
}



NS_IMETHODIMP nsSmartCardEvent::GetOriginalTarget(nsIDOMEventTarget * *aOriginalTarget)
{
  NS_ASSERTION(mNSEvent, "SmartCardEvent called without Init");
  return mNSEvent->GetOriginalTarget(aOriginalTarget);
}

NS_IMETHODIMP nsSmartCardEvent::GetExplicitOriginalTarget(nsIDOMEventTarget * *aTarget)
{
  NS_ASSERTION(mNSEvent, "SmartCardEvent called without Init");
  return mNSEvent->GetExplicitOriginalTarget(aTarget);
}

NS_IMETHODIMP nsSmartCardEvent::GetTmpRealOriginalTarget(nsIDOMEventTarget * *aTarget)
{
  NS_ASSERTION(mNSEvent, "SmartCardEvent called without Init");
  return mNSEvent->GetTmpRealOriginalTarget(aTarget);
}

NS_IMETHODIMP nsSmartCardEvent::PreventBubble(void)
{
  NS_ASSERTION(mNSEvent, "SmartCardEvent called without Init");
  return mNSEvent->PreventBubble();
}

NS_IMETHODIMP nsSmartCardEvent::PreventCapture(void)
{
  NS_ASSERTION(mNSEvent, "SmartCardEvent called without Init");
  return mNSEvent->PreventCapture();
}

NS_IMETHODIMP nsSmartCardEvent::GetIsTrusted(PRBool *aIsTrusted)
{
  NS_ASSERTION(mNSEvent, "SmartCardEvent called without Init");
  return mNSEvent->GetIsTrusted(aIsTrusted);
}

NS_IMETHODIMP
nsSmartCardEvent::GetPreventDefault(PRBool* aReturn)
{
  NS_ASSERTION(mNSEvent, "SmartCardEvent called without Init");
  return mNSEvent->GetPreventDefault(aReturn);
}


NS_IMETHODIMP nsSmartCardEvent::GetType(nsAString & aType)
{
  NS_ASSERTION(mInner, "SmartCardEvent called without Init");
  return mInner->GetType(aType);
}

NS_IMETHODIMP nsSmartCardEvent::GetTarget(nsIDOMEventTarget * *aTarget)
{
  NS_ASSERTION(mInner, "SmartCardEvent called without Init");
  return mInner->GetTarget(aTarget);
}

NS_IMETHODIMP nsSmartCardEvent::GetCurrentTarget(nsIDOMEventTarget * *aCurrentTarget)
{
  NS_ASSERTION(mInner, "SmartCardEvent called without Init");
  return mInner->GetCurrentTarget(aCurrentTarget);
}

NS_IMETHODIMP nsSmartCardEvent::GetEventPhase(PRUint16 *aEventPhase)
{
  NS_ASSERTION(mInner, "SmartCardEvent called without Init");
  return mInner->GetEventPhase(aEventPhase);
}

NS_IMETHODIMP nsSmartCardEvent::GetBubbles(PRBool *aBubbles)
{
  NS_ASSERTION(mInner, "SmartCardEvent called without Init");
  return mInner->GetBubbles(aBubbles);
}

NS_IMETHODIMP nsSmartCardEvent::GetCancelable(PRBool *aCancelable)
{
  NS_ASSERTION(mInner, "SmartCardEvent called without Init");
  return mInner->GetCancelable(aCancelable);
}

NS_IMETHODIMP nsSmartCardEvent::GetTimeStamp(DOMTimeStamp *aTimeStamp)
{
  NS_ASSERTION(mInner, "SmartCardEvent called without Init");
  return mInner->GetTimeStamp(aTimeStamp);
}

NS_IMETHODIMP nsSmartCardEvent::StopPropagation()
{
  NS_ASSERTION(mInner, "SmartCardEvent called without Init");
  return mInner->StopPropagation();
}

NS_IMETHODIMP nsSmartCardEvent::PreventDefault()
{
  NS_ASSERTION(mInner, "SmartCardEvent called without Init");
  return mInner->PreventDefault();
}

NS_IMETHODIMP nsSmartCardEvent::InitEvent(const nsAString & eventTypeArg, PRBool canBubbleArg, PRBool cancelableArg)
{
  NS_ASSERTION(mInner, "SmartCardEvent called without Init");
  return mInner->InitEvent(eventTypeArg, canBubbleArg, cancelableArg);
}

