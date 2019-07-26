





#include "TelephonyCall.h"
#include "mozilla/dom/TelephonyCallBinding.h"

#include "mozilla/dom/DOMError.h"

#include "CallEvent.h"
#include "Telephony.h"
#include "TelephonyCallGroup.h"

using namespace mozilla::dom;
using mozilla::ErrorResult;
using mozilla::dom::telephony::kOutgoingPlaceholderCallIndex;


already_AddRefed<TelephonyCall>
TelephonyCall::Create(Telephony* aTelephony, uint32_t aServiceId,
                      const nsAString& aNumber, uint16_t aCallState,
                      uint32_t aCallIndex, bool aEmergency, bool aIsConference,
                      bool aSwitchable, bool aMergeable)
{
  NS_ASSERTION(aTelephony, "Null pointer!");
  NS_ASSERTION(!aNumber.IsEmpty(), "Empty number!");
  NS_ASSERTION(aCallIndex >= 1, "Invalid call index!");

  nsRefPtr<TelephonyCall> call = new TelephonyCall(aTelephony->GetOwner());

  call->mTelephony = aTelephony;
  call->mServiceId = aServiceId;
  call->mNumber = aNumber;
  call->mCallIndex = aCallIndex;
  call->mError = nullptr;
  call->mEmergency = aEmergency;
  call->mGroup = aIsConference ? aTelephony->ConferenceGroup() : nullptr;
  call->mSwitchable = aSwitchable;
  call->mMergeable = aMergeable;

  call->ChangeStateInternal(aCallState, false);

  return call.forget();
}

TelephonyCall::TelephonyCall(nsPIDOMWindow* aOwner)
  : nsDOMEventTargetHelper(aOwner),
    mCallIndex(kOutgoingPlaceholderCallIndex),
    mCallState(nsITelephonyProvider::CALL_STATE_UNKNOWN),
    mLive(false)
{
}

TelephonyCall::~TelephonyCall()
{
}

JSObject*
TelephonyCall::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return TelephonyCallBinding::Wrap(aCx, aScope, this);
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

  if (aCallState == nsITelephonyProvider::CALL_STATE_DISCONNECTED) {
    NS_ASSERTION(mLive, "Should be live!");
    if (mGroup) {
      mGroup->RemoveCall(this);
    } else {
      mTelephony->RemoveCall(this);
    }
    mLive = false;
  } else if (!mLive) {
    if (mGroup) {
      mGroup->AddCall(this);
    } else {
      mTelephony->AddCall(this);
    }
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
                                 TelephonyCall* aCall)
{
  MOZ_ASSERT(aCall);

  nsRefPtr<CallEvent> event = CallEvent::Create(this, aType, aCall, false, false);

  return DispatchTrustedEvent(event);
}

void
TelephonyCall::NotifyError(const nsAString& aError)
{
  
  NS_ASSERTION(!mError, "Already have an error?");

  mError = new DOMError(GetOwner(), aError);

  
  ChangeStateInternal(nsITelephonyProvider::CALL_STATE_DISCONNECTED, true);

  nsresult rv = DispatchCallEvent(NS_LITERAL_STRING("error"), this);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch error event!");
  }
}

void
TelephonyCall::ChangeGroup(TelephonyCallGroup* aGroup)
{
  mGroup = aGroup;

  nsresult rv = DispatchCallEvent(NS_LITERAL_STRING("groupchange"), this);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch error event!");
  }
}

NS_IMPL_CYCLE_COLLECTION_INHERITED_3(TelephonyCall,
                                     nsDOMEventTargetHelper,
                                     mTelephony,
                                     mError,
                                     mGroup);

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TelephonyCall)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(TelephonyCall, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(TelephonyCall, nsDOMEventTargetHelper)



already_AddRefed<DOMError>
TelephonyCall::GetError() const
{
  nsRefPtr<DOMError> error = mError;
  return error.forget();
}

already_AddRefed<TelephonyCallGroup>
TelephonyCall::GetGroup() const
{
  nsRefPtr<TelephonyCallGroup> group = mGroup;
  return group.forget();
}

void
TelephonyCall::Answer(ErrorResult& aRv)
{
  if (mCallState != nsITelephonyProvider::CALL_STATE_INCOMING) {
    NS_WARNING("Answer on non-incoming call ignored!");
    return;
  }

  nsresult rv = mTelephony->Provider()->AnswerCall(mServiceId, mCallIndex);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  ChangeStateInternal(nsITelephonyProvider::CALL_STATE_CONNECTING, true);
}

void
TelephonyCall::HangUp(ErrorResult& aRv)
{
  if (mCallState == nsITelephonyProvider::CALL_STATE_DISCONNECTING ||
      mCallState == nsITelephonyProvider::CALL_STATE_DISCONNECTED) {
    NS_WARNING("HangUp on previously disconnected call ignored!");
    return;
  }

  nsresult rv = mCallState == nsITelephonyProvider::CALL_STATE_INCOMING ?
                mTelephony->Provider()->RejectCall(mServiceId, mCallIndex) :
                mTelephony->Provider()->HangUp(mServiceId, mCallIndex);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  ChangeStateInternal(nsITelephonyProvider::CALL_STATE_DISCONNECTING, true);
}

void
TelephonyCall::Hold(ErrorResult& aRv)
{
  if (mCallState != nsITelephonyProvider::CALL_STATE_CONNECTED) {
    NS_WARNING("Hold non-connected call ignored!");
    return;
  }

  if (mGroup) {
    NS_WARNING("Hold a call in conference ignored!");
    return;
  }

  if (!mSwitchable) {
    NS_WARNING("Hold a non-switchable call ignored!");
    return;
  }

  nsresult rv = mTelephony->Provider()->HoldCall(mServiceId, mCallIndex);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  if (!mSecondNumber.IsEmpty()) {
    
    
    
    return;
  }

  ChangeStateInternal(nsITelephonyProvider::CALL_STATE_HOLDING, true);
}

void
TelephonyCall::Resume(ErrorResult& aRv)
{
  if (mCallState != nsITelephonyProvider::CALL_STATE_HELD) {
    NS_WARNING("Resume non-held call ignored!");
    return;
  }

  if (mGroup) {
    NS_WARNING("Resume a call in conference ignored!");
    return;
  }

  if (!mSwitchable) {
    NS_WARNING("Resume a non-switchable call ignored!");
    return;
  }

  nsresult rv = mTelephony->Provider()->ResumeCall(mServiceId, mCallIndex);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  ChangeStateInternal(nsITelephonyProvider::CALL_STATE_RESUMING, true);
}
