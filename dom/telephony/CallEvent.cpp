





#include "CallEvent.h"

#include "nsDOMClassInfo.h"

#include "Telephony.h"
#include "TelephonyCall.h"

USING_TELEPHONY_NAMESPACE


already_AddRefed<CallEvent>
CallEvent::Create(TelephonyCall* aCall)
{
  NS_ASSERTION(aCall, "Null pointer!");

  nsRefPtr<CallEvent> event = new CallEvent();

  event->mCall = aCall;

  return event.forget();
}

NS_IMPL_CYCLE_COLLECTION_CLASS(CallEvent)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(CallEvent,
                                                  nsDOMEvent)
  
  
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mCall");
  cb.NoteNativeChild(tmp->mCall->ToISupports(), NS_CYCLE_COLLECTION_PARTICIPANT(TelephonyCall));

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(CallEvent,
                                                nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mCall)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(CallEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCallEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CallEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(CallEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(CallEvent, nsDOMEvent)

DOMCI_DATA(CallEvent, CallEvent)

NS_IMETHODIMP
CallEvent::GetCall(nsIDOMTelephonyCall** aCall)
{
  nsCOMPtr<nsIDOMTelephonyCall> call = mCall.get();
  call.forget(aCall);
  return NS_OK;
}
