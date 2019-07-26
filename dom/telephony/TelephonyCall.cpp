





#include "TelephonyCall.h"

#include "nsIDOMCallEvent.h"

#include "DOMError.h"
#include "GeneratedEvents.h"
#include "nsDOMClassInfo.h"
#include "Telephony.h"
#include "nsITelephonyProvider.h"

USING_TELEPHONY_NAMESPACE


already_AddRefed<TelephonyCall>
TelephonyCall::Create(Telephony* aTelephony, const nsAString& aNumber,
                      uint16_t aCallState, uint32_t aCallIndex)
{
  NS_ASSERTION(aTelephony, "Null pointer!");
  NS_ASSERTION(!aNumber.IsEmpty(), "Empty number!");
  NS_ASSERTION(aCallIndex >= 1, "Invalid call index!");

  nsRefPtr<TelephonyCall> call = new TelephonyCall();

  call->BindToOwner(aTelephony->GetOwner());

  call->mTelephony = aTelephony;
  call->mNumber = aNumber;
  call->mCallIndex = aCallIndex;
  call->mError = nullptr;

  call->ChangeStateInternal(aCallState, false);

  return call.forget();
}

TelephonyCall::TelephonyCall()
  : mCallIndex(kOutgoingPlaceholderCallIndex),
    mCallState(nsITelephonyProvider::CALL_STATE_UNKNOWN),
    mLive(false),
    mOutgoing(false)
{
}

void
TelephonyCall::ChangeStateInternal(uint16_t aCallState, bool aFireEvents)
{
  nsRefPtr<TelephonyCall> kungFuDeathGrip(this);

  nsString stateString;
  switch (aCallState) {
    case nsITelephonyProvider::CALL_STATE_DIALING:
      stateString.AssignLiteral("dialing");
      break;
    case nsITelephonyProvider::CALL_STATE_ALERTING:
      stateString.AssignLiteral("alerting");
      break;
    case nsITelephonyProvider::CALL_STATE_BUSY:
      stateString.AssignLiteral("busy");
      break;
    case nsITelephonyProvider::CALL_STATE_CONNECTING:
      stateString.AssignLiteral("connecting");
      break;
    case nsITelephonyProvider::CALL_STATE_CONNECTED:
      stateString.AssignLiteral("connected");
      break;
    case nsITelephonyProvider::CALL_STATE_HOLDING:
      stateString.AssignLiteral("holding");
      break;
    case nsITelephonyProvider::CALL_STATE_HELD:
      stateString.AssignLiteral("held");
      break;
    case nsITelephonyProvider::CALL_STATE_RESUMING:
      stateString.AssignLiteral("resuming");
      break;
    case nsITelephonyProvider::CALL_STATE_DISCONNECTING:
      stateString.AssignLiteral("disconnecting");
      break;
    case nsITelephonyProvider::CALL_STATE_DISCONNECTED:
      stateString.AssignLiteral("disconnected");
      break;
    case nsITelephonyProvider::CALL_STATE_INCOMING:
      stateString.AssignLiteral("incoming");
      break;
    default:
      NS_NOTREACHED("Unknown state!");
  }

  mState = stateString;
  mCallState = aCallState;

  if (aCallState == nsITelephonyProvider::CALL_STATE_DIALING) {
    mOutgoing = true;
  }

  if (aCallState == nsITelephonyProvider::CALL_STATE_DISCONNECTED) {
    NS_ASSERTION(mLive, "Should be live!");
    mTelephony->RemoveCall(this);
    mLive = false;
  } else if (!mLive) {
    mTelephony->AddCall(this);
    mLive = true;
  }

  if (aFireEvents) {
    nsresult rv = DispatchCallEvent(NS_LITERAL_STRING("statechange"), this);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to dispatch specific event!");
    }

    
    
    if (mCallState == aCallState) {
      rv = DispatchCallEvent(stateString, this);
      if (NS_FAILED(rv)) {
        NS_WARNING("Failed to dispatch specific event!");
      }
    }
  }
}

nsresult
TelephonyCall::DispatchCallEvent(const nsAString& aType,
                                 nsIDOMTelephonyCall* aCall)
{
  MOZ_ASSERT(aCall);

  nsCOMPtr<nsIDOMEvent> event;
  NS_NewDOMCallEvent(getter_AddRefs(event), nullptr, nullptr);
  NS_ASSERTION(event, "This should never fail!");

  nsCOMPtr<nsIDOMCallEvent> callEvent = do_QueryInterface(event);
  MOZ_ASSERT(callEvent);
  nsresult rv = callEvent->InitCallEvent(aType, false, false, aCall);
  NS_ENSURE_SUCCESS(rv, rv);

  return DispatchTrustedEvent(callEvent);
}

void
TelephonyCall::NotifyError(const nsAString& aError)
{
  
  NS_ASSERTION(!mError, "Already have an error?");

  mError = DOMError::CreateWithName(aError);

  
  ChangeStateInternal(nsITelephonyProvider::CALL_STATE_DISCONNECTED, true);

  nsresult rv = DispatchCallEvent(NS_LITERAL_STRING("error"), this);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch error event!");
  }
}

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(TelephonyCall,
                                     nsDOMEventTargetHelper,
                                     mTelephony)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TelephonyCall)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTelephonyCall)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(TelephonyCall)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(TelephonyCall, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(TelephonyCall, nsDOMEventTargetHelper)

DOMCI_DATA(TelephonyCall, TelephonyCall)

NS_IMETHODIMP
TelephonyCall::GetNumber(nsAString& aNumber)
{
  aNumber.Assign(mNumber);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyCall::GetState(nsAString& aState)
{
  aState.Assign(mState);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyCall::GetError(nsIDOMDOMError** aError)
{
  NS_IF_ADDREF(*aError = mError);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyCall::Answer()
{
  if (mCallState != nsITelephonyProvider::CALL_STATE_INCOMING) {
    NS_WARNING("Answer on non-incoming call ignored!");
    return NS_OK;
  }

  nsresult rv = mTelephony->Provider()->AnswerCall(mCallIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  ChangeStateInternal(nsITelephonyProvider::CALL_STATE_CONNECTING, true);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyCall::HangUp()
{
  if (mCallState == nsITelephonyProvider::CALL_STATE_DISCONNECTING ||
      mCallState == nsITelephonyProvider::CALL_STATE_DISCONNECTED) {
    NS_WARNING("HangUp on previously disconnected call ignored!");
    return NS_OK;
  }

  nsresult rv = mCallState == nsITelephonyProvider::CALL_STATE_INCOMING ?
                mTelephony->Provider()->RejectCall(mCallIndex) :
                mTelephony->Provider()->HangUp(mCallIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  ChangeStateInternal(nsITelephonyProvider::CALL_STATE_DISCONNECTING, true);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyCall::Hold()
{
  if (mCallState != nsITelephonyProvider::CALL_STATE_CONNECTED) {
    NS_WARNING("Hold non-connected call ignored!");
    return NS_OK;
  }

  nsresult rv = mTelephony->Provider()->HoldCall(mCallIndex);
  NS_ENSURE_SUCCESS(rv,rv);

  ChangeStateInternal(nsITelephonyProvider::CALL_STATE_HOLDING, true);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyCall::Resume()
{
  if (mCallState != nsITelephonyProvider::CALL_STATE_HELD) {
    NS_WARNING("Resume non-held call ignored!");
    return NS_OK;
  }

  nsresult rv = mTelephony->Provider()->ResumeCall(mCallIndex);
  NS_ENSURE_SUCCESS(rv,rv);

  ChangeStateInternal(nsITelephonyProvider::CALL_STATE_RESUMING, true);
  return NS_OK;
}

NS_IMPL_EVENT_HANDLER(TelephonyCall, statechange)
NS_IMPL_EVENT_HANDLER(TelephonyCall, dialing)
NS_IMPL_EVENT_HANDLER(TelephonyCall, alerting)
NS_IMPL_EVENT_HANDLER(TelephonyCall, busy)
NS_IMPL_EVENT_HANDLER(TelephonyCall, connecting)
NS_IMPL_EVENT_HANDLER(TelephonyCall, connected)
NS_IMPL_EVENT_HANDLER(TelephonyCall, disconnecting)
NS_IMPL_EVENT_HANDLER(TelephonyCall, disconnected)
NS_IMPL_EVENT_HANDLER(TelephonyCall, holding)
NS_IMPL_EVENT_HANDLER(TelephonyCall, held)
NS_IMPL_EVENT_HANDLER(TelephonyCall, resuming)
NS_IMPL_EVENT_HANDLER(TelephonyCall, error)
