






































#include "Telephony.h"

#include "nsIDocument.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsPIDOMWindow.h"

#include "jsapi.h"
#include "mozilla/Preferences.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfo.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"
#include "SystemWorkerManager.h"

#include "CallEvent.h"
#include "TelephonyCall.h"

USING_TELEPHONY_NAMESPACE
using namespace mozilla::dom::gonk;
using mozilla::Preferences;

#define DOM_TELEPHONY_APP_PHONE_URL_PREF "dom.telephony.app.phone.url"

namespace {

typedef nsAutoTArray<Telephony*, 2> TelephonyList;

TelephonyList* gTelephonyList;

template <class T>
inline nsresult
nsTArrayToJSArray(JSContext* aCx, JSObject* aGlobal,
                  const nsTArray<nsRefPtr<T> >& aSourceArray,
                  JSObject** aResultArray)
{
  NS_ASSERTION(aCx, "Null context!");
  NS_ASSERTION(aGlobal, "Null global!");

  JSAutoRequest ar(aCx);
  JSAutoEnterCompartment ac;
  if (!ac.enter(aCx, aGlobal)) {
    NS_WARNING("Failed to enter compartment!");
    return NS_ERROR_FAILURE;
  }

  JSObject* arrayObj;

  if (aSourceArray.IsEmpty()) {
    arrayObj = JS_NewArrayObject(aCx, 0, nsnull);
  } else {
    nsTArray<jsval> valArray;
    valArray.SetLength(aSourceArray.Length());

    for (PRUint32 index = 0; index < valArray.Length(); index++) {
      nsISupports* obj = aSourceArray[index]->ToISupports();
      nsresult rv =
        nsContentUtils::WrapNative(aCx, aGlobal, obj, &valArray[index]);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    arrayObj = JS_NewArrayObject(aCx, valArray.Length(), valArray.Elements());
  }

  if (!arrayObj) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  if (!JS_FreezeObject(aCx, arrayObj)) {
    return NS_ERROR_FAILURE;
  }

  *aResultArray = arrayObj;
  return NS_OK;
}

} 

Telephony::Telephony()
: mActiveCall(nsnull), mCallsArray(nsnull), mRooted(false)
{
  if (!gTelephonyList) {
    gTelephonyList = new TelephonyList();
  }

  gTelephonyList->AppendElement(this);
}

Telephony::~Telephony()
{
  if (mRIL && mRILTelephonyCallback) {
    mRIL->UnregisterCallback(mRILTelephonyCallback);
  }

  if (mRooted) {
    NS_DROP_JS_OBJECTS(this, Telephony);
  }

  NS_ASSERTION(gTelephonyList, "This should never be null!");
  NS_ASSERTION(gTelephonyList->Contains(this), "Should be in the list!");

  if (gTelephonyList->Length() == 1) {
    delete gTelephonyList;
    gTelephonyList = nsnull;
  }
  else {
    gTelephonyList->RemoveElement(this);
  }
}


already_AddRefed<Telephony>
Telephony::Create(nsPIDOMWindow* aOwner, nsIRadioInterfaceLayer* aRIL)
{
  NS_ASSERTION(aOwner, "Null owner!");
  NS_ASSERTION(aRIL, "Null RIL!");

  nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(aOwner);
  NS_ENSURE_TRUE(sgo, nsnull);

  nsCOMPtr<nsIScriptContext> scriptContext = sgo->GetContext();
  NS_ENSURE_TRUE(scriptContext, nsnull);

  nsRefPtr<Telephony> telephony = new Telephony();

  telephony->BindToOwner(aOwner);

  telephony->mRIL = aRIL;
  telephony->mRILTelephonyCallback = new RILTelephonyCallback(telephony);

  nsresult rv = aRIL->EnumerateCalls(telephony->mRILTelephonyCallback);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = aRIL->RegisterCallback(telephony->mRILTelephonyCallback);
  NS_ENSURE_SUCCESS(rv, nsnull);

  return telephony.forget();
}

already_AddRefed<TelephonyCall>
Telephony::CreateNewDialingCall(const nsAString& aNumber)
{
  nsRefPtr<TelephonyCall> call =
    TelephonyCall::Create(this, aNumber,
                          nsIRadioInterfaceLayer::CALL_STATE_DIALING);
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
  nsRefPtr<CallEvent> event = CallEvent::Create(aCall);
  NS_ASSERTION(event, "This should never fail!");

  nsresult rv =
    event->Dispatch(ToIDOMEventTarget(), NS_LITERAL_STRING("callschanged"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(Telephony)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(Telephony,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(incoming)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(callschanged)
  for (PRUint32 index = 0; index < tmp->mCalls.Length(); index++) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mCalls[i]");
    cb.NoteXPCOMChild(tmp->mCalls[index]->ToISupports());
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(Telephony,
                                               nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_CALLBACK(tmp->mCallsArray, "mCallsArray")
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(Telephony,
                                                nsDOMEventTargetHelper)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(incoming)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(callschanged)
  tmp->mCalls.Clear();
  tmp->mActiveCall = nsnull;
  tmp->mCallsArray = nsnull;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(Telephony)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTelephony)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Telephony)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(Telephony, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(Telephony, nsDOMEventTargetHelper)

DOMCI_DATA(Telephony, Telephony)

NS_IMPL_ISUPPORTS1(Telephony::RILTelephonyCallback, nsIRILTelephonyCallback)

NS_IMETHODIMP
Telephony::Dial(const nsAString& aNumber, nsIDOMTelephonyCall** aResult)
{
  NS_ENSURE_ARG(!aNumber.IsEmpty());

  for (PRUint32 index = 0; index < mCalls.Length(); index++) {
    const nsRefPtr<TelephonyCall>& tempCall = mCalls[index];
    if (tempCall->IsOutgoing() &&
        tempCall->CallState() < nsIRadioInterfaceLayer::CALL_STATE_CONNECTED) {
      
      
      NS_WARNING("Only permitted to dial one call at a time!");
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  nsresult rv = mRIL->Dial(aNumber);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<TelephonyCall> call = CreateNewDialingCall(aNumber);

  
  for (PRUint32 index = 0; index < gTelephonyList->Length(); index++) {
    Telephony*& telephony = gTelephonyList->ElementAt(index);
    if (telephony != this) {
      nsRefPtr<Telephony> kungFuDeathGrip = telephony;
      telephony->NoteDialedCallFromOtherInstance(aNumber);
    }
  }

  call.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
Telephony::GetMuted(bool* aMuted)
{
  nsresult rv = mRIL->GetMicrophoneMuted(aMuted);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
Telephony::SetMuted(bool aMuted)
{
  nsresult rv = mRIL->SetMicrophoneMuted(aMuted);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
Telephony::GetSpeakerEnabled(bool* aSpeakerEnabled)
{
  nsresult rv = mRIL->GetSpeakerEnabled(aSpeakerEnabled);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
Telephony::SetSpeakerEnabled(bool aSpeakerEnabled)
{
  nsresult rv = mRIL->SetSpeakerEnabled(aSpeakerEnabled);
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
  if (sc) {
    rv =
      nsContentUtils::WrapNative(sc->GetNativeContext(),
                                 sc->GetNativeGlobal(),
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
    if (sc) {
      rv =
        nsTArrayToJSArray(sc->GetNativeContext(),
                          sc->GetNativeGlobal(), mCalls, &calls);
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

  nsresult rv = mRIL->StartTone(aDTMFChar);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
Telephony::StopTone()
{
  nsresult rv = mRIL->StopTone();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMPL_EVENT_HANDLER(Telephony, incoming)
NS_IMPL_EVENT_HANDLER(Telephony, callschanged)

NS_IMETHODIMP
Telephony::CallStateChanged(PRUint32 aCallIndex, PRUint16 aCallState,
                            const nsAString& aNumber)
{
  NS_ASSERTION(aCallIndex != kOutgoingPlaceholderCallIndex,
               "This should never happen!");

  nsRefPtr<TelephonyCall> modifiedCall;
  nsRefPtr<TelephonyCall> outgoingCall;

  for (PRUint32 index = 0; index < mCalls.Length(); index++) {
    nsRefPtr<TelephonyCall>& tempCall = mCalls[index];
    if (tempCall->CallIndex() == kOutgoingPlaceholderCallIndex) {
      NS_ASSERTION(!outgoingCall, "More than one outgoing call not supported!");
      NS_ASSERTION(tempCall->CallState() ==
                   nsIRadioInterfaceLayer::CALL_STATE_DIALING,
                   "Something really wrong here!");
      
      
      outgoingCall = tempCall;
    } else if (tempCall->CallIndex() == aCallIndex) {
      
      modifiedCall = tempCall;
      outgoingCall = nsnull;
      break;
    }
  }

  
  
  
  if (!modifiedCall &&
      aCallState != nsIRadioInterfaceLayer::CALL_STATE_INCOMING &&
      outgoingCall) {
    outgoingCall->UpdateCallIndex(aCallIndex);
    modifiedCall.swap(outgoingCall);
  }

  if (modifiedCall) {
    
    modifiedCall->ChangeState(aCallState);

    
    if (aCallState == nsIRadioInterfaceLayer::CALL_STATE_CONNECTED) {
      mActiveCall = modifiedCall;
    }

    return NS_OK;
  }

  
  NS_ASSERTION(aCallState == nsIRadioInterfaceLayer::CALL_STATE_INCOMING,
               "Serious logic problem here!");

  nsRefPtr<TelephonyCall> call =
    TelephonyCall::Create(this, aNumber, aCallState, aCallIndex);
  NS_ASSERTION(call, "This should never fail!");

  NS_ASSERTION(mCalls.Contains(call), "Should have auto-added new call!");

  
  nsRefPtr<CallEvent> event = CallEvent::Create(call);
  NS_ASSERTION(event, "This should never fail!");

  nsresult rv =
    event->Dispatch(ToIDOMEventTarget(), NS_LITERAL_STRING("incoming"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
Telephony::EnumerateCallState(PRUint32 aCallIndex, PRUint16 aCallState,
                              const nsAString& aNumber, bool aIsActive,
                              bool* aContinue)
{
#ifdef DEBUG
  
  for (PRUint32 index = 0; index < mCalls.Length(); index++) {
    NS_ASSERTION(mCalls[index]->CallIndex() != aCallIndex,
                 "Something is really wrong here!");
  }
#endif
  nsRefPtr<TelephonyCall> call =
    TelephonyCall::Create(this, aNumber, aCallState, aCallIndex);
  NS_ASSERTION(call, "This should never fail!");

  NS_ASSERTION(mCalls.Contains(call), "Should have auto-added new call!");

  if (aIsActive) {
    NS_ASSERTION(!mActiveCall, "Already have an active call!");
    mActiveCall = call;
  }

  *aContinue = true;
  return NS_OK;
}

nsresult
NS_NewTelephony(nsPIDOMWindow* aWindow, nsIDOMTelephony** aTelephony)
{
  NS_ASSERTION(aWindow, "Null pointer!");

  
  nsPIDOMWindow* innerWindow = aWindow->IsInnerWindow() ?
                               aWindow :
                               aWindow->GetCurrentInnerWindow();
  NS_ENSURE_TRUE(innerWindow, NS_ERROR_FAILURE);

  
  
  if (!nsContentUtils::CanCallerAccess(innerWindow)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  
  nsCOMPtr<nsIDocument> document =
    do_QueryInterface(innerWindow->GetExtantDocument());
  NS_ENSURE_TRUE(document, NS_NOINTERFACE);

  
  
  if (!nsContentUtils::IsSystemPrincipal(document->NodePrincipal())) {
    nsCOMPtr<nsIURI> originalURI;
    nsresult rv =
      document->NodePrincipal()->GetURI(getter_AddRefs(originalURI));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> documentURI;
    rv = originalURI->Clone(getter_AddRefs(documentURI));
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIURL> documentURL = do_QueryInterface(documentURI);
    if (documentURL) {
      rv = documentURL->SetQuery(EmptyCString());
      NS_ENSURE_SUCCESS(rv, rv);
    }

    bool allowed = false;

    
    
    nsCString whitelist;
    if (NS_SUCCEEDED(Preferences::GetCString(DOM_TELEPHONY_APP_PHONE_URL_PREF,
                                             &whitelist))) {
      nsCOMPtr<nsIIOService> ios = do_GetIOService();
      NS_ENSURE_TRUE(ios, NS_ERROR_FAILURE);

      nsCCharSeparatedTokenizer tokenizer(whitelist, ',');
      while (tokenizer.hasMoreTokens()) {
        nsCOMPtr<nsIURI> uri;
        if (NS_SUCCEEDED(NS_NewURI(getter_AddRefs(uri), tokenizer.nextToken(),
                                   nsnull, nsnull, ios))) {
          rv = documentURI->EqualsExceptRef(uri, &allowed);
          NS_ENSURE_SUCCESS(rv, rv);

          if (allowed) {
            break;
          }
        }
      }
    }

    if (!allowed) {
      *aTelephony = nsnull;
      return NS_OK;
    }
  }

  
  nsIInterfaceRequestor* ireq = SystemWorkerManager::GetInterfaceRequestor();
  NS_ENSURE_TRUE(ireq, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIRadioInterfaceLayer> ril = do_GetInterface(ireq);
  NS_ENSURE_TRUE(ril, NS_ERROR_UNEXPECTED);

  nsRefPtr<Telephony> telephony = Telephony::Create(innerWindow, ril);
  NS_ENSURE_TRUE(telephony, NS_ERROR_UNEXPECTED);

  telephony.forget(aTelephony);
  return NS_OK;
}
