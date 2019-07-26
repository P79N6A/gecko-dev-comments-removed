





#include "Telephony.h"

#include "nsIURI.h"
#include "nsIDOMCallEvent.h"
#include "nsPIDOMWindow.h"
#include "nsIPermissionManager.h"

#include "GeneratedEvents.h"
#include "jsapi.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfo.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"
#include "nsTArrayHelpers.h"

#include "TelephonyCall.h"

#define NS_RILCONTENTHELPER_CONTRACTID "@mozilla.org/ril/content-helper;1"

USING_TELEPHONY_NAMESPACE

namespace {

typedef nsAutoTArray<Telephony*, 2> TelephonyList;

TelephonyList* gTelephonyList;

} 

class Telephony::Listener : public nsITelephonyListener
{
  Telephony* mTelephony;

public:
  NS_DECL_ISUPPORTS
  NS_FORWARD_SAFE_NSITELEPHONYLISTENER(mTelephony)

  Listener(Telephony* aTelephony)
    : mTelephony(aTelephony)
  {
    MOZ_ASSERT(mTelephony);
  }

  void
  Disconnect()
  {
    MOZ_ASSERT(mTelephony);
    mTelephony = nullptr;
  }
};

Telephony::Telephony()
: mActiveCall(nullptr), mCallsArray(nullptr), mRooted(false)
{
  if (!gTelephonyList) {
    gTelephonyList = new TelephonyList();
  }

  gTelephonyList->AppendElement(this);
}

Telephony::~Telephony()
{
  if (mListener) {
    mListener->Disconnect();

    if (mProvider) {
      mProvider->UnregisterTelephonyMsg(mListener);
    }
  }

  if (mRooted) {
    mCallsArray = nullptr;
    NS_DROP_JS_OBJECTS(this, Telephony);
  }

  NS_ASSERTION(gTelephonyList, "This should never be null!");
  NS_ASSERTION(gTelephonyList->Contains(this), "Should be in the list!");

  if (gTelephonyList->Length() == 1) {
    delete gTelephonyList;
    gTelephonyList = nullptr;
  }
  else {
    gTelephonyList->RemoveElement(this);
  }
}


already_AddRefed<Telephony>
Telephony::Create(nsPIDOMWindow* aOwner, nsITelephonyProvider* aProvider)
{
  NS_ASSERTION(aOwner, "Null owner!");
  NS_ASSERTION(aProvider, "Null provider!");

  nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(aOwner);
  NS_ENSURE_TRUE(sgo, nullptr);

  nsCOMPtr<nsIScriptContext> scriptContext = sgo->GetContext();
  NS_ENSURE_TRUE(scriptContext, nullptr);

  nsRefPtr<Telephony> telephony = new Telephony();

  telephony->BindToOwner(aOwner);

  telephony->mProvider = aProvider;
  telephony->mListener = new Listener(telephony);

  nsresult rv = aProvider->EnumerateCalls(telephony->mListener);
  NS_ENSURE_SUCCESS(rv, nullptr);

  rv = aProvider->RegisterTelephonyMsg(telephony->mListener);
  NS_ENSURE_SUCCESS(rv, nullptr);

  return telephony.forget();
}

already_AddRefed<TelephonyCall>
Telephony::CreateNewDialingCall(const nsAString& aNumber)
{
  nsRefPtr<TelephonyCall> call =
    TelephonyCall::Create(this, aNumber,
                          nsITelephonyProvider::CALL_STATE_DIALING);
  NS_ASSERTION(call, "This should never fail!");

  NS_ASSERTION(mCalls.Contains(call), "Should have auto-added new call!");

  return call.forget();
}

void
Telephony::NoteDialedCallFromOtherInstance(const nsAString& aNumber)
{
  
  nsRefPtr<TelephonyCall> call = CreateNewDialingCall(aNumber);
}

nsresult
Telephony::NotifyCallsChanged(TelephonyCall* aCall)
{
  if (aCall->CallState() == nsITelephonyProvider::CALL_STATE_DIALING ||
      aCall->CallState() == nsITelephonyProvider::CALL_STATE_ALERTING ||
      aCall->CallState() == nsITelephonyProvider::CALL_STATE_CONNECTED) {
    NS_ASSERTION(!mActiveCall, "Already have an active call!");
    mActiveCall = aCall;
  } else if (mActiveCall && mActiveCall->CallIndex() == aCall->CallIndex()) {
    mActiveCall = nullptr;
  }

  return DispatchCallEvent(NS_LITERAL_STRING("callschanged"), aCall);
}

nsresult
Telephony::DialInternal(bool isEmergency,
                        const nsAString& aNumber,
                        nsIDOMTelephonyCall** aResult)
{
  NS_ENSURE_ARG(!aNumber.IsEmpty());

  for (uint32_t index = 0; index < mCalls.Length(); index++) {
    const nsRefPtr<TelephonyCall>& tempCall = mCalls[index];
    if (tempCall->IsOutgoing() &&
        tempCall->CallState() < nsITelephonyProvider::CALL_STATE_CONNECTED) {
      
      
      NS_WARNING("Only permitted to dial one call at a time!");
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  nsresult rv;
  if (isEmergency) {
    rv = mProvider->DialEmergency(aNumber);
  } else {
    rv = mProvider->Dial(aNumber);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<TelephonyCall> call = CreateNewDialingCall(aNumber);

  
  for (uint32_t index = 0; index < gTelephonyList->Length(); index++) {
    Telephony*& telephony = gTelephonyList->ElementAt(index);
    if (telephony != this) {
      nsRefPtr<Telephony> kungFuDeathGrip = telephony;
      telephony->NoteDialedCallFromOtherInstance(aNumber);
    }
  }

  call.forget(aResult);
  return NS_OK;
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(Telephony,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  for (uint32_t index = 0; index < tmp->mCalls.Length(); index++) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mCalls[i]");
    cb.NoteXPCOMChild(tmp->mCalls[index]->ToISupports());
  }
  
  
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(Telephony,
                                               nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mCallsArray)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(Telephony,
                                                nsDOMEventTargetHelper)
  tmp->mCalls.Clear();
  tmp->mActiveCall = nullptr;
  tmp->mCallsArray = nullptr;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(Telephony)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTelephony)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Telephony)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(Telephony, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(Telephony, nsDOMEventTargetHelper)

DOMCI_DATA(Telephony, Telephony)

NS_IMPL_ISUPPORTS1(Telephony::Listener, nsITelephonyListener)

NS_IMETHODIMP
Telephony::Dial(const nsAString& aNumber, nsIDOMTelephonyCall** aResult)
{
  DialInternal(false, aNumber, aResult);

  return NS_OK;
}

NS_IMETHODIMP
Telephony::DialEmergency(const nsAString& aNumber, nsIDOMTelephonyCall** aResult)
{
  DialInternal(true, aNumber, aResult);

  return NS_OK;
}

NS_IMETHODIMP
Telephony::GetMuted(bool* aMuted)
{
  nsresult rv = mProvider->GetMicrophoneMuted(aMuted);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
Telephony::SetMuted(bool aMuted)
{
  nsresult rv = mProvider->SetMicrophoneMuted(aMuted);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
Telephony::GetSpeakerEnabled(bool* aSpeakerEnabled)
{
  nsresult rv = mProvider->GetSpeakerEnabled(aSpeakerEnabled);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
Telephony::SetSpeakerEnabled(bool aSpeakerEnabled)
{
  nsresult rv = mProvider->SetSpeakerEnabled(aSpeakerEnabled);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
Telephony::GetActive(jsval* aActive)
{
  if (!mActiveCall) {
    aActive->setNull();
    return NS_OK;
  }

  nsresult rv;
  nsIScriptContext* sc = GetContextForEventHandlers(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  AutoPushJSContext cx(sc ? sc->GetNativeContext() : nullptr);
  if (sc) {
    rv =
      nsContentUtils::WrapNative(cx, sc->GetNativeGlobal(),
                                 mActiveCall->ToISupports(), aActive);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_IMETHODIMP
Telephony::GetCalls(jsval* aCalls)
{
  JSObject* calls = mCallsArray;
  if (!calls) {
    nsresult rv;
    nsIScriptContext* sc = GetContextForEventHandlers(&rv);
    NS_ENSURE_SUCCESS(rv, rv);
    AutoPushJSContext cx(sc ? sc->GetNativeContext() : nullptr);
    if (sc) {
      rv = nsTArrayToJSArray(cx, mCalls, &calls);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!mRooted) {
        NS_HOLD_JS_OBJECTS(this, Telephony);
        mRooted = true;
      }

      mCallsArray = calls;
    } else {
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  aCalls->setObject(*calls);
  return NS_OK;
}

NS_IMETHODIMP
Telephony::StartTone(const nsAString& aDTMFChar)
{
  if (aDTMFChar.IsEmpty()) {
    NS_WARNING("Empty tone string will be ignored");
    return NS_OK;
  }

  if (aDTMFChar.Length() > 1) {
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv = mProvider->StartTone(aDTMFChar);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
Telephony::StopTone()
{
  nsresult rv = mProvider->StopTone();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMPL_EVENT_HANDLER(Telephony, incoming)
NS_IMPL_EVENT_HANDLER(Telephony, callschanged)

NS_IMETHODIMP
Telephony::CallStateChanged(uint32_t aCallIndex, uint16_t aCallState,
                            const nsAString& aNumber, bool aIsActive)
{
  NS_ASSERTION(aCallIndex != kOutgoingPlaceholderCallIndex,
               "This should never happen!");

  nsRefPtr<TelephonyCall> modifiedCall;
  nsRefPtr<TelephonyCall> outgoingCall;

  for (uint32_t index = 0; index < mCalls.Length(); index++) {
    nsRefPtr<TelephonyCall>& tempCall = mCalls[index];
    if (tempCall->CallIndex() == kOutgoingPlaceholderCallIndex) {
      NS_ASSERTION(!outgoingCall, "More than one outgoing call not supported!");
      NS_ASSERTION(tempCall->CallState() ==
                   nsITelephonyProvider::CALL_STATE_DIALING,
                   "Something really wrong here!");
      
      
      outgoingCall = tempCall;
    } else if (tempCall->CallIndex() == aCallIndex) {
      
      modifiedCall = tempCall;
      outgoingCall = nullptr;
      break;
    }
  }

  
  
  
  if (!modifiedCall &&
      aCallState != nsITelephonyProvider::CALL_STATE_INCOMING &&
      outgoingCall) {
    outgoingCall->UpdateCallIndex(aCallIndex);
    modifiedCall.swap(outgoingCall);
  }

  if (modifiedCall) {
    
    if (aIsActive) {
        mActiveCall = modifiedCall;
    } else if (mActiveCall && mActiveCall->CallIndex() == aCallIndex) {
      mActiveCall = nullptr;
    }

    
    modifiedCall->ChangeState(aCallState);

    return NS_OK;
  }

  

  if (aCallState == nsITelephonyProvider::CALL_STATE_DISCONNECTED) {
    
    
    return NS_OK;
  }

  nsRefPtr<TelephonyCall> call =
    TelephonyCall::Create(this, aNumber, aCallState, aCallIndex);
  NS_ASSERTION(call, "This should never fail!");

  NS_ASSERTION(mCalls.Contains(call), "Should have auto-added new call!");

  if (aCallState == nsITelephonyProvider::CALL_STATE_INCOMING) {
    nsresult rv = DispatchCallEvent(NS_LITERAL_STRING("incoming"), call);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
Telephony::EnumerateCallState(uint32_t aCallIndex, uint16_t aCallState,
                              const nsAString& aNumber, bool aIsActive,
                              bool* aContinue)
{
  
  for (uint32_t index = 0; index < mCalls.Length(); index++) {
    nsRefPtr<TelephonyCall>& tempCall = mCalls[index];
    if (tempCall->CallIndex() == aCallIndex) {
      
      *aContinue = true;
      return NS_OK;
    }
  }

  nsRefPtr<TelephonyCall> call =
    TelephonyCall::Create(this, aNumber, aCallState, aCallIndex);
  NS_ASSERTION(call, "This should never fail!");

  NS_ASSERTION(mCalls.Contains(call), "Should have auto-added new call!");

  *aContinue = true;
  return NS_OK;
}

NS_IMETHODIMP
Telephony::NotifyError(int32_t aCallIndex,
                       const nsAString& aError)
{
  nsRefPtr<TelephonyCall> callToNotify;
  if (!mCalls.IsEmpty()) {
    
    if (aCallIndex == -1) {
      callToNotify = mCalls[mCalls.Length() - 1];
    } else {
      
      for (uint32_t index = 0; index < mCalls.Length(); index++) {
        nsRefPtr<TelephonyCall>& call = mCalls[index];
        if (call->CallIndex() == aCallIndex) {
          callToNotify = call;
          break;
        }
      }
    }
  }

  if (!callToNotify) {
    NS_ERROR("Don't call me with a bad call index!");
    return NS_ERROR_UNEXPECTED;
  }

  if (mActiveCall && mActiveCall->CallIndex() == callToNotify->CallIndex()) {
    mActiveCall = nullptr;
  }

  
  callToNotify->NotifyError(aError);

  return NS_OK;
}

nsresult
Telephony::DispatchCallEvent(const nsAString& aType,
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

nsresult
NS_NewTelephony(nsPIDOMWindow* aWindow, nsIDOMTelephony** aTelephony)
{
  NS_ASSERTION(aWindow, "Null pointer!");

  nsPIDOMWindow* innerWindow = aWindow->IsInnerWindow() ?
    aWindow :
    aWindow->GetCurrentInnerWindow();

  nsCOMPtr<nsIPermissionManager> permMgr =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(permMgr, NS_ERROR_UNEXPECTED);

  uint32_t permission;
  nsresult rv =
    permMgr->TestPermissionFromWindow(aWindow, "telephony", &permission);
  NS_ENSURE_SUCCESS(rv, rv);

  if (permission != nsIPermissionManager::ALLOW_ACTION) {
    *aTelephony = nullptr;
    return NS_OK;
  }

  nsCOMPtr<nsITelephonyProvider> ril =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE(ril, NS_ERROR_UNEXPECTED);

  nsRefPtr<Telephony> telephony = Telephony::Create(innerWindow, ril);
  NS_ENSURE_TRUE(telephony, NS_ERROR_UNEXPECTED);

  telephony.forget(aTelephony);
  return NS_OK;
}
